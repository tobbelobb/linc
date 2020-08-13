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
      Vertex const point{Vertex{0, 0, 0}};
      Vertex const &itself{point};
      compare(point, itself);
      constexpr auto eps = VertexConstants::eps;
      Vertex const pointEps{Vertex{eps, eps, eps}};
      compare(point, pointEps);
      constexpr double eps2{eps + 1e-16};
      Vertex const pointEps2{Vertex{eps2, eps, eps}};
      check(point != pointEps2);
    }
    {
      // test operator< and operator== for MeshClipper::Edge
      std::vector<Vertex> examplePoints{Vertex{{0, 0, 0}}, Vertex{{1, 0, 0}}};
      MeshClipper::Edge const forwards{examplePoints, {0, 1}};
      MeshClipper::Edge const backwards{examplePoints, {1, 0}};
      compare(forwards, backwards);
      check(not(forwards != backwards));

      // Basing the edge off of another points list should not affect equality
      std::vector<Vertex> examplePoints2{Vertex{{1, 0, 0}}, Vertex{{0, 0, 0}}};
      MeshClipper::Edge const forwards2{examplePoints2, {0, 1}};
      compare(forwards, forwards2);

      // Limit of inequality should be between eps and eps2
      constexpr auto eps = VertexConstants::eps;
      constexpr double eps2{eps + 1e-16};
      std::vector<Vertex> examplePointsEps2{Vertex{{1 + eps2, 0, 0}},
                                            Vertex{{0, 0, 0}}};
      MeshClipper::Edge biggerEdge{examplePointsEps2, {0, 1}};
      check(forwards != biggerEdge);
      check(forwards < biggerEdge);
    }
    {
      // Test operator== for MeshClipper::Triangle
      std::vector<Vertex> examplePoints{{0, 0, 0}, {0, 1, 0}, {1, 0, 0}};
      std::vector<Vertex> examplePoints2{{0, 0, 0}, {0, 1, 0}, {1, 0, 0}};
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
      std::vector<Vertex> examplePointsEps{
          {eps, eps, eps}, {eps, 1, eps}, {1, eps, eps}};
      std::vector<Vertex> examplePointsEps2{
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
      std::vector<Vertex> expectedPoints{
          {Vertex{-5, -5,  0}},
          {Vertex{-5, -5, 10}},
          {Vertex{-5,  5,  0}},
          {Vertex{-5,  5, 10}},
          {Vertex{ 5, -5,  0}},
          {Vertex{ 5, -5, 10}},
          {Vertex{ 5,  5,  0}},
          {Vertex{ 5,  5, 10}}};
      // clang-format on
      std::sort(expectedPoints.begin(), expectedPoints.end());
      compare(meshClipper.m_points, expectedPoints);

      // Edges
      compare(meshClipper.m_edges.size(), 18U);
      // The short vector of m_users is expected to be equal and sorted
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
          {expectedEdges, {0, 2, 6}},    {expectedEdges, {0, 4, 7}},
          {expectedEdges, {1, 2, 8}},    {expectedEdges, {1, 5, 9}},
          {expectedEdges, {3, 4, 13}},   {expectedEdges, {3, 5, 14}},
          {expectedEdges, {6, 7, 10}},   {expectedEdges, {8, 9, 11}},
          {expectedEdges, {10, 12, 16}}, {expectedEdges, {11, 12, 17}},
          {expectedEdges, {13, 14, 15}}, {expectedEdges, {15, 16, 17}}};
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
      // Test getPointsVisibility
      MeshClipper meshClipper{
          Mesh{Stl{getPath("test-models/tetrahedron.ascii.stl")}}};

      compare(meshClipper.m_points.at(0).z(), 0.0_mm);
      compare(meshClipper.m_points.at(1).z(), 1.0_mm);
      compare(meshClipper.m_points.at(2).z(), 0.0_mm);
      compare(meshClipper.m_points.at(3).z(), 0.0_mm);

      auto const ret05 = meshClipper.getPointsVisibility(0.5_mm);
      compare(ret05.at(0), true);
      compare(ret05.at(1), false);
      compare(ret05.at(2), true);
      compare(ret05.at(3), true);

      auto const ret_05 = meshClipper.getPointsVisibility(-0.5_mm);
      compare(ret_05.at(0), false);
      compare(ret_05.at(1), false);
      compare(ret_05.at(2), false);
      compare(ret_05.at(3), false);

      auto const ret15 = meshClipper.getPointsVisibility(1.5_mm);
      compare(ret15.at(0), true);
      compare(ret15.at(1), true);
      compare(ret15.at(2), true);
      compare(ret15.at(3), true);
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
      auto const visible10{meshClipper.softClip(10.0)};
      compare(meshClipper.m_points.size(), visible10.size());
      double const softClippedAt10 = meshClipper.softMaxHeight(visible10);
      compare(softClippedAt10, 10.0);
      check(std::all_of(visible10.begin(), visible10.end(),
                        [](bool const b) { return b; }));

      // If we soft-clip at the bottom there should be another special case
      auto const visible0{meshClipper.softClip(0.0)};
      double const softClippedAt0 = meshClipper.softMaxHeight(visible0);
      compare(softClippedAt0, 0.0);
      // The four points at z=0.0 should not have been cut away
      auto const visiblePoints =
          std::count(visible0.begin(), visible0.end(), true);
      compare(visiblePoints, 4U);
    }
    {
      MeshClipper meshClipper{
          Mesh{Stl{getPath("test-models/broken/standing-triangle.ascii.stl")}}};
      auto const visible5{meshClipper.softClip(5.0)};
      double const softClippedAt5 = meshClipper.softMaxHeight(visible5);
      compare(softClippedAt5, 5.0);

      // POINTS correctness
      compare(meshClipper.m_points.size(), 5U);
      compare(std::count(visible5.begin(), visible5.end(), true), 4U);
      std::vector<Vertex> expectedPoints{
          {0, -5, 0}, {0, 0, 10}, {0, 5, 0}, {0, -2.5, 5}, {0, 2.5, 5}};
      compare(meshClipper.m_points, expectedPoints);

      // EDGES correctness
      compare(meshClipper.m_edges.size(), 5U);
      std::vector<MeshClipper::Edge> expectedEdges{
          {expectedPoints, {0, 3}, {0}},
          {expectedPoints, {0, 2}, {0}},
          {expectedPoints, {2, 4}, {1}},
          {expectedPoints, {3, 4}, {1}},
          {expectedPoints, {2, 3}, {0, 1}}};
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
          {expectedEdges, {0, 1, 4}, true}, {expectedEdges, {2, 3, 4}, true}};
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

      auto const visible5 = meshClipper.softClip(5.0);
      double const softClippedAt5 = meshClipper.softMaxHeight(visible5);
      double constexpr eps = 1e-6;
      check(softClippedAt5 < 5.0 + eps);
      check(softClippedAt5 > 5.0 - eps);

      // POINT correctness
      compare(meshClipper.m_points.size(), 16U);
      compare(std::count(visible5.begin(), visible5.end(), true), 12U);
      // clang-format off
      std::vector<Vertex> expectedPoints{
        {Vertex{-5, -5,  0}},
        {Vertex{-5, -5, 10}},
        {Vertex{-5,  5,  0}},
        {Vertex{-5,  5, 10}},
        {Vertex{ 5, -5,  0}},
        {Vertex{ 5, -5, 10}},
        {Vertex{ 5,  5,  0}},
        {Vertex{ 5,  5, 10}},
        {Vertex{-5, -5,  5}},
        {Vertex{-5,  0,  5}},
        {Vertex{ 0, -5,  5}},
        {Vertex{-5,  5,  5}},
        {Vertex{ 0,  5,  5}},
        {Vertex{ 5, -5,  5}},
        {Vertex{ 5,  0,  5}},
        {Vertex{ 5,  5,  5}}};
      // clang-format on
      std::vector<bool> expectedVisible{true, false, true, false, true, false,
                                        true, false, true, true,  true, true,
                                        true, true,  true, true};
      compare(meshClipper.m_points, expectedPoints);
      compare(visible5.size(), expectedVisible.size());
      for (std::size_t i{0}; i < visible5.size(); ++i) {
        compare(visible5[i], expectedVisible[i]);
      }

      // EDGE correctness
      compare(meshClipper.m_edges.size(), 30U);
      // clang-format off
      std::vector<MeshClipper::Edge> expectedEdges{
          {expectedPoints, { 0,  8}, { 0,  1}},
          {expectedPoints, { 0,  2}, { 2,  3}},
          {expectedPoints, { 0,  9}, { 0,  2}},
          {expectedPoints, { 0,  4}, { 4,  5}},
          {expectedPoints, {10,  0}, { 1,  4}},
          {expectedPoints, { 6,  0}, { 3,  5}},
          {expectedPoints, { 3,  1}, { 0,  6}},
          {expectedPoints, { 5,  1}, { 1,  6}},
          {expectedPoints, {11,  2}, {12,  7}},
          {expectedPoints, { 2,  6}, { 3,  7}},
          {expectedPoints, { 3,  5}, { 6,  8}},
          {expectedPoints, {12,  6}, {14,  9}},
          {expectedPoints, { 7,  3}, { 8,  9}},
          {expectedPoints, {13,  4}, {13, 10}},
          {expectedPoints, { 4,  6}, { 5, 10}},
          {expectedPoints, { 6, 14}, {15, 11}},
          {expectedPoints, { 7,  5}, { 8, 11}},
          {expectedPoints, {15,  6}, { 9, 11}},
          {expectedPoints, { 8,  9}, {     0}},
          {expectedPoints, { 8, 10}, {     1}},
          {expectedPoints, { 9, 11}, {    12}},
          {expectedPoints, { 9,  2}, { 2, 12}},
          {expectedPoints, {10, 13}, {    13}},
          {expectedPoints, {10,  4}, { 4, 13}},
          {expectedPoints, {11, 12}, {    14}},
          {expectedPoints, {11,  6}, { 7, 14}},
          {expectedPoints, {12, 15}, {     9}},
          {expectedPoints, {13, 14}, {    15}},
          {expectedPoints, {13,  6}, {10, 15}},
          {expectedPoints, {14, 15}, {    11}}};
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
      // I got tired.
      // meshClipper.writeBinaryStl(getPath("test-models/broken/clipped-small-cube.binary.stl"));
    }
    {
      Mesh const mesh{Stl{getPath("test-models/small-cube.binary.stl")}};
      double const topHeight = mesh.maxHeight();
      compare(topHeight, 10.0);
      double const minHeight = mesh.minHeight();
      compare(minHeight, 0.0);
      for (size_t h{0}; h <= 10; ++h) {
        MeshClipper partialPrint{mesh};
        auto const newH{static_cast<double>(h)};
        auto const visibleH = partialPrint.softClip(newH);
        // Clip should give exactly the new max height that we ask for.
        // No truncation errors allowed.
        compare(partialPrint.softMaxHeight(visibleH), newH);
        std::vector<Vertex> topVertices = partialPrint.getVerticesAt(newH);
        // Top and bottom cut of the cube should have 4 vertices
        // All in between should have 8
        if (h == 0 or h == 10) {
          compare(topVertices.size(), 4U);
        } else {
          compare(topVertices.size(), 8U);
        }
      }
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

      auto const visible1 = meshClipper.softClip(1.0);
      double const softClippedAt1 =
          maxHeight - meshClipper.softMaxHeight(visible1);
      compare(softClippedAt1, 47.0);

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
      check(meshClipper2.m_points.size() <=
            static_cast<std::size_t>(
                std::count(visible1.begin(), visible1.end(), true)));
      check(meshClipper2.m_triangles.size() <=
            meshClipper.countVisibleTriangles());
    }
  } catch (...) {
    return 1;
  }
  return 0;
}
