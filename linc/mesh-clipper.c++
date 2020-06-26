#include <linc/mesh-clipper.h++>

MeshClipper::MeshClipper(Mesh const &mesh) {
  m_points.assign(mesh.m_vertices.size(), {Vertex::Zero(), 0.0_mm});
  m_edges.assign(mesh.m_edges.size(), {m_points});
  m_triangles.assign(mesh.m_triangles.size(), {m_edges});

  for (auto const &vertex : mesh.m_vertices) {
    m_points.emplace_back(Point{vertex});
  }

  for (auto const &meshEdge : mesh.m_edges) {
    m_edges.emplace_back(
        Edge{m_points, meshEdge.m_vertexIndices, meshEdge.m_users});
  }

  for (auto const &meshTriangle : mesh.m_triangles) {
    m_triangles.emplace_back(Triangle{
        m_edges,
        {meshTriangle.m_edgeIndices.at(0), meshTriangle.m_edgeIndices.at(1),
         meshTriangle.m_edgeIndices.at(2), INVALID_INDEX},
        Normal::Zero(),
        true});
  }
}

// Given a z height, return a copy of the model that is cut
// at that height
auto cut(Mesh const &mesh, Millimeter const zCut) -> Mesh {
  (void)zCut;
  //
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
