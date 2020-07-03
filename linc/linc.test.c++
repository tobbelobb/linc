#include <iostream>

#include <linc/linc.h++>
#include <linc/test-framework.h++>
#include <linc/units.h++>

auto main() -> int {
  try {
    {
      Mesh const mesh{Stl{getPath("test-models/small-cube.ascii.stl")}};
      Pivots pivots{getPath("params-example")};
      check(not willCollide(mesh, pivots, 0.3_mm));
    }
  } catch (...) {
    return 1;
  }
  return 0;
}
