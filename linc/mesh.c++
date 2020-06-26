#include <set>

#include <linc/mesh.h++>
#include <linc/util.h++>

Mesh::Mesh(Stl const &stl) {
  // Build vertices
  for (const auto &facet : stl.m_facets) {
    for (const auto &vertex : facet.vertices) {
      if (std::find(m_vertices.begin(), m_vertices.end(), vertex) ==
          m_vertices.end()) {
        m_vertices.push_back(vertex);
      }
    }
  }

  // Build edges
  std::vector<std::vector<Edge>> allEdgeSuggestions{};
  for (const auto &facet : stl.m_facets) {
    // Find the three vertices that make up this facet
    std::array<size_t, 3> vertexIndices = {INVALID_INDEX, INVALID_INDEX,
                                           INVALID_INDEX};
    for (auto const &[j, facetVertex] : enumerate(facet.vertices)) {
      for (auto const &[i, vertex] : enumerate(m_vertices)) {
        if (vertex == facetVertex) {
          vertexIndices.at(j) = i;
          break;
        }
      }
    }
    std::vector<Edge> const edgeSuggestions = {
        {m_vertices, {vertexIndices.at(0), vertexIndices.at(1)}},
        {m_vertices, {vertexIndices.at(1), vertexIndices.at(2)}},
        {m_vertices, {vertexIndices.at(2), vertexIndices.at(0)}}};
    allEdgeSuggestions.push_back(edgeSuggestions);
    for (auto const &edge : edgeSuggestions) {
      // If the edge is not found already, store it
      if (std::find(m_edges.begin(), m_edges.end(), edge) == m_edges.end()) {
        m_edges.push_back(edge);
      }
    }
  }

  // Build triangles
  for (auto const &[i, facet] : enumerate(stl.m_facets)) {
    std::vector<Edge> const edgeSuggestions = allEdgeSuggestions[i];
    std::array<size_t, 3> edgeIndices{INVALID_INDEX, INVALID_INDEX,
                                      INVALID_INDEX};
    for (auto const &[q, edgeSuggestion] : enumerate(allEdgeSuggestions[i])) {
      for (auto const &[j, edge] : enumerate(m_edges)) {
        if (edgeSuggestion == edge) {
          edgeIndices[q] = j;
          break;
        }
      }
    }
    m_triangles.emplace_back(Triangle{m_edges, edgeIndices});
  }

  for (auto const &[i, triangle] : enumerate(m_triangles)) {
    m_edges[triangle.m_edgeIndices[0]].m_users.push_back(i);
    m_edges[triangle.m_edgeIndices[1]].m_users.push_back(i);
    m_edges[triangle.m_edgeIndices[2]].m_users.push_back(i);
  }
}
