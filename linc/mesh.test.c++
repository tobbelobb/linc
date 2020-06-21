#include <iostream>

#include <linc/test-framework.h++>

#include <linc/mesh.h++>

using namespace mesh;

auto main() -> int {
  try {
    {
      // Double check that we got the VertexCompare right
      std::set<Vertex, VertexCompare> setOfVertices{Vertex{0, 0, 0}};
      // clang-format off
      constexpr double eps{1e-4};
      // All these vertices should be considered equal
      std::vector<Vertex> smalls {
        Vertex{-eps, -eps, -eps},
        Vertex{   0, -eps, -eps},
        Vertex{ eps, -eps, -eps},
        Vertex{-eps,    0, -eps},
        Vertex{   0,    0, -eps},
        Vertex{ eps,    0, -eps},
        Vertex{-eps,  eps, -eps},
        Vertex{   0,  eps, -eps},
        Vertex{ eps,  eps, -eps},
        Vertex{-eps, -eps,    0},
        Vertex{   0, -eps,    0},
        Vertex{ eps, -eps,    0},
        Vertex{-eps,    0,    0},
        Vertex{   0,    0,    0},
        Vertex{ eps,    0,    0},
        Vertex{-eps,  eps,    0},
        Vertex{   0,  eps,    0},
        Vertex{ eps,  eps,    0},
        Vertex{-eps, -eps,  eps},
        Vertex{   0, -eps,  eps},
        Vertex{ eps, -eps,  eps},
        Vertex{-eps,    0,  eps},
        Vertex{   0,    0,  eps},
        Vertex{ eps,    0,  eps},
        Vertex{-eps,  eps,  eps},
        Vertex{   0,  eps,  eps},
        Vertex{ eps,  eps,  eps}
      };

      for (auto const &small : smalls) {
        auto [iterator, insertionTookPlace] = setOfVertices.insert(small);
        check(not insertionTookPlace);
      }
      // All these vertices should be considered different
      // from {0,0,0} and from each other
      constexpr double eps2{eps + 1e-16};
      std::vector<Vertex> bigs {
        Vertex{-eps2, -eps2, -eps2},
        Vertex{    0, -eps2, -eps2},
        Vertex{ eps2, -eps2, -eps2},
        Vertex{-eps2,     0, -eps2},
        Vertex{    0,     0, -eps2},
        Vertex{ eps2,     0, -eps2},
        Vertex{-eps2,  eps2, -eps2},
        Vertex{    0,  eps2, -eps2},
        Vertex{ eps2,  eps2, -eps2},
        Vertex{-eps2, -eps2,     0},
        Vertex{    0, -eps2,     0},
        Vertex{ eps2, -eps2,     0},
        Vertex{-eps2,     0,     0},
        Vertex{ eps2,     0,     0},
        Vertex{-eps2,  eps2,     0},
        Vertex{    0,  eps2,     0},
        Vertex{ eps2,  eps2,     0},
        Vertex{-eps2, -eps2,  eps2},
        Vertex{    0, -eps2,  eps2},
        Vertex{ eps2, -eps2,  eps2},
        Vertex{-eps2,     0,  eps2},
        Vertex{    0,     0,  eps2},
        Vertex{ eps2,     0,  eps2},
        Vertex{-eps2,  eps2,  eps2},
        Vertex{    0,  eps2,  eps2},
        Vertex{ eps2,  eps2,  eps2}
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
      Stl const stl{getPath("test-models/broken/standing-triangle.ascii.stl")};
      Mesh const mesh(stl);

      // Vertices
      compare(mesh.m_vertices.size(), 3U);
      compare(mesh.m_vertices,
              std::set<Vertex, VertexCompare>{Vertex{0, -5, 0}, Vertex{0, 5, 0},
                                              Vertex{0, 0, 10}});

      compare(mesh.m_edges.size(), 3U);
      std::cout << mesh.m_edges;
    }
  } catch (...) {
    return 1;
  }
  return 0;
}
