#pragma once
#include <array>

#include <linc/mesh-clipper.h++>
#include <linc/mesh.h++>
#include <linc/params.h++>
#include <linc/units.h++>
#include <linc/vertex.h++>

struct Triangle {
  std::array<Vertex, 3> m_corners;
  Normal m_normal;

  Triangle(std::array<Vertex, 3> const &corners)
      : m_corners(corners),
        m_normal(m_corners[0].cross(m_corners[1]).normalized()) {}

  Triangle(Mesh::Triangle const &meshTriangle) {
    m_corners[0] = meshTriangle.edge0().vertex0();
    m_corners[1] = meshTriangle.edge0().vertex1();
    m_corners[2] = (meshTriangle.edge1().vertex0() == m_corners[0] or
                    meshTriangle.edge1().vertex0() == m_corners[1])
                       ? meshTriangle.edge1().vertex1()
                       : meshTriangle.edge1().vertex0();

    if (meshTriangle.m_normal.norm() == 0) {
      m_normal = (m_corners[0].cross(m_corners[1])).normalized();
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
      m_normal = (m_corners[0].cross(m_corners[1])).normalized();
    } else {
      m_normal = meshTriangle.m_normal;
    }
  }
};

[[nodiscard]] std::vector<Vertex> hull(std::vector<Vertex> const &);
[[nodiscard]] bool intersect(Triangle const &, Triangle const &);

void orderCounterClockWise(std::vector<Vertex> &v);

[[nodiscard]] bool willCollide(Mesh const &, Pivots const &, Millimeter);
