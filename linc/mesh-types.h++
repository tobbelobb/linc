#pragma once

#include <array>
#include <iostream>
#include <limits>
#include <set>
#include <vector>

#include <Eigen/Geometry>
#include <SI/length.h>

using Millimeter = SI::milli_metre_t<double>;

using Vertex = Eigen::Matrix<double, 3, 1, Eigen::DontAlign>;
using Normal = Eigen::Matrix<double, 3, 1, Eigen::DontAlign>;

struct VertexCompare {
  bool operator()(Vertex const &lhs, Vertex const &rhs) const {
    constexpr auto eps = 1e-4;
    if (lhs.x() + eps < rhs.x()) {
      return true;
    }
    if (rhs.x() + eps < lhs.x()) {
      return false;
    }
    if (lhs.y() + eps < rhs.y()) {
      return true;
    }
    if (rhs.y() + eps < lhs.y()) {
      return false;
    }
    if (lhs.z() + eps < rhs.z()) {
      return true;
    }
    if (rhs.z() + eps < lhs.z()) {
      return false;
    }
    // Vertices lhs and rhs are equal within eps precision
    return false;
  }
};

inline auto operator<(Vertex const &lhs, Vertex const &rhs) -> bool {
  VertexCompare v{};
  return v(lhs, rhs);
}

inline auto operator<<(std::ostream &os,
                       std::set<Vertex, VertexCompare> const &vertices)
    -> std::ostream & {

  for (auto const &vertex : vertices) {
    os << vertex.x() << ' ' << vertex.y() << ' ' << vertex.z() << '\n';
  }
  return os;
}
