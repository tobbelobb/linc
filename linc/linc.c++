#include <linc/linc.h++>

#include <linc/util.h++>

auto willCollide(Mesh const &mesh, Pivots const &pivots,
                 SI::milli_metre_t<double> const &layerHeight) -> bool {
  // MeshClipper partialPrint{mesh};
  // for (Millimeter h{mesh.maxHeight()}; h > layerHeight; h -= layerHeight) {
  //   // Create the partial print
  //   partialPrint.clip(h);
  //   // Extract convex hull of the top points
  //   // This involves removing some points, that are enclosed by the other
  //   points std::vector<Vertex> topPoints = hull(partialPrint.getTopPoints());
  //   // Build the cones
  //   std::array<Mesh, pivots.size()> cones{};
  //   for (auto const& [i, anchorPivot] : enumerate(pivots.anchors)) {
  //     // Add all points to each cone
  //     cones[i].m_vertices = {anchorPivot};
  //     for (auto const& topPoint : topPoints) {
  //       cones[i].m_vertices.emplace_back(topPoint + pivots.effector[i]);
  //     }
  //     // Add edges from pivot to all (offset) top points
  //     // function addStarTopology() somehow?
  //     for (size_t topPointIdx{1}; topPointIdx < cones[i].m_vertices.size();
  //     ++topPointIdx) {
  //       cones[i].m_edges.emplace_back({0, pointIdx});
  //     }
  //     // Add ring of edges through all top points
  //     // function makeClosedCurve() somehow?
  //     for (size_t topPointIdx{1}; topPointIdx < cones[i].m_vertices.size();
  //     ++topPointIdx) {
  //       cones[i].m_edges.emplace_back({pointIdx, (pointIdx + 1) %
  //       cones.[i].m_vertices.size()});
  //     }
  //     // Add edges across top layer, assuming convex top
  //     // function triangulateFlatClosedConvexCurve() somehow
  //     if (cones[i].m_vertices.size() > 4) {
  //       for (size_t topPointIdx{3}; topPointIdx < cones[i].m_vertices.size()
  //       - 1; ++topPointIdx) {
  //         cones[i].m_edges.emplace_back({1, topPointIdx});
  //       }
  //     }
  //     // Add triangles between pivot and (offset) top points
  //     // There are cones[i].m_vertices.size() - 1 top points (and thus
  //     star-topology edges)
  //     // Right after star-topology edges comes as many ring edges
  //     for (size_t starEdgeIdx{0}; starEdgeIdx < cones[i].m_vertices.size() -
  //     1, ++starEdgeIdx) {
  //       cones[i].m_triangles.emplace_back({starEdgeIdx, starEdgeIdx + 1,
  //       starEdgeIdx + (cones[i].m_vertices.size() - 1)});
  //     }
  //
  //     // Add first triangle to cap off top
  //     if (cones[i].m_vertices.size() > 3) {
  //       cones[i].m_triangles.emplace_back({cones[i].m_vertices.size() - 1,
  //                                          cones[i].m_vertices.size(),
  //                                          2*(cones[i].m_vertices.size() -
  //                                          1)});
  //     }
  //
  //     // Add last triangle to cap off top
  //     if (cones[i].m_vertices.size() > 4) {
  //       cones[i].m_triangles.emplace_back({2*(cones[i].m_vertices.size() - 1)
  //       - 1,
  //                                          2*(cones[i].m_vertices.size() - 1)
  //                                          - 2, cones[i].m_edges.size() -
  //                                          1});
  //     }
  //
  //     // Middle triangles to cap off top
  //     if (cones[i].m_vertices.size() > 5) {
  //       for (size_t j{0}; j < cones[i].m_vertices.size() - 5; ++j) {
  //         size_t const ringEdgeIdx = j + cones[i].m_vertices.size() + 1;
  //         size_t const capOffIdx = 2*(cones[i].m_vertices.size() - 1) + j;
  //         cones[i].m_triangles.emplace_back({capOffIdx, ringEdgeIdx,
  //         capOffIdx+1});
  //       }
  //     }
  //   }
  //   // Now cones and partialPrint are built
  //   for (auto const& cone : cones) {
  //     for (const auto& point : partialPrint.m_points) {
  //       if (isInside(point, cone)) {
  //         // return Collision;
  //         return true;
  //       }
  //     }
  //   }
  // }
  //
  (void)pivots;
  (void)mesh;
  (void)layerHeight;
  return false;
}
