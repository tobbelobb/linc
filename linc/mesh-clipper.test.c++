#include <filesystem>
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
          {{ 5, -5,  0}, 0.0_mm, 0, true},
          {{ 5, -5, 10}, 0.0_mm, 0, true},
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
      check(meshClipper.isAllPointsVisible());

      // If we soft-clip at the bottom there should be another special case
      // giving us an unaltered maxHeight
      // Even though some points are still visible, the remaining visible
      // mesh has no height.
      double const softClippedAt0 = meshClipper.softClip(0.0);
      compare(softClippedAt0, maxHeight);
      // The four points at z=0.0 should not have been cut away
      auto const visiblePoints = meshClipper.countVisiblePoints();
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
          Mesh{Stl{getPath("test-models/broken/standing-triangle.ascii.stl")}}};
      double const softClippedAt5 = meshClipper.softClip(5.0);
      compare(softClippedAt5, 5.0);
      compare(meshClipper.softMaxHeight(), 5.0);

      // POINTS correctness
      compare(meshClipper.m_points.size(), 5U);
      compare(meshClipper.countVisiblePoints(), 4U);
      std::vector<MeshClipper::Point> expectedPoints{
          {0, -5, 0}, {0, 0, 10}, {0, 5, 0}, {0, -2.5, 5}, {0, 2.5, 5}};
      compare(meshClipper.m_points, expectedPoints);

      // EDGES correctness
      compare(meshClipper.m_edges.size(), 5U);
      compare(meshClipper.countVisibleEdges(), 5U);
      std::vector<MeshClipper::Edge> expectedEdges{
          {expectedPoints, {0, 3}, {0}, true},
          {expectedPoints, {0, 2}, {0}, true},
          {expectedPoints, {2, 4}, {1}, true},
          {expectedPoints, {3, 4}, {1}, true},
          {expectedPoints, {2, 3}, {0, 1}, true}};
      compare(meshClipper.m_edges, expectedEdges);
      for (auto const &[i, expectedEdge] : enumerate(expectedEdges)) {
        bool fullEquals = expectedEdge.fullEquals(meshClipper.m_edges[i]);
        if (not fullEquals) {
          std::cerr << "Index " << i << " Expected " << expectedEdge << " got "
                    << meshClipper.m_edges[i] << '\n';
        }
        check(fullEquals);
      }

      // TRIANGLE correctness
      compare(meshClipper.m_triangles.size(), 2U);
      compare(meshClipper.countVisibleTriangles(), 2U);
      std::vector<MeshClipper::Triangle> expectedTriangles{
          {expectedEdges, {0, 1, 4}, Normal::Zero(), true},
          {expectedEdges, {2, 3, 4}, Normal::Zero(), true}};
      compare(meshClipper.m_triangles, expectedTriangles);
      for (auto const &[i, expectedTriangle] : enumerate(expectedTriangles)) {
        bool fullEquals =
            expectedTriangle.fullEquals(meshClipper.m_triangles[i]);
        if (not fullEquals) {
          std::cerr << "Index " << i << " Expected " << expectedTriangle
                    << " got " << meshClipper.m_triangles[i] << '\n';
        }
        check(fullEquals);
      }
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

      // POINT correctness
      compare(meshClipper.m_points.size(), 16U);
      compare(meshClipper.countVisiblePoints(), 12U);
      // clang-format off
      std::vector<MeshClipper::Point> expectedPoints{
        {{-5, -5,  0}, -5.0_mm, 0, true},
        {{-5, -5, 10},  5.0_mm, 0, false},
        {{-5,  5,  0}, -5.0_mm, 0, true},
        {{-5,  5, 10},  5.0_mm, 0, false},
        {{ 5, -5,  0}, -5.0_mm, 0, true},
        {{ 5, -5, 10},  5.0_mm, 0, false},
        {{ 5,  5,  0}, -5.0_mm, 0, true},
        {{ 5,  5, 10},  5.0_mm, 0, false},
        {{-5, -5,  5},  0.0_mm, 0, true},
        {{-5,  0,  5},  0.0_mm, 0, true},
        {{ 0, -5,  5},  0.0_mm, 0, true},
        {{-5,  5,  5},  0.0_mm, 0, true},
        {{ 0,  5,  5},  0.0_mm, 0, true},
        {{ 5, -5,  5},  0.0_mm, 0, true},
        {{ 5,  0,  5},  0.0_mm, 0, true},
        {{ 5,  5,  5},  0.0_mm, 0, true}};
      // clang-format on
      compare(meshClipper.m_points, expectedPoints);
      for (auto const &[i, expectedPoint] : enumerate(expectedPoints)) {
        bool const fullEquals =
            expectedPoint.m_distance == meshClipper.m_points[i].m_distance and
            expectedPoint.m_visible == meshClipper.m_points[i].m_visible;
        if (not fullEquals) {
          std::cerr << "Index " << i << " Expected " << expectedPoint << " got "
                    << meshClipper.m_points[i] << '\n';
        }
        check(fullEquals);
      }

      // EDGE correctness
      compare(meshClipper.m_edges.size(), 30U);
      compare(meshClipper.countVisibleEdges(), 25U);
      // clang-format off
      std::vector<MeshClipper::Edge> expectedEdges{
          {expectedPoints, { 0,  8}, { 0,  1},  true},
          {expectedPoints, { 0,  2}, { 2,  3},  true},
          {expectedPoints, { 0,  9}, { 0,  2},  true},
          {expectedPoints, { 0,  4}, { 4,  5},  true},
          {expectedPoints, {10,  0}, { 1,  4},  true},
          {expectedPoints, { 6,  0}, { 3,  5},  true},
          {expectedPoints, { 3,  1}, { 0,  6}, false},
          {expectedPoints, { 5,  1}, { 1,  6}, false},
          {expectedPoints, {11,  2}, {12,  7},  true},
          {expectedPoints, { 2,  6}, { 3,  7},  true},
          {expectedPoints, { 3,  5}, { 6,  8}, false},
          {expectedPoints, {12,  6}, {14,  9},  true},
          {expectedPoints, { 7,  3}, { 8,  9}, false},
          {expectedPoints, {13,  4}, {13, 10},  true},
          {expectedPoints, { 4,  6}, { 5, 10},  true},
          {expectedPoints, { 6, 14}, {15, 11},  true},
          {expectedPoints, { 7,  5}, { 8, 11}, false},
          {expectedPoints, {15,  6}, { 9, 11},  true},
          {expectedPoints, { 8,  9}, {     0},  true},
          {expectedPoints, { 8, 10}, {     1},  true},
          {expectedPoints, { 9, 11}, {    12},  true},
          {expectedPoints, { 9,  2}, { 2, 12},  true},
          {expectedPoints, {10, 13}, {    13},  true},
          {expectedPoints, {10,  4}, { 4, 13},  true},
          {expectedPoints, {11, 12}, {    14},  true},
          {expectedPoints, {11,  6}, { 7, 14},  true},
          {expectedPoints, {12, 15}, {     9},  true},
          {expectedPoints, {13, 14}, {    15},  true},
          {expectedPoints, {13,  6}, {10, 15},  true},
          {expectedPoints, {14, 15}, {    11},  true}};
      // clang-format on
      compare(meshClipper.m_edges, expectedEdges);
      for (auto const &[i, expectedEdge] : enumerate(expectedEdges)) {
        bool fullEquals = expectedEdge.fullEquals(meshClipper.m_edges[i]);
        if (not fullEquals) {
          std::cerr << "Index " << i << " Expected " << expectedEdge << " got "
                    << meshClipper.m_edges[i] << '\n';
        }
        check(fullEquals);
      }

      // TRIANGLE correctness
      compare(meshClipper.m_triangles.size(), 16U);
      // meshClipper.writeBinaryStl(getPath("test-models/broken/clipped-small-cube.binary.stl"));
    }
    {
      // Confirm that we can load a large complicated broken mesh.
      // If this test gets annoyingly slow, work on performance
      // until it's not.
      MeshClipper meshClipper(
          Mesh{Stl{getPath("test-models/broken/3DBenchy.binary.stl")}});
      compare(meshClipper.m_points.size(), 112569U);
      compare(meshClipper.m_edges.size(), 337731U);
      compare(meshClipper.m_triangles.size(), 225154U);

      double constexpr eps = 1e-6;
      double const maxHeight = meshClipper.maxHeight();
      check(maxHeight > 48.0 - eps);
      check(48.0 + eps > maxHeight);

      double const softClippedAt10 = meshClipper.softClip(1.0);
      compare(softClippedAt10, 47.0);

      // Check that we can write a binary stl and read it back again
      auto const clippedBenchyPath{
          getPath("test-models/broken/clipped-benchy.binary.stl")};
      meshClipper.writeBinaryStl(clippedBenchyPath);
      MeshClipper meshClipper2{Mesh{Stl{clippedBenchyPath}}};
      std::filesystem::remove(clippedBenchyPath);

      // Some non-unique vertices, edges, and triangles will have been created
      // during our cutting procedure. These will get cleaned out only when
      // the data is read back in. What we can check is that new vertices,
      // edges, and triangles were invented, or printed although despite being
      // invisible
      check(meshClipper2.m_points.size() <= meshClipper.countVisiblePoints());
      check(meshClipper2.m_edges.size() <= meshClipper.countVisibleEdges());
      check(meshClipper2.m_triangles.size() <=
            meshClipper.countVisibleTriangles());
    }
  } catch (...) {
    return 1;
  }

  return 0;
}
