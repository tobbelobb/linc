#pragma once

#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <vector>

#include <Eigen/Geometry>

#include <linc/units.h++>
#include <linc/util.h++>

using Vertex = Eigen::Matrix<Millimeter, 3, 1, Eigen::DontAlign>;
using Normal = Vertex;

namespace VertexConstants {
constexpr Millimeter eps = 1e-4_mm;
}

namespace Eigen {
inline auto operator<(Vertex const &lhs, Vertex const &rhs) -> bool {
  // We really don't care about sub-millimeter precision in this application
  if (lhs.x() + VertexConstants::eps < rhs.x()) {
    return true;
  }
  if (rhs.x() + VertexConstants::eps < lhs.x()) {
    return false;
  }
  if (lhs.y() + VertexConstants::eps < rhs.y()) {
    return true;
  }
  if (rhs.y() + VertexConstants::eps < lhs.y()) {
    return false;
  }
  if (lhs.z() + VertexConstants::eps < rhs.z()) {
    return true;
  }
  if (rhs.z() + VertexConstants::eps < lhs.z()) {
    return false;
  }
  // Vertices lhs and rhs are equal within eps precision
  return false;
}

inline auto operator==(Vertex const &lhs, Vertex const &rhs) -> bool {
  return not(lhs < rhs) and not(rhs < lhs);
}

inline auto operator!=(Vertex const &lhs, Vertex const &rhs) -> bool {
  return not(lhs == rhs);
}

inline auto operator==(std::array<Vertex, 3> const &lhs,
                       std::vector<Vertex> const &rhs) -> bool {
  for (auto const &[i, lh] : enumerate(lhs)) {
    auto const &rh = rhs[i];
    if (lh != rh) {
      return false;
    }
  }
  return true;
}

inline auto operator==(std::vector<Vertex> const &lhs,
                       std::vector<Vertex> const &rhs) -> bool {
  for (auto const &[i, lh] : enumerate(lhs)) {
    auto const &rh = rhs[i];
    if (lh != rh) {
      return false;
    }
  }
  return true;
}

inline auto operator<<(std::ostream &os, Vertex const &vertex)
    -> std::ostream & {
  constexpr size_t w = 8;
  // std::ios_base::fmtflags f = output.flags(std::ios::right);
  return os << std::setiosflags(std::ios::right) << std::fixed
            << std::setprecision(5) << std::setw(w) << std::setfill(' ')
            << vertex.x() << ", " << std::setw(w) << vertex.y() << ", "
            << std::setw(w) << vertex.z();
}

inline auto operator<<(std::ostream &os, std::vector<Vertex> const &vertices)
    -> std::ostream & {

  std::string delim{""};
  os << '{';
  for (auto const &vertex : vertices) {
    os << delim << '[' << vertex << ']';
    delim = ", ";
  }
  os << '}';
  return os;
}

inline auto operator<<(std::ostream &os, std::array<Vertex, 3> const &vertices)
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
