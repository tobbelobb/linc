#include <iostream>

#include <linc/mesh-clipper.h++>
#include <linc/test-framework.h++>
#include <linc/units.h++>
#include <linc/util.h++>

auto main() -> int {
  try {
    {
      // Check Point construction and comparison works as expected
      MeshClipper::Point const point{Vertex{0, 0, 0}};
      MeshClipper::Point const &itself{point};
      compare(point, itself);
      constexpr auto eps = VertexConstants::eps;
      MeshClipper::Point const pointEps{Vertex{eps, eps, eps}};
      compare(point, pointEps);
      constexpr double eps2{eps + 1e-16};
      MeshClipper::Point const pointEps2{Vertex{eps2, eps, eps}};
      check(point != pointEps2);

      // Only the vertex should affect equality
      MeshClipper::Point const withDistance{Vertex{0, 0, 0}, 1.0};
      compare(point, withDistance);
      MeshClipper::Point const zeroDistance{Vertex{0, 0, 0}, 0.0};
      compare(point, zeroDistance);
      MeshClipper::Point const zeroDistanceOccurs{Vertex{0, 0, 0}, 0.0, 1};
      compare(point, zeroDistanceOccurs);
      MeshClipper::Point const zeroDistanceZeroOccurs{Vertex{0, 0, 0}, 0.0, 0};
      compare(point, zeroDistanceZeroOccurs);
      MeshClipper::Point const zeroDistanceZeroOccursInvisible{Vertex{0, 0, 0},
                                                               0.0, 0, false};
      compare(point, zeroDistanceZeroOccursInvisible);
      MeshClipper::Point const zeroDistanceZeroOccursVisible{Vertex{0, 0, 0},
                                                             0.0, 0, true};
      compare(point, zeroDistanceZeroOccursVisible);
    }
    {
      // test operator< and operator== for MeshClipper::Edge
      std::vector<MeshClipper::Point> examplePoints{
          MeshClipper::Point{{0, 0, 0}}, MeshClipper::Point{{1, 0, 0}}};
      MeshClipper::Edge const forwards{examplePoints, {0, 1}};
      MeshClipper::Edge const backwards{examplePoints, {1, 0}};
      compare(forwards, backwards);
      check(not(forwards != backwards));

      // Basing the edge off of another points list should not affect equality
      std::vector<MeshClipper::Point> examplePoints2{
          MeshClipper::Point{{1, 0, 0}}, MeshClipper::Point{{0, 0, 0}}};
      MeshClipper::Edge const forwards2{examplePoints2, {0, 1}};
      compare(forwards, forwards2);

      // Limit of inequality should be between eps and eps2
      constexpr auto eps = VertexConstants::eps;
      constexpr double eps2{eps + 1e-16};
      std::vector<MeshClipper::Point> examplePointsEps2{
          MeshClipper::Point{{1 + eps2, 0, 0}}, MeshClipper::Point{{0, 0, 0}}};
      MeshClipper::Edge biggerEdge{examplePointsEps2, {0, 1}};
      check(forwards != biggerEdge);
      check(forwards < biggerEdge);
    }
    {
      // Test operator== for MeshClipper::Triangle
      std::vector<MeshClipper::Point> examplePoints{
          {0, 0, 0}, {0, 1, 0}, {1, 0, 0}};
      std::vector<MeshClipper::Point> examplePoints2{
          {0, 0, 0}, {0, 1, 0}, {1, 0, 0}};
      std::vector<MeshClipper::Edge> exampleEdges{{examplePoints, {0, 1}},
                                                  {examplePoints, {1, 2}},
                                                  {examplePoints, {2, 0}}};
      std::vector<MeshClipper::Edge> exampleEdges2{{examplePoints2, {0, 1}},
                                                   {examplePoints2, {1, 2}},
                                                   {examplePoints2, {2, 0}}};

      MeshClipper::Triangle triangle{exampleEdges, {0, 1, 2}};
      MeshClipper::Triangle const &itself = triangle;
      compare(triangle, itself);

      // Equality should not depend on order of edges
      MeshClipper::Triangle triangle2{exampleEdges2, {1, 0, 2}};
      compare(triangle, triangle2);

      constexpr auto eps = VertexConstants::eps;
      constexpr double eps2{eps + 1e-16};
      std::vector<MeshClipper::Point> examplePointsEps{
          {eps, eps, eps}, {eps, 1, eps}, {1, eps, eps}};
      std::vector<MeshClipper::Point> examplePointsEps2{
          {eps2, eps2, eps2}, {eps2, 1, eps2}, {1, eps2, eps2}};
      std::vector<MeshClipper::Edge> exampleEdgesEps{
          {examplePointsEps, {0, 1}},
          {examplePointsEps, {1, 2}},
          {examplePointsEps, {2, 0}}};
      std::vector<MeshClipper::Edge> exampleEdgesEps2{
          {examplePointsEps2, {0, 1}},
          {examplePointsEps2, {1, 2}},
          {examplePointsEps2, {2, 0}}};
      MeshClipper::Triangle triangleEps{exampleEdgesEps, {0, 1, 2}};
      compare(triangle, triangleEps);

      MeshClipper::Triangle triangleEps2{exampleEdgesEps2, {0, 1, 2}};
      check(triangle != triangleEps2);
      compare(triangleEps, triangleEps2);
    }
    {
      MeshClipper const meshClipper{
          Mesh{Stl{getPath("test-models/small-cube.ascii.stl")}}};

      // Points
      compare(meshClipper.m_points.size(), 8U);
      // clang-format off
      std::vector<MeshClipper::Point> expectedPoints{
          {{-5, -5,  0}, 0.0_mm, 0, true},
          {{-5, -5, 10}, 0.0_mm, 0, true},
          {{-5,  5,  0}, 0.0_mm, 0, true},
          {{-5,  5, 10}, 0.0_mm, 0, true},
          {{ 5, -5, 10}, 0.0_mm, 0, true},
          {{ 5, -5,  0}, 0.0_mm, 0, true},
          {{ 5,  5,  0}, 0.0_mm, 0, true},
          {{ 5,  5, 10}, 0.0_mm, 0, true}};
      // clang-format on
      std::sort(expectedPoints.begin(), expectedPoints.end());
      compare(meshClipper.m_points, expectedPoints);

      // Edges
      compare(meshClipper.m_edges.size(), 18U);
      // The short vector of m_users is expected to be equal and sorted
      // And m_visible is expected to be correct
      // For an edge to be fullyEqual another edge
      std::vector<MeshClipper::Edge> expectedEdges{
          {expectedPoints, {0, 1}, {0, 1}},   // 0
          {expectedPoints, {0, 2}, {2, 3}},   // 1
          {expectedPoints, {0, 3}, {0, 2}},   // 2
          {expectedPoints, {0, 4}, {4, 5}},   // 3
          {expectedPoints, {0, 5}, {1, 4}},   // 4
          {expectedPoints, {0, 6}, {3, 5}},   // 5
          {expectedPoints, {1, 3}, {0, 6}},   // 6
          {expectedPoints, {1, 5}, {1, 6}},   // 7
          {expectedPoints, {2, 3}, {2, 7}},   // 8
          {expectedPoints, {2, 6}, {3, 7}},   // 9
          {expectedPoints, {3, 5}, {6, 8}},   // 10
          {expectedPoints, {3, 6}, {7, 9}},   // 11
          {expectedPoints, {3, 7}, {8, 9}},   // 12
          {expectedPoints, {4, 5}, {4, 10}},  // 13
          {expectedPoints, {4, 6}, {5, 10}},  // 14
          {expectedPoints, {5, 6}, {10, 11}}, // 15
          {expectedPoints, {5, 7}, {8, 11}},  // 16
          {expectedPoints, {6, 7}, {9, 11}}}; // 17
      std::sort(expectedEdges.begin(), expectedEdges.end());
      compare(expectedEdges.size(), meshClipper.m_edges.size());

      for (auto const &[i, expectedEdge] : enumerate(expectedEdges)) {
        MeshClipper::Edge const &gotEdge = meshClipper.m_edges[i];
        bool const fullyEqual = gotEdge.fullEquals(expectedEdge);
        if (not fullyEqual) {
          std::cerr << "index: " << i << " expected: " << expectedEdge
                    << " got " << gotEdge << '\n';
        }
        compare(gotEdge, expectedEdge);
        check(fullyEqual);
      }

      // Triangles
      compare(meshClipper.m_triangles.size(), 12U);
      std::vector<MeshClipper::Triangle> const expectedTriangles{
          {expectedEdges, {0, 2, 6}, Normal::Zero()},
          {expectedEdges, {0, 4, 7}, Normal::Zero()},
          {expectedEdges, {1, 2, 8}, Normal::Zero()},
          {expectedEdges, {1, 5, 9}, Normal::Zero()},
          {expectedEdges, {3, 4, 13}, Normal::Zero()},
          {expectedEdges, {3, 5, 14}, Normal::Zero()},
          {expectedEdges, {6, 7, 10}, Normal::Zero()},
          {expectedEdges, {8, 9, 11}, Normal::Zero()},
          {expectedEdges, {10, 12, 16}, Normal::Zero()},
          {expectedEdges, {11, 12, 17}, Normal::Zero()},
          {expectedEdges, {13, 14, 15}, Normal::Zero()},
          {expectedEdges, {15, 16, 17}, Normal::Zero()}};
      compare(expectedTriangles.size(), meshClipper.m_triangles.size());

      for (auto const &[i, expectedTriangle] : enumerate(expectedTriangles)) {
        MeshClipper::Triangle const &gotTriangle = meshClipper.m_triangles[i];
        bool const fullyEqual = gotTriangle.fullEquals(expectedTriangle);
        if (not fullyEqual) {
          std::cerr << "index: " << i << " expected: \n"
                    << expectedTriangle << "\ngot: \n"
                    << gotTriangle << '\n';
        }
        compare(gotTriangle, expectedTriangle);
        check(fullyEqual);
      }
    }
    {
      // Test maxHeight
      MeshClipper const meshClipper(
          Mesh{Stl{getPath("test-models/broken/standing-triangle.ascii.stl")}});
      compare(meshClipper.maxHeight(), 10.0_mm);
    }
    {
      MeshClipper const meshClipper(
          Mesh{Stl{getPath("test-models/benchy-low-poly.binary.stl")}});
      double const maxHeight = meshClipper.maxHeight();
      check(maxHeight > 48.0 - 0.01);
      check(48.0 + 0.01 > maxHeight);
    }
    {
      // Test setDistances and setPointsVisibility
      MeshClipper meshClipper{
          Mesh{Stl{getPath("test-models/tetrahedron.ascii.stl")}}};

      compare(meshClipper.m_points.at(0).z(), 0.0_mm);
      compare(meshClipper.m_points.at(1).z(), 1.0_mm);
      compare(meshClipper.m_points.at(2).z(), 0.0_mm);
      compare(meshClipper.m_points.at(3).z(), 0.0_mm);

      meshClipper.setDistances(0.5_mm);
      compare(meshClipper.m_points.at(0).m_distance, -0.5_mm);
      compare(meshClipper.m_points.at(1).m_distance, 0.5_mm);
      compare(meshClipper.m_points.at(2).m_distance, -0.5_mm);
      compare(meshClipper.m_points.at(3).m_distance, -0.5_mm);
      meshClipper.setPointsVisibility();
      compare(meshClipper.m_points.at(0).m_visible, true);
      compare(meshClipper.m_points.at(1).m_visible, false);
      compare(meshClipper.m_points.at(2).m_visible, true);
      compare(meshClipper.m_points.at(3).m_visible, true);

      meshClipper.setDistances(-0.5_mm);
      compare(meshClipper.m_points.at(0).m_distance, 0.5_mm);
      compare(meshClipper.m_points.at(1).m_distance, 1.5_mm);
      compare(meshClipper.m_points.at(2).m_distance, 0.5_mm);
      compare(meshClipper.m_points.at(3).m_distance, 0.5_mm);
      meshClipper.setPointsVisibility();
      compare(meshClipper.m_points.at(0).m_visible, false);
      compare(meshClipper.m_points.at(1).m_visible, false);
      compare(meshClipper.m_points.at(2).m_visible, false);
      compare(meshClipper.m_points.at(3).m_visible, false);

      meshClipper.setDistances(1.5_mm);
      compare(meshClipper.m_points.at(0).m_distance, -1.5_mm);
      compare(meshClipper.m_points.at(1).m_distance, -0.5_mm);
      compare(meshClipper.m_points.at(2).m_distance, -1.5_mm);
      compare(meshClipper.m_points.at(3).m_distance, -1.5_mm);
      meshClipper.setPointsVisibility();
      compare(meshClipper.m_points.at(0).m_visible, true);
      compare(meshClipper.m_points.at(1).m_visible, true);
      compare(meshClipper.m_points.at(2).m_visible, true);
      compare(meshClipper.m_points.at(3).m_visible, true);
    }
    {
      // Confirm that we can load a large complicated broken mesh.
      // If this test gets annoyingly slow, work on performance
      // until it's not.
      MeshClipper const meshClipper(
          Mesh{Stl{getPath("test-models/broken/3DBenchy.binary.stl")}});
      compare(meshClipper.m_points.size(), 112569U);
      compare(meshClipper.m_edges.size(), 337731U);
      compare(meshClipper.m_triangles.size(), 225154U);
      double constexpr eps = 1e-6;
      double const maxHeight = meshClipper.maxHeight();
      check(maxHeight > 48.0 - eps);
      check(48.0 + eps > maxHeight);
    }
    {
      MeshClipper meshClipper{
          Mesh{Stl{getPath("test-models/small-cube.ascii.stl")}}};
      compare(meshClipper.m_points.size(), 8U);
      compare(meshClipper.m_edges.size(), 18U);
      compare(meshClipper.m_triangles.size(), 12U);
      double const maxHeight = meshClipper.maxHeight();
      // Comparing floating points with == is usually unpredictable
      // but in this case, our parser should have parsed the string
      // "10.0" into a floating point. We should really hope that
      // gives us exactly what the compiler constant 10.0 gives.
      compare(maxHeight, 10.0);

      // If we soft-clip at the top layer, 0 mm should disappear
      // And no points should be set invisible
      // Again, 0.0 should be a compile time constant returned by the
      // function, so it should be ok to compare with == here.
      double const softClippedAt10 = meshClipper.softClip(10.0);
      compare(softClippedAt10, 0.0);
      check(meshClipper.isAllVisible());

      // If we soft-clip at the bottom there should be another special case
      // giving us an unaltered maxHeight
      // Even though some points are still visible, the remaining visible
      // mesh has no height.
      double const softClippedAt0 = meshClipper.softClip(0.0);
      compare(softClippedAt0, maxHeight);
      // The four points at z=0.0 should not have been cut away
      auto const visiblePoints = meshClipper.countVisible();
      compare(visiblePoints, 4U);

      // The value 0.0 is hard coded into the model's bottom layer,
      // and we should get it back here unaltered, as a special case
      // within the softClip function, so that we may compare doubles
      // using operator==
      double const softHeightLeft = meshClipper.softMaxHeight();
      compare(softHeightLeft, 0.0);
    }
    {
      MeshClipper meshClipper{
          Mesh{Stl{getPath("test-models/small-cube.ascii.stl")}}};
      compare(meshClipper.maxHeight(), 10.0);

      double const softClippedAt5 = meshClipper.softClip(5.0);
      double constexpr eps = 1e-6;
      check(softClippedAt5 < 5.0 + eps);
      check(softClippedAt5 > 5.0 - eps);

      double const softHeightLeft = meshClipper.softMaxHeight();
      check(softHeightLeft < 5.0 + eps);
      check(softHeightLeft > 5.0 - eps);
    }
  } catch (...) {
    return 1;
  }

  return 0;
}
