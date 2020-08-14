#include <algorithm>
#include <array>
#include <fstream>
#include <set>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG /* NOLINT */
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <linc/mesh-clipper.h++>
#include <linc/stl.h++>
#include <linc/util.h++>

using PairOfPairs = std::array<std::array<std::size_t, 2>, 2>;

static auto logger = spdlog::get("file_logger");

void MeshClipper::loadVertices(std::vector<Stl::Facet> const &facets) {
  for (const auto &facet : facets) {
    for (const auto &vertex : facet.vertices) {
      m_points.emplace_back(vertex);
    }
  }
  SPDLOG_LOGGER_TRACE(logger, "found {} vertices", m_points.size());
  std::sort(m_points.begin(), m_points.end());
  auto const endOfUniqueVertices =
      std::unique(m_points.begin(), m_points.end());
  m_points.erase(endOfUniqueVertices, m_points.end());
  SPDLOG_LOGGER_TRACE(logger, "found {} unique vertices", m_points.size());
}

auto MeshClipper::extractEdgeTriplets(std::vector<Stl::Facet> const &facets)
    -> std::vector<std::array<MeshClipper::Edge, 3>> {
  std::vector<std::array<Edge, 3>> edgeTriplets{};
  edgeTriplets.reserve(facets.size());
  for (const auto &facet : facets) {
    // Populate the vertex index triplet that matches the facet's vertices
    std::array<size_t, 3> vertexIndexTriplet = {INVALID_INDEX, INVALID_INDEX,
                                                INVALID_INDEX};
    for (auto const &[j, facetVertex] : enumerate(facet.vertices)) {
      auto const distance = static_cast<std::size_t>(std::distance(
          m_points.begin(),
          std::lower_bound(m_points.begin(), m_points.end(), facetVertex)));
      vertexIndexTriplet.at(j) = distance;
    }
    // Never mind edges from  this facet if two vertices are the same
    if (vertexIndexTriplet[0] != vertexIndexTriplet[1] and
        vertexIndexTriplet[0] != vertexIndexTriplet[2] and
        vertexIndexTriplet[1] != vertexIndexTriplet[2]) {
      std::array<Edge, 3> edgeTriplet = {
          Edge{{vertexIndexTriplet[0], vertexIndexTriplet[1]}},
          Edge{{vertexIndexTriplet[1], vertexIndexTriplet[2]}},
          Edge{{vertexIndexTriplet[2], vertexIndexTriplet[0]}}};
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

void MeshClipper::loadEdges(
    std::vector<std::array<MeshClipper::Edge, 3>> const &edgeTriplets) {
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

void MeshClipper::loadTriangles(
    std::vector<std::array<MeshClipper::Edge, 3>> const &edgeTriplets) {
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
      m_triangles.emplace_back(edgeIndices);
    }
  }

  SPDLOG_LOGGER_TRACE(logger, "found {} triangles", m_triangles.size());

  for (auto const &[i, triangle] : enumerate(m_triangles)) {
    m_edges[triangle.m_edgeIndices[0]].m_users.emplace_back(i);
    m_edges[triangle.m_edgeIndices[1]].m_users.emplace_back(i);
    m_edges[triangle.m_edgeIndices[2]].m_users.emplace_back(i);
  }
}

MeshClipper::MeshClipper(Stl const &stl) {
  if (logger == nullptr) {
    logger = spdlog::get("file_logger");
  }
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%s(%#)] [%!] [%l] %v");
  spdlog::set_level(spdlog::level::trace);
  SPDLOG_LOGGER_DEBUG(logger, "Building MeshClipper from {} facets",
                      stl.m_facets.size());
  m_points.reserve(3 * stl.m_facets.size());
  loadVertices(stl.m_facets);

  std::vector<std::array<Edge, 3>> const edgeTriplets =
      extractEdgeTriplets(stl.m_facets);

  m_edges.reserve(edgeTriplets.size() * 3);
  loadEdges(edgeTriplets);

  m_triangles.reserve(edgeTriplets.size() + edgeTriplets.size() / 10);
  loadTriangles(edgeTriplets);

  SPDLOG_LOGGER_DEBUG(logger, "finished loading MeshClipper from Stl");
}

MeshClipper::MeshClipper(MeshClipper const &meshClipper) {
  m_points.reserve(meshClipper.m_points.capacity());
  m_edges.reserve(meshClipper.m_edges.capacity());
  m_triangles.reserve(meshClipper.m_triangles.capacity());

  // Copy over data from Mesh object
  for (auto const &vertex : meshClipper.m_points) {
    m_points.emplace_back(vertex);
  }
  for (auto const &meshEdge : meshClipper.m_edges) {
    m_edges.emplace_back(meshEdge.m_pointIndices, meshEdge.m_users);
  }
  for (auto const &meshTriangle : meshClipper.m_triangles) {
    m_triangles.emplace_back(std::array<size_t, 3>{
        meshTriangle.m_edgeIndices.at(0), meshTriangle.m_edgeIndices.at(1),
        meshTriangle.m_edgeIndices.at(2)});
  }
}

auto MeshClipper::getPointsVisibility(Millimeter const zCut)
    -> std::vector<bool> {
  double constexpr eps = VertexConstants::eps;
  std::vector<bool> visible{};
  visible.reserve(m_points.size() + m_points.size() / 10);
  for (auto &point : m_points) {
    double const distance = point.z() - zCut;
    if (abs(distance) > eps) {
      visible.emplace_back(distance <= 0.0);
    } else {
      point.z() = zCut;
      visible.emplace_back(true);
    }
  }
  return visible;
}

auto MeshClipper::softMaxHeight(std::vector<bool> const &visible) const
    -> double {
  if (m_points.size() != visible.size()) {
    return 0.0;
  }
  if (m_points.empty()) {
    return 0.0;
  }
  if (std::none_of(visible.begin(), visible.end(),
                   [](bool const b) { return b; })) {
    return 0.0;
  }
  // At least one visible point exists. Find it.
  auto itp = m_points.begin();
  auto itv = visible.begin();
  while (not(*itv)) {
    ++itp;
    ++itv;
  }
  double max = (*itp).z();
  for (; itp != m_points.end(); ++itp, ++itv) {
    if ((*itv) and (*itp).z() > max) {
      max = (*itp).z();
    }
  }
  return max;
}

auto MeshClipper::maxHeight() const -> double {
  if (m_points.empty()) {
    return 0.0;
  }
  return (*std::max_element(m_points.begin(), m_points.end(),
                            [](Vertex const &point0, Vertex const &point1) {
                              return point0.z() < point1.z();
                            }))
      .z();
}

auto MeshClipper::minHeight() const -> double {
  if (m_points.empty()) {
    return 0.0;
  }
  return (*std::min_element(m_points.begin(), m_points.end(),
                            [](Vertex const &point0, Vertex const &point1) {
                              return point0.z() < point1.z();
                            }))
      .z();
}

auto MeshClipper::countVisibleTriangles() const -> std::size_t {
  return static_cast<std::size_t>(std::count_if(
      m_triangles.begin(), m_triangles.end(),
      [](Triangle const &triangle) { return triangle.m_visible; }));
}

// Go through edge's users and remove edge
// from them
void MeshClipper::propagateInvisibilityToUsers(size_t const edgeIndex,
                                               Edge const &edge) {
  for (auto const &triangleIndex : edge.m_users) {
    Triangle &triangle = m_triangles[triangleIndex];
    for (auto &triangleEdgeIndex : triangle.m_edgeIndices) {
      if (triangleEdgeIndex == edgeIndex) {
        triangleEdgeIndex = INVALID_INDEX;
      }
    }
    // If this was the triangle's last visible edge,
    // the triangle has also become invisible
    if (std::all_of(triangle.m_edgeIndices.begin(),
                    triangle.m_edgeIndices.end(),
                    [](std::size_t const triangleEdgeIndex) {
                      return triangleEdgeIndex == INVALID_INDEX;
                    })) {
      triangle.m_visible = false;
    }
  }
}

// Creates new point along direction of edge
// newPoint = point0 + t*(point1 - point0)
// t = 0 gives back point0
// t = 1 gives back point1
// 0 < t < 1 gives back a point in between
auto MeshClipper::pointAlong(MeshClipper::Edge const &edge,
                             double const t) const -> Vertex {
  return (1 - t) * m_points[edge.m_pointIndices[0]] +
         t * m_points[edge.m_pointIndices[1]];
}

void MeshClipper::adjustEdges(Millimeter const zCut,
                              std::vector<bool> &pointVisibility) {
  SPDLOG_LOGGER_DEBUG(logger, "Adjusting edges");
  std::size_t const numEdges = m_edges.size();
  for (std::size_t edgeIndex{0}; edgeIndex < numEdges; ++edgeIndex) {
    Edge &edge = m_edges[edgeIndex];
    bool const visible0 = pointVisibility.at(edge.m_pointIndices[0]);
    bool const visible1 = pointVisibility.at(edge.m_pointIndices[1]);

    if ((not visible0 and
         (m_points[edge.m_pointIndices[1]].z() - zCut) >= 0.0) or
        (not visible1 and
         (m_points[edge.m_pointIndices[0]].z() - zCut) >= 0.0)) {
      // Edge is entirely above cutting plane
      propagateInvisibilityToUsers(edgeIndex, edge);
    } else if (not visible0 or not visible1) {
      double const distance0 = m_points[edge.m_pointIndices[0]].z() - zCut;
      double const distance1 = m_points[edge.m_pointIndices[1]].z() - zCut;
      if ((distance0 < 0.0 and distance1 > 0.0) or
          (distance0 > 0.0 and distance1 < 0.0)) {
        // Edge is split by the plane, we need a new point
        Vertex newPoint{pointAlong(edge, distance0 / (distance0 - distance1))};
        newPoint.z() = zCut; // Hedge against truncation errors

        std::size_t const newPointIndex = m_points.size();
        m_points.emplace_back(newPoint);
        pointVisibility.emplace_back(true);

        if (distance0 > 0.0) {
          edge.m_pointIndices[0] = newPointIndex;
        } else {
          edge.m_pointIndices[1] = newPointIndex;
        }
      }
    }
  }
}

void MeshClipper::close2EdgeOpenTriangle(std::size_t const triangleIndex,
                                         Opening const &opening) {
  Triangle &triangle = m_triangles[triangleIndex];

  std::size_t const newEdgeIndex = m_edges.size();
  m_edges.emplace_back(
      Edge{{opening.startPointIndex, opening.endPointIndex}, {triangleIndex}});
  auto *const emptySpot =
      std::find(triangle.m_edgeIndices.begin(), triangle.m_edgeIndices.end(),
                INVALID_INDEX);

  auto const insertEdgeIndexAt = static_cast<std::size_t>(
      std::distance(triangle.m_edgeIndices.begin(), emptySpot));
  triangle.m_edgeIndices.at(insertEdgeIndexAt) = newEdgeIndex;
}

void MeshClipper::close3EdgeOpenTriangle(std::size_t const triangleIndex,
                                         Opening const &opening) {
  Triangle &triangle = m_triangles[triangleIndex];

  // We will add one triangle
  std::size_t const newTriangleIndex = m_triangles.size();
  // We will add two new edges
  std::size_t const newEdgeIndex = m_edges.size();
  std::size_t const newNewEdgeIndex = newEdgeIndex + 1;
  // One from startPointIndex to endPointIndex
  // One from startPointIndex to crossPointIndex
  std::size_t crossPointIndex = INVALID_INDEX;
  // The new triangle will use both new edges, but also one
  // old edge, called the betweenEdge
  // The betweenEdge has one end in the endPointIndex
  // ... and one end in crossPointIndex
  std::size_t betweenEdgeIndex = INVALID_INDEX;
  // The old triangle will use two old edges and the newNewEdge
  // The old triangle will no longer use the betweenEdge

  // Look at the three edges. One of them is the betweenEdge.
  // That edge's other end defines the crossPointIndex
  for (auto &edgeIndex : triangle.m_edgeIndices) {
    if (edgeIndex != INVALID_INDEX) {
      if (m_edges[edgeIndex].m_pointIndices[0] == opening.endPointIndex) {
        crossPointIndex = m_edges[edgeIndex].m_pointIndices[1];
        betweenEdgeIndex = edgeIndex;
        edgeIndex = newNewEdgeIndex;
      } else if (m_edges[edgeIndex].m_pointIndices[1] ==
                 opening.endPointIndex) {
        crossPointIndex = m_edges[edgeIndex].m_pointIndices[0];
        betweenEdgeIndex = edgeIndex;
        edgeIndex = newNewEdgeIndex;
      }
    }
  }

  // The betweenEdge was previously used by old triangle
  // It will be used by new triangle instead
  for (auto &betweenEdgeUserIndex : m_edges[betweenEdgeIndex].m_users) {
    if (betweenEdgeUserIndex == triangleIndex) {
      betweenEdgeUserIndex = newTriangleIndex;
    }
  }

  // The newEdge
  m_edges.emplace_back(Edge{{opening.startPointIndex, opening.endPointIndex},
                            {newTriangleIndex}});
  // The newNewEdge
  m_edges.emplace_back(Edge{{opening.startPointIndex, crossPointIndex},
                            {triangleIndex, newTriangleIndex}});

  // The new triangle
  m_triangles.emplace_back(
      Triangle{{newEdgeIndex, betweenEdgeIndex, newNewEdgeIndex}});
}

void MeshClipper::adjustTriangles() {
  SPDLOG_LOGGER_DEBUG(logger, "Adjusting triangles");
  std::size_t const numTriangles = m_triangles.size();
  for (std::size_t triangleIndex{0}; triangleIndex < numTriangles;
       ++triangleIndex) {
    Triangle &triangle = m_triangles[triangleIndex];
    if (triangle.m_visible) {
      Opening const opening = getOpening(triangle);
      if (opening.endPointIndex != INVALID_INDEX) {
        // Add the new edge
        auto const numEdges = static_cast<std::size_t>(std::count_if(
            triangle.m_edgeIndices.begin(), triangle.m_edgeIndices.end(),
            [](auto const index) { return index != INVALID_INDEX; }));
        if (numEdges == 2) {
          close2EdgeOpenTriangle(triangleIndex, opening);
        } else if (numEdges == 3) {
          close3EdgeOpenTriangle(triangleIndex, opening);
        } else {
          SPDLOG_LOGGER_ERROR(
              logger,
              "Cannot close 1-edge or 0-edge triangle with one new edge.");
        }
      }
    }
  }
}

// Returns vector saying which points are visible
auto MeshClipper::softClip(Millimeter const zCut) -> std::vector<bool> {
  SPDLOG_LOGGER_DEBUG(logger, "Soft clipping at z={}", zCut);

  std::vector<bool> visible{getPointsVisibility(zCut)};
  assert(visible.size() == m_points.size());
  if (std::all_of(visible.begin(), visible.end(),
                  [](bool const b) { return b; })) {
    SPDLOG_LOGGER_INFO(logger, "Special case: All points visible.");
    return visible;
  }
  if (std::none_of(m_points.begin(), m_points.end(),
                   [zCut](Vertex const &point) { return point.z() < zCut; })) {
    SPDLOG_LOGGER_INFO(logger, "Special case: No z-thickness left.");
    for (auto &triangle : m_triangles) {
      triangle.m_visible = false;
    }
    // There might still be visible points
    return visible;
  }

  adjustEdges(zCut, visible);
  SPDLOG_LOGGER_TRACE(
      logger,
      "There exists {} points, {} edges, and {} triangles after adjustEdges().",
      m_points.size(), m_edges.size(), m_triangles.size());
  adjustTriangles();
  SPDLOG_LOGGER_TRACE(logger,
                      "There exists {} points, {} edges, and {} triangles "
                      "after adjustTriangles().",
                      m_points.size(), m_edges.size(), m_triangles.size());

  return visible;
}

void MeshClipper::writeBinaryStl(std::string const &fileName) const {
  SPDLOG_LOGGER_DEBUG(logger, "Writing binary stl: {}", fileName);
  auto myfile = std::fstream(fileName, std::ios::out | std::ios::binary);
  constexpr std::array<char const, LABEL_SIZE> emptyHeader = {'\0'};
  myfile.write(emptyHeader.data(), LABEL_SIZE);

  auto const facetCounter = static_cast<uint32_t>(countVisibleTriangles());
  // We would like to
  // #include <bit>
  // auto facetCounterBytes{std::bit_cast<char,
  // FACET_COUNTER_SIZE>(facetCounter)};
  // ... but bit_cast is not available yet as of July 3, 2020.
  std::array<char, FACET_COUNTER_SIZE> facetCounterBytes{'\0'};
  std::memcpy(facetCounterBytes.data(), &facetCounter, FACET_COUNTER_SIZE);
  SPDLOG_LOGGER_DEBUG(logger, "Writing into header that we have {} facets",
                      facetCounter);
  myfile.write(facetCounterBytes.data(), FACET_COUNTER_SIZE);

  for (auto const &triangle : m_triangles) {
    if (triangle.m_visible) {
      // Only write visible triangles
      std::set<Vertex> points{};
      for (auto const &edgeIndex : triangle.m_edgeIndices) {
        if (edgeIndex != INVALID_INDEX) {
          points.insert(m_points[m_edges[edgeIndex].m_pointIndices[0]]);
          points.insert(m_points[m_edges[edgeIndex].m_pointIndices[1]]);
        }
      }
      SmallFacet smallFacet{};
      smallFacet.normal[0] = 0.0;
      smallFacet.normal[1] = 0.0;
      smallFacet.normal[2] = 0.0;
      size_t i{0};
      for (auto it{points.begin()}; it != points.end(); ++it, ++i) {
        smallFacet.vertices.at(i).at(0) = static_cast<float>((*it).x());
        smallFacet.vertices.at(i).at(1) = static_cast<float>((*it).y());
        smallFacet.vertices.at(i).at(2) = static_cast<float>((*it).z());
      }
      std::array<char, SIZEOF_STL_FACET> stlFacetBytes{'\0'};
      std::memcpy(stlFacetBytes.data(), &smallFacet, SIZEOF_STL_FACET);
      myfile.write(stlFacetBytes.data(), SIZEOF_STL_FACET);
    }
  }

  myfile.close();
}

auto MeshClipper::getVerticesAt(Millimeter const height) const
    -> std::vector<Vertex> {
  std::vector<Vertex> res{};
  for (auto const &point : m_points) {
    if ((point.z() <= VertexConstants::eps + height) and
        (height - VertexConstants::eps <= point.z())) {
      res.emplace_back(point);
    }
  }
  return res;
}

auto MeshClipper::getOpening(MeshClipper::Triangle const &triangle) const
    -> MeshClipper::Opening {
  std::size_t startPointIndex = INVALID_INDEX;
  std::size_t endPointIndex = INVALID_INDEX;
  std::array<std::size_t, 6> indices{INVALID_INDEX};
  if (triangle.m_edgeIndices[0] != INVALID_INDEX) {
    indices[0] = m_edges[triangle.m_edgeIndices[0]].m_pointIndices[0];
    indices[1] = m_edges[triangle.m_edgeIndices[0]].m_pointIndices[1];
  }
  if (triangle.m_edgeIndices[1] != INVALID_INDEX) {
    indices[2] = m_edges[triangle.m_edgeIndices[1]].m_pointIndices[0];
    indices[3] = m_edges[triangle.m_edgeIndices[1]].m_pointIndices[1];
  }
  if (triangle.m_edgeIndices[2] != INVALID_INDEX) {
    indices[4] = m_edges[triangle.m_edgeIndices[2]].m_pointIndices[0];
    indices[5] = m_edges[triangle.m_edgeIndices[2]].m_pointIndices[1];
  }

  if (triangle.m_edgeIndices[0] != INVALID_INDEX) {
    if (indices[0] != indices[1] and indices[0] != indices[2] and
        indices[0] != indices[3] and indices[0] != indices[4] and
        indices[0] != indices[5]) {
      startPointIndex = indices[0];
    }
    if (indices[1] != indices[0] and indices[1] != indices[2] and
        indices[1] != indices[3] and indices[1] != indices[4] and
        indices[1] != indices[5]) {
      if (startPointIndex == INVALID_INDEX) {
        startPointIndex = indices[1];
      } else {
        endPointIndex = indices[1];
      }
    }
  }
  if (triangle.m_edgeIndices[1] != INVALID_INDEX) {
    if (indices[2] != indices[0] and indices[2] != indices[1] and
        indices[2] != indices[3] and indices[2] != indices[4] and
        indices[2] != indices[5]) {
      if (startPointIndex == INVALID_INDEX) {
        startPointIndex = indices[2];
      } else {
        endPointIndex = indices[2];
      }
    }
    if (indices[3] != indices[0] and indices[3] != indices[1] and
        indices[3] != indices[2] and indices[3] != indices[4] and
        indices[3] != indices[5]) {
      if (startPointIndex == INVALID_INDEX) {
        startPointIndex = indices[3];
      } else {
        endPointIndex = indices[3];
      }
    }
  }
  if (triangle.m_edgeIndices[2] != INVALID_INDEX) {
    if (indices[4] != indices[0] and indices[4] != indices[1] and
        indices[4] != indices[2] and indices[4] != indices[3] and
        indices[4] != indices[5]) {
      if (startPointIndex == INVALID_INDEX) {
        startPointIndex = indices[4];
      } else {
        endPointIndex = indices[4];
      }
    }
    if (indices[5] != indices[0] and indices[5] != indices[1] and
        indices[5] != indices[2] and indices[5] != indices[3] and
        indices[5] != indices[4]) {
      if (startPointIndex == INVALID_INDEX) {
        startPointIndex = indices[5];
      } else {
        endPointIndex = indices[5];
      }
    }
  }
  return {startPointIndex, endPointIndex};
}
