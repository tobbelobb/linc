#pragma once
#include <iomanip>
#include <vector>

#include <spdlog/spdlog.h>

#include <linc/stl.h++>
#include <linc/units.h++>
#include <linc/vertex.h++>

constexpr std::size_t INVALID_INDEX{std::numeric_limits<std::size_t>::max()};
using EdgePointIndices = std::array<std::size_t, 2>;

class Mesh {
public:
  struct Edge {
    EdgePointIndices m_pointIndices{INVALID_INDEX, INVALID_INDEX};
    std::array<std::size_t, 2> m_users{INVALID_INDEX, INVALID_INDEX};

    friend std::ostream &operator<<(std::ostream &os, Mesh::Edge const &edge) {
      os << "{ " << std::setiosflags(std::ios::right) << std::setw(3)
         << std::setfill(' ') << edge.m_pointIndices[0] << " <-> "
         << std::setw(3) << edge.m_pointIndices[1] << " users: (";
      std::string delim{""};
      if (std::count(edge.m_users.begin(), edge.m_users.end(), INVALID_INDEX) ==
          1) {
        delim = "    ";
      }
      for (auto const &user : edge.m_users) {
        if (user != INVALID_INDEX) {
          os << delim << std::setw(3) << user;
          delim = ",";
        }
      }
      return os << ") }";
    }
  };

  struct Triangle {
    std::array<std::size_t, 3> m_edgeIndices{INVALID_INDEX, INVALID_INDEX,
                                             INVALID_INDEX};
    friend std::ostream &operator<<(std::ostream &os,
                                    Triangle const &triangle) {
      os << '{';
      os << triangle.m_edgeIndices[0];
      os << ", " << triangle.m_edgeIndices[1];
      os << ", " << triangle.m_edgeIndices[2];
      return os << '}';
    }
  };

  struct Opening {
    std::size_t startPointIndex = INVALID_INDEX;
    std::size_t endPointIndex = INVALID_INDEX;
  };

  std::vector<Vertex> m_points{};
  std::vector<Edge> m_edges{};
  std::vector<Triangle> m_triangles{};

  Mesh(Mesh const &meshClipper);
  Mesh(Stl const &stl);
  Mesh(Vertex v) : m_points{{v}} {}
  Mesh() = default;

  Vertex &point0(Edge const &edge) { return m_points[edge.m_pointIndices[0]]; }
  Vertex &point1(Edge const &edge) { return m_points[edge.m_pointIndices[1]]; }

  void loadVertices(std::vector<Stl::Facet> const &facets);
  std::vector<std::array<Edge, 3>>
  extractEdgeTriplets(std::vector<Stl::Facet> const &facets);
  void loadEdges(std::vector<std::array<Edge, 3>> const &edgeTriplets);
  void loadTriangles(std::vector<std::array<Edge, 3>> const &edgeTriplets);
  Millimeter maxHeight() const;
  Millimeter softMaxHeight(std::vector<bool> const &visible) const;
  Millimeter minHeight() const;
  std::size_t countVisibleTriangles() const;
  void writeBinaryStl(std::string const &fileName) const;
  std::vector<Vertex> getVerticesAt(Millimeter height) const;
  std::vector<bool> getPointsVisibility(Millimeter zCut);
  void getTrianglesVisibility(std::vector<bool> &visible) const;
  void propagateInvisibilityToUsers(std::size_t const edgeIndex,
                                    Edge const &edge);
  void adjustEdges(Millimeter zCut, std::vector<bool> &pointVisibility,
                   std::vector<std::size_t> &clippedTriangles);
  void adjustTriangles(std::vector<std::size_t> const &triangleIndices);
  Vertex pointAlong(Edge const &edge, Millimeter t) const;
  void close2EdgeOpenTriangle(std::size_t triangleIndex,
                              Opening const &opening);
  void close3EdgeOpenTriangle(std::size_t triangleIndex,
                              Opening const &opening);
  std::vector<bool> softClip(Millimeter zCut,
                             std::vector<std::size_t> &clippedTriangles);
  Opening getOpening(Mesh::Triangle const &triangle) const;
  void reset(Mesh const &originalMesh,
             std::vector<std::size_t> const &clippedTriangles,
             std::vector<bool> const &trianglesVisibility);
};

inline bool operator<(Mesh::Edge const &lhs, Mesh::Edge const &rhs) {
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

inline bool operator==(Mesh::Edge const &lhs, Mesh::Edge const &rhs) {
  return (lhs.m_pointIndices[0] == rhs.m_pointIndices[0] and
          lhs.m_pointIndices[1] == rhs.m_pointIndices[1]) or
         (lhs.m_pointIndices[1] == rhs.m_pointIndices[0] and
          lhs.m_pointIndices[0] == rhs.m_pointIndices[1]);
}

inline bool operator!=(Mesh::Edge const &lhs, Mesh::Edge const &rhs) {
  return not(lhs == rhs);
}

inline bool operator<(Mesh::Triangle const &lhs, Mesh::Triangle const &rhs) {
  std::set<std::size_t> lhsEdges{lhs.m_edgeIndices[0], lhs.m_edgeIndices[1],
                                 lhs.m_edgeIndices[2]};
  std::set<std::size_t> rhsEdges{rhs.m_edgeIndices[0], rhs.m_edgeIndices[1],
                                 rhs.m_edgeIndices[2]};
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

inline bool operator==(Mesh::Triangle const &lhs, Mesh::Triangle const &rhs) {
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

inline bool operator!=(Mesh::Triangle const &lhs, Mesh::Triangle const &rhs) {
  return not(lhs == rhs);
}

inline auto operator<<(std::ostream &os, std::vector<Mesh::Edge> const &edges)
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

inline auto operator<<(std::ostream &os, std::vector<Mesh::Triangle> triangles)
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

template <> struct fmt::formatter<Mesh::Edge> {
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
  auto format(const Mesh::Edge &edge, FormatContext &ctx) {
    // ctx.out() is an output iterator to write to.
    return format_to(ctx.out(), "{{({}) --- ({})}}", edge.m_pointIndices[0],
                     edge.m_pointIndices[1]);
  }
};
