#include <iostream>
#include <random>

#include <linc/linc.h++>
#include <linc/mesh-clipper.h++>
#include <linc/test-framework.h++>
#include <linc/units.h++>
#include <linc/vertex.h++>

auto main() -> int {
  try {
    {
      std::vector<Vertex> vertices{{1, -2, 0}, {-2, -0.1, 0}, {1, 2, 0}};
      std::vector<Vertex> const sortedCcw{{-2, -0.1, 0}, {1, -2, 0}, {1, 2, 0}};
      sortCcwInPlace(vertices);
      compare(sortedCcw, vertices);
    }
    {
      std::vector<Vertex> const vertices{
          {1, -2, 0}, {-2, -0.1, 0}, {1, 2, 0}, {-0.1, -0.1, 0}};
      std::vector<Vertex> const hullOfThem{
          {-2, -0.1, 0}, {1, -2, 0}, {1, 2, 0}};
      compare(hullOfThem, hullAndSortCcw(vertices));
    }
    {
      std::vector<Vertex> const hullOfThem{
          {-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0}};
      std::vector<Vertex> vertices;
      vertices.reserve(10004);
      vertices.emplace_back(hullOfThem[0]);
      vertices.emplace_back(hullOfThem[1]);
      vertices.emplace_back(hullOfThem[2]);
      vertices.emplace_back(hullOfThem[3]);

      std::random_device rd;
      std::mt19937 gen(rd());
      // Generate numbers in open interval (-1.0, 1.0)
      std::uniform_real_distribution<> dis(std::nexttoward(-1.0, 0), 1.0);
      for (size_t i{0}; i < 10000; ++i) {
        vertices.emplace_back(dis(gen), dis(gen), dis(gen));
      }
      auto const res = hullAndSortCcw(vertices);

      compare(res.size(), 4U);
      compare(hullOfThem, res);
    }
    {
      // Next representable values from -1 towards 0 and from 1 towards 0
      // Our code is not numerically stable enough to sort away 16 points along
      // the perimeter of the convex hull reliably. Only inner points are
      // reliably sorted out
      double const minusIn = std::nexttoward(-1.0, 0.0);
      double const plusIn = std::nexttoward(1.0, 0.0);
      std::vector<Vertex> const vertices{
          {-1, -1, 0},        {1, -1, 0},         {1, 1, 0},
          {-1, 1, 0},         {-0.9, minusIn, 0}, {-0.8, minusIn, 0},
          {-0.7, minusIn, 0}, {plusIn, -0.9, 0},  {plusIn, -0.8, 0},
          {plusIn, -0.7, 0},  {-0.9, plusIn, 0},  {-0.8, plusIn, 0},
          {-0.7, plusIn, 0},  {minusIn, 0.9, 0},  {minusIn, 0.8, 0},
          {minusIn, 0.7, 0}};
      std::vector<Vertex> const hullOfThem{
          {-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0}};
      auto const res = hullAndSortCcw(vertices);

      compare(res.size(), 4U);
      compare(hullOfThem, res);
    }
    {
      // Test Triangle constructor from MeshClipper types
      std::vector<MeshClipper::Point> points{
          {{-2, 0, 0}, {0, -1, 0}, {0, 1, 0}}};
      std::vector<MeshClipper::Edge> edges{
          {points, {0, 1}}, {points, {1, 2}}, {points, {2, 0}}};
      MeshClipper::Triangle triangle{edges, {0, 1, 2}};

      Triangle const t0{triangle};
      compare(t0.m_normal, Normal{0, 0, 1});
      // This test is maybe a bit fragile.
      // The Triangle constructor should use the first three vertices that it
      // can find, so edge0's 0'th vertex, then edge0's 1'st vertex, then
      // edge1's 0'th or 1'st vertex in that order.
      for (auto const &[i, corner] : enumerate(t0.m_corners)) {
        Vertex const &vertex = points[i].m_vertex;
        compare(corner, vertex);
      }

      triangle.m_normal = Normal{20.0, 1.1, -2.0};
      // The triangle constructor should not check if the normal make sense
      // If it finds one, it should just use it
      Triangle const t1{triangle};
      compare(t1.m_normal, Normal{20.0, 1.1, -2.0});
    }
    {
      // Test Triangle constructor from Mesh types
      std::vector<Vertex> vertices{
          {Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      std::vector<Mesh::Edge> edges{
          {vertices, {0, 1}}, {vertices, {1, 2}}, {vertices, {2, 0}}};
      Mesh::Triangle triangle{edges, {0, 1, 2}};

      Triangle const t0{triangle};
      compare(t0.m_normal, Normal{0, 0, 1});
      compare(t0.m_corners, vertices);

      triangle.m_normal = Normal{20.0, 1.1, -2.0};
      Triangle const t1{triangle};
      compare(t1.m_normal, Normal{20.0, 1.1, -2.0});
    }
    // Triangle intersection detection
    {
      // One edge of t0 goes straight through t1 and vice versa
      // "Chained" configuration not 90 degrees
      Triangle const t0{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t1{
          {Vertex{-1, -0.1, 1}, Vertex{-1, 0.1, -1}, Vertex{1, 0.1, 0}}};

      compare(t0.m_normal, Normal{0, 0, 1});
      compare(t1.m_normal, Normal{0.049690, -0.993808, -0.099381});

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
      // Two edges of t1 does not go through middle of t0
      // no "Dunk", but one point of t1 is on plane of t0. no 90 degrees
      Triangle const t0{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t1{
          {Vertex{-2, 0.1, 3}, Vertex{0, -0.1, 3}, Vertex{-1, -0.1, 0}}};
      check(not intersect(t0, t1));
    }
    {
      // Two edges of t1 does not go through middle of t0
      // no "Dunk", no touch. no 90 degrees
      Triangle const t0{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t1{
          {Vertex{-2, 0.1, 4}, Vertex{0, -0.1, 4}, Vertex{-1, -0.1, 1}}};
      check(not intersect(t0, t1));
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
    {
      Triangle const t0{{Vertex{134.0, -134.0, 268.0},
                         Vertex{134.0, -134.0, 0.0},
                         Vertex{134.0, 134.0, 0.0}}};
      Triangle const t1{{Vertex{220.0, -1744.5, 15.9},
                         Vertex{354.0, -6.0, 398.0},
                         Vertex{86.0, -6.0, 398.0}}};
      compare(t0.m_normal, Normal{1, 0, 0});
      compare(t1.m_normal, Normal{0.0, -0.214664, 0.976688});

      check(not intersect(t0, t1));
    }
    // Real world collision detections examples for cube prints
    {
      Mesh const mesh{Stl{getPath("test-models/small-cube.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(mesh, pivots, 1.0_mm));
    }
    {
      Mesh const mesh{Stl{getPath("test-models/cube-100.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(mesh, pivots, 10.0_mm));
    }
    {
      Mesh const mesh{Stl{getPath("test-models/cube-268.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(mesh, pivots, 10.0_mm));
    }
    {
      Mesh const mesh{Stl{getPath("test-models/cube-468.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(mesh, pivots, 10.0_mm));
    }
    {
      Mesh const mesh{Stl{getPath("test-models/cube-469.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(mesh, pivots, 10.0_mm));
    }
    {
      Mesh const mesh{Stl{getPath("test-models/cube-470.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(mesh, pivots, 10.0_mm));
    }
    {
      Mesh const mesh{Stl{getPath("test-models/cube-471.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(willCollide(mesh, pivots, 10.0_mm));
    }
    {
      Mesh const mesh{Stl{getPath(
          "test-models/towards-anchors-star-800-300-twisted-30.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(willCollide(mesh, pivots, 10.0_mm, true));
    }
    {
      Mesh const mesh{Stl{getPath(
          "test-models/towards-anchors-star-800-300-twisted-30.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(mesh, pivots, 10.0_mm, false));
    }
  } catch (...) {
    return 1;
  }
  return 0;
}
