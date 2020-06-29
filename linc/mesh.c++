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

  SPDLOG_LOGGER_DEBUG(logger, "found {} unique vertices", m_vertices.size());

  // EDGES
  std::vector<std::array<Edge, 3>> consideredEdgeTriplets{};
  consideredEdgeTriplets.reserve(stl.m_facets.size());
  m_edges.reserve(stl.m_facets.size() * 3);
  for (const auto &facet : stl.m_facets) {
    // Populate the vertex index triplet that matches the facet's vertices
    std::array<size_t, 3> vertexIndexTriplet = {INVALID_INDEX, INVALID_INDEX,
                                                INVALID_INDEX};
    for (auto const &[j, facetVertex] : enumerate(facet.vertices)) {
      auto const distance = std::distance(
          m_vertices.begin(),
          std::lower_bound(m_vertices.begin(), m_vertices.end(), facetVertex));
      vertexIndexTriplet.at(j) = std::size_t(distance);
    }

    // Never mind edges from  this facet if two vertices are the same
    if (vertexIndexTriplet.at(0) != vertexIndexTriplet.at(1) and
        vertexIndexTriplet.at(0) != vertexIndexTriplet.at(2) and
        vertexIndexTriplet.at(1) != vertexIndexTriplet.at(2)) {
      std::array<Edge, 3> edgeTriplet = {
          Edge{m_vertices,
               {vertexIndexTriplet.at(0), vertexIndexTriplet.at(1)}},
          Edge{m_vertices,
               {vertexIndexTriplet.at(1), vertexIndexTriplet.at(2)}},
          Edge{m_vertices,
               {vertexIndexTriplet.at(2), vertexIndexTriplet.at(0)}}};
      std::sort(edgeTriplet.begin(), edgeTriplet.end());

      consideredEdgeTriplets.emplace_back(edgeTriplet);
      for (auto const &edgeSuggestion : edgeTriplet) {
        m_edges.emplace_back(edgeSuggestion);
      }
    }
  }
  std::sort(m_edges.begin(), m_edges.end());
  auto const endOfUniqueEdges = std::unique(m_edges.begin(), m_edges.end());
  m_edges.erase(endOfUniqueEdges, m_edges.end());

  std::sort(consideredEdgeTriplets.begin(), consideredEdgeTriplets.end());
  auto const endOfUniqueEdgeTriplets =
      std::unique(consideredEdgeTriplets.begin(), consideredEdgeTriplets.end());
  consideredEdgeTriplets.erase(endOfUniqueEdgeTriplets,
                               consideredEdgeTriplets.end());

  SPDLOG_LOGGER_DEBUG(logger, "found {} unique edges", m_edges.size());
  SPDLOG_LOGGER_DEBUG(logger, "found {} unique edge triplets",
                      consideredEdgeTriplets.size());

  // TRIANGLES
  m_triangles.reserve(consideredEdgeTriplets.size());
  for (auto const &[i, edgeTriplet] : enumerate(consideredEdgeTriplets)) {
    std::array<size_t, 3> edgeIndices{INVALID_INDEX, INVALID_INDEX,
                                      INVALID_INDEX};
    for (auto const &[q, edgeSuggestion] : enumerate(edgeTriplet)) {
      auto const distance = std::distance(
          m_edges.begin(),
          std::lower_bound(m_edges.begin(), m_edges.end(), edgeSuggestion));
      (void)distance;
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

  SPDLOG_LOGGER_DEBUG(logger, "found {} triangles", m_triangles.size());

  for (auto const &[i, triangle] : enumerate(m_triangles)) {
    m_edges[triangle.m_edgeIndices[0]].m_users.push_back(i);
    m_edges[triangle.m_edgeIndices[1]].m_users.push_back(i);
    m_edges[triangle.m_edgeIndices[2]].m_users.push_back(i);
  }

  SPDLOG_LOGGER_DEBUG(logger, "finished loading Mesh from Stl");
}
