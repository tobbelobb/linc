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
    Millimeter m_distance = 0.0;
    unsigned short m_occurs = 0;
    bool m_visible = true;
    bool m_checkIt = true;

    Point(double x, double y, double z) : m_vertex(x, y, z) {}
    Point(Vertex vertex) : m_vertex(vertex) {}
    Point(Vertex vertex, Millimeter distance)
        : m_vertex(vertex), m_distance(distance) {}
    Point(Vertex vertex, Millimeter distance, unsigned short occurs)
        : m_vertex(vertex), m_distance(distance), m_occurs(occurs) {}
    Point(Vertex vertex, Millimeter distance, unsigned short occurs,
          bool visible)
        : m_vertex(vertex), m_distance(distance), m_occurs(occurs),
          m_visible(visible) {}

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
    struct Integrity {
      bool isOpen = false;
      size_t startPointIndex = INVALID_INDEX;
      size_t endPointIndex = INVALID_INDEX;
      size_t numEdges = 0;
    };

    std::vector<Edge> &m_edges;
    // TODO: Remove the fourth edge. It's unused
    std::array<size_t, 4> m_edgeIndices{INVALID_INDEX, INVALID_INDEX,
                                        INVALID_INDEX, INVALID_INDEX};
    Normal m_normal = Normal::Zero();
    bool m_visible = true;
    Integrity m_integrity{};

    Edge &edge0() const { return m_edges[m_edgeIndices[0]]; }
    Edge &edge1() const { return m_edges[m_edgeIndices[1]]; }
    Edge &edge2() const { return m_edges[m_edgeIndices[2]]; }
    Edge &edge3() const { return m_edges[m_edgeIndices[3]]; }

    Triangle &operator=(Triangle const &other) {
      m_edges = other.m_edges;
      m_edgeIndices = other.m_edgeIndices;
      m_normal = other.m_normal;
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
      if (triangle.m_edgeIndices[3] != INVALID_INDEX) {
        os << ",\n  " << triangle.edge3();
      }
      return os << '}';
    }

    bool fullEquals(Triangle const &other) const {
      return *this == other and m_normal == other.m_normal and
             m_visible == other.m_visible;
    }

    void updateIntegrity() {
      bool isOpen = false;
      size_t startPointIndex = INVALID_INDEX;
      size_t endPointIndex = INVALID_INDEX;
      if (m_edgeIndices[0] != INVALID_INDEX) {
        edge0().point0().m_occurs = 0;
        edge0().point1().m_occurs = 0;
      }
      if (m_edgeIndices[1] != INVALID_INDEX) {
        edge1().point0().m_occurs = 0;
        edge1().point1().m_occurs = 0;
      }
      if (m_edgeIndices[2] != INVALID_INDEX) {
        edge2().point0().m_occurs = 0;
        edge2().point1().m_occurs = 0;
      }
      if (m_edgeIndices[3] != INVALID_INDEX) {
        edge3().point0().m_occurs = 0;
        edge3().point1().m_occurs = 0;
      }
      if (m_edgeIndices[0] != INVALID_INDEX) {
        edge0().point0().m_occurs++;
        edge0().point1().m_occurs++;
      }
      if (m_edgeIndices[1] != INVALID_INDEX) {
        edge1().point0().m_occurs++;
        edge1().point1().m_occurs++;
      }
      if (m_edgeIndices[2] != INVALID_INDEX) {
        edge2().point0().m_occurs++;
        edge2().point1().m_occurs++;
      }
      if (m_edgeIndices[3] != INVALID_INDEX) {
        edge3().point0().m_occurs++;
        edge3().point1().m_occurs++;
      }
      if (m_edgeIndices[0] != INVALID_INDEX) {
        if (edge0().point0().m_occurs == 1) {
          isOpen = true;
          startPointIndex = edge0().m_pointIndices[0];
        }
        if (edge0().point1().m_occurs == 1) {
          isOpen = true;
          if (startPointIndex == INVALID_INDEX) {
            startPointIndex = edge0().m_pointIndices[1];
          } else {
            endPointIndex = edge0().m_pointIndices[1];
          }
        }
      }
      if (m_edgeIndices[1] != INVALID_INDEX) {
        if (edge1().point0().m_occurs == 1) {
          isOpen = true;
          if (startPointIndex == INVALID_INDEX) {
            startPointIndex = edge1().m_pointIndices[0];
          } else {
            endPointIndex = edge1().m_pointIndices[0];
          }
        }
        if (edge1().point1().m_occurs == 1) {
          isOpen = true;
          if (startPointIndex == INVALID_INDEX) {
            startPointIndex = edge1().m_pointIndices[1];
          } else {
            endPointIndex = edge1().m_pointIndices[1];
          }
        }
      }
      if (m_edgeIndices[2] != INVALID_INDEX) {
        if (edge2().point0().m_occurs == 1) {
          isOpen = true;
          if (startPointIndex == INVALID_INDEX) {
            startPointIndex = edge2().m_pointIndices[0];
          } else {
            endPointIndex = edge2().m_pointIndices[0];
          }
        }
        if (edge2().point1().m_occurs == 1) {
          isOpen = true;
          if (startPointIndex == INVALID_INDEX) {
            startPointIndex = edge2().m_pointIndices[1];
          } else {
            endPointIndex = edge2().m_pointIndices[1];
          }
        }
      }
      if (m_edgeIndices[3] != INVALID_INDEX) {
        if (edge3().point0().m_occurs == 1) {
          isOpen = true;
          if (startPointIndex == INVALID_INDEX) {
            startPointIndex = edge3().m_pointIndices[0];
          } else {
            endPointIndex = edge3().m_pointIndices[0];
          }
        }
        if (edge3().point1().m_occurs == 1) {
          isOpen = true;
          if (startPointIndex == INVALID_INDEX) {
            startPointIndex = edge3().m_pointIndices[1];
          } else {
            endPointIndex = edge3().m_pointIndices[1];
          }
        }
      }
      auto const numEdges = static_cast<std::size_t>(std::count_if(
          m_edgeIndices.begin(), m_edgeIndices.end(),
          [](auto const index) { return index != INVALID_INDEX; }));
      m_integrity = {isOpen, startPointIndex, endPointIndex, numEdges};
    }
  };

  std::vector<Point> m_points{};
  std::vector<Edge> m_edges{};
  std::vector<Triangle> m_triangles{};

  void clear() {
    m_points.clear();
    m_edges.clear();
    m_triangles.clear();
  }

  MeshClipper(Mesh const &mesh);

  double maxHeight() const;
  double softMaxHeight() const;
  double minHeight() const;
  size_t countVisiblePoints() const;
  size_t countVisibleEdges() const;
  size_t countVisibleTriangles() const;
  bool isAllPointsVisible() const;
  void writeBinaryStl(std::string const &fileName) const;
  std::vector<Vertex> getTopVertices() const;
  void setDistances(Millimeter zCut);
  void setPointsVisibility();
  void adjustEdges(Millimeter zCut);
  void adjustTriangles();
  void propagateInvisibilityToUsers(size_t edgeIndex, Edge const &edge);
  void close2EdgeOpenTriangle(size_t triangleIndex);
  void close3EdgeOpenTriangle(size_t triangleIndex);
  bool removeNonTriangularTriangles();
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
