#include <algorithm>

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
                              meshTriangle.m_normal,
                              true};
  }
}

void MeshClipper::setDistances(Millimeter const zCut) {
  for (auto &point : m_points) {
    point.m_distance = point.z() - zCut;
  }
}

void MeshClipper::setPointsVisibility() {
  for (auto &point : m_points) {
    point.m_visible = (point.m_distance <= 0.0);
  }
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

// Return how much was clipped
auto MeshClipper::clip(Millimeter const zCut) -> double {
  setDistances(zCut);
  setPointsVisibility();
  double const oldMaxHeight = maxHeight();
  if (std::all_of(m_points.begin(), m_points.end(),
                  [](auto const &point) { return point.m_visible; })) {
    SPDLOG_LOGGER_WARN(logger, "Tried to cut away 0 points. Returning early.");
    return 0.0;
  }
  if (std::all_of(m_points.begin(), m_points.end(),
                  [](auto const &point) { return not point.m_visible; })) {
    SPDLOG_LOGGER_WARN(logger,
                       "Cuts away all points. Clearing and returning early.");
    clear();
    return oldMaxHeight;
  }
  return oldMaxHeight - zCut;
}
