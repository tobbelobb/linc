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

      // Only vertices should affect equality
      compare(forwards, MeshClipper::Edge{examplePoints, {0, 1}, {1, 2}, true});
      compare(forwards,
              MeshClipper::Edge{examplePoints, {0, 1}, {1, 2}, false});

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
          {expectedPoints, {0, 1}, {0, 1}, true},   // 0
          {expectedPoints, {0, 2}, {2, 3}, true},   // 1
          {expectedPoints, {0, 3}, {0, 2}, true},   // 2
          {expectedPoints, {0, 4}, {4, 5}, true},   // 3
          {expectedPoints, {0, 5}, {1, 4}, true},   // 4
          {expectedPoints, {0, 6}, {3, 5}, true},   // 5
          {expectedPoints, {1, 3}, {0, 6}, true},   // 6
          {expectedPoints, {1, 5}, {1, 6}, true},   // 7
          {expectedPoints, {2, 3}, {2, 7}, true},   // 8
          {expectedPoints, {2, 6}, {3, 7}, true},   // 9
          {expectedPoints, {3, 5}, {6, 8}, true},   // 10
          {expectedPoints, {3, 6}, {7, 9}, true},   // 11
          {expectedPoints, {3, 7}, {8, 9}, true},   // 12
          {expectedPoints, {4, 5}, {4, 10}, true},  // 13
          {expectedPoints, {4, 6}, {5, 10}, true},  // 14
          {expectedPoints, {5, 6}, {10, 11}, true}, // 15
          {expectedPoints, {5, 7}, {8, 11}, true},  // 16
          {expectedPoints, {6, 7}, {9, 11}, true}}; // 17
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
          {expectedEdges, {0, 2, 6}, Normal::Zero(), true},
          {expectedEdges, {0, 4, 7}, Normal::Zero(), true},
          {expectedEdges, {1, 2, 8}, Normal::Zero(), true},
          {expectedEdges, {1, 5, 9}, Normal::Zero(), true},
          {expectedEdges, {3, 4, 13}, Normal::Zero(), true},
          {expectedEdges, {3, 5, 14}, Normal::Zero(), true},
          {expectedEdges, {6, 7, 10}, Normal::Zero(), true},
          {expectedEdges, {8, 9, 11}, Normal::Zero(), true},
          {expectedEdges, {10, 12, 16}, Normal::Zero(), true},
          {expectedEdges, {11, 12, 17}, Normal::Zero(), true},
          {expectedEdges, {13, 14, 15}, Normal::Zero(), true},
          {expectedEdges, {15, 16, 17}, Normal::Zero(), true}};
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
      compare(meshClipper.maxHeight(), 10.0);
    }
    {
      MeshClipper const meshClipper(
          Mesh{Stl{getPath("test-models/benchy-low-poly.binary.stl")}});
      double const maxHeight = meshClipper.maxHeight();
      check(maxHeight > 48.0 - 0.01);
      check(48.0 + 0.01 > maxHeight);
    }
    {
      // Confirm that we can load a large complicated mesh.
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
      // Stl const tetrahedronStl{"test-models/tetrahedron.ascii.stl"};
      // MeshClipper meshClipper{};
    }
  } catch (...) {
    return 1;
  }

  return 0;
}
