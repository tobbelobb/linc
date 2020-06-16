/*  ADMesh -- process triangulated solid meshes
 *  Copyright (C) 1995, 1996  Anthony D. Martin <amartin@engr.csulb.edu>
 *  Copyright (C) 2013, 2014  several contributors, see AUTHORS
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Questions, comments, suggestions, etc to
 *           https://github.com/admesh/admesh/issues
 */

#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>

#include <gsl/pointers>
#include <gsl/span_ext>

// Set the default logger to file logger
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG /* NOLINT */
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <linc/stl.h++>

#ifndef SEEK_SET
#error "SEEK_SET not defined"
#endif

template <size_t N>
constexpr auto length(char const (&/*unused*/)[N]) /* NOLINT */
    -> size_t {
  return N - 1;
}

auto divide(auto dividend, auto divisor) {
  struct result {
    size_t quotient;
    size_t remainder;
  };
  return result{dividend / divisor, dividend % divisor};
}

static auto logger = spdlog::basic_logger_mt("logger_name", "linc.log");

static auto openFile(std::string const &fileName)
    -> std::tuple<gsl::owner<FILE *>, Stl::Type> {
  SPDLOG_TRACE("({})", fileName);
  // Open the file in binary mode first.
  gsl::owner<FILE *> fp = fopen(fileName.c_str(), "rb");
  if (fp == nullptr) {
    SPDLOG_ERROR("Could not open file. Returning.");
    return {nullptr, Stl::Type::UNKNOWN};
  }
  // Check for binary or ASCII file.
  fseek(fp, HEADER_SIZE, SEEK_SET);
  std::array<unsigned char, ASCII_TABLE_SIZE> chtest{'\0'};
  if (fread(chtest.data(), ASCII_TABLE_SIZE, 1, fp) == 0) {
    SPDLOG_DEBUG("File is shorter than {} bytes. Returning.",
                 HEADER_SIZE + sizeof(chtest));
    return {nullptr, Stl::Type::UNKNOWN};
  }
  Stl::Type stlType = Stl::Type::ASCII;
  // std::any_of?
  for (unsigned char s : chtest) {
    if (s > ASCII_TABLE_SIZE - 1) {
      stlType = Stl::Type::BINARY;
      rewind(fp);
      break;
    }
  }
  if (stlType == Stl::Type::ASCII) {
    // Reopen the file in text mode for getting correct newlines on Windows
    fclose(fp);
    fp = fopen(fileName.c_str(), "r");
  }

  return {fp, stlType};
}

static auto getFileSize(FILE *fp) -> size_t {
  SPDLOG_TRACE("(fp)");
  // Want to get rid of FILE * and just use
  // return std::filesystem::file_size(fileName);
  fseek(fp, 0, SEEK_END);
  return static_cast<size_t>(ftell(fp));
}

static auto countBinaryFacets(FILE *fp) -> size_t {
  SPDLOG_TRACE("(fp)");

  size_t const file_size = getFileSize(fp);

  // Test if the STL file has the right size.
  auto const [quotient, reminder] =
      divide(file_size - HEADER_SIZE, SIZEOF_STL_FACET);
  if ((reminder != 0) or (file_size < STL_MIN_FILE_SIZE)) {
    SPDLOG_ERROR("Wrong file size. Aborting binary stl facet count.");
    return 0;
  }
  size_t const numberOfFacets = quotient;

  // Read the binary header
  std::array<char, LABEL_SIZE + 1> headerBuf{'\0'};
  if (fread(headerBuf.data(), LABEL_SIZE, 1, fp) > LABEL_SIZE - 1) {
    headerBuf[LABEL_SIZE] = '\0';
  }

  // Read the int following the header.  This should contain # of facets.
  size_t headerNumFacets = 0;
  fread(&headerNumFacets, sizeof(uint32_t), 1, fp);
  if (headerNumFacets != numberOfFacets) {
    SPDLOG_WARN("Binary header says file contains {} facets, but file "
                "actually contains {} facets.",
                headerNumFacets, numberOfFacets);
  }

  SPDLOG_INFO("Found {} facets", numberOfFacets);
  return numberOfFacets;
}

static auto countAsciiFacets(FILE *fp) -> size_t {
  SPDLOG_TRACE("(fp)");

  constexpr auto LINEBUF_SIZE{100};
  std::array<char, LINEBUF_SIZE> linebuf{'\0'};
  auto num_lines = 1U;
  while (fgets(linebuf.data(), linebuf.size(), fp) != nullptr) {
    // Don't count short lines.
    constexpr auto SHORT_LINE_LIMIT{4};
    if (strlen(linebuf.data()) <= SHORT_LINE_LIMIT) {
      continue;
    }
    // Skip solid/endsolid lines as broken STL file generators may put
    // several of them.
    if (strncmp(linebuf.data(), "solid", length("solid")) == 0 ||
        strncmp(linebuf.data(), "endsolid", length("endsolid")) == 0) {
      continue;
    }
    ++num_lines;
  }
  auto const numberOfFacets = num_lines / ASCII_LINES_PER_FACET;
  SPDLOG_INFO("Found {} facets", numberOfFacets);
  return numberOfFacets;
}

static auto countFacets(FILE *fp, Stl::Type const stlType) -> size_t {
  SPDLOG_TRACE("(fp, {})", stlType);

  if (stlType == Stl::Type::BINARY) {
    return countBinaryFacets(fp);
  }
  if (stlType == Stl::Type::ASCII) {
    return countAsciiFacets(fp);
  }
  return 0;
}

static inline void skipWhitespace(FILE *fp) {
  fscanf(fp, " "); // NOLINT
}

static void skipSolidEndsolid(FILE *fp) {
  SPDLOG_TRACE("(fp)", stlType);

  fscanf(fp, " endsolid%*[^\n]\n"); // NOLINT
  fscanf(fp, " solid%*[^\n]\n");    // NOLINT
  skipWhitespace(fp);
}

static auto parseNormal(FILE *fp, Stl::Facet &facet) -> bool {
  SPDLOG_TRACE("(fp, facet)");

  constexpr auto BUF_SIZE{2048};
  std::array<char, BUF_SIZE> buf{'\0'};
  fgets(buf.data(), BUF_SIZE - 1, fp);
  constexpr auto CHARS_PER_NUMBER{32};
  std::array<std::array<char, CHARS_PER_NUMBER>, 3> normal_buf{{'\0'}};
  int res_normal =
      sscanf(buf.data(), "facet normal %31s %31s %31s", /* NOLINT */
             normal_buf[0].data(), normal_buf[1].data(), normal_buf[2].data());
  // The facet normal has been parsed as a single string as to workaround
  // for not a numbers in the normal definition.
  return (
      res_normal == 3 and
      sscanf(normal_buf[0].data(), "%lf", &facet.normal.x()) == 1 and // NOLINT
      sscanf(normal_buf[1].data(), "%lf", &facet.normal.y()) == 1 and // NOLINT
      sscanf(normal_buf[2].data(), "%lf", &facet.normal.z()) == 1 and // NOLINT
      not(std::isnan(facet.normal.x()) or std::isnan(facet.normal.y()) or
          std::isnan(facet.normal.z()))); // NOLINT
}

static auto parseOuterLoop(FILE *fp) -> bool {
  SPDLOG_TRACE("(fp)");

  return fscanf(fp, " outer loop") != EOF; /* NOLINT */
}

static auto parseVertices(FILE *fp, Stl::Facet &facet) {
  for (auto const vertex : {0UL, 1UL, 2UL}) {
    auto matchedNumbers = fscanf(fp, " vertex %lf %lf %lf ",      /* NOLINT */
                                 &facet.vertices[vertex].x(),  /* NOLINT */
                                 &facet.vertices[vertex].y(),  /* NOLINT */
                                 &facet.vertices[vertex].z()); /* NOLINT */
    if (matchedNumbers != 3) {
      return false;
    }
  }
  return true;
}

static auto parseShadowVertex(char *buf) -> bool {
  Vertex throwaway{0.0F, 0.0F, 0.0F};
  return sscanf(buf, "vertex %lf %lf %lf ", /* NOLINT */
                &throwaway.x(),          /* NOLINT */
                &throwaway.y(),          /* NOLINT */
                &throwaway.z()) == 3;    /* NOLINT */
}

static auto parseEndloop(FILE *fp) -> bool {
  constexpr auto BUF_SIZE{2048}; // large buffer to parse whole line
  auto endloopFound = [](std::array<char, BUF_SIZE> buffer) {
    constexpr auto CHARS_IN_ENDLOOP{length("endloop")};
    return strncmp(buffer.data(), "endloop", CHARS_IN_ENDLOOP) == 0 and
           (buffer[CHARS_IN_ENDLOOP] == '\r' or /* NOLINT */
            buffer[CHARS_IN_ENDLOOP] == '\n' or /* NOLINT */
            buffer[CHARS_IN_ENDLOOP] == ' ' or  /* NOLINT */
            buffer[CHARS_IN_ENDLOOP] == '\t');  /* NOLINT */
  };

  std::array<char, BUF_SIZE> buf{'\0'};
  fgets(buf.data(), buf.size(), fp);
  if (not endloopFound(buf)) {
    // Try to parse a fourth throwaway vertex
    if (parseShadowVertex(buf.data())) {
      SPDLOG_WARN(
          "Found 4 vertices in single facet. Throwing away fourth vertex.");
      skipWhitespace(fp);
      if (not parseEndloop(fp)) {
        SPDLOG_ERROR("Could not find endloop. Aborting file parse.");
        return false;
      }
    } else {
      SPDLOG_ERROR("File is not a proper ascii stl. Aborting file parse.");
      return false;
    }
  }
  return true;
}

static auto parseEndFacet(FILE *fp) -> bool {
  skipWhitespace(fp);

  constexpr auto CHARS_IN_ENDFACET{length("endfacet")};
  constexpr auto BUF_SIZE{2048}; // large buffer to parse whole line
  std::array<char, BUF_SIZE> buf{'\0'};
  fgets(buf.data(), BUF_SIZE - 1, fp);
  return strncmp(buf.data(), "endfacet", CHARS_IN_ENDFACET) == 0 and
         std::isspace(buf[CHARS_IN_ENDFACET]) != 0;
}

// Read a single facet from an ASCII .STL file
static auto readAsciiFacet(FILE *fp, Stl::Facet &facet) -> bool {
  skipSolidEndsolid(fp);
  if (not parseNormal(fp, facet)) {
    SPDLOG_WARN("Found bogus normal. Will set normal to zero and continue.");
    facet.normal = Normal::Zero();
  }
  if (not parseOuterLoop(fp)) {
    SPDLOG_ERROR("Unexpected end of file. Aborting file parse.");
    return false;
  }
  if (not parseVertices(fp, facet)) {
    SPDLOG_ERROR("Ill formed vertex. Aborting file parse.");
    return false;
  }
  if (not parseEndloop(fp)) {
    SPDLOG_ERROR("Did not find endloop. Aborting file parse.");
    return false;
  }
  return parseEndFacet(fp);
}

auto Stl::readAsciiFacets(FILE *fp) -> bool {
  SPDLOG_TRACE("(fp)");
  fseek(fp, 0, SEEK_SET);
  skipSolidEndsolid(fp);
  rewind(fp);
  for (Facet &facet : m_facets) {
    if (not readAsciiFacet(fp, facet)) {
      SPDLOG_ERROR("Could not parse facet. Aborting file parse");
      return false;
    }
  }
  return true;
}

auto Stl::readBinaryFacets(FILE *fp) -> bool {
  using SmallVertex = Eigen::Matrix<float, 3, 1, Eigen::DontAlign>;
  using SmallNormal = Eigen::Matrix<float, 3, 1, Eigen::DontAlign>;
  struct SmallFacet {
    SmallNormal normal;
    std::array<SmallVertex, 3> vertices;
    std::array<std::byte, 2> extra;
  };
  static_assert(sizeof(SmallVertex) == 12, /* NOLINT */
                "size of Vertex incorrect");
  static_assert(sizeof(SmallNormal) == 12, /* NOLINT */
                "size of Normal incorrect");
  static_assert(offsetof(SmallFacet, normal) == 0, /* NOLINT */
                "SmallFacet.normal offset is not 0");
  static_assert(offsetof(SmallFacet, vertices) == 12, /* NOLINT */
                "SmallFacet.vertex offset is not 12");
  static_assert(offsetof(SmallFacet, extra) == 48, /* NOLINT */
                "SmallFacet.extra offset is not 48");
  static_assert(sizeof(Stl::Facet) >= SIZEOF_STL_FACET,
                "size of SmallFacet is too small");

  fseek(fp, HEADER_SIZE, SEEK_SET);
  for (Facet &facet : m_facets) {
    SmallFacet smallFacet{};
    if (fread(&smallFacet, 1, SIZEOF_STL_FACET, fp) != SIZEOF_STL_FACET) {
      return false;
    }
    facet.normal = smallFacet.normal.cast<double>();
    facet.vertices[0] = smallFacet.vertices[0].cast<double>();
    facet.vertices[1] = smallFacet.vertices[1].cast<double>();
    facet.vertices[2] = smallFacet.vertices[2].cast<double>();
  }
  return true;
}

// Reads file into appropriately allocated vector m_facets
auto Stl::readFacets(FILE *fp) -> bool {
  SPDLOG_TRACE("(fp)");
  if (m_type == Stl::Type::BINARY) {
    return readBinaryFacets(fp);
  }
  if (m_type == Stl::Type::ASCII) {
    return readAsciiFacets(fp);
  }
  SPDLOG_WARN("Unknown stl type. Don't know how to read facets.");
  return false;
}

void Stl::computeSomeStats() {
  SPDLOG_TRACE("()");
  m_stats.min = m_facets[0].vertices[0];
  m_stats.max = m_facets[0].vertices[0];
  Vertex diff = (m_facets[0].vertices[1] - m_facets[0].vertices[0]).cwiseAbs();
  m_stats.shortest_edge = std::max(diff.x(), std::max(diff.y(), diff.z()));
  for (auto const &facet : m_facets) {
    // Now find the max and min values.
    for (const auto &i : facet.vertices) {
      m_stats.min = m_stats.min.cwiseMin(i);
      m_stats.max = m_stats.max.cwiseMax(i);
    }
  }
  m_stats.size = m_stats.max - m_stats.min;
  m_stats.bounding_diameter = m_stats.size.norm();
}

void Stl::allocate(size_t const numberOfFacets) {
  SPDLOG_TRACE("(numberOfFacets={})", numberOfFacets);
  m_facets.assign(numberOfFacets, Facet());
  m_neighbors.assign(numberOfFacets, Neighbors());
}

Stl::Stl(std::string const &fileName) {
  spdlog::set_default_logger(logger);
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s(%#)] [%!] [%l] %v");
  spdlog::set_level(spdlog::level::trace);
  SPDLOG_DEBUG("Stl constructor start: {}", fileName);

  auto [fp, type] = openFile(fileName);
  if (fp == nullptr) {
    return;
  }
  m_type = type;

  allocate(countFacets(fp, m_type));
  m_initialized = readFacets(fp);
  fclose(fp);

  if (m_initialized) {
    computeSomeStats();
  } else {
    SPDLOG_DEBUG("Stl not initialized. Clearing");
    clear();
  }
  SPDLOG_DEBUG("Stl constructor done", fileName);
}
