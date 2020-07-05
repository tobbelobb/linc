#include <iostream>

#include <linc/linc.h++>
#include <linc/test-framework.h++>
#include <linc/units.h++>
#include <linc/vertex.h++>

auto main() -> int {
  try {
    {
      Mesh const mesh{Stl{getPath("test-models/small-cube.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(mesh, pivots, 0.3_mm));
    }
    // Triangle intersection detection
    {
      // Test Triangle constructor
      std::vector<Vertex> vertices{
          {Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      std::vector<Mesh::Edge> edges{
          {vertices, {0, 1}}, {vertices, {1, 2}}, {vertices, {2, 0}}};
      Mesh::Triangle triangle{edges, {0, 1, 2}};

      Triangle const t0{triangle};
      compare(t0.m_normal, Normal{0, 0, 1});
      // This test is maybe a bit fragile.
      // The Triangle constructor should use the first three vertices that it
      // can find, so edge0's 0'th vertex, then edge0's 1'st vertex, then
      // edge1's 0'th or 1'st vertex in that order.
      compare(t0.m_corners, vertices);

      triangle.m_normal = Normal{20.0, 1.1, -2.0};
      // The triangle constructor should not check if the normal make sense
      // If it finds one, it should just use it
      Triangle const t1{triangle};
      compare(t1.m_normal, Normal{20.0, 1.1, -2.0});
    }
    {
      // One edge of t0 goes straight through t1 and vice versa
      // "Chained" configuration not 90 degrees
      Triangle const t0{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t1{
          {Vertex{-1, -0.1, 1}, Vertex{-1, 0.1, -1}, Vertex{1, 0.1, 0}}};

      compare(t0.m_normal, Normal{0, 0, 1});
      compare(t1.m_normal, Normal{0.0, -0.995, -0.0995});

      check(intersect(t0, t1));
    }
    {
      // One edge of t0 goes straight through t1 and vice versa
      // "Chained" configuration 90 degrees  (normals are perpendicular)
      Triangle const t0{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t1{{Vertex{-1, 0, 1}, Vertex{-1, 0, -1}, Vertex{1, 0, 0}}};
      check(intersect(t0, t1));
    }
    {
      // Two edges of t1 goes through middle of t0
      // "Dunk" configuration not 90 degrees
      Triangle const t0{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t1{
          {Vertex{-2, 0.1, 1}, Vertex{0, -0.1, 1}, Vertex{-1, -0.1, -1}}};
      check(intersect(t0, t1));
    }
    {
      // Two edges of t0 goes through middle of t1
      // Same "Dunk" configuration not 90 degrees as above
      // but with t0 and t1 reversed
      Triangle const t1{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t0{
          {Vertex{-2, 0.1, 1}, Vertex{0, -0.1, 1}, Vertex{-1, -0.1, -1}}};
      check(intersect(t0, t1));
    }
    {
      // Two edges of t1 goes through middle of t0
      // "Dunk" configuration 90 degrees (normals are perpendicular)
      Triangle const t0{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t1{{Vertex{-2, 0, 1}, Vertex{0, 0, 1}, Vertex{-1, 0, -1}}};
      check(intersect(t0, t1));
    }
    {
      // Two edges of t0 goes through middle of t1
      // Same "Dunk" configuration 90 degrees (normals are perpendicular) as
      // above but with t0 and t1 reversed
      Triangle const t1{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t0{{Vertex{-2, 0, 1}, Vertex{0, 0, 1}, Vertex{-1, 0, -1}}};
      check(intersect(t0, t1));
    }
    {
      // Separate triangles (normals are perpendicular)
      Triangle const t0{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t1{{Vertex{1, 0, 1}, Vertex{1, 0, -1}, Vertex{3, 0, 0}}};
      check(not intersect(t0, t1));
    }
    {
      // Separate triangles in the same plane
      Triangle const t0{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t1{{Vertex{2, -1, 0}, Vertex{2, 1, 0}, Vertex{3, 0, 0}}};
      check(not intersect(t0, t1));
    }
    {
      // Separate triangles in the same plane sharing one vertex
      // but not intersecting
      Triangle const t0{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t1{
          {Vertex{-2, 0, 0}, Vertex{-3, -1, 0}, Vertex{-3, 1, 0}}};
      check(not intersect(t0, t1));
    }
    {
      // Separate triangles not in the same plane sharing one vertex
      // and not intersecting
      Triangle const t0{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t1{
          {Vertex{-2, 0, 0}, Vertex{-3, -1, 1}, Vertex{-3, 1, 2}}};
      check(not intersect(t0, t1));
    }
    {
      // Separate triangles in the same plane sharing the direction alon one
      // edge This implies that they don't intersect for our purposes
      Triangle const t0{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t1{
          {Vertex{-1, -0.5, 0}, Vertex{1, -1.5, 0}, Vertex{-2, -1, 0}}};
      check(not intersect(t0, t1));
    }
    {
      // Completely overlapping triangles
      // Since they share direction along at least one edge,
      // we regard them as not intersecting
      Triangle const t0{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t1{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      check(not intersect(t0, t1));
    }
  } catch (...) {
    return 1;
  }
  return 0;
}
