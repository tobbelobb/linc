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

#include <Eigen/Geometry>
#include <SI/length.h>
#include <gsl/pointers>

using Millimeter = SI::milli_metre_t<double>;

// Size of the ascii table
auto constexpr ASCII_TABLE_SIZE = 128U;
// Size of the binary STL header, free form.
auto constexpr LABEL_SIZE = 80U;
// Binary STL, sizeof header (free form) + number of faces.
auto constexpr HEADER_SIZE = 84U;
auto constexpr STL_MIN_FILE_SIZE = 284L;
auto constexpr ASCII_LINES_PER_FACET = 7U;
auto constexpr SIZEOF_STL_FACET = 50U;

using Vertex = Eigen::Matrix<double, 3, 1, Eigen::DontAlign>;
using Normal = Eigen::Matrix<double, 3, 1, Eigen::DontAlign>;
using TriangleVertexIndices = Eigen::Matrix<int, 3, 1, Eigen::DontAlign>;

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

  struct Neighbors {
    std::array<int, 3> neighbors = {-1, -1, -1};

    int num_neighbors_missing() const {
      return (neighbors[0] == -1) + (neighbors[1] == -1) + (neighbors[2] == -1);
    }
    int num_neighbors() const { return 3 - num_neighbors_missing(); }
  };

  enum class Type { BINARY, ASCII, INMEMORY, UNKNOWN };

  struct Facet {
    Normal normal;
    std::array<Vertex, 3> vertices;

    friend std::ostream &operator<<(std::ostream &os, Facet const facet) {
      auto formatted = [](auto const &vec) -> std::string {
        std::stringstream ss;
        ss << vec;
        std::string s{ss.str()};
        std::replace(s.begin(), s.end(), '\n', ' ');
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
  std::vector<Neighbors> m_neighbors;
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
  size_t memsize() const {
    return sizeof(*this) + sizeof(Facet) * m_facets.size() +
           sizeof(Neighbors) * m_neighbors.size();
  }

  void clear() {
    m_facets.clear();
    m_neighbors.clear();
    m_stats = {};
    m_initialized = false;
    m_type = Stl::Type::UNKNOWN;
  }

  void allocate(size_t numberOfFacets);
  bool readFacets(FILE *fp);
  bool readBinaryFacets(FILE *fp);
  bool readAsciiFacets(FILE *fp);
  void computeSomeStats();
};

struct indexed_triangle_set {
  indexed_triangle_set() {}

  void clear() {
    indices.clear();
    vertices.clear();
  }

  size_t memsize() const {
    return sizeof(*this) + sizeof(TriangleVertexIndices) * indices.size() +
           sizeof(Vertex) * vertices.size();
  }

  std::vector<TriangleVertexIndices> indices;
  std::vector<Vertex> vertices;
};

extern bool stl_write_ascii(Stl *stl, const char *file, const char *label);
extern bool stl_write_binary(Stl *stl, const char *file, const char *label);
extern void stl_check_facets_exact(Stl *stl);
extern void stl_check_facets_nearby(Stl *stl, float tolerance);
extern void stl_remove_unconnected_facets(Stl *stl);
extern void stl_write_vertex(Stl *stl, int facet, int vertex);
extern void stl_write_facet(Stl *stl, char *label, int facet);
extern void stl_write_neighbor(Stl *stl, int facet);
extern bool stl_write_quad_object(Stl *stl, char *file);
extern void stl_verify_neighbors(Stl *stl);
extern void stl_fill_holes(Stl *stl);
extern void stl_fix_normal_directions(Stl *stl);
extern void stl_fix_normal_values(Stl *stl);
extern void stl_reverse_all_facets(Stl *stl);
extern void stl_translate(Stl *stl, float x, float y, float z);
extern void stl_translate_relative(Stl *stl, float x, float y, float z);
extern void stl_scale_versor(Stl *stl, const Vertex &versor);
inline void stl_scale(Stl *stl, float factor) {
  stl_scale_versor(stl, Vertex(factor, factor, factor));
}
extern void stl_mirror_xy(Stl *stl);
extern void stl_mirror_yz(Stl *stl);
extern void stl_mirror_xz(Stl *stl);

extern void stl_get_size(Stl *stl);

template <typename T>
extern void its_transform(indexed_triangle_set &its, T *trafo3x4) {
  for (Vertex &v_dst : its.vertices) {
    Vertex v_src = v_dst;
    v_dst(0) = T(trafo3x4[0] * v_src(0) + trafo3x4[1] * v_src(1) +
                 trafo3x4[2] * v_src(2) + trafo3x4[3]);
    v_dst(1) = T(trafo3x4[4] * v_src(0) + trafo3x4[5] * v_src(1) +
                 trafo3x4[6] * v_src(2) + trafo3x4[7]);
    v_dst(2) = T(trafo3x4[8] * v_src(0) + trafo3x4[9] * v_src(1) +
                 trafo3x4[10] * v_src(2) + trafo3x4[11]);
  }
}

extern void stl_generate_shared_vertices(Stl *stl, indexed_triangle_set &its);

extern bool stl_write_dxf(Stl *stl, const char *file, char *label);
inline void stl_calculate_normal(Normal &normal, Stl::Facet const &facet) {
  normal = (facet.vertices[1] - facet.vertices[0])
               .cross(facet.vertices[2] - facet.vertices[0]);
}
inline void Normalize_vector(Normal &normal) {
  double length = normal.cast<double>().norm();
  if (length < 0.000000000001)
    normal = Normal::Zero();
  else
    normal *= float(1.0 / length);
}
