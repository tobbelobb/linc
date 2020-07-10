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

MeshClipper::MeshClipper(Mesh const &mesh) {
  if (logger == nullptr) {
    logger = spdlog::get("file_logger");
  }
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s(%#)] [%!] [%l] %v");
  spdlog::set_level(spdlog::level::trace);
  // Allocate some memory and fill with default data
  m_points.assign(mesh.m_vertices.size(), {Vertex::Zero(), 0.0_mm});
  m_edges.assign(mesh.m_edges.size(), {m_points});
  m_triangles.assign(mesh.m_triangles.size(), {m_edges});

  // Copy over data from Mesh object
  for (auto const &[i, vertex] : enumerate(mesh.m_vertices)) {
    m_points[i] = Point{vertex};
  }
  for (auto const &[i, meshEdge] : enumerate(mesh.m_edges)) {
    m_edges[i] = Edge{m_points, meshEdge.m_vertexIndices, meshEdge.m_users};
  }
  for (auto const &[i, meshTriangle] : enumerate(mesh.m_triangles)) {
    m_triangles[i] = Triangle{m_edges,
                              {meshTriangle.m_edgeIndices.at(0),
                               meshTriangle.m_edgeIndices.at(1),
                               meshTriangle.m_edgeIndices.at(2), INVALID_INDEX},
                              meshTriangle.m_normal};
  }
  if (removeNonTriangularTriangles()) {
    SPDLOG_LOGGER_DEBUG(logger, "Removed something");
  }
}

auto MeshClipper::removeNonTriangularTriangles() -> bool {
  // Make triangles with 1, 2, or 4 points invisible
  size_t const visibleTrianglesBeforeClean = countVisibleTriangles();
  for (auto &triangle : m_triangles) {
    if (triangle.m_visible) {
      std::set<Point> points{};
      for (auto const &edgeIndex : triangle.m_edgeIndices) {
        if (edgeIndex != INVALID_INDEX) {
          points.insert(triangle.m_edges[edgeIndex].point0());
          points.insert(triangle.m_edges[edgeIndex].point1());
        }
      }
      if (points.size() != 3) {
        triangle.m_visible = false;
      }
    }
  }
  size_t const visibleTrianglesAfterClean = countVisibleTriangles();
  if (visibleTrianglesAfterClean != visibleTrianglesBeforeClean) {
    SPDLOG_LOGGER_DEBUG(logger,
                        "Made {} triangles invisible because they had"
                        " 1, 2, or 4 corners.",
                        visibleTrianglesBeforeClean -
                            visibleTrianglesAfterClean);
    return true;
  }
  return false;
}

void MeshClipper::setDistances(Millimeter const zCut) {
  double constexpr eps = 1e-4;
  for (auto &point : m_points) {
    double const distance = point.z() - zCut;
    if (abs(distance) > eps) {
      point.m_distance = distance;
    } else {
      point.m_vertex.z() = zCut;
      point.m_distance = 0.0;
    }
  }
}

void MeshClipper::setPointsVisibility() {
  for (auto &point : m_points) {
    point.m_visible = (point.m_distance <= 0.0);
  }
}

auto MeshClipper::softMaxHeight() const -> double {
  if (m_points.empty()) {
    return 0.0;
  }
  if (std::none_of(m_points.begin(), m_points.end(),
                   [](Point const &p) { return p.m_visible; })) {
    return 0.0;
  }
  // At least one visible point exists. Find it.
  auto it = m_points.begin();
  while (not((*it).m_visible)) {
    ++it;
  }
  double max = (*it).z();
  for (; it != m_points.end(); ++it) {
    if ((*it).m_visible and (*it).z() > max) {
      max = (*it).z();
    }
  }
  return max;
}

auto MeshClipper::maxHeight() const -> double {
  if (m_points.empty()) {
    return 0.0;
  }
  return (*std::max_element(m_points.begin(), m_points.end(),
                            [](Point const &point0, Point const &point1) {
                              return point0.z() < point1.z();
                            }))
      .z();
}

auto MeshClipper::minHeight() const -> double {
  if (m_points.empty()) {
    return 0.0;
  }
  return (*std::min_element(m_points.begin(), m_points.end(),
                            [](Point const &point0, Point const &point1) {
                              return point0.z() < point1.z();
                            }))
      .z();
}

auto MeshClipper::countVisiblePoints() const -> std::size_t {
  return static_cast<std::size_t>(
      std::count_if(m_points.begin(), m_points.end(),
                    [](Point const &point) { return point.m_visible; }));
}

auto MeshClipper::countVisibleEdges() const -> std::size_t {
  return static_cast<std::size_t>(
      std::count_if(m_edges.begin(), m_edges.end(),
                    [](Edge const &edge) { return edge.m_visible; }));
}

auto MeshClipper::countVisibleTriangles() const -> std::size_t {
  return static_cast<std::size_t>(std::count_if(
      m_triangles.begin(), m_triangles.end(),
      [](Triangle const &triangle) { return triangle.m_visible; }));
}

auto MeshClipper::isAllPointsVisible() const -> bool {
  return std::all_of(m_points.begin(), m_points.end(),
                     [](auto const &point) { return point.m_visible; });
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
static auto pointAlong(MeshClipper::Edge const &edge, double const t)
    -> MeshClipper::Point {
  Vertex const newVertex =
      (1 - t) * edge.point0().m_vertex + t * edge.point1().m_vertex;
  return {{newVertex.x(), newVertex.y(), newVertex.z()}, 0.0, 0, true};
}

void MeshClipper::adjustEdges(Millimeter const zCut) {
  SPDLOG_LOGGER_DEBUG(logger, "Adjusting edges");
  std::size_t const numEdges = m_edges.size();
  for (std::size_t edgeIndex{0}; edgeIndex < numEdges; ++edgeIndex) {
    Edge &edge = m_edges[edgeIndex];
    if (edge.m_visible) {
      double const distance0 = edge.point0().m_distance;
      double const distance1 = edge.point1().m_distance;
      if (distance0 >= 0.0 and distance1 >= 0.0) {
        // Edge is entirely above cutting plane
        edge.m_visible = false;
        propagateInvisibilityToUsers(edgeIndex, edge);
      } else if ((distance0 > 0.0 and distance1 < 0.0) or
                 (distance0 < 0.0 and distance1 > 0.0)) {
        // Edge is split by the plane, we need a new point
        Point newPoint{pointAlong(edge, distance0 / (distance0 - distance1))};
        newPoint.m_vertex.z() = zCut; // Hedge against truncation errors

        std::size_t const newPointIndex = m_points.size();
        m_points.emplace_back(newPoint);

        if (distance0 > 0.0) {
          edge.m_pointIndices[0] = newPointIndex;
        } else {
          edge.m_pointIndices[1] = newPointIndex;
        }
      }
    }
  }
}

void MeshClipper::close2EdgeOpenTriangle(size_t const triangleIndex) {
  Triangle &triangle = m_triangles[triangleIndex];

  std::size_t const newEdgeIndex = m_edges.size();
  m_edges.emplace_back(Edge{m_points,
                            {triangle.m_integrity.startPointIndex,
                             triangle.m_integrity.endPointIndex},
                            {triangleIndex}});
  auto *const emptySpot =
      std::find(triangle.m_edgeIndices.begin(), triangle.m_edgeIndices.end(),
                INVALID_INDEX);

  auto const insertEdgeIndexAt = static_cast<std::size_t>(
      std::distance(triangle.m_edgeIndices.begin(), emptySpot));
  triangle.m_edgeIndices.at(insertEdgeIndexAt) = newEdgeIndex;
}

void MeshClipper::close3EdgeOpenTriangle(size_t const triangleIndex) {
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
      if (m_edges[edgeIndex].m_pointIndices[0] ==
          triangle.m_integrity.endPointIndex) {
        crossPointIndex = m_edges[edgeIndex].m_pointIndices[1];
        betweenEdgeIndex = edgeIndex;
        edgeIndex = newNewEdgeIndex;
      } else if (m_edges[edgeIndex].m_pointIndices[1] ==
                 triangle.m_integrity.endPointIndex) {
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
  m_edges.emplace_back(Edge{m_points,
                            {triangle.m_integrity.startPointIndex,
                             triangle.m_integrity.endPointIndex},
                            {newTriangleIndex}});
  // The newNewEdge
  m_edges.emplace_back(
      Edge{m_points,
           {triangle.m_integrity.startPointIndex, crossPointIndex},
           {triangleIndex, newTriangleIndex}});

  // The new triangle
  m_triangles.emplace_back(
      Triangle{m_edges,
               {newEdgeIndex, betweenEdgeIndex, newNewEdgeIndex, INVALID_INDEX},
               triangle.m_normal});
}

void MeshClipper::adjustTriangles() {
  SPDLOG_LOGGER_DEBUG(logger, "Adjusting triangles");
  std::size_t const numTriangles = m_triangles.size();
  for (std::size_t triangleIndex{0}; triangleIndex < numTriangles;
       ++triangleIndex) {
    Triangle &triangle = m_triangles[triangleIndex];
    if (triangle.m_visible) {
      triangle.updateIntegrity();
      if (triangle.m_integrity.isOpen) {
        // Add the new edge
        if (triangle.m_integrity.numEdges == 2) {
          close2EdgeOpenTriangle(triangleIndex);
        } else if (triangle.m_integrity.numEdges == 3) {
          close3EdgeOpenTriangle(triangleIndex);
        } else {
          SPDLOG_LOGGER_ERROR(
              logger, "Cannot close 1- or 4-edge triangle with one new edge.");
        }
      }
    }
  }
}

// Return how much was soft-clipped
auto MeshClipper::softClip(Millimeter const zCut) -> double {
  SPDLOG_LOGGER_DEBUG(logger, "Soft clipping at z={}", zCut);
  setDistances(zCut);
  double const oldSoftMaxHeight = softMaxHeight();
  setPointsVisibility();
  if (isAllPointsVisible()) {
    SPDLOG_LOGGER_INFO(logger, "Special case: All points visible.");
    return 0.0;
  }
  if (not std::any_of(m_points.begin(), m_points.end(),
                      [zCut](Point const &point) {
                        return point.m_visible and point.z() < zCut;
                      })) {
    SPDLOG_LOGGER_INFO(logger, "Special case: No z-thickness left.");
    return oldSoftMaxHeight;
  }

  adjustEdges(zCut);
  SPDLOG_LOGGER_DEBUG(
      logger,
      "There exists {} points, {} edges, and {} triangles after adjustEdges().",
      m_points.size(), m_edges.size(), m_triangles.size());
  adjustTriangles();
  SPDLOG_LOGGER_DEBUG(logger,
                      "There exists {} points, {} edges, and {} triangles "
                      "after adjustTriangles().",
                      m_points.size(), m_edges.size(), m_triangles.size());

  if (removeNonTriangularTriangles()) {
    SPDLOG_LOGGER_DEBUG(logger, "Removed something");
  }
  return oldSoftMaxHeight - zCut;
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
      std::set<Point> points{};
      for (auto const &edgeIndex : triangle.m_edgeIndices) {
        if (edgeIndex != INVALID_INDEX) {
          points.insert(triangle.m_edges[edgeIndex].point0());
          points.insert(triangle.m_edges[edgeIndex].point1());
        }
      }
      SmallFacet smallFacet{};
      smallFacet.normal[0] = static_cast<float>(triangle.m_normal.x());
      smallFacet.normal[1] = static_cast<float>(triangle.m_normal.y());
      smallFacet.normal[2] = static_cast<float>(triangle.m_normal.z());
      size_t i{0};
      for (auto it{points.begin()}; it != points.end(); ++it, ++i) {
        smallFacet.vertices.at(i).at(0) =
            static_cast<float>((*it).m_vertex.x());
        smallFacet.vertices.at(i).at(1) =
            static_cast<float>((*it).m_vertex.y());
        smallFacet.vertices.at(i).at(2) =
            static_cast<float>((*it).m_vertex.z());
      }
      std::array<char, SIZEOF_STL_FACET> stlFacetBytes{'\0'};
      std::memcpy(stlFacetBytes.data(), &smallFacet, SIZEOF_STL_FACET);
      myfile.write(stlFacetBytes.data(), SIZEOF_STL_FACET);
    }
  }

  myfile.close();
}

auto MeshClipper::getTopVertices() const -> std::vector<Vertex> {
  double const height{softMaxHeight()};
  std::vector<Vertex> res{};
  for (auto const &point : m_points) {
    if (point.m_visible and (point.z() <= VertexConstants::eps + height) and
        (height - VertexConstants::eps <= point.z())) {
      res.emplace_back(point.m_vertex);
    }
  }
  return res;
}
