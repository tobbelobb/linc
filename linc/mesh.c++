#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG /* NOLINT */
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <linc/mesh.h++>
#include <linc/util.h++>
static auto logger = spdlog::get("file_logger");

auto Mesh::maxHeight() const -> double {
  if (m_vertices.empty()) {
    return 0.0;
  }
  return (*std::max_element(m_vertices.begin(), m_vertices.end(),
                            [](Vertex const &vertex0, Vertex const &vertex1) {
                              return vertex0.z() < vertex1.z();
                            }))
      .z();
}

auto Mesh::minHeight() const -> double {
  if (m_vertices.empty()) {
    return 0.0;
  }
  return (*std::min_element(m_vertices.begin(), m_vertices.end(),
                            [](Vertex const &vertex0, Vertex const &vertex1) {
                              return vertex0.z() < vertex1.z();
                            }))
      .z();
}

void Mesh::loadVertices(std::vector<Stl::Facet> const &facets) {
  m_vertices.reserve(facets.size() * 3);
  for (const auto &facet : facets) {
    for (const auto &vertex : facet.vertices) {
      m_vertices.push_back(vertex);
    }
  }
  SPDLOG_LOGGER_TRACE(logger, "found {} vertices", m_vertices.size());
  std::sort(m_vertices.begin(), m_vertices.end());
  auto const endOfUniqueVertices =
      std::unique(m_vertices.begin(), m_vertices.end());
  m_vertices.erase(endOfUniqueVertices, m_vertices.end());
  SPDLOG_LOGGER_TRACE(logger, "found {} unique vertices", m_vertices.size());
}

auto Mesh::extractEdgeTriplets(std::vector<Stl::Facet> const &facets)
    -> std::vector<std::array<Mesh::Edge, 3>> {
  std::vector<std::array<Edge, 3>> edgeTriplets{};
  edgeTriplets.reserve(facets.size());
  for (const auto &facet : facets) {
    // Populate the vertex index triplet that matches the facet's vertices
    std::array<size_t, 3> vertexIndexTriplet = {INVALID_INDEX, INVALID_INDEX,
                                                INVALID_INDEX};
    for (auto const &[j, facetVertex] : enumerate(facet.vertices)) {
      auto const distance = static_cast<std::size_t>(std::distance(
          m_vertices.begin(),
          std::lower_bound(m_vertices.begin(), m_vertices.end(), facetVertex)));
      vertexIndexTriplet.at(j) = distance;
    }
    // Never mind edges from  this facet if two vertices are the same
    if (vertexIndexTriplet[0] != vertexIndexTriplet[1] and
        vertexIndexTriplet[0] != vertexIndexTriplet[2] and
        vertexIndexTriplet[1] != vertexIndexTriplet[2]) {
      std::array<Edge, 3> edgeTriplet = {
          Edge{m_vertices, {vertexIndexTriplet[0], vertexIndexTriplet[1]}},
          Edge{m_vertices, {vertexIndexTriplet[1], vertexIndexTriplet[2]}},
          Edge{m_vertices, {vertexIndexTriplet[2], vertexIndexTriplet[0]}}};
      std::sort(edgeTriplet.begin(), edgeTriplet.end());

      edgeTriplets.emplace_back(edgeTriplet);
    }
  }
  SPDLOG_LOGGER_TRACE(logger, "found {} edge triplets",
                      consideredEdgeTriplets.size());
  std::sort(edgeTriplets.begin(), edgeTriplets.end());
  auto const endOfUniqueEdgeTriplets =
      std::unique(edgeTriplets.begin(), edgeTriplets.end());
  edgeTriplets.erase(endOfUniqueEdgeTriplets, edgeTriplets.end());
  SPDLOG_LOGGER_TRACE(logger, "found {} unique edge triplets",
                      edgeTriplets.size());

  return edgeTriplets;
}

void Mesh::loadEdges(
    std::vector<std::array<Mesh::Edge, 3>> const &edgeTriplets) {
  m_edges.reserve(edgeTriplets.size() * 3);
  for (auto const &edgeTriplet : edgeTriplets) {
    for (auto const &edge : edgeTriplet) {
      m_edges.emplace_back(edge);
    }
  }
  SPDLOG_LOGGER_TRACE(logger, "found {} edges", m_edges.size());
  std::sort(m_edges.begin(), m_edges.end());
  auto const endOfUniqueEdges = std::unique(m_edges.begin(), m_edges.end());
  m_edges.erase(endOfUniqueEdges, m_edges.end());
  SPDLOG_LOGGER_TRACE(logger, "found {} unique edges", m_edges.size());
}

void Mesh::loadTriangles(
    std::vector<std::array<Mesh::Edge, 3>> const &edgeTriplets) {
  m_triangles.reserve(edgeTriplets.size());
  for (auto const &[i, edgeTriplet] : enumerate(edgeTriplets)) {
    std::array<size_t, 3> edgeIndices{INVALID_INDEX, INVALID_INDEX,
                                      INVALID_INDEX};
    for (auto const &[q, edgeSuggestion] : enumerate(edgeTriplet)) {
      auto const distance = static_cast<std::size_t>(std::distance(
          m_edges.begin(),
          std::lower_bound(m_edges.begin(), m_edges.end(), edgeSuggestion)));
      edgeIndices.at(q) = distance;
    }
    if (edgeIndices[0] != edgeIndices[1] and
        edgeIndices[0] != edgeIndices[2] and edgeIndices[1] != edgeIndices[2]) {
      m_triangles.emplace_back(Triangle{m_edges, edgeIndices});
    }
  }

  SPDLOG_LOGGER_TRACE(logger, "found {} triangles", m_triangles.size());

  for (auto const &[i, triangle] : enumerate(m_triangles)) {
    m_edges[triangle.m_edgeIndices[0]].m_users.push_back(i);
    m_edges[triangle.m_edgeIndices[1]].m_users.push_back(i);
    m_edges[triangle.m_edgeIndices[2]].m_users.push_back(i);
  }
}

Mesh::Mesh(Stl const &stl) {
  if (logger == nullptr) {
    logger = spdlog::get("file_logger");
  }
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s(%#)] [%!] [%l] %v");
  spdlog::set_level(spdlog::level::trace);
  SPDLOG_LOGGER_DEBUG(logger, "Building Mesh from {} facets",
                      stl.m_facets.size());

  loadVertices(stl.m_facets);

  std::vector<std::array<Edge, 3>> const edgeTriplets =
      extractEdgeTriplets(stl.m_facets);

  loadEdges(edgeTriplets);

  loadTriangles(edgeTriplets);

  SPDLOG_LOGGER_DEBUG(logger, "finished loading Mesh from Stl");
}
