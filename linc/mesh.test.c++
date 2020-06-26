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
    // Test operator==(Mesh::Edge const&, Mesh::Edge const&) implementation
    {
      std::vector<Vertex> exampleVertices{{0, 0, 0}, {0, 1, 0}};
      std::vector<Vertex> exampleVertices2{{0, 0, 0}, {0, 1, 0}};
      // Edges have no direction
      compare(Mesh::Edge{exampleVertices, {0, 1}},
              Mesh::Edge{exampleVertices, {1, 0}});
      // Edges are compared by vertex value, even if underlying vertex vectors
      // are different
      compare(Mesh::Edge{exampleVertices, {0, 1}},
              Mesh::Edge{exampleVertices2, {0, 1}});
      // Limit of inequality should be between eps and eps2
      constexpr double eps{1e-4};
      constexpr double eps2{eps + 1e-16};
      std::vector<Vertex> exampleVerticesEps{{eps, eps, eps}, {eps, 1, eps}};
      std::vector<Vertex> exampleVerticesEps2{{eps2, eps2, eps2},
                                              {eps2, 1, eps2}};
      compare(Mesh::Edge{exampleVertices, {0, 1}},
              Mesh::Edge{exampleVerticesEps, {0, 1}});
      check(Mesh::Edge{exampleVertices, {0, 1}} !=
            Mesh::Edge{exampleVerticesEps2, {0, 1}});
      compare(Mesh::Edge{exampleVerticesEps, {0, 1}},
              Mesh::Edge{exampleVerticesEps2, {0, 1}});
    }
    // Test operator==(Mesh::Triangle const&, Mesh::Triangle const&)
    {
      std::vector<Vertex> exampleVertices{{0, 0, 0}, {0, 1, 0}, {1, 0, 0}};
      std::vector<Vertex> exampleVertices2{{0, 0, 0}, {0, 1, 0}, {1, 0, 0}};
      std::vector<Mesh::Edge> exampleEdges{{exampleVertices, {0, 1}},
                                           {exampleVertices, {1, 2}},
                                           {exampleVertices, {2, 0}}};
      std::vector<Mesh::Edge> exampleEdges2{{exampleVertices2, {0, 1}},
                                            {exampleVertices, {1, 2}},
                                            {exampleVertices, {2, 0}}};
      Mesh::Triangle triangle{exampleEdges, {0, 1, 2}};
      Mesh::Triangle const &itself = triangle;
      check(triangle == itself);

      Mesh::Triangle triangle2{exampleEdges2, {1, 0, 2}};
      // Equality should not depend on order of edges
      check(triangle == triangle2);

      constexpr double eps{1e-4};
      constexpr double eps2{1e-4 + 1e-16};
      std::vector<Vertex> exampleVerticesEps{
          {eps, eps, eps}, {eps, 1, eps}, {1, eps, eps}};
      std::vector<Vertex> exampleVerticesEps2{
          {eps2, eps2, eps2}, {eps2, 1, eps2}, {1, eps2, eps2}};
      std::vector<Mesh::Edge> exampleEdgesEps{{exampleVerticesEps, {0, 1}},
                                              {exampleVerticesEps, {1, 2}},
                                              {exampleVerticesEps, {2, 0}}};
      std::vector<Mesh::Edge> exampleEdgesEps2{{exampleVerticesEps2, {0, 1}},
                                               {exampleVerticesEps2, {1, 2}},
                                               {exampleVerticesEps2, {2, 0}}};
      Mesh::Triangle triangleEps{exampleEdgesEps, {0, 1, 2}};
      Mesh::Triangle triangleEps2{exampleEdgesEps2, {0, 1, 2}};
      check(triangle == triangleEps);
      check(triangle != triangleEps2);
      check(triangleEps == triangleEps2);
    }
    {
      Mesh const mesh(Stl{getPath("test-models/tetrahedron.ascii.stl")});

      compare(mesh.m_vertices.size(), 4U);
      std::vector<Vertex> expectedVertices{
          {0, 0, 0}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}};
      compare(mesh.m_vertices, expectedVertices);

      compare(mesh.m_edges.size(), 6U);
      std::vector<Mesh::Edge> expectedEdges{{expectedVertices, {0, 1}, {0, 2}},
                                            {expectedVertices, {1, 2}, {0, 3}},
                                            {expectedVertices, {2, 0}, {0, 1}},
                                            {expectedVertices, {2, 3}, {1, 3}},
                                            {expectedVertices, {3, 0}, {1, 2}},
                                            {expectedVertices, {3, 1}, {2, 3}}};
      compare(mesh.m_edges, expectedEdges);

      compare(mesh.m_triangles.size(), 4U);
      std::vector<Mesh::Triangle> const expectedTriangles{
          {expectedEdges, {0, 1, 2}},
          {expectedEdges, {2, 3, 4}},
          {expectedEdges, {4, 5, 0}},
          {expectedEdges, {3, 1, 5}}};
      compare(mesh.m_triangles, expectedTriangles);
    }
    {
      Mesh const mesh(
          Stl{getPath("test-models/broken/standing-triangle.ascii.stl")});

      // Vertices
      compare(mesh.m_vertices.size(), 3U);
      std::vector<Vertex> expectedVertices{{0, -5, 0}, {0, 5, 0}, {0, 0, 10}};
      compare(mesh.m_vertices, expectedVertices);

      // Edges
      compare(mesh.m_edges.size(), 3U);
      std::vector<Mesh::Edge> expectedEdges{{expectedVertices, {0, 1}, {0}},
                                            {expectedVertices, {1, 2}, {0}},
                                            {expectedVertices, {2, 0}, {0}}};
      compare(mesh.m_edges, expectedEdges);

      // Triangles
      compare(mesh.m_triangles.size(), 1U);
      std::vector<Mesh::Triangle> const expectedTriangles{
          {expectedEdges, {0, 1, 2}}};
      compare(mesh.m_triangles, expectedTriangles);
    }
  } catch (...) {
    return 1;
  }
  return 0;
}
