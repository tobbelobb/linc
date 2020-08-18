#pragma once
#include <array>

#include <spdlog/spdlog.h>

#include <linc/mesh.h++>
#include <linc/params.h++>
#include <linc/units.h++>
#include <linc/vertex.h++>

struct Triangle {
  std::array<Vertex, 3> m_corners;

  Triangle() : m_corners({Vertex::Zero(), Vertex::Zero(), Vertex::Zero()}) {}

  Triangle(std::array<Vertex, 3> const &corners) : m_corners(corners) {}

  Triangle(Mesh::Triangle const &meshTriangle,
           std::vector<Vertex> const &points,
           std::vector<Mesh::Edge> const &edges) {
    m_corners[0] =
        points[edges[meshTriangle.m_edgeIndices[0]].m_pointIndices[0]];
    m_corners[1] =
        points[edges[meshTriangle.m_edgeIndices[0]].m_pointIndices[1]];
    m_corners[2] =
        (points[edges[meshTriangle.m_edgeIndices[1]].m_pointIndices[0]] ==
             m_corners[0] or
         points[edges[meshTriangle.m_edgeIndices[1]].m_pointIndices[0]] ==
             m_corners[1])
            ? points[edges[meshTriangle.m_edgeIndices[1]].m_pointIndices[1]]
            : points[edges[meshTriangle.m_edgeIndices[1]].m_pointIndices[0]];
  }

  Normal getNormalDirection() const {
    return (m_corners[0] - m_corners[1]).cross(m_corners[0] - m_corners[2]);
  }
};

template <> struct fmt::formatter<Triangle> {
  constexpr auto parse(format_parse_context &ctx) {
    auto it = ctx.begin();
    auto end = ctx.end();
    if (it != end and *it != '}') {
      it++;
    }
    if (it != end and *it != '}') {
      it++;
    }
    if (it != end and *it != '}') {
      throw format_error("invalid format");
    }
    return it;
  }

  template <typename FormatContext>
  auto format(const Triangle &triangle, FormatContext &ctx) {
    // ctx.out() is an output iterator to write to.
    return format_to(ctx.out(),
                     "{{[{:.1f}, {:.1f}, {:.1f}], [{:.1f}, {:.1f}, {:.1f}], "
                     "[{:.1f}, {:.1f}, {:.1f}]}}",
                     triangle.m_corners[0].x(), triangle.m_corners[0].y(),
                     triangle.m_corners[0].z(), triangle.m_corners[1].x(),
                     triangle.m_corners[1].y(), triangle.m_corners[1].z(),
                     triangle.m_corners[2].x(), triangle.m_corners[2].y(),
                     triangle.m_corners[2].z());
  }
};

// Disregards z and constructs hull as if all vertices were at z=0
[[nodiscard]] std::vector<Vertex> hullAndSortCcw(std::vector<Vertex> &);
[[nodiscard]] bool intersect(Triangle const &, Triangle const &);

// Sorts vertices in counterClockwise order around mean point
// Disregards z and constructs hull as if all vertices were at z=0
void sortCcwInPlace(std::vector<Vertex> &v);

struct Collision {
  bool const m_isCollision;
  Millimeter m_height;
  Triangle const m_coneTriangle;
  Vertex const m_effectorPivot;

  Collision(bool isCollision)
      : m_isCollision(isCollision), m_height(0.0), m_coneTriangle(),
        m_effectorPivot(Vertex::Zero()) {}
  Collision(bool isCollision, Millimeter const height, Triangle const &triangle,
            Vertex const &effectorPivot)
      : m_isCollision(isCollision), m_height(height), m_coneTriangle(triangle),
        m_effectorPivot(effectorPivot) {}

  bool operator!() const { return not m_isCollision; }

  operator bool() const { return m_isCollision; }
};

[[nodiscard]] Collision willCollide(Mesh const &, Pivots const &, Millimeter,
                                    bool hullIt = true);

void makeDebugModel(Mesh const &meshClipper, Pivots const &pivots,
                    Collision const &collision, std::string const &outFileName);
