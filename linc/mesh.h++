#pragma once

#include <iostream>
#include <set>
#include <vector>

#include <linc/mesh-types.h++>
#include <linc/stl.h++>

// Describing an edge by indexing into a sequence of vertices.
// An edge must have exactly two endpoints.
using EdgeByVertexIterators = std::array<std::set<Vertex>::iterator, 2>;

// An edge knows about which vertexes it's made of
struct Edge {
  EdgeByVertexIterators m_vertexIterators{};
};

// Describing a face by indexing into a sequence of edges.
// A triangle must have exactly three edges.
using TriangleEdgeIterators = std::array<std::set<Edge>::iterator, 3>;

// An edge must be used by exactly two triangles.
struct Triangle {
  TriangleEdgeIterators m_edgeIterators;
  Normal normal = Normal::Zero();
};

using EdgeUserIterators = std::array<std::vector<Triangle>::iterator, 2>;

// A super edge knows its vertices and which faces that uses itself
struct SuperEdge : public Edge {
  EdgeUserIterators m_users{};
  friend std::ostream &operator<<(std::ostream &os,
                                  SuperEdge const &superEdge) {
    os << (*superEdge.m_vertexIterators.at(0)).x() << ' '
       << (*superEdge.m_vertexIterators.at(0)).y() << ' '
       << (*superEdge.m_vertexIterators.at(0)).z() << " --- "
       << (*superEdge.m_vertexIterators.at(1)).x() << ' '
       << (*superEdge.m_vertexIterators.at(1)).y() << ' '
       << (*superEdge.m_vertexIterators.at(1)).z() << '\n';
    return os;
  }
};

struct SuperEdgeCompare {
  bool operator()(SuperEdge const &lhs, SuperEdge const &rhs) const {

    Vertex const lhsVertex0 = *lhs.m_vertexIterators.at(0);
    Vertex const lhsVertex1 = *lhs.m_vertexIterators.at(1);
    Vertex const rhsVertex0 = *rhs.m_vertexIterators.at(0);
    Vertex const rhsVertex1 = *rhs.m_vertexIterators.at(1);
    Vertex const lhsVertexLow =
        (lhsVertex0 < lhsVertex1) ? lhsVertex0 : lhsVertex1;
    Vertex const rhsVertexLow =
        (rhsVertex0 < rhsVertex1) ? rhsVertex0 : rhsVertex1;
    Vertex const lhsVertexHigh =
        (lhsVertex0 < lhsVertex1) ? lhsVertex1 : lhsVertex0;
    Vertex const rhsVertexHigh =
        (rhsVertex0 < rhsVertex1) ? rhsVertex1 : rhsVertex0;

    if (lhsVertexLow < rhsVertexLow) {
      return true;
    }
    if (rhsVertexLow < lhsVertexLow) {
      return false;
    }
    if (lhsVertexHigh < rhsVertexHigh) {
      return true;
    }
    if (rhsVertexHigh < lhsVertexHigh) {
      return false;
    }
    return false;
  }
};

inline auto operator<<(std::ostream &s,
                       std::set<SuperEdge, SuperEdgeCompare> superEdges)
    -> std::ostream & {
  (void)superEdges;

  for (auto const &superEdge : superEdges) {
    s << superEdge << '\n';
  }
  return s;
}

class Mesh {
public:
  std::set<Vertex, VertexCompare> m_vertices{};
  std::set<SuperEdge, SuperEdgeCompare> m_edges{};
  std::vector<Triangle> m_triangles{};

  Mesh(Stl const &stl);
};
