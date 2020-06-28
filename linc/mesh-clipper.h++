#pragma once
#include <vector>

#include <linc/mesh.h++>
#include <linc/units.h++>

using EdgePointIndices = std::array<size_t, 2>;

class MeshClipper {
public:
  struct Point {
    Vertex m_vertex;
    Millimeter m_distance = 0.0;
    unsigned short m_occurs = 0;
    bool m_visible = true;

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
      return os << point.x() << ' ' << point.y() << ' ' << point.z();
    }
  };

  struct Edge {
    std::vector<Point> &m_points;
    EdgePointIndices m_pointIndices{INVALID_INDEX, INVALID_INDEX};
    EdgeUsers m_users{};
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

    Edge(MeshClipper::Edge const &other)
        : m_points(other.m_points), m_pointIndices(other.m_pointIndices),
          m_users(other.m_users), m_visible(other.m_visible) {}

    Edge(std::vector<Point> &points) : m_points(points) {}
    Edge(std::vector<Point> &points, EdgePointIndices pointIndices)
        : m_points(points), m_pointIndices(pointIndices) {}
    Edge(std::vector<Point> &points, EdgePointIndices pointIndices,
         EdgeUsers users)
        : m_points(points), m_pointIndices(pointIndices), m_users(users) {}
    Edge(std::vector<Point> &points, EdgePointIndices pointIndices,
         EdgeUsers users, bool visible)
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
      os << '{' << p0 << "---" << p1 << " users: (";
      std::string delim{""};
      for (auto const &user : edge.m_users) {
        os << delim << user;
        delim = ", ";
      }
      return os << ") " << (edge.m_visible ? "Visible" : "Invisible") << '}';
    }
  };

  struct Triangle {
    std::vector<Edge> &m_edges;
    // A triangle might briefly need to keep track of
    // four corners during a cut operation
    std::array<size_t, 4> m_edgeIndices{INVALID_INDEX, INVALID_INDEX,
                                        INVALID_INDEX, INVALID_INDEX};
    Normal m_normal = Normal::Zero();
    bool m_visible = true;

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
      os << '{' << triangle.edge0() << ", " << triangle.edge1() << ", "
         << triangle.edge2();
      if (triangle.m_edgeIndices[3] != INVALID_INDEX) {
        os << ", " << triangle.edge3();
      }
      return os << '}';
    }

    bool fullEquals(Triangle const &other) const {
      return *this == other and m_normal == other.m_normal and
             m_visible == other.m_visible;
    }
  };

  std::vector<Point> m_points{};
  std::vector<Edge> m_edges{};
  std::vector<Triangle> m_triangles{};

  MeshClipper(Mesh const &mesh);

  void setDistances(Millimeter zCut);
};

inline auto operator<<(std::ostream &os,
                       std::vector<MeshClipper::Point> const &points)
    -> std::ostream & {

  std::string delim{""};
  os << '{';
  for (auto const &point : points) {
    os << delim << point;
    delim = ", ";
  }
  os << '}';
  return os;
}

inline auto operator<<(std::ostream &os,
                       std::vector<MeshClipper::Edge> const &edges)
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

Mesh clip(Mesh const &mesh, Millimeter const zCut);
