#include <set>

#include <linc/mesh.h++>

Mesh::Mesh(Stl const &stl) {
  // Build vertices
  for (const auto &facet : stl.m_facets) {
    for (const auto &vertex : facet.vertices) {
      if (std::find_if(m_vertices.begin(), m_vertices.end(),
                       [&vertex](Vertex const &v) {
                         return vertexEquals(vertex, v);
                       }) == m_vertices.end()) {
        m_vertices.push_back(vertex);
      }
    }
  }

  // Build edges
  std::vector<std::vector<Edge>> allEdgeSuggestions{};
  for (const auto &facet : stl.m_facets) {
    std::array<size_t, 3> vertexIndices = {INVALID_INDEX, INVALID_INDEX,
                                           INVALID_INDEX};
    for (size_t j{0}; j < 3; ++j) {
      for (size_t i{0}; i < m_vertices.size(); ++i) {
        if (vertexEquals(m_vertices[i], facet.vertices[j])) {
          vertexIndices[j] = i;
        }
      }
    }
    std::vector<Edge> const edgeSuggestions = {
        {m_vertices, {vertexIndices[0], vertexIndices[1]}},
        {m_vertices, {vertexIndices[1], vertexIndices[2]}},
        {m_vertices, {vertexIndices[2], vertexIndices[0]}}};
    allEdgeSuggestions.push_back(edgeSuggestions);
    for (auto const &edge : edgeSuggestions) {
      if (std::find(m_edges.begin(), m_edges.end(), edge) == m_edges.end()) {
        m_edges.push_back(edge);
      }
    }
  }

  // Build triangles
  for (size_t i{0}; i < stl.m_facets.size(); ++i) {
    std::vector<Edge> const edgeSuggestions = allEdgeSuggestions[i];
    TriangleEdgeIndices edgeIndices{INVALID_INDEX, INVALID_INDEX,
                                    INVALID_INDEX};
    for (size_t q{0}; q < 3; ++q) {
      for (size_t j{0}; j < m_edges.size(); ++j) {
        if (edgeSuggestions[q] == m_edges[j]) {
          edgeIndices[q] = j;
        }
      }
    }
    m_triangles.push_back({m_edges, edgeIndices});
  }

  for (size_t i{0}; i < m_triangles.size(); ++i) {
    m_edges[m_triangles[i].m_edgeIndices[0]].m_users.push_back(i);
    m_edges[m_triangles[i].m_edgeIndices[1]].m_users.push_back(i);
    m_edges[m_triangles[i].m_edgeIndices[2]].m_users.push_back(i);
  }
}
