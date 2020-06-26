#include <iostream>

#include <SI/length.h>

#include <linc/mesh-clipper.h++>
#include <linc/test-framework.h++>

using namespace SI::literals;

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
      check(point == withDistance);
      MeshClipper::Point const zeroDistance{Vertex{0, 0, 0}, 0.0};
      compare(point, zeroDistance);
      MeshClipper::Point const zeroDistanceOccurs{Vertex{0, 0, 0}, 0.0, 1};
      check(point == zeroDistanceOccurs);
      MeshClipper::Point const zeroDistanceZeroOccurs{Vertex{0, 0, 0}, 0.0, 0};
      compare(point, zeroDistanceZeroOccurs);
      MeshClipper::Point const zeroDistanceZeroOccursInvisible{Vertex{0, 0, 0},
                                                               0.0, 0, false};
      check(point == zeroDistanceZeroOccursInvisible);
      MeshClipper::Point const zeroDistanceZeroOccursVisible{Vertex{0, 0, 0},
                                                             0.0, 0, true};
      compare(point, zeroDistanceZeroOccursVisible);
    }
    {
      //std::vector<MeshClipper::Point> examplePoints{
      //    MeshClipper::Point{{0, 0, 0}}, MeshClipper::Point{{1, 0, 0}}};
      //MeshClipper::Edge const forwards{examplePoints, {0, 1}};
      //MeshClipper::Edge const backwards{examplePoints, {1, 0}};
      //compare(forwards, backwards);
      //check(not(forwards != backwards));
    }
    {
      MeshClipper const meshClipper{
          Mesh{Stl{getPath("test-models/small-cube.ascii.stl")}}};
    }
  } catch (...) {
    return 1;
  }

  return 0;
}
