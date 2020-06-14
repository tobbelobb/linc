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

// TODO: un-NOLINT this file
#include <Eigen/src/Core/util/DisableStupidWarnings.h>
static auto logger = spdlog::basic_logger_mt("logger_name", "linc.log");

template <size_t N>
constexpr auto length(char const (&/*unused*/)[N]) /* NOLINT */
    -> size_t {
  return N - 1;
}

auto Stl::openCountFacets(std::string const &fileName) -> gsl::owner<FILE *> {
  SPDLOG_TRACE("({})", fileName);
  // Open the file in binary mode first.
  gsl::owner<FILE *> fp = fopen(fileName.c_str(), "rb");
  if (fp == nullptr) {
    SPDLOG_DEBUG("Could not open file. Returning.");
    return nullptr;
  }
  // Find size of file.
  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);

  // Check for binary or ASCII file.
  fseek(fp, HEADER_SIZE, SEEK_SET);
  unsigned char chtest[ASCII_TABLE_SIZE]; // NOLINT
  if (fread((unsigned char *)chtest, sizeof(chtest), 1, fp) == 0) {
    SPDLOG_DEBUG("File is shorter than {} bytes. Returning.",
                 HEADER_SIZE + sizeof(chtest));
    fclose(fp);
    return nullptr;
  }
  m_stats.type = Stl::Type::ASCII;
  for (unsigned char s : chtest) {
    if (s > ASCII_TABLE_SIZE - 1) {
      m_stats.type = Stl::Type::BINARY;
      break;
    }
  }
  rewind(fp);

  // Get the header and the number of facets in the .STL file.
  // If the .STL file is binary, then do the following:
  if (m_stats.type == Stl::Type::BINARY) {
    // Test if the STL file has the right size.
    if (((file_size - HEADER_SIZE) % SIZEOF_STL_FACET != 0) ||
        (file_size < STL_MIN_FILE_SIZE)) {
      SPDLOG_WARN("Wrong file size. Aborting binary stl facet count.");
      fclose(fp);
      return nullptr;
    }

    m_stats.number_of_facets = (file_size - HEADER_SIZE) / SIZEOF_STL_FACET;

    // Read the header.
    if (fread(m_stats.header, LABEL_SIZE, 1, fp) > LABEL_SIZE - 1) { // NOLINT
      m_stats.header[LABEL_SIZE] = '\0';                             // NOLINT
    }

    // Read the int following the header.  This should contain # of facets.
    fread(&m_stats.header_num_facets, sizeof(uint32_t), 1, fp);
    if (m_stats.header_num_facets != m_stats.number_of_facets) {
      SPDLOG_WARN("Binary header says file contains {} facets, but file "
                  "actually contains {} facets.",
                  m_stats.header_num_facets, m_stats.number_of_facets);
    }
  }
  // Otherwise, if the .STL file is ASCII, then do the following:
  else {
    // Reopen the file in text mode (for getting correct newlines on Windows)
    // fix to silence a warning about unused return value.
    // obviously if it fails we have problems....
    fclose(fp);
    fp = fopen(fileName.c_str(), "r");

    // do another null check to be safe
    if (fp == nullptr) {
      fclose(fp);
      return nullptr;
    }

    // Find the number of facets.
    constexpr auto LINEBUF_SIZE{100};
    char linebuf[LINEBUF_SIZE]; // NOLINT
    auto num_lines = 1U;
    while (fgets((char *)linebuf, LINEBUF_SIZE, fp) != nullptr) {
      // Don't count short lines.
      constexpr auto SHORT_LINE_LIMIT{4};
      if (strlen((char *)linebuf) <= SHORT_LINE_LIMIT) {
        continue;
      }
      // Skip solid/endsolid lines as broken STL file generators may put
      // several of them.
      if (strncmp((char *)linebuf, "solid", strlen("solid")) == 0 ||
          strncmp((char *)linebuf, "endsolid", strlen("endsolid")) == 0) {
        continue;
      }
      ++num_lines;
    }

    rewind(fp);

    // Get the header.
    size_t i = 0;
    for (; i < LABEL_SIZE &&
           (gsl::at(m_stats.header, i) = (char)getc(fp)) != '\n';
         ++i) {
      ;
    }
    gsl::at(m_stats.header, i) = '\0'; // Lose the '\n'
    gsl::at(m_stats.header, LABEL_SIZE) = '\0';

    m_stats.number_of_facets = num_lines / ASCII_LINES_PER_FACET;
  }

  SPDLOG_INFO("Found {} facets", m_stats.number_of_facets);

  return fp;
}

// Reads the contents of the file pointed to by fp into the stl structure
auto Stl::read(FILE *fp) -> bool {
  if (m_stats.type == Stl::Type::BINARY) {
    fseek(fp, HEADER_SIZE, SEEK_SET);
  } else {
    rewind(fp);
  }

  constexpr auto CHARS_PER_FLOAT{32};
  char normal_buf[3][CHARS_PER_FLOAT]; // NOLINT
  for (Facet &facet : m_facets) {
    if (m_stats.type == Stl::Type::BINARY) {
      // Read a single facet from a binary .STL file.
      // We assume little-endian architecture!
      if (fread(&facet, 1, SIZEOF_STL_FACET, fp) != SIZEOF_STL_FACET) {
        return false;
      }
    } else {
      // Read a single facet from an ASCII .STL file
      // skip solid/endsolid
      // (in this order, otherwise it won't work when they are paired in the
      // middle of a file)
      fscanf(fp, " endsolid%*[^\n]\n"); // NOLINT
      // name might contain spaces so %*s doesn't
      // work and it also can be empty (just "solid")
      fscanf(fp, " solid%*[^\n]\n"); // NOLINT
      fscanf(fp, " ");
      constexpr auto BUF_SIZE{2048};
      char buf[BUF_SIZE]; // NOLINT
      fgets((char *)buf, BUF_SIZE - 1, fp);
      int res_normal =
          sscanf(buf, "facet normal %31s %31s %31s",            /* NOLINT */
                 (char *)normal_buf[0],                         /* NOLINT */
                 (char *)normal_buf[1], (char *)normal_buf[2]); // NOLINT
      // The facet normal has been parsed as a single string as to workaround
      // for not a numbers in the normal definition.
      if (res_normal != 3 or
          sscanf(normal_buf[0], "%f", &facet.normal.x()) != 1 or // NOLINT
          sscanf(normal_buf[1], "%f", &facet.normal.y()) != 1 or // NOLINT
          sscanf(normal_buf[2], "%f", &facet.normal.z()) != 1 or
          std::isnan(facet.normal.x()) or std::isnan(facet.normal.y()) or
          std::isnan(facet.normal.z())) { // NOLINT
        SPDLOG_WARN(
            "Found bogus normal. Will set normal to zero and continue.");
        facet.normal = Normal::Zero();
      }
      if (fscanf(fp, " outer loop") == EOF) { /* NOLINT */
        SPDLOG_ERROR("Unexpected end of file. Aborting file parse.");
        return false;
      }

      for (auto const vertex : {0, 1, 2}) {
        auto matchedNumbers = fscanf(fp, " vertex %f %f %f ",      /* NOLINT */
                                     &facet.vertices[vertex].x(),  /* NOLINT */
                                     &facet.vertices[vertex].y(),  /* NOLINT */
                                     &facet.vertices[vertex].z()); // NOLINT

        if (matchedNumbers != 3) {
          SPDLOG_ERROR("Ill formed vertex. Aborting file parse.");
          return false;
        }
      }

      auto endloopFound = [](char *buffer)
          -> bool { /* NOLINT */
                    constexpr auto CHARS_IN_ENDLOOP{length("endloop")};
                    return strncmp(buffer, "endloop", CHARS_IN_ENDLOOP) == 0 and
                           (buffer[CHARS_IN_ENDLOOP] == '\r' or /* NOLINT */
                            buffer[CHARS_IN_ENDLOOP] == '\n' or /* NOLINT */
                            buffer[CHARS_IN_ENDLOOP] == ' ' or  /* NOLINT */
                            buffer[CHARS_IN_ENDLOOP] == '\t');  /* NOLINT */
      };

      fgets((char *)buf, BUF_SIZE - 1, fp);
      bool endloop_ok = endloopFound((char *)buf);
      if (not endloop_ok) {
        // Try to parse a fourth throwaway vertex
        Vertex throwaway{0.0F, 0.0F, 0.0F};
        int res_vertex4 = sscanf(buf, "vertex %f %f %f ", /* NOLINT */
                                 &throwaway.x(),          /* NOLINT */
                                 &throwaway.y(),          /* NOLINT */
                                 &throwaway.z());         // NOLINT
        if (res_vertex4 == 3) {
          SPDLOG_WARN(
              "Found 4 vertices in single facet. Throwing away fourth vertex.");
          fscanf(fp, " ");                      // NOLINT
          fgets((char *)buf, BUF_SIZE - 1, fp); // NOLINT
          endloop_ok = endloopFound((char *)buf);
          if (not endloop_ok) {
            SPDLOG_ERROR("Could not find endloop. Aborting file parse.");
            return false;
          }
        } else {
          SPDLOG_ERROR("File is not proper stl. Aborting file parse.");
          return false;
        }
      }
      // Skip the trailing whitespaces and empty lines.
      fscanf(fp, " "); // NOLINT
      fgets((char *)buf, BUF_SIZE - 1, fp);
      constexpr auto CHARS_IN_ENDFACET{length("endfacet")};
      bool const endfacet_ok =
          strncmp((char *)buf, "endfacet", CHARS_IN_ENDFACET) == 0 &&
          (buf[CHARS_IN_ENDFACET] == '\r' || buf[CHARS_IN_ENDFACET] == '\n' ||
           buf[CHARS_IN_ENDFACET] == ' ' || buf[CHARS_IN_ENDFACET] == '\t');
      assert(endfacet_ok); // NOLINT
      if (!endloop_ok || !endfacet_ok) {
        return false;
      }
    }
  }
  return true;
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

Stl::Stl(std::string const &fileName) {
  spdlog::set_default_logger(logger);
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s(%#)] [%!] [%l] %v");
  spdlog::set_level(spdlog::level::trace);
  SPDLOG_TRACE("({})", fileName);

  gsl::owner<FILE *> fp = openCountFacets(fileName);
  if (fp == nullptr) {
    return;
  }
  allocate();
  m_initialized = read(fp);
  fclose(fp);
  if (m_initialized) {
    computeSomeStats();
  } else {
    clear();
  }
}

void Stl::allocate() {
  //  Allocate memory for the entire .STL file.
  m_facets.assign(m_stats.number_of_facets, Facet());
  // Allocate memory for the neighbors list.
  m_neighbors.assign(m_stats.number_of_facets, Neighbors());
}

#include <Eigen/src/Core/util/ReenableStupidWarnings.h>
