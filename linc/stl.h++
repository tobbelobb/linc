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

#include <cstddef>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>

#include <Eigen/Geometry>
#include <gsl/pointers>

// Size of the ascii table
auto constexpr ASCII_TABLE_SIZE = 128U;
// Size of the binary STL header, free form.
auto constexpr LABEL_SIZE = 80U;
// Binary STL, sizeof header (free form) + number of faces.
auto constexpr HEADER_SIZE = 84U;
auto constexpr STL_MIN_FILE_SIZE = 284L;
auto constexpr ASCII_LINES_PER_FACET = 7U;
auto constexpr SIZEOF_STL_FACET = 50U;

using Vertex = Eigen::Matrix<float, 3, 1, Eigen::DontAlign>;
using Normal = Eigen::Matrix<float, 3, 1, Eigen::DontAlign>;
using TriangleVertexIndices = Eigen::Matrix<int, 3, 1, Eigen::DontAlign>;
static_assert(sizeof(Vertex) == 12, "size of Vertex incorrect");
static_assert(sizeof(Normal) == 12, "size of Normal incorrect");

class Stl {
public:
  struct Facet {
    Normal normal;
    Vertex vertices[3];
    std::byte extra[2];

    friend std::ostream &operator<<(std::ostream &os, Facet const facet) {
      return os << "facet normal \n"
                << facet.normal << "\nouter loop vertex \n"
                << facet.vertices[0] << "\nvertex \n"
                << facet.vertices[1] << "\nvertex \n"
                << facet.vertices[2] << "\nendloop endfacet\n";
    }
  };

  struct Neighbors {
    // Index of a neighbor facet.
    int neighbor[3] = {-1, -1, -1};
    // Index of an opposite vertex at the neighbor face.
    int8_t which_vertex_not[3] = {-1, -1, -1};

    void reset() { *this = {}; }

    int num_neighbors_missing() const {
      return (this->neighbor[0] == -1) + (this->neighbor[1] == -1) +
             (this->neighbor[2] == -1);
    }
    int num_neighbors() const { return 3 - this->num_neighbors_missing(); }
  };

  enum class Type { BINARY, ASCII, INMEMORY };

  friend std::ostream &operator<<(std::ostream &os, Type const type) {
    switch (type) {
    case (Type::BINARY):
      return os << "BINARY";
    case (Type::ASCII):
      return os << "ASCII";
    case (Type::INMEMORY):
      return os << "INMEMORY";
    default:
      return os << "NONE";
    }
  }

  struct Stats {
    char header[LABEL_SIZE + 1] = {0};
    Type type = Stl::Type::BINARY;
    size_t number_of_facets = 0;
    Vertex max = Vertex::Zero();
    Vertex min = Vertex::Zero();
    Vertex size = Vertex::Zero();
    float bounding_diameter = 0.0F;
    float shortest_edge = 0.0F;
    float volume = -1.0F;
  };

  bool m_initialized = false;
  std::vector<Facet> m_facets;
  std::vector<Neighbors> m_neighbors;
  Stats m_stats;

  Stl() = delete;
  explicit Stl(std::string const &fileName);

  void clear() {
    m_facets.clear();
    m_neighbors.clear();
    m_stats = {};
    m_initialized = false;
  }

  size_t memsize() const {
    return sizeof(*this) + sizeof(Facet) * m_facets.size() +
           sizeof(Neighbors) * m_neighbors.size();
  }

private:
  gsl::owner<FILE *> openCountFacets(std::string const &fileName);
  void allocate();
  bool read(FILE *fp, int first_facet, bool first);
  void saveFacetStats(const Facet &facet, bool &first);
};

static_assert(offsetof(Stl::Facet, normal) == 0,
              "stl_facet.normal offset is not 0");
static_assert(offsetof(Stl::Facet, vertices) == 12,
              "stl_facet.vertex offset is not 12");
static_assert(offsetof(Stl::Facet, extra) == 48,
              "stl_facet.extra offset is not 48");
static_assert(sizeof(Stl::Facet) >= SIZEOF_STL_FACET,
              "size of stl_facet is too small");

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
  // FIXME add normals once we get rid of the Stl from TriangleMesh
  // completely. std::vector<Normal>
  // normals
};

extern void stl_stats_out(Stl *stl, FILE *file, char *input_file);
extern bool stl_print_neighbors(Stl *stl, char *file);
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
extern void stl_rotate_x(Stl *stl, float angle);
extern void stl_rotate_y(Stl *stl, float angle);
extern void stl_rotate_z(Stl *stl, float angle);
extern void stl_mirror_xy(Stl *stl);
extern void stl_mirror_yz(Stl *stl);
extern void stl_mirror_xz(Stl *stl);

extern void stl_get_size(Stl *stl);

template <typename T>
inline void stl_transform(
    Stl *stl,
    const Eigen::Transform<T, 3, Eigen::Affine, Eigen::DontAlign> &t) {
  const Eigen::Matrix<T, 3, 3, Eigen::DontAlign> r =
      t.matrix().template block<3, 3>(0, 0).inverse().transpose();
  for (size_t i = 0; i < stl->m_stats.number_of_facets; ++i) {
    Stl::Facet &f = stl->m_facets[i];
    for (size_t j = 0; j < 3; ++j)
      f.vertices[j] =
          (t * f.vertices[j].template cast<T>()).template cast<float>().eval();
    f.normal = (r * f.normal.template cast<T>()).template cast<float>().eval();
  }

  stl_get_size(stl);
}

template <typename T>
inline void stl_transform(Stl *stl,
                          const Eigen::Matrix<T, 3, 3, Eigen::DontAlign> &m) {
  const Eigen::Matrix<T, 3, 3, Eigen::DontAlign> r = m.inverse().transpose();
  for (size_t i = 0; i < stl->m_stats.number_of_facets; ++i) {
    Stl::Facet &f = stl->m_facets[i];
    for (size_t j = 0; j < 3; ++j)
      f.vertices[j] =
          (m * f.vertices[j].template cast<T>()).template cast<float>().eval();
    f.normal = (r * f.normal.template cast<T>()).template cast<float>().eval();
  }

  stl_get_size(stl);
}

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

template <typename T>
inline void its_transform(
    indexed_triangle_set &its,
    const Eigen::Transform<T, 3, Eigen::Affine, Eigen::DontAlign> &t) {
  // const Eigen::Matrix<double, 3, 3, Eigen::DontAlign> r = t.matrix().template
  // block<3, 3>(0, 0);
  for (Vertex &v : its.vertices)
    v = (t * v.template cast<T>()).template cast<float>().eval();
}

template <typename T>
inline void its_transform(indexed_triangle_set &its,
                          const Eigen::Matrix<T, 3, 3, Eigen::DontAlign> &m) {
  for (Vertex &v : its.vertices)
    v = (m * v.template cast<T>()).template cast<float>().eval();
}

extern void its_rotate_x(indexed_triangle_set &its, float angle);
extern void its_rotate_y(indexed_triangle_set &its, float angle);
extern void its_rotate_z(indexed_triangle_set &its, float angle);

extern void stl_generate_shared_vertices(Stl *stl, indexed_triangle_set &its);
extern bool its_write_obj(const indexed_triangle_set &its, const char *file);
extern bool its_write_off(const indexed_triangle_set &its, const char *file);
extern bool its_write_vrml(const indexed_triangle_set &its, const char *file);

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
