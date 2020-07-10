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

void Mesh::loadBoundingVolumes(Vertex const &min, Vertex const &max) {
  (void)max;
  BoundingVolume boundingVolume{};
  // Construct the bounding volumes
  // The 8 corner of a cube
  std::vector<Vertex> &verts = boundingVolume.m_vertices;
  verts.emplace_back(min.x(), min.y(), min.z());
  verts.emplace_back(min.x() + 50.0, min.y(), min.z());
  verts.emplace_back(min.x() + 50.0, min.y() + 50.0, min.z());
  verts.emplace_back(min.x(), min.y() + 50.0, min.z());
  verts.emplace_back(min.x(), min.y(), min.z() + 50.0);
  verts.emplace_back(min.x() + 50.0, min.y(), min.z() + 50.0);
  verts.emplace_back(min.x() + 50.0, min.y() + 50.0, min.z() + 50.0);
  verts.emplace_back(min.x(), min.y() + 50.0, min.z() + 50.0);
  // 18 Edges of a cube
  // Bottom plane
  std::vector<Edge> &edges = boundingVolume.m_edges;
  edges.emplace_back(Edge{verts, {0, 1}}); // 0
  edges.emplace_back(Edge{verts, {0, 2}}); // 1
  edges.emplace_back(Edge{verts, {0, 3}}); // 2
  edges.emplace_back(Edge{verts, {1, 2}}); // 3
  edges.emplace_back(Edge{verts, {2, 3}}); // 4
  // Top Plane
  edges.emplace_back(Edge{verts, {4, 5}}); // 5
  edges.emplace_back(Edge{verts, {4, 6}}); // 6
  edges.emplace_back(Edge{verts, {4, 7}}); // 7
  edges.emplace_back(Edge{verts, {5, 6}}); // 8
  edges.emplace_back(Edge{verts, {6, 7}}); // 9
  // Verticals
  edges.emplace_back(Edge{verts, {0, 4}}); // 10
  edges.emplace_back(Edge{verts, {1, 5}}); // 11
  edges.emplace_back(Edge{verts, {2, 6}}); // 12
  edges.emplace_back(Edge{verts, {3, 7}}); // 13
  // Vertical crossovers
  edges.emplace_back(Edge{verts, {0, 5}}); // 14
  edges.emplace_back(Edge{verts, {1, 6}}); // 15
  edges.emplace_back(Edge{verts, {2, 7}}); // 16
  edges.emplace_back(Edge{verts, {3, 4}}); // 17

  // 12 faces of a cube
  // Bottom plane
  std::vector<Triangle> &triangles = boundingVolume.m_triangles;
  triangles.emplace_back(Triangle{edges, {0, 1, 3}, {0, 0, -1}});
  triangles.emplace_back(Triangle{edges, {4, 1, 2}, {0, 0, -1}});
  // Top plane
  triangles.emplace_back(Triangle{edges, {5, 6, 8}, {0, 0, 1}});
  triangles.emplace_back(Triangle{edges, {9, 6, 7}, {0, 0, 1}});
  // -y side
  triangles.emplace_back(Triangle{edges, {0, 14, 11}, {0, -1, 0}});
  triangles.emplace_back(Triangle{edges, {5, 14, 10}, {0, -1, 0}});
  // x side
  triangles.emplace_back(Triangle{edges, {3, 15, 12}, {1, 0, 0}});
  triangles.emplace_back(Triangle{edges, {8, 15, 11}, {1, 0, 0}});
  // y side
  triangles.emplace_back(Triangle{edges, {4, 16, 13}, {0, 1, 0}});
  triangles.emplace_back(Triangle{edges, {9, 16, 12}, {0, 1, 0}});
  // -x side
  triangles.emplace_back(Triangle{edges, {2, 17, 10}, {-1, 0, 0}});
  triangles.emplace_back(Triangle{edges, {7, 17, 13}, {-1, 0, 0}});

  m_boundingVolumes.emplace_back(boundingVolume);

  // All vertices...
  // for (Millimeter x{min.x()}; x < max.x(); x += 50.0) {
  //  for (Millimeter y{min.y()}; y < max.y(); y += 50.0) {
  //    for (Millimeter z{min.z()}; z < max.z(); z += 50.0) {
  //      m_boundingVolumes.m_vertices.emplace_back(x, y, z);
  //    }
  //    m_boundingVolumes.m_vertices.emplace_back(x, y, max.z());
  //  }
  //  for (Millimeter z{min.z()}; z < max.z(); z += 50.0) {
  //    m_boundingVolumes.m_vertices.emplace_back(x, max.y(), z);
  //  }
  //  m_boundingVolumes.m_vertices.emplace_back(x, max.y(), max.z());
  //}
  // for (Millimeter y{min.y()}; y < max.y(); y += 50.0) {
  //  for (Millimeter z{min.z()}; z < max.z(); z += 50.0) {
  //    m_boundingVolumes.m_vertices.emplace_back(max.x(), y, z);
  //  }
  //  m_boundingVolumes.m_vertices.emplace_back(max.x(), y, max.z());
  //}
  // for (Millimeter z{min.z()}; z < max.z(); z += 50.0) {
  //  m_boundingVolumes.m_vertices.emplace_back(max.x(), max.y(), z);
  //}
  // m_boundingVolumes.m_vertices.emplace_back(max.x(), max.y(), max.z());
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

  constexpr Millimeter BOUNDING_VOLUME_THRESHOLD =
      (50.0_mm * 20 * 50.0_mm * 20 * 50.0_mm * 20);
  if (stl.m_stats.size.x() * stl.m_stats.size.y() * stl.m_stats.size.z() >
      BOUNDING_VOLUME_THRESHOLD) {
    loadBoundingVolumes(stl.m_stats.min, stl.m_stats.max);
  }

  SPDLOG_LOGGER_DEBUG(logger, "finished loading Mesh from Stl");
}
