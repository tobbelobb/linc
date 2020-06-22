#include <set>

#include <linc/mesh.h++>

Mesh::Mesh(Stl const &stl) {
  // Save vertices
  for (const auto &facet : stl.m_facets) {
    for (const auto &vertex : facet.vertices) {
      m_vertices.insert(vertex);
    }
  }

  // Save edges' vertices
  for (const auto &facet : stl.m_facets) {
    m_superEdges.insert({{m_vertices.find(facet.vertices[0]),
                          m_vertices.find(facet.vertices[1])}});
    m_superEdges.insert({{m_vertices.find(facet.vertices[1]),
                          m_vertices.find(facet.vertices[2])}});
    m_superEdges.insert({{m_vertices.find(facet.vertices[2]),
                          m_vertices.find(facet.vertices[0])}});
  }

  // Save triangles' edges
  for (const auto &facet : stl.m_facets) {
    std::set<SuperEdge, EdgeCompare>::iterator superEdge1{
        m_superEdges.find(SuperEdge{{m_vertices.find(facet.vertices[0]),
                                     m_vertices.find(facet.vertices[1])}})};
    std::set<SuperEdge, EdgeCompare>::iterator superEdge2{
        m_superEdges.find(SuperEdge{{m_vertices.find(facet.vertices[1]),
                                     m_vertices.find(facet.vertices[2])}})};
    std::set<SuperEdge, EdgeCompare>::iterator superEdge3{
        m_superEdges.find(SuperEdge{{m_vertices.find(facet.vertices[2]),
                                     m_vertices.find(facet.vertices[0])}})};
    m_triangles.insert(
        {{std::make_unique(*superEdge1), std::make_unique(*superEdge2),
          std::make_unique(*superEdge3)}});
  }

  // Save super edges' triangles
  auto triangleIterator = m_triangles.begin();
  while (triangleIterator != m_triangles.end()) {
    Triangle const &triangle = *triangleIterator;
    for (auto &edgePtr : triangle.m_edgePtrs) {
      (*edgePtr).m_users.insert(triangle);
    }
    ++triangleIterator;
  }
}
