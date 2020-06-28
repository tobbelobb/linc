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
      constexpr double eps{1e-4};
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
      constexpr double eps{1e-4};
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

      constexpr double eps{1e-4};
      constexpr double eps2{1e-4 + 1e-16};
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
      std::vector<MeshClipper::Point> expectedPoints{
          {{-5, 5, 10}, 0.0_mm, 0, true}, {{5, -5, 10}, 0.0_mm, 0, true},
          {{5, 5, 10}, 0.0_mm, 0, true},  {{-5, -5, 10}, 0.0_mm, 0, true},
          {{-5, -5, 0}, 0.0_mm, 0, true}, {{5, 5, 0}, 0.0_mm, 0, true},
          {{5, -5, 0}, 0.0_mm, 0, true},  {{-5, 5, 0}, 0.0_mm, 0, true}};
      compare(meshClipper.m_points, expectedPoints);

      // Edges
      compare(meshClipper.m_edges.size(), 18U);
      std::vector<MeshClipper::Edge> expectedEdges{
          {expectedPoints, {0, 1}, {0, 1}, true},
          {expectedPoints, {1, 2}, {0, 6}, true},
          {expectedPoints, {2, 0}, {0, 8}, true},
          {expectedPoints, {0, 3}, {1, 11}, true},
          {expectedPoints, {3, 1}, {1, 4}, true},
          {expectedPoints, {4, 5}, {2, 3}, true},
          {expectedPoints, {5, 6}, {2, 7}, true},
          {expectedPoints, {6, 4}, {2, 5}, true},
          {expectedPoints, {4, 7}, {3, 10}, true},
          {expectedPoints, {7, 5}, {3, 9}, true},
          {expectedPoints, {4, 1}, {4, 5}, true},
          {expectedPoints, {3, 4}, {4, 11}, true},
          {expectedPoints, {6, 1}, {5, 7}, true},
          {expectedPoints, {1, 5}, {6, 7}, true},
          {expectedPoints, {5, 2}, {6, 8}, true},
          {expectedPoints, {5, 0}, {8, 9}, true},
          {expectedPoints, {7, 0}, {9, 10}, true},
          {expectedPoints, {4, 0}, {10, 11}, true}};
      compare(expectedEdges.size(), meshClipper.m_edges.size());
      for (auto const &[i, expectedEdge] : enumerate(expectedEdges)) {
        MeshClipper::Edge const &gotEdge = meshClipper.m_edges[i];
        bool const fullyEqual = gotEdge.fullEquals(expectedEdge);
        if (not fullyEqual) {
          std::cerr << "index: " << i << " expected: " << expectedEdge
                    << " got " << gotEdge << '\n';
        }
        compare(expectedEdge, gotEdge);
        check(fullyEqual);
      }

      // Triangles
      compare(meshClipper.m_triangles.size(), 12U);
      std::vector<MeshClipper::Triangle> const expectedTriangles{
          {expectedEdges, {0, 1, 2}, Normal::Zero(), true},
          {expectedEdges, {0, 3, 4}, Normal::Zero(), true},
          {expectedEdges, {5, 6, 7}, Normal::Zero(), true},
          {expectedEdges, {5, 8, 9}, Normal::Zero(), true},
          {expectedEdges, {4, 10, 11}, Normal::Zero(), true},
          {expectedEdges, {7, 10, 12}, Normal::Zero(), true},
          {expectedEdges, {1, 13, 14}, Normal::Zero(), true},
          {expectedEdges, {6, 12, 13}, Normal::Zero(), true},
          {expectedEdges, {2, 14, 15}, Normal::Zero(), true},
          {expectedEdges, {9, 15, 16}, Normal::Zero(), true},
          {expectedEdges, {8, 16, 17}, Normal::Zero(), true},
          {expectedEdges, {3, 11, 17}, Normal::Zero(), true}};
      compare(expectedTriangles.size(), meshClipper.m_triangles.size());
      for (auto const &[i, expectedTriangle] : enumerate(expectedTriangles)) {
        MeshClipper::Triangle const &gotTriangle = meshClipper.m_triangles[i];
        bool const fullyEqual = gotTriangle.fullEquals(expectedTriangle);
        if (not fullyEqual) {
          std::cerr << "index: " << i << " expected: " << expectedTriangle
                    << " got " << gotTriangle << '\n';
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
        // MeshClipper const meshClipper{mesh};
        //(void)meshClipper;
    } {
      // Stl const tetrahedronStl{"test-models/tetrahedron.ascii.stl"};
      // MeshClipper meshClipper{};
    }
  } catch (...) {
    return 1;
  }

  return 0;
}
