#pragma once
#include <iomanip>
#include <vector>

#include <spdlog/spdlog.h>

#include <linc/mesh.h++>
#include <linc/units.h++>
#include <linc/vertex.h++>

using EdgePointIndices = std::array<size_t, 2>;

class MeshClipper {
public:
  struct Edge {
    EdgePointIndices m_pointIndices{INVALID_INDEX, INVALID_INDEX};
    std::vector<size_t> m_users{};

    Edge &operator=(Edge const &other) {
      m_pointIndices = other.m_pointIndices;
      m_users = other.m_users;
      return *this;
    }

    Edge(MeshClipper::Edge const &other) = default;
    Edge(EdgePointIndices pointIndices)
        : m_pointIndices(std::move(pointIndices)) {}
    Edge(EdgePointIndices pointIndices, std::vector<size_t> users)
        : m_pointIndices(std::move(pointIndices)), m_users(std::move(users)) {}

    friend std::ostream &operator<<(std::ostream &os,
                                    MeshClipper::Edge const &edge) {
      os << "{ " << std::setiosflags(std::ios::right) << std::setw(3)
         << std::setfill(' ') << edge.m_pointIndices[0] << " (" << std::setw(3)
         << edge.m_pointIndices[1] << ") users: (";
      std::string delim{""};
      if (edge.m_users.size() == 1) {
        delim = "    ";
      }
      for (auto const &user : edge.m_users) {
        os << delim << std::setw(3) << user;
        delim = ",";
      }
      return os << ") }";
    }
  };

  struct Triangle {
    struct Opening {
      size_t startPointIndex = INVALID_INDEX;
      size_t endPointIndex = INVALID_INDEX;
    };

    std::vector<Edge> &m_edges;
    std::array<size_t, 3> m_edgeIndices{INVALID_INDEX, INVALID_INDEX,
                                        INVALID_INDEX};
    bool m_visible = true;

    Edge &edge0() const { return m_edges[m_edgeIndices[0]]; }
    Edge &edge1() const { return m_edges[m_edgeIndices[1]]; }
    Edge &edge2() const { return m_edges[m_edgeIndices[2]]; }

    Triangle &operator=(Triangle const &other) {
      m_edges = other.m_edges;
      m_edgeIndices = other.m_edgeIndices;
      m_visible = other.m_visible;
      return *this;
    }

    friend std::ostream &operator<<(std::ostream &os,
                                    Triangle const &triangle) {
      os << '{';
      if (triangle.m_edgeIndices[0] != INVALID_INDEX) {
        os << triangle.edge0();
      }
      if (triangle.m_edgeIndices[1] != INVALID_INDEX) {
        os << ",\n " << triangle.edge1();
      }
      if (triangle.m_edgeIndices[2] != INVALID_INDEX) {
        os << ",\n " << triangle.edge2();
      }
      return os << '}';
    }

    Opening getOpening() const {
      size_t startPointIndex = INVALID_INDEX;
      size_t endPointIndex = INVALID_INDEX;
      std::array<std::size_t, 6> indices{INVALID_INDEX};
      if (m_edgeIndices[0] != INVALID_INDEX) {
        indices[0] = edge0().m_pointIndices[0];
        indices[1] = edge0().m_pointIndices[1];
      }
      if (m_edgeIndices[1] != INVALID_INDEX) {
        indices[2] = edge1().m_pointIndices[0];
        indices[3] = edge1().m_pointIndices[1];
      }
      if (m_edgeIndices[2] != INVALID_INDEX) {
        indices[4] = edge2().m_pointIndices[0];
        indices[5] = edge2().m_pointIndices[1];
      }

      if (m_edgeIndices[0] != INVALID_INDEX) {
        if (indices[0] != indices[1] and indices[0] != indices[2] and
            indices[0] != indices[3] and indices[0] != indices[4] and
            indices[0] != indices[5]) {
          startPointIndex = indices[0];
        }
        if (indices[1] != indices[0] and indices[1] != indices[2] and
            indices[1] != indices[3] and indices[1] != indices[4] and
            indices[1] != indices[5]) {
          if (startPointIndex == INVALID_INDEX) {
            startPointIndex = indices[1];
          } else {
            endPointIndex = indices[1];
          }
        }
      }
      if (m_edgeIndices[1] != INVALID_INDEX) {
        if (indices[2] != indices[0] and indices[2] != indices[1] and
            indices[2] != indices[3] and indices[2] != indices[4] and
            indices[2] != indices[5]) {
          if (startPointIndex == INVALID_INDEX) {
            startPointIndex = indices[2];
          } else {
            endPointIndex = indices[2];
          }
        }
        if (indices[3] != indices[0] and indices[3] != indices[1] and
            indices[3] != indices[2] and indices[3] != indices[4] and
            indices[3] != indices[5]) {
          if (startPointIndex == INVALID_INDEX) {
            startPointIndex = indices[3];
          } else {
            endPointIndex = indices[3];
          }
        }
      }
      if (m_edgeIndices[2] != INVALID_INDEX) {
        if (indices[4] != indices[0] and indices[4] != indices[1] and
            indices[4] != indices[2] and indices[4] != indices[3] and
            indices[4] != indices[5]) {
          if (startPointIndex == INVALID_INDEX) {
            startPointIndex = indices[4];
          } else {
            endPointIndex = indices[4];
          }
        }
        if (indices[5] != indices[0] and indices[5] != indices[1] and
            indices[5] != indices[2] and indices[5] != indices[3] and
            indices[5] != indices[4]) {
          if (startPointIndex == INVALID_INDEX) {
            startPointIndex = indices[5];
          } else {
            endPointIndex = indices[5];
          }
        }
      }
      return {startPointIndex, endPointIndex};
    }
  };

  std::vector<Vertex> m_points{};
  std::vector<Edge> m_edges{};
  std::vector<Triangle> m_triangles{};

  MeshClipper(Mesh const &mesh);

  Vertex &point0(Edge const &edge) { return m_points[edge.m_pointIndices[0]]; }
  Vertex &point1(Edge const &edge) { return m_points[edge.m_pointIndices[1]]; }

  double maxHeight() const;
  double softMaxHeight(std::vector<bool> const &visible) const;
  double minHeight() const;
  size_t countVisibleTriangles() const;
  void writeBinaryStl(std::string const &fileName) const;
  std::vector<Vertex> getVerticesAt(Millimeter height) const;
  std::vector<bool> getPointsVisibility(Millimeter zCut);
  void adjustEdges(Millimeter zCut, std::vector<bool> &pointVisibility);
  void adjustTriangles();
  void propagateInvisibilityToUsers(size_t edgeIndex, Edge const &edge);
  Vertex pointAlong(Edge const &edge, double t) const;
  void close2EdgeOpenTriangle(size_t triangleIndex,
                              Triangle::Opening const &opening);
  void close3EdgeOpenTriangle(size_t triangleIndex,
                              Triangle::Opening const &opening);
  std::vector<bool> softClip(Millimeter zCut);
};

inline bool operator<(MeshClipper::Edge const &lhs,
                      MeshClipper::Edge const &rhs) {
  auto const &[lhsPointLow, lhsPointHigh] =
      std::minmax(lhs.m_pointIndices[0], lhs.m_pointIndices[1]);
  auto const &[rhsPointLow, rhsPointHigh] =
      std::minmax(rhs.m_pointIndices[0], rhs.m_pointIndices[1]);

  if (lhsPointLow < rhsPointLow) {
    return true;
  }
  if (rhsPointLow < lhsPointLow) {
    return false;
  }
  if (lhsPointHigh < rhsPointHigh) {
    return true;
  }
  if (rhsPointHigh < lhsPointHigh) {
    return false;
  }
  return false;
}

inline bool operator==(MeshClipper::Edge const &lhs,
                       MeshClipper::Edge const &rhs) {
  return (lhs.m_pointIndices[0] == rhs.m_pointIndices[0] and
          lhs.m_pointIndices[1] == rhs.m_pointIndices[1]) or
         (lhs.m_pointIndices[1] == rhs.m_pointIndices[0] and
          lhs.m_pointIndices[0] == rhs.m_pointIndices[1]);
}

inline bool operator!=(MeshClipper::Edge const &lhs,
                       MeshClipper::Edge const &rhs) {
  return not(lhs == rhs);
}

inline bool operator<(MeshClipper::Triangle const &lhs,
                      MeshClipper::Triangle const &rhs) {
  std::set<MeshClipper::Edge> lhsEdges{lhs.edge0(), lhs.edge1(), lhs.edge2()};
  std::set<MeshClipper::Edge> rhsEdges{rhs.edge0(), rhs.edge1(), rhs.edge2()};
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

inline bool operator==(MeshClipper::Triangle const &lhs,
                       MeshClipper::Triangle const &rhs) {
  return (lhs.m_edgeIndices[0] == rhs.m_edgeIndices[0] and
          ((lhs.m_edgeIndices[1] == rhs.m_edgeIndices[1] and
            lhs.m_edgeIndices[2] == rhs.m_edgeIndices[2]) or
           (lhs.m_edgeIndices[1] == rhs.m_edgeIndices[2] and
            lhs.m_edgeIndices[2] == rhs.m_edgeIndices[1]))) or
         (lhs.m_edgeIndices[0] == rhs.m_edgeIndices[1] and
          ((lhs.m_edgeIndices[1] == rhs.m_edgeIndices[0] and
            lhs.m_edgeIndices[2] == rhs.m_edgeIndices[2]) or
           (lhs.m_edgeIndices[1] == rhs.m_edgeIndices[2] and
            lhs.m_edgeIndices[2] == rhs.m_edgeIndices[0]))) or
         (lhs.m_edgeIndices[0] == rhs.m_edgeIndices[2] and
          ((lhs.m_edgeIndices[1] == rhs.m_edgeIndices[1] and
            lhs.m_edgeIndices[2] == rhs.m_edgeIndices[0]) or
           (lhs.m_edgeIndices[1] == rhs.m_edgeIndices[0] and
            lhs.m_edgeIndices[2] == rhs.m_edgeIndices[1])));
}

inline bool operator!=(MeshClipper::Triangle const &lhs,
                       MeshClipper::Triangle const &rhs) {
  return not(lhs == rhs);
}

inline auto operator<<(std::ostream &os,
                       std::vector<MeshClipper::Edge> const &edges)
    -> std::ostream & {

  std::string delim{""};
  os << "\n{";
  for (auto const &edge : edges) {
    os << delim << edge;
    delim = ",\n ";
  }
  os << "}\n";
  return os;
}

inline auto operator<<(std::ostream &os,
                       std::vector<MeshClipper::Triangle> triangles)
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

template <> struct fmt::formatter<MeshClipper::Edge> {
  constexpr auto parse(format_parse_context &ctx) {
    // [ctx.begin(), ctx.end()) is a character range that contains a part of
    // the format string starting from the format specifications to be parsed,
    // e.g. in
    //
    //   fmt::format("{:f} - point of interest", point{1, 2});
    //
    // the range will contain "f} - point of interest". The formatter should
    // parse specifiers until '}' or the end of the range. In this example
    // the formatter should parse the 'f' specifier and return an iterator
    // pointing to '}'.

    // Parse the presentation format and store it in the formatter:
    auto it = ctx.begin();
    auto end = ctx.end();

    if (it != end and *it != '}') {
      it++;
    }
    if (it != end and *it != '}') {
      it++;
    }

    // Check if reached the end of the range:
    if (it != end and *it != '}') {
      throw format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }

  template <typename FormatContext>
  auto format(const MeshClipper::Edge &edge, FormatContext &ctx) {
    // ctx.out() is an output iterator to write to.
    return format_to(ctx.out(), "{{({}) --- ({})}}", edge.m_pointIndices[0],
                     edge.m_pointIndices[1]);
  }
};
