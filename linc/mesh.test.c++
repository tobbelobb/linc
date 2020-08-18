#include <filesystem>
#include <iostream>

#include <linc/mesh.h++>
#include <linc/test-framework.h++>
#include <linc/units.h++>
#include <linc/util.h++>

auto main() -> int {
  try {
    {
      // Double check that we got the VertexCompare right
      std::set<Vertex> setOfVertices{Vertex{0, 0, 0}};
      constexpr auto eps = VertexConstants::eps;
      // All these vertices should be considered equal to {0, 0, 0}
      // clang-format off
      std::vector<Vertex> smalls {
        {-eps, -eps, -eps},
        {   0, -eps, -eps},
        { eps, -eps, -eps},
        {-eps,    0, -eps},
        {   0,    0, -eps},
        { eps,    0, -eps},
        {-eps,  eps, -eps},
        {   0,  eps, -eps},
        { eps,  eps, -eps},
        {-eps, -eps,    0},
        {   0, -eps,    0},
        { eps, -eps,    0},
        {-eps,    0,    0},
        {   0,    0,    0},
        { eps,    0,    0},
        {-eps,  eps,    0},
        {   0,  eps,    0},
        { eps,  eps,    0},
        {-eps, -eps,  eps},
        {   0, -eps,  eps},
        { eps, -eps,  eps},
        {-eps,    0,  eps},
        {   0,    0,  eps},
        { eps,    0,  eps},
        {-eps,  eps,  eps},
        {   0,  eps,  eps},
        { eps,  eps,  eps}
      };
      // clang-format on

      for (auto const &small : smalls) {
        auto [iterator, insertionTookPlace] = setOfVertices.insert(small);
        check(not insertionTookPlace);
      }
      // All these vertices should be considered different
      // from {0,0,0} and from each other
      constexpr Millimeter eps2{std::nexttoward(2 * eps, eps)};
      // clang-format off
      std::vector<Vertex> bigs {
        {-eps2, -eps2, -eps2},
        {    0, -eps2, -eps2},
        { eps2, -eps2, -eps2},
        {-eps2,     0, -eps2},
        {    0,     0, -eps2},
        { eps2,     0, -eps2},
        {-eps2,  eps2, -eps2},
        {    0,  eps2, -eps2},
        { eps2,  eps2, -eps2},
        {-eps2, -eps2,     0},
        {    0, -eps2,     0},
        { eps2, -eps2,     0},
        {-eps2,     0,     0},
        { eps2,     0,     0},
        {-eps2,  eps2,     0},
        {    0,  eps2,     0},
        { eps2,  eps2,     0},
        {-eps2, -eps2,  eps2},
        {    0, -eps2,  eps2},
        { eps2, -eps2,  eps2},
        {-eps2,     0,  eps2},
        {    0,     0,  eps2},
        { eps2,     0,  eps2},
        {-eps2,  eps2,  eps2},
        {    0,  eps2,  eps2},
        { eps2,  eps2,  eps2}
      };
      // clang-format on
      for (auto const &big : bigs) {
        auto [iterator, insertionTookPlace] = setOfVertices.insert(big);
        check(insertionTookPlace);
      }
      for (auto const &small : smalls) {
        auto [iterator, insertionTookPlace] = setOfVertices.insert(small);
        check(not insertionTookPlace);
      }
    }
    {
      // Check Point construction and comparison works as expected
      Vertex const point{Vertex{0, 0, 0}};
      Vertex const &itself{point};
      compare(point, itself);
      constexpr auto eps = VertexConstants::eps;
      Vertex const pointEps{Vertex{eps, eps, eps}};
      compare(point, pointEps);
      constexpr double eps2{eps + std::nexttoward(2 * eps, eps)};
      Vertex const pointEps2{Vertex{eps2, eps, eps}};
      check(point != pointEps2);
    }
    {
      // test operator< and operator== for Mesh::Edge
      Mesh::Edge const forwards{{0, 1}};
      Mesh::Edge const backwards{{1, 0}};
      compare(forwards, backwards);
      check(not(forwards != backwards));
    }
    {
      // Test operator== for Mesh::Triangle
      std::vector<Vertex> examplePoints{{0, 0, 0}, {0, 1, 0}, {1, 0, 0}};
      std::vector<Vertex> examplePoints2{{0, 0, 0}, {0, 1, 0}, {1, 0, 0}};
      std::vector<Mesh::Edge> exampleEdges{{{0, 1}}, {{1, 2}}, {{2, 0}}};
      std::vector<Mesh::Edge> exampleEdges2{{{0, 1}}, {{1, 2}}, {{2, 0}}};

      Mesh::Triangle triangle{{0, 1, 2}};
      Mesh::Triangle const &itself = triangle;
      compare(triangle, itself);

      // Equality should not depend on order of edges
      Mesh::Triangle triangle2{{1, 0, 2}};
      compare(triangle, triangle2);
    }
    {
      std::vector<Vertex> v{};
      std::vector<Mesh::Edge> e{};
      std::vector<Mesh::Triangle> t{};
      Mesh const meshClipper(
          Stl{getPath("test-models/broken/standing-triangle.ascii.stl")}, v, e,
          t);

      // Vertices
      compare(meshClipper.m_points.size(), 3U);
      std::vector<Vertex> expectedVertices{{0, -5, 0}, {0, 0, 10}, {0, 5, 0}};
      std::sort(expectedVertices.begin(), expectedVertices.end());
      compare(meshClipper.m_points, expectedVertices);

      // Edges
      compare(meshClipper.m_edges.size(), 3U);
      std::vector<Mesh::Edge> expectedEdges{
          {{0, 1}, {0}}, {{0, 2}, {0}}, {{1, 2}, {0}}};
      std::sort(expectedEdges.begin(), expectedEdges.end());
      compare(meshClipper.m_edges, expectedEdges);

      // Triangles
      compare(meshClipper.m_triangles.size(), 1U);
      std::vector<Mesh::Triangle> const expectedTriangles{{{0, 1, 2}}};
      compare(meshClipper.m_triangles, expectedTriangles);
    }
    {
      std::vector<Vertex> v{};
      std::vector<Mesh::Edge> e{};
      std::vector<Mesh::Triangle> t{};
      Mesh const meshClipper(Stl{getPath("test-models/tetrahedron.ascii.stl")},
                             v, e, t);

      compare(meshClipper.m_points.size(), 4U);
      std::vector<Vertex> expectedVertices{
          {0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}};
      compare(meshClipper.m_points, expectedVertices);

      compare(meshClipper.m_edges.size(), 6U);
      // All vertices are connected in a tetrahedron
      // Never mind the m_users lists. They are not
      // considered by the operator==.
      std::vector<Mesh::Edge> expectedEdges{{{0, 1}}, {{0, 2}}, {{0, 3}},
                                            {{1, 2}}, {{1, 3}}, {{2, 3}}};
      // We expect the constructor to have sorted the edges
      std::sort(expectedEdges.begin(), expectedEdges.end());

      compare(meshClipper.m_edges, expectedEdges);

      compare(meshClipper.m_triangles.size(), 4U);
      // Constructing these triangles manually from edges is tricky
      std::vector<Mesh::Triangle> expectedTriangles{
          {{0, 1, 3}}, {{0, 2, 4}}, {{1, 2, 5}}, {{3, 4, 5}}};
      // We expect the constructor to have sorted the triangles
      std::sort(expectedTriangles.begin(), expectedTriangles.end());
      compare(meshClipper.m_triangles, expectedTriangles);
    }
    {
      std::vector<Vertex> v{};
      std::vector<Mesh::Edge> e{};
      std::vector<Mesh::Triangle> t{};
      Mesh const meshClipper{Stl{getPath("test-models/small-cube.ascii.stl")},
                             v, e, t};

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
      // For an edge to be fully equal to another edge
      std::vector<Mesh::Edge> expectedEdges{{{0, 1}, {0, 1}},   // 0
                                            {{0, 2}, {2, 3}},   // 1
                                            {{0, 3}, {0, 2}},   // 2
                                            {{0, 4}, {4, 5}},   // 3
                                            {{0, 5}, {1, 4}},   // 4
                                            {{0, 6}, {3, 5}},   // 5
                                            {{1, 3}, {0, 6}},   // 6
                                            {{1, 5}, {1, 6}},   // 7
                                            {{2, 3}, {2, 7}},   // 8
                                            {{2, 6}, {3, 7}},   // 9
                                            {{3, 5}, {6, 8}},   // 10
                                            {{3, 6}, {7, 9}},   // 11
                                            {{3, 7}, {8, 9}},   // 12
                                            {{4, 5}, {4, 10}},  // 13
                                            {{4, 6}, {5, 10}},  // 14
                                            {{5, 6}, {10, 11}}, // 15
                                            {{5, 7}, {8, 11}},  // 16
                                            {{6, 7}, {9, 11}}}; // 17
      std::sort(expectedEdges.begin(), expectedEdges.end());
      compare(expectedEdges.size(), meshClipper.m_edges.size());

      for (auto const &[i, expectedEdge] : enumerate(expectedEdges)) {
        Mesh::Edge const &gotEdge = meshClipper.m_edges[i];
        bool const usersEqual = gotEdge.m_users == expectedEdge.m_users;
        if (not usersEqual) {
          std::cerr << "index: " << i << " expected: " << expectedEdge
                    << " got " << gotEdge << '\n';
        }
        compare(gotEdge, expectedEdge);
        check(usersEqual);
      }

      // Triangles
      compare(meshClipper.m_triangles.size(), 12U);
      std::vector<Mesh::Triangle> const expectedTriangles{
          {{0, 2, 6}},    {{0, 4, 7}},    {{1, 2, 8}},    {{1, 5, 9}},
          {{3, 4, 13}},   {{3, 5, 14}},   {{6, 7, 10}},   {{8, 9, 11}},
          {{10, 12, 16}}, {{11, 12, 17}}, {{13, 14, 15}}, {{15, 16, 17}}};
      compare(expectedTriangles.size(), meshClipper.m_triangles.size());

      for (auto const &[i, expectedTriangle] : enumerate(expectedTriangles)) {
        Mesh::Triangle const &gotTriangle = meshClipper.m_triangles[i];
        bool const visibleEqual =
            gotTriangle.m_visible == expectedTriangle.m_visible;
        if (not visibleEqual) {
          std::cerr << "index: " << i << " expected: \n"
                    << expectedTriangle << "\ngot: \n"
                    << gotTriangle << '\n';
        }
        compare(gotTriangle, expectedTriangle);
        check(visibleEqual);
      }
    }
    {
      // Test maxHeight
      std::vector<Vertex> v{};
      std::vector<Mesh::Edge> e{};
      std::vector<Mesh::Triangle> t{};
      Mesh const meshClipper{
          Stl{getPath("test-models/broken/standing-triangle.ascii.stl")}, v, e,
          t};
      compare(meshClipper.maxHeight(), 10.0_mm);
    }
    {
      std::vector<Vertex> v{};
      std::vector<Mesh::Edge> e{};
      std::vector<Mesh::Triangle> t{};
      Mesh const meshClipper{
          Stl{getPath("test-models/benchy-low-poly.binary.stl")}, v, e, t};
      double const maxHeight = meshClipper.maxHeight();
      check(maxHeight > 48.0 - 0.01);
      check(48.0 + 0.01 > maxHeight);
    }
    {
      // Test getPointsVisibility
      std::vector<Vertex> v{};
      std::vector<Mesh::Edge> e{};
      std::vector<Mesh::Triangle> t{};
      Mesh meshClipper{Stl{getPath("test-models/tetrahedron.ascii.stl")}, v, e,
                       t};

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
      std::vector<Vertex> v{};
      std::vector<Mesh::Edge> e{};
      std::vector<Mesh::Triangle> t{};
      Mesh meshClipper{Stl{getPath("test-models/small-cube.ascii.stl")}, v, e,
                       t};
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
      std::vector<std::size_t> clippedTriangles{};
      auto const visible10{meshClipper.softClip(10.0, clippedTriangles)};
      compare(meshClipper.m_points.size(), visible10.size());
      double const softClippedAt10 = meshClipper.softMaxHeight(visible10);
      compare(softClippedAt10, 10.0);
      check(std::all_of(visible10.begin(), visible10.end(),
                        [](bool const b) { return b; }));

      // If we soft-clip at the bottom there should be another special case
      clippedTriangles.clear();
      auto const visible0{meshClipper.softClip(0.0, clippedTriangles)};
      double const softClippedAt0 = meshClipper.softMaxHeight(visible0);
      compare(softClippedAt0, 0.0);
      // The four points at z=0.0 should not have been cut away
      auto const visiblePoints =
          std::count(visible0.begin(), visible0.end(), true);
      compare(visiblePoints, 4U);
    }
    {
      std::vector<Vertex> v{};
      std::vector<Mesh::Edge> e{};
      std::vector<Mesh::Triangle> t{};
      Mesh meshClipper{
          Stl{getPath("test-models/broken/standing-triangle.ascii.stl")}, v, e,
          t};
      std::vector<std::size_t> clippedTriangles{};
      auto const visible5{meshClipper.softClip(5.0, clippedTriangles)};
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
      std::vector<Mesh::Edge> expectedEdges{{{0, 3}, {0}},
                                            {{0, 2}, {0}},
                                            {{2, 4}, {1}},
                                            {{3, 4}, {1}},
                                            {{2, 3}, {0, 1}}};
      compare(meshClipper.m_edges, expectedEdges);
      for (auto const &[i, expectedEdge] : enumerate(expectedEdges)) {
        bool usersEquals =
            expectedEdge.m_users == meshClipper.m_edges[i].m_users;
        if (not usersEquals) {
          std::cerr << "Index " << i << " Expected " << expectedEdge << " got "
                    << meshClipper.m_edges[i] << '\n';
        }
        check(usersEquals);
      }

      // TRIANGLE correctness
      compare(meshClipper.m_triangles.size(), 2U);
      compare(meshClipper.countVisibleTriangles(), 2U);
      std::vector<Mesh::Triangle> expectedTriangles{{{0, 1, 4}, true},
                                                    {{2, 3, 4}, true}};
      compare(meshClipper.m_triangles, expectedTriangles);
      for (auto const &[i, expectedTriangle] : enumerate(expectedTriangles)) {
        bool visibleEquals =
            expectedTriangle.m_visible == meshClipper.m_triangles[i].m_visible;
        if (not visibleEquals) {
          std::cerr << "Index " << i << " Expected " << expectedTriangle
                    << " got " << meshClipper.m_triangles[i] << '\n';
        }
        check(visibleEquals);
      }
    }
    {
      std::vector<Vertex> v{};
      std::vector<Mesh::Edge> e{};
      std::vector<Mesh::Triangle> t{};
      Mesh meshClipper{Stl{getPath("test-models/small-cube.ascii.stl")}, v, e,
                       t};
      compare(meshClipper.maxHeight(), 10.0_mm);

      std::vector<std::size_t> clippedTriangles{};
      auto const visible5 = meshClipper.softClip(5.0_mm, clippedTriangles);
      Millimeter const softClippedAt5 = meshClipper.softMaxHeight(visible5);
      Millimeter constexpr eps = 1e-6_mm;
      check(softClippedAt5 < 5.0_mm + eps);
      check(softClippedAt5 > 5.0_mm - eps);

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
      std::vector<Mesh::Edge> expectedEdges{
          {{ 0,  8}, { 0,  1}},
          {{ 0,  2}, { 2,  3}},
          {{ 0,  9}, { 0,  2}},
          {{ 0,  4}, { 4,  5}},
          {{10,  0}, { 1,  4}},
          {{ 6,  0}, { 3,  5}},
          {{ 3,  1}, { 0,  6}},
          {{ 5,  1}, { 1,  6}},
          {{11,  2}, {12,  7}},
          {{ 2,  6}, { 3,  7}},
          {{ 3,  5}, { 6,  8}},
          {{12,  6}, {14,  9}},
          {{ 7,  3}, { 8,  9}},
          {{13,  4}, {13, 10}},
          {{ 4,  6}, { 5, 10}},
          {{ 6, 14}, {15, 11}},
          {{ 7,  5}, { 8, 11}},
          {{15,  6}, { 9, 11}},
          {{ 8,  9}, {     0}},
          {{ 8, 10}, {     1}},
          {{ 9, 11}, {    12}},
          {{ 9,  2}, { 2, 12}},
          {{10, 13}, {    13}},
          {{10,  4}, { 4, 13}},
          {{11, 12}, {    14}},
          {{11,  6}, { 7, 14}},
          {{12, 15}, {     9}},
          {{13, 14}, {    15}},
          {{13,  6}, {10, 15}},
          {{14, 15}, {    11}}};
      // clang-format on
      compare(meshClipper.m_edges, expectedEdges);
      for (auto const &[i, expectedEdge] : enumerate(expectedEdges)) {
        bool usersEquals =
            expectedEdge.m_users == meshClipper.m_edges[i].m_users;
        if (not usersEquals) {
          std::cerr << "Index " << i << " Expected " << expectedEdge << " got "
                    << meshClipper.m_edges[i] << '\n';
        }
        check(usersEquals);
      }

      // TRIANGLE correctness
      compare(meshClipper.m_triangles.size(), 16U);
      // I got tired.
      // meshClipper.writeBinaryStl(getPath("test-models/broken/clipped-small-cube.binary.stl"));
    }
    {
      std::vector<Vertex> v{};
      std::vector<Mesh::Edge> e{};
      std::vector<Mesh::Triangle> t{};
      Mesh const meshClipper{Stl{getPath("test-models/small-cube.binary.stl")},
                             v, e, t};
      double const topHeight = meshClipper.maxHeight();
      compare(topHeight, 10.0);
      double const minHeight = meshClipper.minHeight();
      compare(minHeight, 0.0);
      for (size_t h{0}; h <= 10; ++h) {
        std::vector<Vertex> v2{};
        std::vector<Mesh::Edge> e2{};
        std::vector<Mesh::Triangle> t2{};
        std::vector<std::size_t> clippedTriangles{};
        Mesh partialPrint{meshClipper, v2, e2, t2, clippedTriangles};
        auto const newH{static_cast<Millimeter>(h)};

        auto const visibleH = partialPrint.softClip(newH, clippedTriangles);
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
      std::vector<Vertex> v{};
      std::vector<Mesh::Edge> e{};
      std::vector<Mesh::Triangle> t{};
      Mesh meshClipper{Stl{getPath("test-models/broken/3DBenchy.binary.stl")},
                       v, e, t};
      compare(meshClipper.m_points.size(), 112569U);
      compare(meshClipper.m_edges.size(), 337731U);
      compare(meshClipper.m_triangles.size(), 225154U);

      auto constexpr eps = 1e-4_mm;
      Millimeter const maxHeight = meshClipper.maxHeight();
      check(maxHeight > 48.0_mm - eps);
      check(48.0_mm + eps > maxHeight);

      std::vector<std::size_t> clippedTriangles{};
      auto const visible1 = meshClipper.softClip(1.0, clippedTriangles);
      Millimeter const softClippedAt1 =
          maxHeight - meshClipper.softMaxHeight(visible1);
      compare(softClippedAt1, 47.0_mm);

      // Check that we can write a binary stl and read it back again
      auto const clippedBenchyPath{
          getPath("test-models/broken/clipped-benchy.binary.stl")};
      meshClipper.writeBinaryStl(clippedBenchyPath);
      std::vector<Vertex> v2{};
      std::vector<Mesh::Edge> e2{};
      std::vector<Mesh::Triangle> t2{};
      Mesh meshClipper2{Stl{clippedBenchyPath}, v2, e2, t2};
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
