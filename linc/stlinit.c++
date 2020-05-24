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

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <gsl/pointers>
#include <gsl/span_ext>

#include <linc/stl.h++>

#ifndef SEEK_SET
#error "SEEK_SET not defined"
#endif

// TODO: un-NOLINT this file
#include <Eigen/src/Core/util/DisableStupidWarnings.h>

template <size_t N>
constexpr auto length(char const (&/*unused*/)[N]) /* NOLINT */
    -> size_t {
  return N - 1;
}

auto Stl::openCountFacets(std::string const &fileName) -> gsl::owner<FILE *> {
  // Open the file in binary mode first.
  gsl::owner<FILE *> fp = fopen(fileName.c_str(), "rb");
  if (fp == nullptr) {
    return nullptr;
  }
  // Find size of file.
  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);

  // Check for binary or ASCII file.
  fseek(fp, HEADER_SIZE, SEEK_SET);
  constexpr auto chtest_size{128};
  unsigned char chtest[chtest_size]; // NOLINT
  if (fread((unsigned char *)chtest, sizeof(chtest), 1, fp) == 0) {
    fclose(fp);
    return nullptr;
  }
  m_stats.type = Stl::Type::ASCII;
  for (unsigned char s : chtest) {
    if (s > chtest_size - 1) {
      m_stats.type = Stl::Type::BINARY;
      break;
    }
  }
  rewind(fp);

  auto num_facets = 0L;

  // Get the header and the number of facets in the .STL file.
  // If the .STL file is binary, then do the following:
  if (m_stats.type == Stl::Type::BINARY) {
    // Test if the STL file has the right size.
    if (((file_size - HEADER_SIZE) % SIZEOF_STL_FACET != 0) ||
        (file_size < STL_MIN_FILE_SIZE)) {
      fclose(fp);
      return nullptr;
    }
    num_facets = (file_size - HEADER_SIZE) / SIZEOF_STL_FACET;

    // Read the header.
    if (fread(m_stats.header, LABEL_SIZE, 1, fp) > LABEL_SIZE - 1) { // NOLINT
      m_stats.header[LABEL_SIZE] = '\0';                             // NOLINT
    }

    // Read the int following the header.  This should contain # of facets.
    uint32_t header_num_facets = 0;
    fread(&header_num_facets, sizeof(uint32_t), 1, fp);
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
    auto num_lines = 1L;
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

    num_facets = num_lines / ASCII_LINES_PER_FACET;
  }

  m_stats.number_of_facets += num_facets;
  m_stats.original_num_facets = m_stats.number_of_facets;

  // Set the default logger to file logger
  auto file_logger = spdlog::basic_logger_mt("basic_logger", "logs/basic.txt");
  spdlog::set_default_logger(file_logger);
  spdlog::info("Found {} facets", m_stats.number_of_facets);

  return fp;
}

/* Reads the contents of the file pointed to by fp into the stl structure,
   starting at facet first_facet.  The second argument says if it's our first
   time running this for the stl and therefore we should reset our max and min
   stats. */
auto Stl::read(FILE *fp, int first_facet, bool first) -> bool {
  if (m_stats.type == Stl::Type::BINARY) {
    fseek(fp, HEADER_SIZE, SEEK_SET);
  } else {
    rewind(fp);
  }

  constexpr auto CHARS_PER_FLOAT{32};
  char normal_buf[3][CHARS_PER_FLOAT]; // NOLINT
  for (uint32_t i = first_facet; i < m_stats.number_of_facets; ++i) {
    Facet facet;

    if (m_stats.type == Stl::Type::BINARY) {
      // Read a single facet from a binary .STL file. We assume little-endian
      // architecture!
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
      // Leading space in the fscanf format skips all leading white spaces
      // including numerous new lines and tabs.
      int res_normal =
          fscanf(fp, " facet normal %31s %31s %31s",            /* NOLINT */
                 (char *)normal_buf[0],                         /* NOLINT */
                 (char *)normal_buf[1], (char *)normal_buf[2]); // NOLINT
      assert(res_normal == 3);                                  // NOLINT
      int res_outer_loop = fscanf(fp, " outer loop");           // NOLINT
      assert(res_outer_loop == 0);                              // NOLINT
      int res_vertex1 = fscanf(fp, " vertex %f %f %f",          /* NOLINT */
                               &facet.vertex[0](0),             /* NOLINT */
                               &facet.vertex[0](1),             /* NOLINT */
                               &facet.vertex[0](2));            // NOLINT
      assert(res_vertex1 == 3);                                 // NOLINT
      int res_vertex2 = fscanf(fp, " vertex %f %f %f",          /* NOLINT */
                               &facet.vertex[1](0),             /* NOLINT */
                               &facet.vertex[1](1),             /* NOLINT */
                               &facet.vertex[1](2));            // NOLINT
      assert(res_vertex2 == 3);                                 // NOLINT
      // Trailing whitespace is there to eat all whitespaces and empty lines up
      // to the next non-whitespace.
      int res_vertex3 = fscanf(fp, " vertex %f %f %f ", /* NOLINT */
                               &facet.vertex[2](0),     /* NOLINT */
                               &facet.vertex[2](1),     /* NOLINT */
                               &facet.vertex[2](2));    // NOLINT
      assert(res_vertex3 == 3);                         // NOLINT
      // Some G-code generators tend to produce text after "endloop" and
      // "endfacet". Just ignore it.
      constexpr auto BUF_SIZE{2048};
      char buf[BUF_SIZE]; // NOLINT
      fgets((char *)buf, BUF_SIZE - 1, fp);
      constexpr auto chars_in_endloop{length("endloop")};
      bool endloop_ok =
          strncmp((char *)buf, "endloop", chars_in_endloop) == 0 &&
          (buf[chars_in_endloop] == '\r' || buf[chars_in_endloop] == '\n' ||
           buf[chars_in_endloop] == ' ' || buf[chars_in_endloop] == '\t');
      assert(endloop_ok); // NOLINT
      // Skip the trailing whitespaces and empty lines.
      fscanf(fp, " "); // NOLINT
      fgets((char *)buf, BUF_SIZE - 1, fp);
      constexpr auto chars_in_endfacet{length("endfacet")};
      bool const endfacet_ok =
          strncmp((char *)buf, "endfacet", chars_in_endfacet) == 0 &&
          (buf[chars_in_endfacet] == '\r' || buf[chars_in_endfacet] == '\n' ||
           buf[chars_in_endfacet] == ' ' || buf[chars_in_endfacet] == '\t');
      assert(endfacet_ok); // NOLINT
      if (!endloop_ok || !endfacet_ok) {
        return false;
      }

      // The facet normal has been parsed as a single string as to workaround
      // for not a numbers in the normal definition.
      if (sscanf(normal_buf[0], "%f", &facet.normal(0)) != 1 || // NOLINT
          sscanf(normal_buf[1], "%f", &facet.normal(1)) != 1 || // NOLINT
          sscanf(normal_buf[2], "%f", &facet.normal(2)) != 1) { // NOLINT
        // Normal was mangled. Maybe denormals or "not a number" were stored?
        // Just reset the normal and silently ignore it.
        facet.normal = Normal::Zero();
      }
    }

    // Write the facet into memory.
    m_facets[i] = facet;
    saveFacetStats(facet, first);
  }

  m_stats.size = m_stats.max - m_stats.min;
  m_stats.bounding_diameter = m_stats.size.norm();
  return true;
}

Stl::Stl(std::string const &fileName) {
  gsl::owner<FILE *> fp = this->openCountFacets(fileName);
  if (fp == nullptr) {
    return;
  }
  allocate();
  m_initialized = read(fp, 0, true);
  fclose(fp);
}

void Stl::allocate() {
  //  Allocate memory for the entire .STL file.
  m_facets.assign(m_stats.number_of_facets, Facet());
  // Allocate memory for the neighbors list.
  m_neighbors.assign(m_stats.number_of_facets, Neighbors());
}

void Stl::saveFacetStats(Facet const &facet, bool &first) {
  // While we are going through all of the facets, let's find the
  // maximum and minimum values for x, y, and z

  if (first) {
    // Initialize the max and min values the first time through
    m_stats.min = facet.vertex[0];
    m_stats.max = facet.vertex[0];
    Vertex diff = (facet.vertex[1] - facet.vertex[0]).cwiseAbs();
    m_stats.shortest_edge = std::max(diff(0), std::max(diff(1), diff(2)));
    first = false;
  }

  // Now find the max and min values.
  for (const auto &i : facet.vertex) {
    m_stats.min = m_stats.min.cwiseMin(i);
    m_stats.max = m_stats.max.cwiseMax(i);
  }
}

#include <Eigen/src/Core/util/ReenableStupidWarnings.h>
