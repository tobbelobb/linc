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
      Stl const stl{getPath("test-models/broken/standing-triangle.ascii.stl")};
      Mesh const mesh(stl);

      // Vertices
      compare(mesh.m_vertices.size(), 3U);

      std::set<Vertex, VertexCompare> const expectedVertices{
          Vertex{0, -5, 0}, Vertex{0, 5, 0}, Vertex{0, 0, 10}};
      compare(mesh.m_vertices, expectedVertices);

      std::set<Triangle, TriangleCompare> expectedTriangles{};
      std::set<SuperEdge, EdgeCompare> expectedSuperEdges{};
      expectedSuperEdges.insert(
          SuperEdge{{expectedVertices.find(Vertex{0, -5, 0}),
                     expectedVertices.find(Vertex{0, 5, 0})}});
      expectedSuperEdges.insert(
          SuperEdge{{expectedVertices.find(Vertex{0, 5, 0}),
                     expectedVertices.find(Vertex{0, 0, 10})}});
      expectedSuperEdges.insert(
          SuperEdge{{expectedVertices.find(Vertex{0, 0, 10}),
                     expectedVertices.find(Vertex{0, -5, 0})}});

      compare(mesh.m_superEdges.size(), 3U);
      compare(mesh.m_superEdges, expectedSuperEdges);

      std::set<SuperEdge, EdgeCompare>::iterator it1{expectedSuperEdges.find(
          SuperEdge{{expectedVertices.find(Vertex{0, -5, 0}),
                     expectedVertices.find(Vertex{0, 5, 0})}})};
      std::set<SuperEdge, EdgeCompare>::iterator it2{expectedSuperEdges.find(
          SuperEdge{{expectedVertices.find(Vertex{0, 5, 0}),
                     expectedVertices.find(Vertex{0, 0, 10})}})};
      std::set<SuperEdge, EdgeCompare>::iterator it3{expectedSuperEdges.find(
          SuperEdge{{expectedVertices.find(Vertex{0, 0, 10}),
                     expectedVertices.find(Vertex{0, -5, 0})}})};
      TriangleEdgeIterators trit{it1, it2, it3};

      Triangle triangle{trit};
      expectedTriangles.insert(triangle);

      compare(mesh.m_triangles.size(), 1U);
      compare(mesh.m_triangles, expectedTriangles);

      for (const auto &superEdge : mesh.m_superEdges) {

        std::cerr << "Looking into edge: " << superEdge;
        // std::cerr << "\nIt has user: " << *superEdge.m_users[0];

        // check(std::find(superEdge.m_users.begin(), superEdge.m_users.end(),
        //                mesh.m_triangles.begin()) != superEdge.m_users.end());
      }
    }
  } catch (...) {
    return 1;
  }
  return 0;
}
