#pragma once

#include <linc/stl.h++>
#include <string>

struct TriangleMesh {
  struct IndexedTriangleSet {
    IndexedTriangleSet() {}
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

  Stl m_stl;
  IndexedTriangleSet m_indexedTriangleSet;

  TriangleMesh() = delete;
  TriangleMesh(std::string fileName) : m_stl(fileName) {}
  TriangleMesh(TriangleMesh const &) = default;

  static bool writeBinaryStl(std::string const &fileName);
  bool isGood() const { return m_stl.m_initialized; }
};
