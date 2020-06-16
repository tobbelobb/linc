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
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE /* NOLINT */
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <linc/stl.h++>

#ifndef SEEK_SET
#error "SEEK_SET not defined"
#endif

#include <Eigen/src/Core/util/DisableStupidWarnings.h>

template <size_t N>
constexpr auto length(char const (&/*unused*/)[N]) /* NOLINT */
    -> size_t {
  return N - 1;
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

struct FacetCountResult {
  FILE *fp;
  size_t numberOfFacets;
};

static auto getFileSize(FILE *fp) -> long {
  fseek(fp, 0, SEEK_END);
  long const file_size = ftell(fp);
  rewind(fp);
  return file_size;
}

auto divide(auto dividend, auto divisor) {
  struct result {
    long int quotient;
    long int remainder;
  };
  return result{dividend / divisor, dividend % divisor};
}

static auto countBinaryFacets(FILE *fp) -> std::tuple<FILE *, size_t> {
  long const file_size = getFileSize(fp);

  // Test if the STL file has the right size.
  auto const [quotient, reminder] =
      divide(file_size - HEADER_SIZE, SIZEOF_STL_FACET);
  if ((reminder != 0) or (file_size < STL_MIN_FILE_SIZE)) {
    SPDLOG_ERROR("Wrong file size. Aborting binary stl facet count.");
    return {fp, 0};
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
  return {fp, numberOfFacets};
}

static auto countAsciiFacets(FILE *fp) -> std::tuple<FILE *, size_t> {
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

  rewind(fp);

  auto const numberOfFacets = num_lines / ASCII_LINES_PER_FACET;
  SPDLOG_INFO("Found {} facets", numberOfFacets);
  return {fp, numberOfFacets};
}

static auto countFacets(FILE *fp, Stl::Type const stlType)
    -> std::tuple<FILE *, size_t> {
  if (stlType == Stl::Type::BINARY) {
    return countBinaryFacets(fp);
  }
  if (stlType == Stl::Type::ASCII) {
    return countAsciiFacets(fp);
  }
  return {fp, 0};
}

static inline void skipWhitespace(FILE *fp) {
  fscanf(fp, " "); // NOLINT
}

static void skipSolidEndsolid(FILE *fp) {
  fscanf(fp, " endsolid%*[^\n]\n"); // NOLINT
  fscanf(fp, " solid%*[^\n]\n");    // NOLINT
  skipWhitespace(fp);
}

static auto parseNormal(FILE *fp, Stl::Facet &facet) -> bool {
  constexpr auto BUF_SIZE{2048};
  std::array<char, BUF_SIZE> buf{'\0'};
  fgets(buf.data(), BUF_SIZE - 1, fp);
  constexpr auto CHARS_PER_FLOAT{32};
  std::array<std::array<char, CHARS_PER_FLOAT>, 3> normal_buf{{'\0'}};
  int res_normal =
      sscanf(buf.data(), "facet normal %31s %31s %31s", /* NOLINT */
             normal_buf[0].data(), normal_buf[1].data(), normal_buf[2].data());
  // The facet normal has been parsed as a single string as to workaround
  // for not a numbers in the normal definition.
  return (
      res_normal == 3 and
      sscanf(normal_buf[0].data(), "%f", &facet.normal.x()) == 1 and // NOLINT
      sscanf(normal_buf[1].data(), "%f", &facet.normal.y()) == 1 and // NOLINT
      sscanf(normal_buf[2].data(), "%f", &facet.normal.z()) == 1 and // NOLINT
      not(std::isnan(facet.normal.x()) or std::isnan(facet.normal.y()) or
          std::isnan(facet.normal.z()))); // NOLINT
}

static auto parseOuterLoop(FILE *fp) -> bool {
  return fscanf(fp, " outer loop") != EOF; /* NOLINT */
}

static auto parseVertices(FILE *fp, Stl::Facet &facet) {
  for (auto const vertex : {0, 1, 2}) {
    auto matchedNumbers = fscanf(fp, " vertex %f %f %f ",      /* NOLINT */
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
  return sscanf(buf, "vertex %f %f %f ", /* NOLINT */
                &throwaway.x(),          /* NOLINT */
                &throwaway.y(),          /* NOLINT */
                &throwaway.z()) == 3;    // NOLINT
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
      SPDLOG_ERROR("File is not proper stl. Aborting file parse.");
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

auto Stl::readAscii(FILE *fp) -> bool {
  SPDLOG_TRACE("(fp)");
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

auto Stl::readBinary(FILE *fp) -> bool {
  fseek(fp, HEADER_SIZE, SEEK_SET);
  for (Facet &facet : m_facets) {
    if (fread(&facet, 1, SIZEOF_STL_FACET, fp) != SIZEOF_STL_FACET) {
      return false;
    }
  }
  return true;
}

// Reads file into appropriately allocated vector m_facets
auto Stl::read(FILE *fp) -> bool {
  SPDLOG_TRACE("(fp)");
  if (m_type == Stl::Type::BINARY) {
    return readBinary(fp);
  }
  if (m_type == Stl::Type::ASCII) {
    return readAscii(fp);
  }
  SPDLOG_WARN("Unknown stl type. Don't know how to read.");
  return false;
}

void Stl::computeSomeStats() {
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

void Stl::allocate() {
  m_facets.assign(m_stats.number_of_facets, Facet());
  m_neighbors.assign(m_stats.number_of_facets, Neighbors());
}

Stl::Stl(std::string const &fileName) {
  spdlog::set_default_logger(logger);
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s(%#)] [%!] [%l] %v");
  spdlog::set_level(spdlog::level::trace);
  SPDLOG_TRACE("({})", fileName);

  auto [fp, type] = openFile(fileName);
  if (fp == nullptr) {
    return;
  }

  m_type = type;
  auto [fpAfterCount, numberOfFacets] = countFacets(fp, m_type);
  m_stats.number_of_facets = numberOfFacets;

  allocate();
  m_initialized = read(fpAfterCount);
  fclose(fp);

  if (m_initialized) {
    computeSomeStats();
  } else {
    SPDLOG_DEBUG("Stl not initialized. Clearing");
    clear();
  }
}

#include <Eigen/src/Core/util/ReenableStupidWarnings.h>
