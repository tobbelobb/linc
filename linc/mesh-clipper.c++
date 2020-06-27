#include <linc/mesh-clipper.h++>
#include <linc/util.h++>

MeshClipper::MeshClipper(Mesh const &mesh) {
  // Allocate some memory and fill with default data
  m_points.assign(mesh.m_vertices.size(), {Vertex::Zero(), 0.0_mm});
  m_edges.assign(mesh.m_edges.size(), {m_points});
  m_triangles.assign(mesh.m_triangles.size(), {m_edges});

  // Copy over data from Mesh object
  for (auto const &[i, vertex] : enumerate(mesh.m_vertices)) {
    m_points[i] = Point{vertex};
  }
  for (auto const &[i, meshEdge] : enumerate(mesh.m_edges)) {
    m_edges[i] = Edge{m_points, meshEdge.m_vertexIndices, meshEdge.m_users};
  }
  for (auto const &[i, meshTriangle] : enumerate(mesh.m_triangles)) {
    m_triangles[i] = Triangle{m_edges,
                              {meshTriangle.m_edgeIndices.at(0),
                               meshTriangle.m_edgeIndices.at(1),
                               meshTriangle.m_edgeIndices.at(2), INVALID_INDEX},
                              meshTriangle.m_normal,
                              true};
  }
}

void MeshClipper::setDistances(Millimeter const zCut) {
  for (auto &point : m_points) {
    point.m_distance = point.z() - zCut.raw_value();
  }
}

// Given a z height, return a copy of the model that is cut
// at that height
auto cut(Mesh const &mesh, Millimeter const zCut) -> Mesh {
  (void)zCut;
  MeshClipper meshClipper{mesh};
  meshClipper.setDistances(zCut);

  //  size_t facetsToRemove = 0;
  //  for (auto const &facet : m_facets) {
  //    std::vector<double
  //    facetsAbove = facet.vertices[0].z() > zCut
  //    if (
  //    if ( and facet.vertices[1].z() > zCut and
  //        facet.vertices[2].z() > zCut) {
  //      ++facetsToRemove;
  //    }
  //
  //    facet.edgeFromBelow(z);
  //  }
  //
  return mesh;
  // return Stl{facets, neighbors, stats, true, Type::INMEMORY};
}
