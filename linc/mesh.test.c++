#include <iostream>

#include <linc/test-framework.h++>

#include <linc/mesh.h++>

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
      constexpr double eps2{eps + 1e-16};
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
      // Test operator== for Mesh::Edge
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
      constexpr auto eps = VertexConstants::eps;
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
    {
      // Test operator== for Mesh::Triangle
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

      constexpr auto eps = VertexConstants::eps;
      constexpr double eps2{eps + 1e-16};
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
          {0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}};
      compare(mesh.m_vertices, expectedVertices);

      compare(mesh.m_edges.size(), 6U);
      // All vertices are connected in a tetrahedron
      // Never mind the m_users lists. They are not
      // considered by the operator==.
      std::vector<Mesh::Edge> expectedEdges{
          {expectedVertices, {0, 1}}, {expectedVertices, {0, 2}},
          {expectedVertices, {0, 3}}, {expectedVertices, {1, 2}},
          {expectedVertices, {1, 3}}, {expectedVertices, {2, 3}}};
      // We expect the constructor to have sorted the edges
      std::sort(expectedEdges.begin(), expectedEdges.end());

      compare(mesh.m_edges, expectedEdges);

      compare(mesh.m_triangles.size(), 4U);
      // Constructing these triangles manually from edges is tricky
      std::vector<Mesh::Triangle> expectedTriangles{{expectedEdges, {0, 1, 3}},
                                                    {expectedEdges, {0, 2, 4}},
                                                    {expectedEdges, {1, 2, 5}},
                                                    {expectedEdges, {3, 4, 5}}};
      // We expect the constructor to have sorted the triangles
      std::sort(expectedTriangles.begin(), expectedTriangles.end());
      compare(mesh.m_triangles, expectedTriangles);
    }
    {
      Mesh const mesh(
          Stl{getPath("test-models/broken/standing-triangle.ascii.stl")});

      // Vertices
      compare(mesh.m_vertices.size(), 3U);
      std::vector<Vertex> expectedVertices{{0, -5, 0}, {0, 0, 10}, {0, 5, 0}};
      std::sort(expectedVertices.begin(), expectedVertices.end());
      compare(mesh.m_vertices, expectedVertices);

      // Edges
      compare(mesh.m_edges.size(), 3U);
      std::vector<Mesh::Edge> expectedEdges{{expectedVertices, {0, 1}, {0}},
                                            {expectedVertices, {0, 2}, {0}},
                                            {expectedVertices, {1, 2}, {0}}};
      std::sort(expectedEdges.begin(), expectedEdges.end());
      compare(mesh.m_edges, expectedEdges);

      // Triangles
      compare(mesh.m_triangles.size(), 1U);
      std::vector<Mesh::Triangle> const expectedTriangles{
          {expectedEdges, {0, 1, 2}}};
      compare(mesh.m_triangles, expectedTriangles);
    }
    {
      Mesh const mesh(Stl{getPath(
          "test-models/broken/standing-triangle-close-point.ascii.stl")});
      compare(mesh.m_vertices.size(), 3U);
      compare(mesh.m_edges.size(), 3U);
      compare(mesh.m_triangles.size(), 1U);
    }
  } catch (...) {
    return 1;
  }
  return 0;
}
