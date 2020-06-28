#pragma once

#include <array>
#include <iostream>
#include <limits>
#include <set>
#include <vector>

#include <Eigen/Geometry>

#include <linc/units.h++>

using Vertex = Eigen::Matrix<Millimeter, 3, 1, Eigen::DontAlign>;
using Normal = Vertex;

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

namespace Eigen {
inline auto operator<(Vertex const &lhs, Vertex const &rhs) -> bool {
  VertexCompare v{};
  return v(lhs, rhs);
}

inline auto operator==(Vertex const &lhs, Vertex const &rhs) -> bool {
  return not(lhs < rhs) and not(rhs < lhs);
}

inline auto operator<<(std::ostream &os, Vertex const &vertex)
    -> std::ostream & {
  return os << vertex.x() << ' ' << vertex.y() << ' ' << vertex.z();
}

inline auto operator<<(std::ostream &os, std::vector<Vertex> const &vertices)
    -> std::ostream & {

  std::string delim{""};
  os << '{';
  for (auto const &vertex : vertices) {
    os << delim << vertex;
    delim = ", ";
  }
  os << '}';
  return os;
}
} // namespace Eigen
