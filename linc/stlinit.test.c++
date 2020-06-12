#include <iostream>

#include <linc/test-framework.h++>

#include <linc/stl.h++>

static auto
getPath(std::string const &newFile,
        std::string const &thisFile = SourceLoc::current().file_name())
    -> std::string {
  return thisFile.substr(0, thisFile.find("stlinit.test.c++")) + newFile;
}

auto main() -> int {
  auto constexpr allowedMisPrecisionPercentage = 0.0001F;
  try {
    {
      Stl const stl{getPath("test-models/small-cube-ascii.stl")};
      compare(stl.m_stats.number_of_facets, 12U);
      compare(stl.m_stats.type, Stl::Type::ASCII);
      check(stl.m_stats.size.isApprox(Vertex{10, 10, 10},
                                      allowedMisPrecisionPercentage));
    }
    {
      Stl const stl{getPath("test-models/small-cube-binary.stl")};
      compare(stl.m_stats.number_of_facets, 12U);
      compare(stl.m_stats.type, Stl::Type::BINARY);
      check(stl.m_stats.size.isApprox(Vertex{10, 10, 10},
                                      allowedMisPrecisionPercentage));
    }
    {
      Stl const stl{getPath("test-models/3DBenchy.stl")};
      compare(stl.m_stats.number_of_facets, 225706U);
      compare(stl.m_stats.type, Stl::Type::BINARY);
      check(stl.m_stats.size.isApprox(Vertex{60, 31, 48},
                                      allowedMisPrecisionPercentage));
      check(stl.m_stats.shortest_edge < 0.07401F and
            stl.m_stats.shortest_edge > 0.07399F);
    }
  } catch (...) {
    return 1;
  }

  return 0;
}
