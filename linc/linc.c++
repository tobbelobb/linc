#include <linc/linc.h++>
#include <linc/mesh-clipper.h++>
#include <linc/util.h++>

auto willCollide(Mesh const &mesh, Pivots const &pivots,
                 Millimeter const layerHeight) -> bool {
  (void)pivots;

  MeshClipper partialPrint{mesh};

  double const minHeight = partialPrint.minHeight();

  assert(minHeight > -VertexConstants::eps);  // NOLINT
  assert(minHeight < VertexConstants::eps);   // NOLINT
  assert(layerHeight > VertexConstants::eps); // NOLINT

  double const topHeight = partialPrint.maxHeight();

  // clang-format might complain that h-=layerHeight will accumulate an error.
  // We don't care here, since an extra iteration doesn't matter for us
  for (Millimeter h{topHeight}; h > 2 * layerHeight; /* NOLINT */
       h -= layerHeight) {                           /* NOLINT */
    partialPrint.softClip(h);

    //   // Extract convex hull of the top points
    //   // This involves removing points that are enclosed by other points
    //   points std::vector<Vertex> topPoints =
    //   hull(partialPrint.getTopPoints()); size_t const numTopPoints =
    //   topPoints.size();
    //
    //   // Could be achieved with std::sort and a function that finds an inner
    //   point,
    //   // And does < on angle from inner point to outer point (use (1,0) as
    //   reference 0 angle) orderCounterClockWise(topPoints);
    //
    //   // Build the cones and check for collision, one by one
    //   for (auto const& [i, anchorPivot] : enumerate(pivots.anchors)) {
    //     // POINTS
    //     Mesh cone{.m_vertices = {anchorPivot}};
    //     for (auto const& topPoint : topPoints) {
    //       cone.m_vertices.emplace_back(topPoint + pivots.effectors[i]);
    //     }
    //     size_t const numPoints = cone.m_vertices.size();
    //
    //     // EDGES
    //     // Add star topology down to anchorPivot
    //     for (size_t pointIdx{1}; pointIdx < numPoints; ++pointIdx) {
    //       cone.m_edges.emplace_back({0, pointIdx});
    //     }
    //     // Add ring of edges through all top points
    //     for (size_t pointIdx{1}; pointIdx < numPoints - 1; ++pointIdx) {
    //       cone.m_edges.emplace_back({pointIdx, pointIdx + 1});
    //     }
    //     cone.m_edges.emplace_back({numPoints - 1, 0});
    //
    //     // TRIANGLES
    //     // There are numTopPoints top points (and thus star-topology edges)
    //     // Right after star-topology edges comes as many ring edges
    //     for (size_t starEdgeIdx{0}; starEdgeIdx < numTopPoints,
    //     ++starEdgeIdx)
    //     {
    //       size_t const ringEdgeIdx = starEdgeIdx + numTopPoints;
    //       cone.m_triangles.emplace_back({starEdgeIdx, (starEdgeIdx + 1) %
    //       numTopPoints, ringEdgeIdx});
    //     }
    //
    //     // Check for collision
    //     // An intersection between a print triangle and a cone triangle
    //     // means the two meshes intersect, and we regard that as a line
    //     collision for (auto const& partialPrintTriangle :
    //     partialPrint.m_triangles) {
    //       for (auto const& coneTriangle : cone.m_triangles) {
    //         if (intersect(partialPrintTriangle, coneTriangle)) {
    //           return true;
    //         }
    //       }
    //     }
    //   }
  }

  return false;
}
