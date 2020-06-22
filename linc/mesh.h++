#pragma once

#include <iostream>
#include <memory>
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

struct SuperEdge;
struct EdgeCompare {
  bool operator()(Edge const &lhs, Edge const &rhs) const {

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

// Describing a face by indexing into a sequence of edges.
// A triangle must have exactly three edges.
using TriangleEdgePtrs = std::array<std::unique_ptr<SuperEdge>, 3>;

// An edge must be used by exactly two triangles.
struct Triangle {
  TriangleEdgePtrs m_edgePtrs;
  Normal normal = Normal::Zero();
};

bool operator<(SuperEdge const &lhs, SuperEdge const &rhs);
struct TriangleCompare {
  bool operator()(Triangle const &lhs, Triangle const &rhs) const {
    std::set<SuperEdge, EdgeCompare> lhsEdges{};
    std::set<SuperEdge, EdgeCompare> rhsEdges{};
    for (auto const &ptr : lhs.m_edgePtrs) {
      lhsEdges.insert(*ptr);
    }
    for (auto const &ptr : rhs.m_edgePtrs) {
      rhsEdges.insert(*ptr);
    }
    auto lhsIt = lhsEdges.begin();
    auto rhsIt = rhsEdges.begin();
    while (lhsIt != lhsEdges.end() and rhsIt != rhsEdges.end()) {
      if (*lhsIt < *rhsIt) {
        return true;
      }
      if (*rhsIt < *lhsIt) {
        return false;
      }
      ++lhsIt;
      ++rhsIt;
    }
    return false;
  }
};

inline auto operator<(Triangle const &lhs, Triangle const &rhs) -> bool {
  TriangleCompare t{};
  return t(lhs, rhs);
}

inline auto operator==(Triangle const &lhs, Triangle const &rhs) -> bool {
  return not(lhs < rhs) and not(rhs < lhs);
}

using EdgeUserIterators = std::set<Triangle, TriangleCompare>;

// A super edge knows its vertices and which faces that uses itself
inline std::ostream &operator<<(std::ostream &os, Triangle const &triangle);
struct SuperEdge : public Edge {
  EdgeUserIterators m_users;

  SuperEdge(EdgeByVertexIterators const &e) : Edge(e) {}

  SuperEdge(EdgeByVertexIterators const &e, EdgeUserIterators const &users)
      : Edge(e), m_users(users) {}

  friend std::ostream &operator<<(std::ostream &os,
                                  SuperEdge const &superEdge) {
    os << (*superEdge.m_vertexIterators.at(0)).x() << ' '
       << (*superEdge.m_vertexIterators.at(0)).y() << ' '
       << (*superEdge.m_vertexIterators.at(0)).z() << " --- "
       << (*superEdge.m_vertexIterators.at(1)).x() << ' '
       << (*superEdge.m_vertexIterators.at(1)).y() << ' '
       << (*superEdge.m_vertexIterators.at(1)).z();
    return os;
  }
};

inline auto operator<(SuperEdge const &lhs, SuperEdge const &rhs) -> bool {
  EdgeCompare s{};
  return s(lhs, rhs);
}

inline auto operator==(SuperEdge const &lhs, SuperEdge const &rhs) -> bool {
  return not(lhs < rhs) and not(rhs < lhs);
}

inline auto operator<<(std::ostream &os,
                       std::set<SuperEdge, EdgeCompare> superEdges)
    -> std::ostream & {
  std::string delim{""};
  os << '{';
  for (auto const &superEdge : superEdges) {
    os << delim << superEdge;
    delim = ", ";
  }
  os << '}';
  return os;
}

inline auto operator<<(std::ostream &os, Triangle const &triangle)
    -> std::ostream & {
  std::string delim{""};
  os << '{';
  for (auto const &edgePtr : triangle.m_edgePtrs) {
    os << delim << *edgePtr;
    delim = ", ";
  }
  os << '}';
  return os;
}

inline auto operator<<(std::ostream &os,
                       std::set<Triangle, TriangleCompare> triangles)
    -> std::ostream & {
  std::string delim{""};
  os << '{';
  for (auto const &triangle : triangles) {
    os << delim << triangle;
    delim = ", ";
  }
  os << '}';
  return os;
}

class Mesh {
public:
  std::set<Vertex, VertexCompare> m_vertices{};
  std::set<SuperEdge, EdgeCompare> m_superEdges{};
  std::set<Triangle, TriangleCompare> m_triangles{};

  Mesh(Stl const &stl);
};
