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
  return (*std::max_element(m_points.begin(), m_points.end(),
                            [](Point const &point0, Point const &point1) {
                              return point1.m_visible and
                                     point0.z() < point1.z();
                            }))
      .z();
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

void MeshClipper::adjustEdges() {
  SPDLOG_LOGGER_DEBUG(logger, "Adjusting edges");
  for (std::size_t edgeIndex{0}; edgeIndex < m_edges.size(); ++edgeIndex) {
    Edge &edge = m_edges[edgeIndex];
    if (edge.m_visible) {
      double const distance0 = edge.point0().m_distance;
      double const distance1 = edge.point1().m_distance;
      if (distance0 >= 0.0 and distance1 >= 0.0) {
        // Edge is entirely above cutting plane
        // Go through edge's users and remove edge
        // from them
        for (auto const &triangleIndex : edge.m_users) {
          Triangle &triangle = m_triangles[triangleIndex];
          for (auto &triangleEdgeIndex : triangle.m_edgeIndices) {
            if (triangleEdgeIndex == edgeIndex) {
              triangleEdgeIndex = INVALID_INDEX;
            }
          }
          if (std::all_of(triangle.m_edgeIndices.begin(),
                          triangle.m_edgeIndices.end(),
                          [](std::size_t const triangleEdgeIndex) {
                            return triangleEdgeIndex == INVALID_INDEX;
                          })) {
            triangle.m_visible = false;
          }
        }
        edge.m_visible = false;
      } else if ((distance0 > 0.0 and distance1 < 0.0) or
                 (distance0 < 0.0 and distance1 > 0.0)) {
        // Edge is split by the plane
        // edge = point0 + t*(point1 - point0)
        // where t goes from 0 to 1.
        auto const t = distance0 / (distance0 - distance1);
        Vertex const newVertex =
            (1 - t) * edge.point0().m_vertex + t * edge.point1().m_vertex;
        Point newPoint{newVertex, 0.0, 0, true};
        std::size_t newPointIndex = m_points.size();
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

void MeshClipper::adjustTriangles() {
  SPDLOG_LOGGER_DEBUG(logger, "Adjusting triangles");
  for (std::size_t triangleIndex{0}; triangleIndex < m_triangles.size();
       ++triangleIndex) {
    Triangle &triangle = m_triangles[triangleIndex];
    if (triangle.m_visible) {
      SPDLOG_LOGGER_TRACE(logger, "Triangle {} is visible", triangleIndex);
      auto const [isOpen, startPointIndex, endPointIndex] = triangle.isOpen();
      if (isOpen) {
        SPDLOG_LOGGER_TRACE(logger, "Triangle {} is open", triangleIndex);
        std::size_t const newEdgeIndex = m_edges.size();
        m_edges.emplace_back(
            Edge{m_points, {startPointIndex, endPointIndex}, {triangleIndex}});
        SPDLOG_LOGGER_TRACE(
            logger, "Creating a new edge to close open triangle: {}, index {}",
            m_edges[newEdgeIndex], newEdgeIndex);
        auto *const emptySpot =
            std::find(triangle.m_edgeIndices.begin(),
                      triangle.m_edgeIndices.end(), INVALID_INDEX);
        if (emptySpot == triangle.m_edgeIndices.end()) {
          SPDLOG_LOGGER_ERROR(
              logger, "Triangle had no space for another edge. Returning.");
          return;
        }
        auto const whichEdge = static_cast<std::size_t>(
            std::distance(triangle.m_edgeIndices.begin(), emptySpot));
        SPDLOG_LOGGER_TRACE(logger, "Triangle has space for new edge at {}",
                            whichEdge);
        triangle.m_edgeIndices.at(whichEdge) = newEdgeIndex;
        if (std::all_of(triangle.m_edgeIndices.begin(),
                        triangle.m_edgeIndices.end(),
                        [](std::size_t const index) {
                          return index != INVALID_INDEX;
                        })) {
          SPDLOG_LOGGER_TRACE(logger,
                              "This triangle has four edges. Splitting it.");
          SPDLOG_LOGGER_TRACE(logger, "Disabling edge {} for triangle {}",
                              newEdgeIndex, triangleIndex);
          triangle.m_edgeIndices.at(whichEdge) = INVALID_INDEX;

          std::size_t const newTriangleIndex = m_triangles.size();
          std::size_t const newNewEdgeIndex = m_edges.size();

          // Find the non-new edge that shares the point at endPointIndex
          // That edge's other end is where we want to connect to from
          // startPointIndex
          std::size_t crossPointIndex = INVALID_INDEX;
          std::size_t betweenEdgeIndex = INVALID_INDEX;
          for (auto &edgeIndex : triangle.m_edgeIndices) {
            if (edgeIndex != INVALID_INDEX) {
              SPDLOG_LOGGER_TRACE(logger, "Investigating edge {}", edgeIndex);
              for (auto const &[a, b] : PairOfPairs{{{0, 1}, {1, 0}}}) {
                if (m_edges[edgeIndex].m_pointIndices[a] == endPointIndex) {
                  crossPointIndex = m_edges[edgeIndex].m_pointIndices[b];
                  betweenEdgeIndex = edgeIndex;
                  SPDLOG_LOGGER_TRACE(logger, "Found the betweenEdgeIndex: {}",
                                      betweenEdgeIndex);
                  // This edge shall switch triangleIndex to newTriangleIndex
                  // in its user list
                  for (auto &userIndex : m_edges[betweenEdgeIndex].m_users) {
                    if (userIndex == triangleIndex) {
                      SPDLOG_LOGGER_TRACE(
                          logger,
                          "Switching user {} to user {} for the betweenEdge",
                          userIndex, newTriangleIndex);
                      userIndex = newTriangleIndex;
                    }
                  }
                  // This triangle shall no longer use the betweenEdge.
                  // The newNewEdge shall be used instead.
                  SPDLOG_LOGGER_TRACE(
                      logger,
                      "Switching edge index {} to edge {} for triangle {}",
                      edgeIndex, newNewEdgeIndex, triangleIndex);
                  edgeIndex = newNewEdgeIndex;
                }
              }
            }
          }

          // Create the newNewEdge
          m_edges.emplace_back(Edge{m_points,
                                    {startPointIndex, crossPointIndex},
                                    {triangleIndex, newTriangleIndex}});
          SPDLOG_LOGGER_TRACE(logger, "Creating the newNewEdge: {}",
                              m_edges[newNewEdgeIndex]);
          m_triangles.emplace_back(Triangle{m_edges,
                                            {newEdgeIndex, betweenEdgeIndex,
                                             newNewEdgeIndex, INVALID_INDEX}});
          SPDLOG_LOGGER_TRACE(logger, "Creating a new triangle.");

          for (auto &userIndex : m_edges[newEdgeIndex].m_users) {
            if (userIndex == triangleIndex) {
              SPDLOG_LOGGER_TRACE(
                  logger, "Switching user {} to user {} for the newEdge",
                  userIndex, newTriangleIndex);
              userIndex = newTriangleIndex;
            }
          }
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

  adjustEdges();
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
  SPDLOG_LOGGER_DEBUG(logger, "Writing into header that we have {} facets",
                      facetCounter);
  myfile.write(reinterpret_cast<char const *>(&facetCounter),
               FACET_COUNTER_SIZE);

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
      smallFacet.normal = triangle.m_normal.cast<float>();
      size_t i{0};
      for (auto it{points.begin()}; it != points.end(); ++it, ++i) {
        smallFacet.vertices.at(i) = (*it).m_vertex.cast<float>();
      }
      myfile.write(reinterpret_cast<char const *>(&smallFacet),
                   SIZEOF_STL_FACET);
    }
  }

  myfile.close();
}
