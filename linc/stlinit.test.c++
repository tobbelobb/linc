#include <iostream>

#include <experimental/source_location>

#include <linc/stl.h++>

using SourceLoc = std::experimental::source_location;

template <typename T> constexpr auto WRONG_NUMBER_OF_FACETS2(T x) {
  std::cerr << __LINE__ << ": Wrong number of facets: " << (x) << '\n';
  return 1;
}

#define WRONG_NUMBER_OF_FACETS(x)                                              \
  std::cerr << __LINE__ << ": Wrong number of facets: " << (x) << '\n';        \
  return 1

#define WRONG_STL_TYPE(x)                                                      \
  std::cerr << __LINE__ << ": Wrong stl type: " << (x) << '\n';                \
  return 1

#define WRONG_SIZE(x)                                                          \
  std::cerr << __LINE__ << ": Wrong size: " << (x) << '\n';                    \
  return 1

#define WRONG_SHORTEST_EDGE(x)                                                 \
  std::cerr << __LINE__ << ": Wrong shortest edge: " << (x) << '\n';           \
  return 1

static auto getPath(std::string const &newFile) -> std::string {
  std::string const thisFile{__FILE__};
  return thisFile.substr(0, thisFile.find("stlinit.test.c++")) + newFile;
}

auto main() -> int {
  {
    Stl const stl{getPath("test-models/small-cube-ascii.stl")};

    if (not(stl.m_stats.number_of_facets == 13)) {
      WRONG_NUMBER_OF_FACETS2(stl.m_stats.number_of_facets);
    }

    if (not(stl.m_stats.type == Stl::Type::ASCII)) {
      WRONG_STL_TYPE(stl.m_stats.type);
    }

    if (not stl.m_stats.size.isApprox(Vertex{10, 10, 10})) {
      WRONG_SIZE(stl.m_stats.size);
    }
  }

  {
    Stl const stl{getPath("test-models/small-cube-binary.stl")};

    if (not(stl.m_stats.number_of_facets == 12)) {
      WRONG_NUMBER_OF_FACETS(stl.m_stats.number_of_facets);
    }

    if (not(stl.m_stats.type == Stl::Type::BINARY)) {
      WRONG_STL_TYPE(stl.m_stats.type);
    }

    if (not stl.m_stats.size.isApprox(Vertex{10, 10, 10})) {
      WRONG_SIZE(stl.m_stats.size);
    }
  }

  {
    Stl const stl{getPath("test-models/3DBenchy.stl")};

    if (not(stl.m_stats.number_of_facets == 225706)) {
      WRONG_NUMBER_OF_FACETS(stl.m_stats.number_of_facets);
    }

    if (not(stl.m_stats.type == Stl::Type::BINARY)) {
      WRONG_STL_TYPE(stl.m_stats.type);
    }

    if (not stl.m_stats.size.isApprox(Vertex{60, 31, 48},
                                      0.0001F)) { // Allow +-0.0001%
      WRONG_SIZE(stl.m_stats.size);
    }

    if (stl.m_stats.shortest_edge > 0.07401F or
        stl.m_stats.shortest_edge < 0.07399F) {
      WRONG_SHORTEST_EDGE(stl.m_stats.shortest_edge);
    }
  }

  return 0;
}
