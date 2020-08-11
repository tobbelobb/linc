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

  Triangle()
      : m_corners({Vertex::Zero(), Vertex::Zero(), Vertex::Zero()}),
        m_normal(Normal::Zero()) {}

  Triangle(std::array<Vertex, 3> const &corners)
      : m_corners(corners), m_normal(computeNormal(corners)) {}

  Triangle(Mesh::Triangle const &meshTriangle) {
    m_corners[0] = meshTriangle.edge0().vertex0();
    m_corners[1] = meshTriangle.edge0().vertex1();
    m_corners[2] = (meshTriangle.edge1().vertex0() == m_corners[0] or
                    meshTriangle.edge1().vertex0() == m_corners[1])
                       ? meshTriangle.edge1().vertex1()
                       : meshTriangle.edge1().vertex0();
    m_normal = computeNormal(m_corners);
  }

  Triangle(MeshClipper::Triangle const &meshTriangle) {
    m_corners[0] = meshTriangle.edge0().point0().m_vertex;
    m_corners[1] = meshTriangle.edge0().point1().m_vertex;
    m_corners[2] = (meshTriangle.edge1().point0().m_vertex == m_corners[0] or
                    meshTriangle.edge1().point0().m_vertex == m_corners[1])
                       ? meshTriangle.edge1().point1().m_vertex
                       : meshTriangle.edge1().point0().m_vertex;

    m_normal = computeNormal(m_corners);
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

void makeDebugModel(Mesh const &mesh, Pivots const &pivots,
                    Collision const &collision, std::string const &outFileName);
