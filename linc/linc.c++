#include <algorithm>
#include <cmath>
#include <iterator>
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

static inline auto signed2DCross(Vertex const &v0, Vertex const &v1,
                                 Vertex const &v2) {
  return (v1.x() - v0.x()) * (v2.y() - v0.y()) -
         (v2.x() - v0.x()) * (v1.y() - v0.y());
}

static inline auto isLeft(Vertex const &v0, Vertex const &v1, Vertex const &v2)
    -> bool {
  return signed2DCross(v0, v1, v2) > 0.0;
}

static void fanSort(std::vector<Vertex> &fan) {
  // Establish a reference point
  std::iter_swap(fan.begin(), std::min_element(fan.begin(), fan.end()));
  // Since this reference point is min_element, we know that all other
  // points are on one side of a line that goes through this point
  const auto &pivot = fan[0];
  // Sort points in a ccw radially ordered "fan" with pivot in fan[0]
  std::sort(std::next(fan.begin()), fan.end(),
            [&pivot](Vertex const &lhs, Vertex const &rhs) -> bool {
              return isLeft(pivot, lhs, rhs);
            });
}

// Graham Scan Algorithm
auto hullAndSortCcw(std::vector<Vertex> const &vertices)
    -> std::vector<Vertex> {
  auto fan{vertices};
  if (fan.size() < 3) {
    return fan;
  }
  fanSort(fan);

  std::vector<Vertex> ret;
  ret.reserve(fan.size());
  ret.emplace_back(fan[0]);
  ret.emplace_back(fan[1]);

  for (size_t k{2}; k < fan.size(); ++k) {
    if (isLeft(ret[ret.size() - 2], ret.back(), fan[k])) {
      // Enclosed no previous elements with new line to fan[k]
      ret.emplace_back(fan[k]);
    } else {
      // Enclosed one or more previous elements with new line to fan[k]
      while (ret.size() >= 3 and not isLeft(ret.at(ret.size() - 3),
                                            ret.at(ret.size() - 2), fan[k])) {
        ret.pop_back();
      }
      ret.back() = fan[k];
    }
  }
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

void sortCcwInPlace(std::vector<Vertex> &vertices) {
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
                 Millimeter const layerHeight, bool hullIt) -> bool {
  if (logger == nullptr) {
    logger = spdlog::get("file_logger");
  }
  if (layerHeight < 0.1) {
    SPDLOG_LOGGER_WARN(logger,
                       "Layer height {} is very low. Execution might take "
                       "a very long time",
                       layerHeight);
  }

  Millimeter const minHeight = mesh.minHeight();
  if (minHeight < 0.0) {
    SPDLOG_LOGGER_WARN(logger, "Mesh goes below z=0.0");
  }

  Millimeter const topHeight = mesh.maxHeight();
  Millimeter const lowestAnchorZ =
      (*std::min_element(pivots.anchors.begin(), pivots.anchors.end(),
                         [](Vertex const &lhs, Vertex const &rhs) {
                           return lhs.z() < rhs.z();
                         }))
          .z();
  if (topHeight < lowestAnchorZ) {
    SPDLOG_LOGGER_INFO(
        logger, "Special case: Mesh entirely below anchors. Collision impossible.");
    return false;
  }

  Millimeter const startAnalysisAt = topHeight;
  Millimeter const stopAnalysisAt = std::max(minHeight, std::max(lowestAnchorZ, 0.0));

  // clang-format might complain that h-=layerHeight will accumulate an error.
  // We don't care here, since an extra iteration more or less when we're
  // getting close to the bottom of the print doesn't matter
  for (Millimeter h{startAnalysisAt}; h > stopAnalysisAt; /* NOLINT */
       h -= layerHeight) {                                /* NOLINT */

    MeshClipper partialPrint{mesh};
    partialPrint.softClip(h);
    SPDLOG_LOGGER_DEBUG(logger, "New soft max height after clip is {}",
                        partialPrint.softMaxHeight());

    // Extract convex hull of the top points
    // This involves removing points that are enclosed by other points
    std::vector<Vertex> topVertices{partialPrint.getTopVertices()};
    SPDLOG_LOGGER_DEBUG(logger, "Found {} top vertices", topVertices.size());
    if (topVertices.size() < 3) {
      SPDLOG_LOGGER_WARN(logger,
                         "Found zero area layer at z={}. Did user forget to "
                         "add support structures?",
                         h);
      continue;
    }

    if (hullIt) {
      topVertices = hullAndSortCcw(topVertices);
      SPDLOG_LOGGER_DEBUG(logger, "The hull of those has {} vertices",
                          topVertices.size());
    } else {
      // TODO: We should use edges from model here instead of sorting ccw
      sortCcwInPlace(topVertices);
    }

    // Build the cones and check for collision, one by one
    for (auto const &[anchorIndex, anchorPivot] : enumerate(pivots.anchors)) {
      // POINTS
      Mesh cone{anchorPivot};
      for (auto const &topVertex : topVertices) {
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
      size_t const numTopVertices = topVertices.size();
      for (size_t starEdgeIdx{0}; starEdgeIdx < numTopVertices; ++starEdgeIdx) {
        size_t const ringEdgeIdx = starEdgeIdx + numTopVertices;
        cone.m_triangles.emplace_back(Mesh::Triangle{
            cone.m_edges,
            {starEdgeIdx, (starEdgeIdx + 1) % numTopVertices, ringEdgeIdx}});
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
