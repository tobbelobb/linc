#include <iostream>

#include <SI/length.h>

#include <linc/mesh-clipper.h++>
#include <linc/stl.h++>
#include <linc/test-framework.h++>

using namespace SI::literals;

auto main() -> int {
  // auto constexpr maxRelativeError = 0.0000001F;
  try {
    {
      Stl const stl{getPath("test-models/small-cube.ascii.stl")};
      Stl const shorter = cut(stl, 5.0_mm);
      // compare(stl.m_facets.size(), 12U);
      // compare(stl.m_type, Stl::Type::ASCII);
      // check(stl.m_stats.size.isApprox(Vertex{10, 10, 10}, maxRelativeError));
      // check(stl.m_initialized);
    }
  } catch (...) {
    return 1;
  }

  return 0;
}
