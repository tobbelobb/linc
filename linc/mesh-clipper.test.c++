#include <iostream>

#include <SI/length.h>

#include <linc/mesh-clipper.h++>
#include <linc/test-framework.h++>

using namespace SI::literals;

auto main() -> int {
  // auto constexpr maxRelativeError = 0.0000001F;
  try {
    {
      Mesh const mesh{Stl{getPath("test-models/small-cube.ascii.stl")}};
      Mesh const shorter = cut(mesh, 5.0_mm);
    }
  } catch (...) {
    return 1;
  }

  return 0;
}
