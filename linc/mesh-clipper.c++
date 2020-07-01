#include <algorithm>
#include <array>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG /* NOLINT */
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <linc/mesh-clipper.h++>
#include <linc/util.h++>

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

auto MeshClipper::isAllPointsVisible() const -> bool {
  return std::all_of(m_points.begin(), m_points.end(),
                     [](auto const &point) { return point.m_visible; });
}

void MeshClipper::adjustEdges() {
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
        if (distance0 < 0.0) {
          edge.m_pointIndices[0] = newPointIndex;
        } else {
          edge.m_pointIndices[1] = newPointIndex;
        }
      }
    }
  }
}

void MeshClipper::adjustTriangles() {
  for (std::size_t triangleIndex{0}; triangleIndex < m_triangles.size();
       ++triangleIndex) {
    Triangle &triangle = m_triangles[triangleIndex];
    if (triangle.m_visible) {
      auto const [isOpen, startPointIndex, endPointIndex] = triangle.isOpen();
      if (isOpen) {
        std::size_t const newEdgeIndex = m_edges.size();
        m_edges.emplace_back(
            Edge{m_points, {startPointIndex, endPointIndex}, {triangleIndex}});
        auto const emptySpot =
            std::find(triangle.m_edgeIndices.begin(),
                      triangle.m_edgeIndices.end(), INVALID_INDEX);
        if (emptySpot == triangle.m_edgeIndices.end()) {
          SPDLOG_LOGGER_ERROR(
              logger, "Triangle had no space for another edge. Returning.");
          return;
        }
        std::size_t const whichEdge = static_cast<std::size_t>(
            std::distance(triangle.m_edgeIndices.begin(), emptySpot));
        triangle.m_edgeIndices[whichEdge] = newEdgeIndex;
        if (std::all_of(triangle.m_edgeIndices.begin(),
                        triangle.m_edgeIndices.end(),
                        [](std::size_t const index) {
                          return index != INVALID_INDEX;
                        })) {
          // This triangle has four edges. Split it into two triangles.
          // Create a new edge
          std::size_t const newTriangleIndex = m_edges.size();
          std::size_t const newNewEdgeIndex = m_edges.size();

          // Find the non-new edge that shares the point at endPointIndex
          // That edge's other end is where we want to connect to from
          // startPointIndex
          std::size_t crossPointIndex = INVALID_INDEX;
          std::size_t betweenEdgeIndex = INVALID_INDEX;
          for (auto &edgeIndex : triangle.m_edgeIndices) {
            if (edgeIndex != newEdgeIndex) {
              std::array<std::array<std::size_t, 2>, 2> constexpr ab{
                  {{0, 1}, {1, 0}}};
              for (auto const &[a, b] : ab) {
                if (m_edges[edgeIndex].m_pointIndices[a] == endPointIndex) {
                  crossPointIndex = m_edges[edgeIndex].m_pointIndices[b];
                  betweenEdgeIndex = edgeIndex;
                  // This edge shall switch triangleIndex to newTriangleIndex
                  // in its user list
                  for (auto &userIndex : m_edges[betweenEdgeIndex].m_users) {
                    if (userIndex == triangleIndex) {
                      userIndex = newTriangleIndex;
                    }
                  }
                  // This triangle shall no longer use the betweenEdge.
                  // The newNewEdge shall be used instead.
                  edgeIndex = newNewEdgeIndex;
                }
              }
            }
          }

          // Create the newNewEdge
          m_edges.emplace_back(Edge{m_points,
                                    {startPointIndex, crossPointIndex},
                                    {triangleIndex, newTriangleIndex}});
          // Create the new triangle
          m_triangles.emplace_back(Triangle{
              m_edges, {newEdgeIndex, betweenEdgeIndex, newNewEdgeIndex}});
        }
      }
    }
  }
}

void MeshClipper::adjustTriangles() {
  for (auto &triangle : m_triangles) {
    if (triangle.visible()) {
      // Initiate all vertices touched by triangle with m_occurs = 0
      for (auto const edgeIndex : triangle.m_edgeIndices) {
        if (edgeIndex != INVALID_INDEX) {
          triangle.m_edges[edgeIndex].vertex0().m_occurs = 0;
          triangle.m_edges[edgeIndex].vertex1().m_occurs = 0;
        }
      }
      if (triangle.isOpenPolyLine())
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
  adjustTriangles();

  return oldSoftMaxHeight - zCut;
}
