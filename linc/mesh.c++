#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG /* NOLINT */
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <linc/mesh.h++>
#include <linc/util.h++>
static auto logger = spdlog::get("file_logger");

Mesh::Mesh(Stl const &stl) {
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s(%#)] [%!] [%l] %v");
  spdlog::set_level(spdlog::level::trace);
  // Build vertices
  SPDLOG_LOGGER_DEBUG(logger, "there are {} facets", stl.m_facets.size());

  m_vertices.reserve(stl.m_facets.size() * 3);
  for (const auto &facet : stl.m_facets) {
    for (const auto &vertex : facet.vertices) {
      m_vertices.push_back(vertex);
    }
  }
  std::sort(m_vertices.begin(), m_vertices.end());
  auto const endOfUniqueVertices =
      std::unique(m_vertices.begin(), m_vertices.end());
  m_vertices.erase(endOfUniqueVertices, m_vertices.end());

  SPDLOG_LOGGER_DEBUG(logger, "found {} vertices", m_vertices.size());

  // EDGES
  std::vector<std::vector<Edge>> allEdgeSuggestions{};
  allEdgeSuggestions.reserve(stl.m_facets.size());
  for (const auto &facet : stl.m_facets) {
    // Find the three vertices that make up this facet
    std::array<size_t, 3> vertexIndices = {INVALID_INDEX, INVALID_INDEX,
                                           INVALID_INDEX};
    for (auto const &[j, facetVertex] : enumerate(facet.vertices)) {
      auto const distance = std::distance(
          m_vertices.begin(),
          std::lower_bound(m_vertices.begin(), m_vertices.end(), facetVertex));
      if (distance >= 0) {
        vertexIndices.at(j) = std::size_t(distance);
      }
    }
    std::vector<Edge> const edgeSuggestions = {
        {m_vertices, {vertexIndices.at(0), vertexIndices.at(1)}},
        {m_vertices, {vertexIndices.at(1), vertexIndices.at(2)}},
        {m_vertices, {vertexIndices.at(2), vertexIndices.at(0)}}};
    allEdgeSuggestions.emplace_back(edgeSuggestions);
    for (auto const &edge : edgeSuggestions) {
      m_edges.emplace_back(edge);
    }
  }
  std::sort(m_edges.begin(), m_edges.end());
  auto const endOfUniqueEdges = std::unique(m_edges.begin(), m_edges.end());
  m_edges.erase(endOfUniqueEdges, m_edges.end());

  SPDLOG_LOGGER_DEBUG(logger, "found {} edges", m_edges.size());

  // TRIANGLES
  m_triangles.reserve(stl.m_facets.size());
  for (auto const &[i, facet] : enumerate(stl.m_facets)) {
    std::array<size_t, 3> edgeIndices{INVALID_INDEX, INVALID_INDEX,
                                      INVALID_INDEX};
    for (auto const &[q, edgeSuggestion] : enumerate(allEdgeSuggestions[i])) {
      auto const distance = std::distance(
          m_edges.begin(),
          std::lower_bound(m_edges.begin(), m_edges.end(), edgeSuggestion));
      if (distance >= 0) {
        edgeIndices.at(q) = std::size_t(distance);
      }
    }
    if (edgeIndices.at(0) != edgeIndices.at(1) and
        edgeIndices.at(0) != edgeIndices.at(2) and
        edgeIndices.at(1) != edgeIndices.at(2)) {
      m_triangles.emplace_back(Triangle{m_edges, edgeIndices});
    }
  }
  std::sort(m_triangles.begin(), m_triangles.end());
  auto const endOfUniqueTriangles =
      std::unique(m_triangles.begin(), m_triangles.end());
  m_triangles.erase(endOfUniqueTriangles, m_triangles.end());

  SPDLOG_LOGGER_DEBUG(logger, "found {} triangles", m_triangles.size());

  for (auto const &[i, triangle] : enumerate(m_triangles)) {
    m_edges[triangle.m_edgeIndices[0]].m_users.push_back(i);
    m_edges[triangle.m_edgeIndices[1]].m_users.push_back(i);
    m_edges[triangle.m_edgeIndices[2]].m_users.push_back(i);
  }

  SPDLOG_LOGGER_DEBUG(logger, "finished loading Mesh from Stl");
}
