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
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

#include <gsl/pointers>

#include <linc/vertex.h++>

// Size of the ascii table
auto constexpr ASCII_TABLE_SIZE = 128U;
// Size of the binary STL header, free form.
auto constexpr LABEL_SIZE = 80U;
auto constexpr FACET_COUNTER_SIZE = 4U;
// Binary STL, sizeof header (free form) + number of facets.
auto constexpr HEADER_SIZE = 84U;
auto constexpr STL_MIN_FILE_SIZE = 284L;
auto constexpr ASCII_LINES_PER_FACET = 7U;
auto constexpr SIZEOF_STL_FACET = 50U;

// We need this stuff to read/write binary stls portably and safely
using SmallVertex = std::array<float, 3>;
using SmallNormal = std::array<float, 3>;
struct SmallFacet {
  SmallNormal normal;
  std::array<SmallVertex, 3> vertices;
  std::array<std::byte, 2> extra;
};

// If SmallFacet is not trivially copyable, then we cannot predictably
// write it into a binary file.
static_assert(std::is_trivially_copyable<SmallNormal>::value, /* NOLINT */
              "SmallNormal is not trivially copyable");
static_assert(std::is_trivially_copyable<SmallVertex>::value, /* NOLINT */
              "SmallVertex is not trivially copyable");
static_assert(
    std::is_trivially_copyable<std::array<SmallVertex, 3>>::value, /* NOLINT */
    "Array of SmallVertex is not trivially copyable");
static_assert(std::is_trivially_copyable<std::byte>::value, /* NOLINT */
              "std::byte is not trivially copyable");
static_assert(
    std::is_trivially_copyable<std::array<std::byte, 2>>::value, /* NOLINT */
    "Array of std::byte is not trivially copyable");
static_assert(std::is_trivially_copyable<SmallFacet>::value, /* NOLINT */
              "SmallFacet is not trivially copyable");
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

class Stl {
public:
  // Types
  struct Stats {
    Vertex max = Vertex::Zero();
    Vertex min = Vertex::Zero();
    Vertex size = Vertex::Zero();
    double bounding_diameter = 0.0F;
    double shortest_edge = 0.0F;
    double volume = -1.0F;
  };

  enum class Type { BINARY, ASCII, INMEMORY, UNKNOWN };

  struct Facet {
    Normal normal;
    std::array<Vertex, 3> vertices;

    friend std::ostream &operator<<(std::ostream &os, Facet const facet) {
      auto formatted = [](auto const &vec) -> std::string {
        std::stringstream ss;
        ss << vec[0] << ' ' << vec[1] << ' ' << vec[2];
        std::string s{ss.str()};
        return s;
      };

      return os << "  facet normal " << formatted(facet.normal)
                << "\n    outer loop\n      vertex "
                << formatted(facet.vertices[0]) << "\n      vertex "
                << formatted(facet.vertices[1]) << "\n      vertex "
                << formatted(facet.vertices[2])
                << "\n    endloop\n  endfacet\n";
    }
  };

  // public members
  Type m_type = Stl::Type::UNKNOWN;
  bool m_initialized = false;
  std::vector<Facet> m_facets;
  Stats m_stats;

  Stl() = delete;
  explicit Stl(std::string const &fileName);
  Stl cut(Millimeter) const;

  void toAscii(std::ostream &os) const {
    os << "solid linc-model\n";
    for (Stl::Facet const &facet : m_facets) {
      os << facet;
    }
    os << "endsolid linc-model";
  }

  friend std::ostream &operator<<(std::ostream &os, Type const type) {
    switch (type) {
    case (Type::BINARY):
      return os << "BINARY";
    case (Type::ASCII):
      return os << "ASCII";
    case (Type::INMEMORY):
      return os << "INMEMORY";
    case (Type::UNKNOWN):
      return os << "UNKNOWN";
    default:
      return os;
    }
  }

private:
  void clear() {
    m_facets.clear();
    m_stats = {};
    m_initialized = false;
    m_type = Stl::Type::UNKNOWN;
  }

  void allocate(std::size_t numberOfFacets);
  bool readFacets(FILE *fp);
  bool readBinaryFacets(FILE *fp);
  bool readAsciiFacets(FILE *fp);
  void computeSomeStats();
};

static_assert(sizeof(Stl::Facet) >= SIZEOF_STL_FACET,
              "size of SmallFacet is too small");
