#pragma once
#include <array>

#include <spdlog/spdlog.h>

#include <linc/mesh-clipper.h++>
#include <linc/mesh.h++>
#include <linc/params.h++>
#include <linc/units.h++>
#include <linc/vertex.h++>

struct Triangle {
  std::array<Vertex, 3> m_corners;
  Normal m_normal;

  Normal computeNormal(std::array<Vertex, 3> const &corners) const {
    Vertex const edge0 = corners[0] - corners[1];
    Vertex const edge1 = corners[0] - corners[2];
    return edge0.cross(edge1).normalized();
  }

  Triangle(std::array<Vertex, 3> const &corners)
      : m_corners(corners), m_normal(computeNormal(corners)) {}

  Triangle(Mesh::Triangle const &meshTriangle) {
    m_corners[0] = meshTriangle.edge0().vertex0();
    m_corners[1] = meshTriangle.edge0().vertex1();
    m_corners[2] = (meshTriangle.edge1().vertex0() == m_corners[0] or
                    meshTriangle.edge1().vertex0() == m_corners[1])
                       ? meshTriangle.edge1().vertex1()
                       : meshTriangle.edge1().vertex0();

    if (meshTriangle.m_normal.norm() == 0) {
      m_normal = computeNormal(m_corners);
    } else {
      m_normal = meshTriangle.m_normal;
    }
  }

  Triangle(MeshClipper::Triangle const &meshTriangle) {
    m_corners[0] = meshTriangle.edge0().point0().m_vertex;
    m_corners[1] = meshTriangle.edge0().point1().m_vertex;
    m_corners[2] = (meshTriangle.edge1().point0().m_vertex == m_corners[0] or
                    meshTriangle.edge1().point0().m_vertex == m_corners[1])
                       ? meshTriangle.edge1().point1().m_vertex
                       : meshTriangle.edge1().point0().m_vertex;

    if (meshTriangle.m_normal.norm() == 0) {
      m_normal = computeNormal(m_corners);
    } else {
      m_normal = meshTriangle.m_normal;
    }
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
[[nodiscard]] std::vector<Vertex> hullAndSortCcw(std::vector<Vertex> const &);
[[nodiscard]] bool intersect(Triangle const &, Triangle const &);

// Sorts vertices in counterClockwise order around mean point
// Disregards z and constructs hull as if all vertices were at z=0
void sortCcwInPlace(std::vector<Vertex> &v);

[[nodiscard]] bool willCollide(Mesh const &, Pivots const &, Millimeter,
                               bool hullIt = true);
