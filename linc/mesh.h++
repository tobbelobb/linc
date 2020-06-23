#pragma once

#include <iostream>
#include <memory>
#include <set>
#include <vector>

#include <linc/mesh-types.h++>
#include <linc/stl.h++>

// Describing an edge by indexing into a sequence of vertices.
// An edge must have exactly two endpoints.
using EdgeVertexIndices = std::array<size_t, 2>;
// Pointers into triangle vector
using EdgeUsers = std::vector<size_t>;

// An edge knows about which vertexes it's made of
struct Edge {
  std::vector<Vertex> &m_vertices;
  EdgeVertexIndices m_vertexIndices{INVALID_INDEX, INVALID_INDEX};
  EdgeUsers m_users{};

  Edge() = delete;
  Edge(std::vector<Vertex> &vertices, EdgeVertexIndices const &vertexIndices)
      : m_vertices(vertices), m_vertexIndices(vertexIndices) {}

  Edge(std::vector<Vertex> &vertices, EdgeVertexIndices const &vertexIndices,
       EdgeUsers const &users)
      : m_vertices(vertices), m_vertexIndices(vertexIndices), m_users(users) {}

  Vertex &vertex0() const { return m_vertices[m_vertexIndices[0]]; }

  Vertex &vertex1() const { return m_vertices[m_vertexIndices[1]]; }

  friend std::ostream &operator<<(std::ostream &os, Edge const &edge) {
    Vertex const &v0 = edge.vertex0();
    Vertex const &v1 = edge.vertex1();
    os << v0.x() << ' ' << v0.y() << ' ' << v0.z() << "---" << v1.x() << ' '
       << v1.y() << ' ' << v1.z() << " users: (";
    std::string delim{""};
    for (auto const &user : edge.m_users) {
      os << delim << user;
      delim = ", ";
    }
    os << ')';
    return os;
  }
};

struct EdgeCompare {
  bool operator()(Edge const &lhs, Edge const &rhs) const {
    // Cannot use std::min and std::max here
    // Because Eigen plays very badly with STL algorithms
    // Why do we use Eigen again?
    Vertex const &lhsVertexLow =
        lhs.vertex0() < lhs.vertex1() ? lhs.vertex0() : lhs.vertex1();
    Vertex const &rhsVertexLow =
        rhs.vertex0() < rhs.vertex1() ? rhs.vertex0() : rhs.vertex1();
    Vertex const &lhsVertexHigh =
        lhs.vertex1() < lhs.vertex0() ? lhs.vertex0() : lhs.vertex1();
    Vertex const &rhsVertexHigh =
        rhs.vertex1() < rhs.vertex0() ? rhs.vertex0() : rhs.vertex1();

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

inline auto operator<(Edge const &lhs, Edge const &rhs) -> bool {
  EdgeCompare comparator{};
  return comparator(lhs, rhs);
}

inline auto operator==(Edge const &lhs, Edge const &rhs) -> bool {
  return not(lhs < rhs) and not(rhs < lhs) and lhs.m_users == rhs.m_users;
}

inline auto operator!=(Edge const &lhs, Edge const &rhs) -> bool {
  return not(lhs == rhs);
}

inline auto operator<<(std::ostream &os, std::vector<Edge> const &edges)
    -> std::ostream & {
  std::string delim{""};
  os << '{';
  for (auto const &edge : edges) {
    os << delim << edge;
    delim = ", ";
  }
  os << '}';
  return os;
}

using TriangleEdgeIndices = std::array<size_t, 3>;

struct Triangle {
  std::vector<Edge> &m_edges;
  TriangleEdgeIndices m_edgeIndices{INVALID_INDEX, INVALID_INDEX,
                                    INVALID_INDEX};
  Normal normal = Normal::Zero();

  Triangle() = delete;
  Triangle(std::vector<Edge> &edges, TriangleEdgeIndices edgeIndices)
      : m_edges(edges), m_edgeIndices(edgeIndices) {}

  Edge &edge0() const { return m_edges[m_edgeIndices[0]]; }
  Edge &edge1() const { return m_edges[m_edgeIndices[1]]; }
  Edge &edge2() const { return m_edges[m_edgeIndices[2]]; }

  friend auto operator<<(std::ostream &os, Triangle const &triangle)
      -> std::ostream & {
    return os << '{' << triangle.edge0() << ", " << triangle.edge1() << ", "
              << triangle.edge2() << '}';
  }
};

struct TriangleCompare {
  bool operator()(Triangle const &lhs, Triangle const &rhs) const {
    std::set<Edge> lhsEdges{lhs.edge0(), lhs.edge1(), lhs.edge2()};
    std::set<Edge> rhsEdges{rhs.edge0(), rhs.edge1(), rhs.edge2()};
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

inline auto operator!=(Triangle const &lhs, Triangle const &rhs) -> bool {
  return not(lhs == rhs);
}

inline auto operator<<(std::ostream &os, std::vector<Triangle> triangles)
    -> std::ostream & {
  std::string delim{""};
  os << "\n{";
  for (auto const &triangle : triangles) {
    os << delim << triangle;
    delim = ",\n ";
  }
  os << "}\n";
  return os;
}

class Mesh {
public:
  std::vector<Vertex> m_vertices{};
  std::vector<Edge> m_edges{};
  std::vector<Triangle> m_triangles{};

  Mesh(Stl const &stl);
};
