#include <iostream>
#include <random>

#include <linc/linc.h++>
#include <linc/mesh.h++>
#include <linc/test-framework.h++>
#include <linc/units.h++>
#include <linc/vertex.h++>

auto main() -> int {
  try {
    {
      std::vector<Vertex> vertices{{1, -2, 0}, {-2, -0.1_mm, 0}, {1, 2, 0}};
      std::vector<Vertex> const sortedCcw{
          {-2, -0.1_mm, 0}, {1, -2, 0}, {1, 2, 0}};
      sortCcwInPlace(vertices);
      compare(sortedCcw, vertices);
    }
    {
      std::vector<Vertex> vertices{
          {1, -2, 0}, {-2, -0.1_mm, 0}, {1, 2, 0}, {-0.1_mm, -0.1_mm, 0}};
      std::vector<Vertex> const hullOfThem{
          {-2, -0.1_mm, 0}, {1, -2, 0}, {1, 2, 0}};
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
      std::uniform_real_distribution<> dis(std::nexttoward(-1.0_mm, 0), 1.0_mm);
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
      Millimeter const minusIn = std::nexttoward(-1.0_mm, 0.0);
      Millimeter const plusIn = std::nexttoward(1.0_mm, 0.0);
      std::vector<Vertex> vertices{{-1, -1, 0},
                                   {1, -1, 0},
                                   {1, 1, 0},
                                   {-1, 1, 0},
                                   {-0.9_mm, minusIn, 0},
                                   {-0.8_mm, minusIn, 0},
                                   {-0.7_mm, minusIn, 0},
                                   {plusIn, -0.9_mm, 0},
                                   {plusIn, -0.8_mm, 0},
                                   {plusIn, -0.7_mm, 0},
                                   {-0.9_mm, plusIn, 0},
                                   {-0.8_mm, plusIn, 0},
                                   {-0.7_mm, plusIn, 0},
                                   {minusIn, 0.9_mm, 0},
                                   {minusIn, 0.8_mm, 0},
                                   {minusIn, 0.7_mm, 0}};
      std::vector<Vertex> const hullOfThem{
          {-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0}};
      auto const res = hullAndSortCcw(vertices);

      compare(res.size(), 4U);
      compare(hullOfThem, res);
    }
    {
      // clang-format off
      std::vector<Vertex> vertices{
        {-8.4133226949427389_mm, -2.4653934331841514_mm,  37.002063751220703_mm},
        {-8.4124251995815591_mm, -8.1294532317023762_mm,  37.002063751220703_mm},
        {-8.3626040412103464_mm,  8.165689337317211_mm,   37.002063751220703_mm},
        {11.29540035413685_mm,    9.0778556148976577_mm,  37.002063751220703_mm},
        { 0.11786159679080921_mm,-8.5317579404585935_mm,  37.002063751220703_mm},
        {11.117633629511277_mm,  -8.9904489111814865_mm,  37.002063751220703_mm},
        {-5.4239606842457171_mm, -0.79774998946316511_mm, 37.002063751220703_mm},
        {-5.4237506708504357_mm, -0.7982242693999887_mm,  37.002063751220703_mm},
        {-4.2773683387527308_mm,  1.7918731748828649_mm,  37.002063751220703_mm},
        {-2.6968491870886022_mm, -1.1869607558383748_mm,  37.002063751220703_mm},
        {-4.276813171056979_mm,   1.7919523219831139_mm,  37.002063751220703_mm},
        {-2.6964950581817204_mm, -1.1862934687213125_mm,  37.002063751220703_mm},
        {12.79215502966948_mm,    9.1513481598733719_mm,  37.002063751220703_mm},
        {12.904533305849444_mm,  -9.0796324033022557_mm,  37.002063751220703_mm},
        {13.328262340348294_mm,   9.1872183769657063_mm,  37.002063751220703_mm},
        {13.408803374646267_mm,  -9.1090169944695027_mm,  37.002063751220703_mm},
        {13.477824978807732_mm,   9.1971278269892913_mm,  37.002063751220703_mm},
        {13.567507838067048_mm,  -9.1166511506937109_mm,  37.002063751220703_mm},
        {13.481689911091925_mm,   9.1593489695996855_mm,  37.002063751220703_mm},
        {13.571581553551676_mm,  -9.0721032888627615_mm,  37.002063751220703_mm},
        {13.473311509829887_mm,   7.1861093310879998_mm,  37.002063751220703_mm},
        {13.550536903361083_mm,  -8.7726839544549708_mm,  37.002063751220703_mm},
        {13.482861235996145_mm,   8.8877447866298738_mm,  37.002063751220703_mm}};
      // clang-format on

      std::vector<Vertex> const ccwHullOfThem{
          {-8.4133226949427389_mm, -2.4653934331841514_mm,
           37.002063751220703_mm},
          {-8.4124251995815591_mm, -8.1294532317023762_mm,
           37.002063751220703_mm},
          {0.11786159679080921_mm, -8.5317579404585935_mm,
           37.002063751220703_mm},
          {13.567507838067048_mm, -9.1166511506937109_mm,
           37.002063751220703_mm},
          {13.571581553551676_mm, -9.0721032888627615_mm,
           37.002063751220703_mm},
          {13.481689911091925_mm, 9.1593489695996855_mm, 37.002063751220703_mm},
          {13.477824978807732_mm, 9.1971278269892913_mm, 37.002063751220703_mm},
          {-8.3626040412103464_mm, 8.165689337317211_mm, 37.002063751220703_mm},
      };
      auto const res = hullAndSortCcw(vertices);

      compare(res.size(), ccwHullOfThem.size());
      compare(res, ccwHullOfThem);
    }
    {
      // Test Triangle constructor from Mesh types
      std::vector<Vertex> points{
          {Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      std::vector<Mesh::Edge> edges{{{0, 1}}, {{1, 2}}, {{2, 0}}};
      Mesh::Triangle triangle{{0, 1, 2}};

      Triangle const t0{triangle, points, edges};
      compare(Normal{t0.getNormalDirection().normalized()}, Normal{0, 0, 1});
      // This test is maybe a bit fragile.
      // The Triangle constructor should use the first three vertices that it
      // can find, so edge0's 0'th vertex, then edge0's 1'st vertex, then
      // edge1's 0'th or 1'st vertex in that order.
      for (auto const &[i, corner] : enumerate(t0.m_corners)) {
        Vertex const &vertex = points[i];
        compare(corner, vertex);
      }
    }
    // Triangle intersection detection
    {
      // One edge of t0 goes straight through t1 and vice versa
      // "Chained" configuration not 90 degrees
      Triangle const t0{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t1{{Vertex{-1, -0.1_mm, 1}, Vertex{-1, 0.1_mm, -1},
                         Vertex{1, 0.1_mm, 0}}};

      compare(Normal{t0.getNormalDirection().normalized()}, Normal{0, 0, 1});
      compare(Normal{t1.getNormalDirection().normalized()},
              Normal{0.049690_mm, -0.993808_mm, -0.099381_mm});

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
      Triangle const t1{{Vertex{-2, 0.1_mm, 1}, Vertex{0, -0.1_mm, 1},
                         Vertex{-1, -0.1_mm, -1}}};
      check(intersect(t0, t1));
    }
    {
      // Two edges of t0 goes through middle of t1
      // Same "Dunk" configuration not 90 degrees as above
      // but with t0 and t1 reversed
      Triangle const t1{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t0{{Vertex{-2, 0.1_mm, 1}, Vertex{0, -0.1_mm, 1},
                         Vertex{-1, -0.1_mm, -1}}};
      check(intersect(t0, t1));
    }
    {
      // Two edges of t1 does not go through middle of t0
      // no "Dunk", but one point of t1 is on plane of t0. no 90 degrees
      Triangle const t0{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t1{{Vertex{-2, 0.1_mm, 3}, Vertex{0, -0.1_mm, 3},
                         Vertex{-1, -0.1_mm, 0}}};
      check(not intersect(t0, t1));
    }
    {
      // Two edges of t1 does not go through middle of t0
      // no "Dunk", no touch. no 90 degrees
      Triangle const t0{{Vertex{-2, 0, 0}, Vertex{0, -1, 0}, Vertex{0, 1, 0}}};
      Triangle const t1{{Vertex{-2, 0.1_mm, 4}, Vertex{0, -0.1_mm, 4},
                         Vertex{-1, -0.1_mm, 1}}};
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
          {Vertex{-1, -0.5_mm, 0}, Vertex{1, -1.5_mm, 0}, Vertex{-2, -1, 0}}};
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
      Triangle const t0{{Vertex{134.0_mm, -134.0_mm, 268.0_mm},
                         Vertex{134.0_mm, -134.0_mm, 0.0_mm},
                         Vertex{134.0_mm, 134.0_mm, 0.0_mm}}};
      Triangle const t1{{Vertex{220.0_mm, -1744.5_mm, 15.9_mm},
                         Vertex{354.0_mm, -6.0_mm, 398.0_mm},
                         Vertex{86.0_mm, -6.0_mm, 398.0_mm}}};
      compare(Normal{t0.getNormalDirection().normalized()}, Normal{1, 0, 0});
      compare(Normal{t1.getNormalDirection().normalized()},
              Normal{0.0_mm, -0.214664_mm, 0.976688_mm});

      check(not intersect(t0, t1));
    }
    // Real world collision detections examples for cube prints
    {
      Mesh const meshClipper{Stl{getPath("test-models/small-cube.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(meshClipper, pivots, 1.0_mm));
    }
    {
      Mesh const meshClipper{Stl{getPath("test-models/cube-100.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(meshClipper, pivots, 10.0_mm));
    }
    {
      Mesh const meshClipper{Stl{getPath("test-models/cube-268.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(meshClipper, pivots, 10.0_mm));
    }
    {
      Mesh const meshClipper{Stl{getPath("test-models/cube-468.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(meshClipper, pivots, 10.0_mm));
    }
    {
      Mesh const meshClipper{Stl{getPath("test-models/cube-469.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(meshClipper, pivots, 10.0_mm));
    }
    {
      Mesh const meshClipper{Stl{getPath("test-models/cube-470.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(meshClipper, pivots, 10.0_mm));
    }
    {
      Mesh const meshClipper{Stl{getPath("test-models/cube-471.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(willCollide(meshClipper, pivots, 10.0_mm));
    }
    {
      Mesh const meshClipper{Stl{getPath(
          "test-models/towards-anchors-star-800-300-twisted-30.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(willCollide(meshClipper, pivots, 10.0_mm, true));
    }
    {
      Mesh const meshClipper{Stl{getPath(
          "test-models/towards-anchors-star-800-300-twisted-30.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(meshClipper, pivots, 10.0_mm, false));
    }
    {
      // We got core dumps before, when doing this
      Mesh const meshClipper{Stl{getPath("test-models/clips/chimney3.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(meshClipper, pivots, 1.0_mm, false));
    }
  } catch (...) {
    return 1;
  }
  return 0;
}
