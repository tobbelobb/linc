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
  struct Point {
    Vertex m_vertex;
    bool m_visible = true;

    Point(double x, double y, double z) : m_vertex(x, y, z) {}
    Point(Vertex vertex) : m_vertex(vertex) {}
    Point(Vertex vertex, bool visible) : m_vertex(vertex), m_visible(visible) {}

    auto x() const { return m_vertex.x(); }
    auto y() const { return m_vertex.y(); }
    auto z() const { return m_vertex.z(); }

    bool operator<(Point const &other) const {
      return this->m_vertex < other.m_vertex;
    }
    bool operator==(Point const &other) const {
      return not(this->m_vertex < other.m_vertex) and
             not(other.m_vertex < this->m_vertex);
    };

    friend std::ostream &operator<<(std::ostream &os, Point const &point) {
      return os << point.m_vertex;
    }
  };

  struct Edge {
    std::vector<Point> &m_points;
    EdgePointIndices m_pointIndices{INVALID_INDEX, INVALID_INDEX};
    std::vector<size_t> m_users{};
    bool m_visible = true;

    Point &point0() const { return m_points[m_pointIndices[0]]; }
    Point &point1() const { return m_points[m_pointIndices[1]]; }

    Edge &operator=(Edge const &other) {
      m_points = other.m_points;
      m_pointIndices = other.m_pointIndices;
      m_users = other.m_users;
      m_visible = other.m_visible;
      return *this;
    }

    Edge(MeshClipper::Edge const &other) = default;
    Edge(std::vector<Point> &points) : m_points(points) {}
    Edge(std::vector<Point> &points, EdgePointIndices pointIndices)
        : m_points(points), m_pointIndices(pointIndices) {}
    Edge(std::vector<Point> &points, EdgePointIndices pointIndices,
         std::vector<size_t> users)
        : m_points(points), m_pointIndices(pointIndices), m_users(users) {}
    Edge(std::vector<Point> &points, EdgePointIndices pointIndices,
         std::vector<size_t> users, bool visible)
        : m_points(points), m_pointIndices(pointIndices), m_users(users),
          m_visible(visible) {}

    bool operator<(Edge const &other) const {
      auto const &[lhsPointLow, lhsPointHigh] =
          std::minmax(this->point0(), this->point1());
      auto const &[rhsPointLow, rhsPointHigh] =
          std::minmax(other.point0(), other.point1());

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

    bool operator==(Edge const &other) const {
      return not(*this < other) and not(other < *this);
    }

    bool fullEquals(Edge const &other) const {
      return *this == other and m_users == other.m_users and
             m_visible == other.m_visible;
    }

    bool operator!=(Edge const &other) const { return not(*this == other); }

    friend std::ostream &operator<<(std::ostream &os, Edge const &edge) {
      Point const &p0 = edge.point0();
      Point const &p1 = edge.point1();
      os << '{' << p0 << " (" << std::setiosflags(std::ios::right)
         << std::setw(3) << std::setfill(' ') << edge.m_pointIndices[0]
         << ") --- " << p1 << " (" << std::setw(3) << edge.m_pointIndices[1]
         << ") users: (";
      std::string delim{""};
      if (edge.m_users.size() == 1) {
        delim = "    ";
      }
      for (auto const &user : edge.m_users) {
        os << delim << std::setw(3) << user;
        delim = ",";
      }
      return os << ") " << (edge.m_visible ? "  Visible" : "Invisible") << '}';
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

    bool operator<(Triangle const &other) const {
      std::set<MeshClipper::Edge> lhsEdges{this->edge0(), this->edge1(),
                                           this->edge2()};
      std::set<MeshClipper::Edge> rhsEdges{other.edge0(), other.edge1(),
                                           other.edge2()};
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

    bool operator==(Triangle const &other) const {
      return not(*this < other) and not(other < *this);
    }

    bool operator!=(Triangle const &other) const { return not(*this == other); }

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

    bool fullEquals(Triangle const &other) const {
      return *this == other and m_visible == other.m_visible;
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

  std::vector<Point> m_points{};
  std::vector<Edge> m_edges{};
  std::vector<Triangle> m_triangles{};

  MeshClipper(Mesh const &mesh);

  double maxHeight() const;
  double softMaxHeight() const;
  double minHeight() const;
  size_t countVisiblePoints() const;
  size_t countVisibleEdges() const;
  size_t countVisibleTriangles() const;
  bool isAllPointsVisible() const;
  void writeBinaryStl(std::string const &fileName) const;
  std::vector<Vertex> getVerticesAt(Millimeter height) const;
  void setPointsVisibility(Millimeter zCut);
  void adjustEdges(Millimeter zCut);
  void adjustTriangles();
  void propagateInvisibilityToUsers(size_t edgeIndex, Edge const &edge);
  void close2EdgeOpenTriangle(size_t triangleIndex,
                              Triangle::Opening const &opening);
  void close3EdgeOpenTriangle(size_t triangleIndex,
                              Triangle::Opening const &opening);
  double softClip(Millimeter zCut);
};

inline auto operator<<(std::ostream &os,
                       std::vector<MeshClipper::Point> const &points)
    -> std::ostream & {

  std::string delim{""};
  os << "\n{";
  for (auto const &point : points) {
    os << delim << point;
    delim = ",\n ";
  }
  os << '}';
  return os;
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
    return format_to(
        ctx.out(),
        "{{{:.1f} {:.1f} {:.1f} ({}) --- {:.1f} {:.1f} {:.1f} ({})}}",
        edge.point0().x(), edge.point0().y(), edge.point0().z(),
        edge.m_pointIndices[0], edge.point1().x(), edge.point1().y(),
        edge.point1().z(), edge.m_pointIndices[1]);
  }
};
