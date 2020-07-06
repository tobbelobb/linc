#include <cmath>
#include <numeric>
#include <utility>
#include <vector>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG /* NOLINT */
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <linc/linc.h++>
#include <linc/mesh-clipper.h++>
#include <linc/util.h++>
#include <linc/vertex.h++>

static auto logger = spdlog::get("file_logger");

// Disregard z and construct hull as if all vertices were at z=0
auto hull(std::vector<Vertex> const &vertices) -> std::vector<Vertex> {
  auto ret{vertices};
  return ret;
}

enum class Side { ABOVE, BELOW, BOTH };

static auto whichSide(Triangle const &triangle,
                      Normal const &separationDirection, Vertex const &point,
                      Millimeter const precision) -> Side {
  bool isAbove = false;
  bool isBelow = false;
  for (auto const &corner : triangle.m_corners) {
    auto const t = separationDirection.dot(corner - point);
    if (t < -precision) {
      isBelow = true;
    } else if (precision < t) {
      isAbove = true;
    }
    if (isAbove and isBelow) {
      return Side::BOTH;
    }
  }
  return isAbove ? Side::ABOVE : Side::BELOW;
}

static auto whichSide(Triangle const &triangle0,
                      Normal const &separationDirection,
                      Triangle const &triangle1, Millimeter const precision)
    -> Side {
  bool isAbove = false;
  bool isBelow = false;
  for (auto const &point : triangle1.m_corners) {
    for (auto const &corner : triangle0.m_corners) {
      auto const t = separationDirection.dot(corner - point);
      if (t < -precision) {
        isBelow = true;
      } else if (precision < t) {
        isAbove = true;
      }
      if (isAbove and isBelow) {
        return Side::BOTH;
      }
    }
  }
  return isAbove ? Side::ABOVE : Side::BELOW;
}

auto intersect(Triangle const &triangle0, Triangle const &triangle1) -> bool {
  for (auto const &point : triangle0.m_corners) {
    Side const side =
        whichSide(triangle1, triangle0.m_normal, point, VertexConstants::eps);
    if (side != Side::BOTH) {
      // Found an axis that completely separates the triangles
      return false;
    }
  }
  for (auto const &point : triangle1.m_corners) {
    Side const side =
        whichSide(triangle0, triangle1.m_normal, point, VertexConstants::eps);
    if (side != Side::BOTH) {
      // Found an axis that completely separates the triangles
      return false;
    }
  }
  std::array<Vertex, 3> const edges0 = {
      triangle0.m_corners[0] - triangle0.m_corners[1],
      triangle0.m_corners[1] - triangle0.m_corners[2],
      triangle0.m_corners[2] - triangle0.m_corners[0]};
  std::array<Vertex, 3> const edges1 = {
      triangle1.m_corners[0] - triangle1.m_corners[1],
      triangle1.m_corners[1] - triangle1.m_corners[2],
      triangle1.m_corners[2] - triangle1.m_corners[0]};
  constexpr std::array<std::pair<size_t, size_t>, 9> mixPairs = {
      std::pair{0, 0}, std::pair{0, 1}, std::pair{0, 2},
      std::pair{1, 0}, std::pair{1, 1}, std::pair{1, 2},
      std::pair{2, 0}, std::pair{2, 1}, std::pair{2, 2}};

  for (auto const &mixPair : mixPairs) {
    Vertex const mixedCrossProduct =
        edges0.at(mixPair.first).cross(edges1.at(mixPair.second));
    Side const side = whichSide(triangle0, mixedCrossProduct, triangle1,
                                VertexConstants::eps);
    if (side != Side::BOTH) {
      // Found an axis that completely separates the triangles
      return false;
    }
  }

  return true;
}

void orderCounterClockWise(std::vector<Vertex> &vertices) {
  Vertex middlePoint{Vertex::Zero()};
  for (auto const &vertex : vertices) {
    middlePoint += vertex;
  }
  middlePoint = middlePoint / vertices.size();

  std::sort(vertices.begin(), vertices.end(),
            [&middlePoint](Vertex const &vertex0, Vertex const &vertex1) {
              Vertex const offsetPoint0 = vertex0 - middlePoint;
              Vertex const offsetPoint1 = vertex1 - middlePoint;
              auto const angle0 = atan2(offsetPoint0.y(), offsetPoint0.x());
              auto const angle1 = atan2(offsetPoint1.y(), offsetPoint1.x());
              return angle0 < angle1;
            });
}

auto willCollide(Mesh const &mesh, Pivots const &pivots,
                 Millimeter const layerHeight) -> bool {
  if (logger == nullptr) {
    logger = spdlog::get("file_logger");
  }

  double const minHeight = mesh.minHeight();

  assert(minHeight > -VertexConstants::eps);  // NOLINT
  assert(minHeight < VertexConstants::eps);   // NOLINT
  assert(layerHeight > VertexConstants::eps); // NOLINT

  double const topHeight = mesh.maxHeight();

  // clang-format might complain that h-=layerHeight will accumulate an error.
  // We don't care here, since an extra iteration more or less when we're
  // getting close to the bottom of the print doesn't matter for us
  for (Millimeter h{topHeight}; h > 2 * layerHeight; /* NOLINT */
       h -= layerHeight) {                           /* NOLINT */

    MeshClipper partialPrint{mesh};
    partialPrint.softClip(h);
    SPDLOG_LOGGER_DEBUG(logger, "New soft max height after clip is {}",
                        partialPrint.softMaxHeight());

    // Extract convex hull of the top points
    // This involves removing points that are enclosed by other points
    std::vector<Vertex> topVertices{partialPrint.getTopVertices()};
    SPDLOG_LOGGER_DEBUG(logger, "Found {} top vertices", topVertices.size());
    std::vector<Vertex> topVerticesHull{hull(topVertices)};
    SPDLOG_LOGGER_DEBUG(logger, "The hull of those has {} vertices",
                        topVerticesHull.size());

    orderCounterClockWise(topVerticesHull);

    // Build the cones and check for collision, one by one
    for (auto const &[anchorIndex, anchorPivot] : enumerate(pivots.anchors)) {
      // POINTS
      Mesh cone{anchorPivot};
      for (auto const &topVertex : topVerticesHull) {
        cone.m_vertices.emplace_back(topVertex +
                                     pivots.effector.at(anchorIndex));
      }
      size_t const numPoints = cone.m_vertices.size();

      // EDGES
      // Add star topology down to anchorPivot
      for (size_t pointIdx{1}; pointIdx < numPoints; ++pointIdx) {
        cone.m_edges.emplace_back(Mesh::Edge{cone.m_vertices, {0, pointIdx}});
      }
      // Add ring of edges through all top points
      for (size_t pointIdx{1}; pointIdx < numPoints - 1; ++pointIdx) {
        cone.m_edges.emplace_back(
            Mesh::Edge{cone.m_vertices, {pointIdx, pointIdx + 1}});
      }
      cone.m_edges.emplace_back(
          Mesh::Edge{cone.m_vertices, {numPoints - 1, 0}});

      // TRIANGLES
      // There are numTopPoints top points (and thus star-topology-edges)
      // Right after star-topology edges comes as many ring edges
      size_t const numTopHullVertices = topVerticesHull.size();
      for (size_t starEdgeIdx{0}; starEdgeIdx < numTopHullVertices;
           ++starEdgeIdx) {
        size_t const ringEdgeIdx = starEdgeIdx + numTopHullVertices;
        cone.m_triangles.emplace_back(
            Mesh::Triangle{cone.m_edges,
                           {starEdgeIdx, (starEdgeIdx + 1) % numTopHullVertices,
                            ringEdgeIdx}});
      }
      // Cone built.

      // Check for collision
      // An intersection between a print triangle and a cone triangle
      // means the two meshes intersect, and we regard that as a line
      // collision
      for (auto const &partialPrintTriangle : partialPrint.m_triangles) {
        if (partialPrintTriangle.m_visible) {
          for (auto const &coneTriangle : cone.m_triangles) {
            if (intersect(partialPrintTriangle, coneTriangle)) {
              SPDLOG_LOGGER_INFO(logger, "Found collision! Between {} and {}",
                                 Triangle{partialPrintTriangle},
                                 Triangle{coneTriangle});
              return true;
            }
          }
        }
      }
    }
  }
  return false;
}
