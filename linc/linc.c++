#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <iterator>
#include <numeric>
#include <thread>
#include <utility>
#include <vector>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO /* NOLINT */
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <linc/linc.h++>
#include <linc/mesh.h++>
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
  return signed2DCross(v0, v1, v2) > 0.0_mm;
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
auto hullAndSortCcw(std::vector<Vertex> &vertices) -> std::vector<Vertex> {

  if (vertices.size() < 3) {
    return vertices;
  }
  fanSort(vertices);

  std::vector<Vertex> ret{};
  ret.reserve(vertices.size());
  ret.emplace_back(vertices[0]);
  ret.emplace_back(vertices[1]);

  for (size_t k{2}; k < vertices.size(); ++k) {
    if (isLeft(ret[ret.size() - 2], ret.back(), vertices[k])) {
      // Enclosed no previous elements with new line to vertices[k]
      ret.emplace_back(vertices[k]);
    } else {
      // Enclosed one or more previous elements with new line to vertices[k]
      while (ret.size() >= 3 and
             not isLeft(ret.at(ret.size() - 3), ret.at(ret.size() - 2),
                        vertices[k])) {
        ret.pop_back();
      }
      ret.back() = vertices[k];
    }
  }
  return ret;
}

enum class Side { ABOVE_OR_BELOW, BOTH };

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
  return Side::ABOVE_OR_BELOW;
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
  return Side::ABOVE_OR_BELOW;
}

auto intersect(Triangle const &triangle0, Triangle const &triangle1) -> bool {
  for (auto const &point : triangle0.m_corners) {
    Side const side = whichSide(triangle1, triangle0.getNormalDirection(),
                                point, VertexConstants::eps);
    if (side != Side::BOTH) {
      // Found an axis that completely separates the triangles
      return false;
    }
  }
  for (auto const &point : triangle1.m_corners) {
    Side const side = whichSide(triangle0, triangle1.getNormalDirection(),
                                point, VertexConstants::eps);
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
  Vertex meanPoint{Vertex::Zero()};
  for (auto const &vertex : vertices) {
    meanPoint += vertex;
  }
  meanPoint = meanPoint / vertices.size();

  std::sort(vertices.begin(), vertices.end(),
            [&meanPoint](Vertex const &vertex0, Vertex const &vertex1) {
              Vertex const offsetPoint0 = vertex0 - meanPoint;
              Vertex const offsetPoint1 = vertex1 - meanPoint;
              auto const angle0 = atan2(offsetPoint0.y(), offsetPoint0.x());
              auto const angle1 = atan2(offsetPoint1.y(), offsetPoint1.x());
              return angle0 < angle1;
            });
}

void scaleOffsetInPlace(std::vector<Vertex> &vertices,
                        Millimeter const offset) {
  std::vector<Vertex> newVertices;
  newVertices.reserve(std::size(vertices));

  Vertex A = vertices.back();
  Vertex B = vertices[0];
  Vertex C = vertices[1];
  Vertex BA = A - B;
  Vertex BC = C - B;
  double alpha_half = acos(BA.dot(BC) / (BA.norm() * BC.norm())) / 2.0;
  double x = offset / sin(alpha_half);
  Vertex BA_dir = BA / BA.norm();
  Vertex BC_dir = BC / BC.norm();
  Normal x_dir = -(BA_dir + BC_dir) / (BA_dir + BC_dir).norm();

  newVertices[0] = B + x * x_dir;

  for (size_t i{1}; i < std::size(vertices) - 1; i++) {
    A = vertices[i - 1];
    B = vertices[i];
    C = vertices[i + 1];
    BA = A - B;
    BC = C - B;
    alpha_half = acos(BA.dot(BC) / (BA.norm() * BC.norm())) / 2.0;
    x = offset / sin(alpha_half);
    BA_dir = BA / BA.norm();
    BC_dir = BC / BC.norm();
    x_dir = -(BA_dir + BC_dir) / (BA_dir + BC_dir).norm();

    newVertices[i] = B + x * x_dir;
  }

  A = vertices[std::size(vertices) - 2];
  B = vertices[std::size(vertices) - 1];
  C = vertices[0];
  BA = A - B;
  BC = C - B;
  alpha_half = acos(BA.dot(BC) / (BA.norm() * BC.norm())) / 2.0;
  x = offset / sin(alpha_half);
  BA_dir = BA / BA.norm();
  BC_dir = BC / BC.norm();
  x_dir = -(BA_dir + BC_dir) / (BA_dir + BC_dir).norm();

  newVertices[std::size(vertices) - 1] = B + x * x_dir;

  for (size_t i{0}; i < std::size(vertices); i++) {
    vertices[i] = newVertices[i];
  }
}

static void buildCone(Vertex const &anchorPivot, Vertex const &effectorPivot,
                      std::vector<Vertex> const &topVertices, Mesh &cone) {
  // POINTS
  cone.m_points.emplace_back(anchorPivot);
  for (auto const &topVertex : topVertices) {
    cone.m_points.emplace_back(topVertex + effectorPivot);
  }
  std::size_t const numTopPoints = topVertices.size();

  // EDGES
  // Add star topology down to anchorPivot
  for (std::size_t pointIdx{1}; pointIdx < numTopPoints; ++pointIdx) {
    cone.m_edges.emplace_back(EdgePointIndices{0, pointIdx});
  }
  // Add ring of edges through all top points
  for (std::size_t pointIdx{1}; pointIdx < numTopPoints - 1; ++pointIdx) {
    cone.m_edges.emplace_back(EdgePointIndices{pointIdx, pointIdx + 1});
  }
  cone.m_edges.emplace_back(EdgePointIndices{numTopPoints - 1, 0});

  // TRIANGLES
  // There are numTopPoints top points (and thus star-topology-edges)
  // Right after star-topology edges comes as many ring edges
  for (size_t starEdgeIdx{0}; starEdgeIdx < numTopPoints; ++starEdgeIdx) {
    size_t const ringEdgeIdx = starEdgeIdx + numTopPoints;
    cone.m_triangles.emplace_back(std::array<size_t, 3>{
        starEdgeIdx, (starEdgeIdx + 1) % numTopPoints, ringEdgeIdx});
  }
}

static auto findCollision(std::vector<Millimeter> const &heights,
                          Mesh const &partialPrintOriginal,
                          Pivots const &pivots, bool hullIt, Millimeter offset,
                          std::stop_token st) -> Collision {
  std::vector<std::size_t> clippedTriangles{};
  clippedTriangles.reserve(partialPrintOriginal.m_triangles.size() / 5);
  Mesh partialPrint{partialPrintOriginal};
  std::vector<bool> trianglesVisibility{};
  trianglesVisibility.reserve(partialPrintOriginal.m_triangles.size() +
                              partialPrintOriginal.m_triangles.size() / 10);
  for (auto const h : heights) {
    if (st.stop_requested()) {
      SPDLOG_LOGGER_DEBUG(
          logger, "Another thread already found a collision. Returning.");
      return {false};
    }
    clippedTriangles.clear();
    auto const pointsVisibility = partialPrint.softClip(h, clippedTriangles);
    if (pointsVisibility.empty()) {
      SPDLOG_LOGGER_WARN(logger, "No points are visible");
      continue;
    }
    SPDLOG_LOGGER_DEBUG(logger, "New soft max height after clip is {}",
                        partialPrint.softMaxHeight(pointsVisibility));
    trianglesVisibility.clear();
    partialPrint.getTrianglesVisibility(trianglesVisibility);

    // Extract convex hull of the top points
    // This involves removing points that are enclosed by other points
    std::vector<Vertex> topVertices{partialPrint.getVerticesAt(h)};
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
    if (std::abs(offset) > VertexConstants::eps) {
      scaleOffsetInPlace(topVertices, offset);
    }

    std::vector<bool> checkIts(partialPrint.m_points.size(), true);
    Mesh cone{};
    cone.m_points.reserve(topVertices.size() + 1);
    cone.m_edges.reserve(topVertices.size() * 2);
    cone.m_triangles.reserve(topVertices.size());
    for (auto const &[anchorIndex, anchorPivot] : enumerate(pivots.anchors)) {
      Vertex const &effectorPivot{pivots.effector.at(anchorIndex)};
      Vertex const anchorToEffector{effectorPivot - anchorPivot};

      cone.m_points.clear();
      cone.m_edges.clear();
      cone.m_triangles.clear();
      buildCone(anchorPivot, effectorPivot, topVertices, cone);

      // Find the sharpest angle towards xy-plane the line will have on this
      // layer. Effector pivot will not travel farther away from anchorPivot
      // than this.
      auto const farthest =
          *std::max_element(
              topVertices.begin(), topVertices.end(),
              [&anchorToEffector](Vertex const &v0, Vertex const &v1) {
                return (v0 + anchorToEffector).norm() <
                       (v1 + anchorToEffector).norm();
              }) +
          effectorPivot;
      // to get from anchorPivot to farthest, travel along edge01
      // This is where our line is
      auto const edge01 = farthest - anchorPivot;
      // If we rotate the effector with constant line length and constant z
      // height we will travel in a direction orthogonal to edge01 and z-unit
      // vector. We want our plane along this orthogonal direction. Cross
      // product of edge01 = (x, y, z) with Z normal gives (x, y, z) x (0, 0, 1)
      // = (y, -x, 0) So we could have done Vertex const edge12 =
      // Vertex{edge01.y(), -edge01.x(), 0};
      //
      // So we've defined a plane with a triangle that has corners in
      // farthest, anchorPivot, and (farthest + edge12)
      // Normal to the plane is edge12 x edge01
      // So we could have done
      // Normal const normal = edge12.cross(edge01);
      // But we save some cycles by hand-simplifying this to:
      Normal const normal =
          Vertex{-edge01.x() * edge01.z(), -edge01.z() * edge01.y(),
                 edge01.x() * edge01.x() + edge01.y() * edge01.y()};
      // Note the guaranteed positive z-component of this normal.

      // Check if a visible point is above the checkit plane
      for (auto const &[i, point] : enumerate(partialPrint.m_points)) {
        Vertex const fromFarthestToPoint = point - farthest;
        checkIts[i] = (fromFarthestToPoint.dot(normal) >= 0.0_mm);
      }

      // Check for collision
      // An intersection between a print triangle and a cone triangle
      // means the two meshes intersect, and we regard that as a line
      // collision
      for (auto const &[i, partialPrintTriangle] :
           enumerate(partialPrint.m_triangles)) {
        if (trianglesVisibility[i] and
            (checkIts[partialPrint
                          .m_edges[partialPrintTriangle.m_edgeIndices[0]]
                          .m_pointIndices[0]] or
             checkIts[partialPrint
                          .m_edges[partialPrintTriangle.m_edgeIndices[0]]
                          .m_pointIndices[1]] or
             checkIts[partialPrint
                          .m_edges[partialPrintTriangle.m_edgeIndices[1]]
                          .m_pointIndices[0]] or
             checkIts[partialPrint
                          .m_edges[partialPrintTriangle.m_edgeIndices[1]]
                          .m_pointIndices[1]])) {
          for (auto const &coneTriangle : cone.m_triangles) {
            if (intersect({partialPrintTriangle, partialPrint.m_points,
                           partialPrint.m_edges},
                          {coneTriangle, cone.m_points, cone.m_edges}))
              [[unlikely]] {
                SPDLOG_LOGGER_INFO(
                    logger, "Found collision! Between {} and {}",
                    Triangle{partialPrintTriangle, partialPrint.m_points,
                             partialPrint.m_edges},
                    Triangle{coneTriangle, cone.m_points, cone.m_edges});
                return {true,
                        h,
                        {coneTriangle, cone.m_points, cone.m_edges},
                        effectorPivot};
              }
          }
        }
      }
    }
    partialPrint.reset(partialPrintOriginal, clippedTriangles,
                       trianglesVisibility);
  }
  return {false};
}

auto willCollide(Mesh const &mesh, Pivots const &pivots,
                 Millimeter const maxLayerHeight, bool const hullIt,
                 Millimeter const offset) -> Collision {
  if (logger == nullptr) {
    logger = spdlog::get("file_logger");
  }

  constexpr Millimeter SMALL_LAYER_HEIGHT{0.1_mm};
  if (maxLayerHeight < SMALL_LAYER_HEIGHT) {
    SPDLOG_LOGGER_WARN(logger,
                       "Layer height {} is very small. Execution might take "
                       "a long time",
                       maxLayerHeight);
  }

  Millimeter const minHeight = mesh.minHeight();
  if (minHeight < 0.0_mm) {
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
        logger,
        "Special case: Mesh entirely below anchors. Collision impossible.");
    return {false};
  }

  Millimeter const startAnalysisAt = topHeight;
  Millimeter const stopAnalysisAt =
      std::max(minHeight, std::max(lowestAnchorZ, 0.0_mm));

  // Create some worker threads
  auto const numThreads = std::thread::hardware_concurrency();
  // And some futures, that lets us check on their progress and result
  std::vector<std::future<Collision>> futures;
  std::vector<std::jthread> threads{};

  for (size_t i{0}; i < numThreads; ++i) {
    // Which heights should this thread check?
    // Give each thread a separate interval (a, b] to check
    // Evenly split segments from the top (highest value, which is b)
    // and downwards
    auto const b =
        stopAnalysisAt + (startAnalysisAt - stopAnalysisAt) *
                             (static_cast<Millimeter>(numThreads - i) /
                              static_cast<Millimeter>(numThreads));
    auto const a =
        stopAnalysisAt + (startAnalysisAt - stopAnalysisAt) *
                             (static_cast<Millimeter>(numThreads - i - 1) /
                              static_cast<Millimeter>(numThreads));
    auto const intervalLength = b - a;

    std::vector<Millimeter> linearHeights{};
    linearHeights.reserve(static_cast<std::size_t>(
        std::ceil(intervalLength / maxLayerHeight) + 1));
    for (Millimeter ba{b}; a < ba; ba -= maxLayerHeight) {
      linearHeights.emplace_back(ba);
    }
    std::vector<Millimeter> heights{};
    heights.reserve(linearHeights.size());
    // Force topmost layer to be analyzed first
    heights.emplace_back(linearHeights[0]);
    // Then jump back and forth in the interval in
    // a binary search fashion
    for (auto const index : binarySearchSequence(1, linearHeights.size() - 1)) {
      heights.emplace_back(linearHeights[index]);
    }

    std::packaged_task<Collision(std::stop_token st)> task(
        [heights, &mesh, &pivots, hullIt, offset](std::stop_token st) {
          return findCollision(heights, mesh, pivots, hullIt, offset, st);
        });
    futures.emplace_back(task.get_future());
    threads.emplace_back(std::move(task));
  }

  using namespace std::chrono_literals;
  while (std::any_of(futures.begin(), futures.end(),
                     [](std::future<Collision> const &future) {
                       return future.valid() and
                              future.wait_for(0ms) != std::future_status::ready;
                     })) {
    for (auto &future : futures) {
      if (future.valid() and
          future.wait_for(0ms) == std::future_status::ready) {
        Collision found{future.get()};
        if (found.m_isCollision) {
          return found;
        }
      }
    }
    if (futures.at(0).valid() and
        futures.at(0).wait_for(50ms) == std::future_status::ready) {
      // This if statement saves us 50 ms for every unit test that
      // is run on a simple model
      Collision found{futures.at(0).get()};
      if (found.m_isCollision) {
        return found;
      }
    } else {
      // If thread 0 didn't find anything, sleep a while before
      // checking the others
      std::this_thread::sleep_for(50ms);
    }
  }

  for (auto &future : futures) {
    if (future.valid()) {
      Collision found{future.get()};
      if (found.m_isCollision) {
        return found;
      }
    }
  }
  return {false};
}

void makeDebugModel(Mesh const &meshClipper, Pivots const &pivots,
                    Collision const &collision,
                    std::string const &outFileName) {
  if (not collision) {
    SPDLOG_LOGGER_WARN(logger, "Can not make debug model from no collision");
    return;
  }
  SPDLOG_LOGGER_DEBUG(logger, "Making debug model");

  std::vector<std::size_t> c{};
  Mesh partialPrint{meshClipper};
  partialPrint.softClip(collision.m_height, c);
  // Add the intersecting cone triangle to partialPrint
  std::array<std::size_t, 3> cornerIndices{INVALID_INDEX, INVALID_INDEX,
                                           INVALID_INDEX};
  for (size_t i{0}; i < 3; ++i) {
    cornerIndices.at(i) = partialPrint.m_points.size();
    partialPrint.m_points.emplace_back(
        collision.m_coneTriangle.m_corners.at(i));
  }
  std::array<std::size_t, 3> const edgeIndices{partialPrint.m_edges.size(),
                                               partialPrint.m_edges.size() + 1,
                                               partialPrint.m_edges.size() + 2};
  partialPrint.m_edges.emplace_back(
      EdgePointIndices{cornerIndices[0], cornerIndices[1]});
  partialPrint.m_edges.emplace_back(
      EdgePointIndices{cornerIndices[1], cornerIndices[2]});
  partialPrint.m_edges.emplace_back(
      EdgePointIndices{cornerIndices[2], cornerIndices[0]});

  partialPrint.m_triangles.emplace_back(edgeIndices);

  auto const effectorPosition =
      *std::max_element(
          collision.m_coneTriangle.m_corners.begin(),
          collision.m_coneTriangle.m_corners.end(),
          [](Vertex const &v0, Vertex const &v1) { return v0.z() < v1.z(); }) -
      collision.m_effectorPivot;

  // Add effector into partialPrint
  size_t const effectorPositionIndex{partialPrint.m_points.size()};
  partialPrint.m_points.emplace_back(effectorPosition);
  std::array<std::size_t, Pivots::COLS> effectorPivotIndices{INVALID_INDEX};
  for (size_t i{0}; i < Pivots::COLS; ++i) {
    effectorPivotIndices.at(i) = partialPrint.m_points.size();
    partialPrint.m_points.emplace_back(effectorPosition +
                                       pivots.effector.at(i));
  }

  for (std::size_t i{0}; i < Pivots::COLS; i += 2) {
    std::array<std::size_t, 3> const edgeIndicesABC{
        partialPrint.m_edges.size(), partialPrint.m_edges.size() + 1,
        partialPrint.m_edges.size() + 2};
    partialPrint.m_edges.emplace_back(
        EdgePointIndices{effectorPositionIndex, effectorPivotIndices.at(i)});
    partialPrint.m_edges.emplace_back(EdgePointIndices{
        effectorPositionIndex, effectorPivotIndices.at(i + 1)});
    partialPrint.m_edges.emplace_back(EdgePointIndices{
        effectorPivotIndices.at(i), effectorPivotIndices.at(i + 1)});
    partialPrint.m_triangles.emplace_back(edgeIndicesABC);
  }

  partialPrint.writeBinaryStl(outFileName);
}
