#pragma once
#include <vector>

#include <SI/length.h>

#include <linc/mesh.h++>

using namespace SI::literals;

using EdgePointIndices = std::array<size_t, 2>;

class MeshClipper {
public:
  struct Point {
    Vertex m_vertex;
    Millimeter distance = 0.0;
    unsigned short occurs = 0;
    bool m_visible = true;

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
    }

    // Ooops. Not like this:
    //bool operator==(Edge const &other) const {
    //  return m_points == other.m_points and
    //         m_pointIndices == other.m_pointIndices and
    //         m_users == other.m_users and m_visible == other.m_visible;
    //}
    //bool operator!=(Edge const &other) const { return not(*this == other); }

    friend std::ostream &operator<<(std::ostream &os, Edge const &edge) {
      std::string delim{""};
      os << '(';
      for (auto const &point : edge.m_points) {
        os << delim << point;
        delim = ", ";
      }
      os << ") ";
      delim = "";
      for (auto const &user : edge.m_users) {
        os << delim << user;
        delim = ", ";
      }
      return os << ") " << (edge.m_visible ? " Visible" : " Invisible");
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

    Triangle &operator=(Triangle const &other) {
      m_edges = other.m_edges;
      m_edgeIndices = other.m_edgeIndices;
      m_normal = other.m_normal;
      m_visible = other.m_visible;
      return *this;
    }
  };

  std::vector<Point> m_points{};
  std::vector<Edge> m_edges{};
  std::vector<Triangle> m_triangles{};

  MeshClipper(Mesh const &mesh);
};

Mesh cut(Mesh const &mesh, Millimeter const zCut);
