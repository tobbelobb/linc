#include <iostream>

#include <linc/test-framework.h++>

#include <linc/mesh.h++>

auto main() -> int {
  try {
    {
      // Double check that we got the VertexCompare right
      std::set<Vertex, VertexCompare> setOfVertices{Vertex{0, 0, 0}};
      constexpr double eps{1e-4};
      // All these vertices should be considered equal to {0, 0, 0}
      // clang-format off
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
      // clang-format on

      for (auto const &small : smalls) {
        auto [iterator, insertionTookPlace] = setOfVertices.insert(small);
        check(not insertionTookPlace);
      }
      // All these vertices should be considered different
      // from {0,0,0} and from each other
      constexpr double eps2{eps + 1e-16};
      // clang-format off
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
      Mesh const mesh(Stl{getPath("test-models/tetrahedron.ascii.stl")});

      // Vertices
      compare(mesh.m_vertices.size(), 4U);
      compare(mesh.m_edges.size(), 6U);
    }
    {
      Mesh const mesh(
          Stl{getPath("test-models/broken/standing-triangle.ascii.stl")});
      //
      // Vertices
      compare(mesh.m_vertices.size(), 3U);
      std::vector<Vertex> expectedVertices{{0, -5, 0}, {0, 5, 0}, {0, 0, 10}};
      compare(mesh.m_vertices, expectedVertices);

      // Edges
      compare(mesh.m_edges.size(), 3U);
      std::vector<Edge> expectedEdges{{expectedVertices, {0, 1}, {0}},
                                      {expectedVertices, {1, 2}, {0}},
                                      {expectedVertices, {2, 0}, {0}}};
      compare(mesh.m_edges, expectedEdges);

      // Triangles
      std::vector<Triangle> expectedTriangles{
          Triangle{expectedEdges, {0, 1, 2}}};
      compare(mesh.m_triangles, expectedTriangles);
    }
  } catch (...) {
    return 1;
  }
  return 0;
}
