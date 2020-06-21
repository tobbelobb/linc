#include <set>

#include <linc/mesh.h++>

using namespace mesh;

Mesh::Mesh(Stl const &stl) {
  // Save vertices
  for (const auto &facet : stl.m_facets) {
    for (const auto &vertex : facet.vertices) {
      m_vertices.insert(vertex);
    }
  }

  // Save edges
  for (const auto &facet : stl.m_facets) {
    m_edges.insert({{m_vertices.find(facet.vertices[0]),
                     m_vertices.find(facet.vertices[1])},
                    {m_triangles.end(), m_triangles.end()}});
    m_edges.insert({{m_vertices.find(facet.vertices[1]),
                     m_vertices.find(facet.vertices[2])},
                    {m_triangles.end(), m_triangles.end()}});
    m_edges.insert({{m_vertices.find(facet.vertices[2]),
                     m_vertices.find(facet.vertices[0])},
                    {m_triangles.end(), m_triangles.end()}});
  }

  //  for (size_t i{0}; i < stl.m_facets.size(); ++i) {
  //    std::set<Vertex>::iterator const iterator0 =
  //        m_vertices.find(stl.m_facets[i].vertices[0]);
  //    std::set<Vertex>::iterator const iterator1 =
  //        m_vertices.find(stl.m_facets[i].vertices[1]);
  //    std::set<Vertex>::iterator const iterator2 =
  //        m_vertices.find(stl.m_facets[i].vertices[2]);
  //    std::array<std::set<Vertex>::iterator, 3> const vertexIterators{
  //        iterator0, iterator1, iterator2};
  //    m_edges.push_back(
  //        {.m_vertexIterators = {vertexIterators[0], vertexIterators[1]},
  //         .m_userIterators = {INVALID_INDEX, INVALID_INDEX}});
  //    m_edges.push_back(
  //        {.m_vertexIterators = {vertexIterators[1], vertexIterators[2]},
  //         .m_userIterators = {INVALID_INDEX, INVALID_INDEX}});
  //    m_edges.push_back(
  //        {.m_vertexIterators = {vertexIterators[2], vertexIterators[0]},
  //         .m_userIndices = {INVALID_INDEX, INVALID_INDEX}});
  //  }
}
