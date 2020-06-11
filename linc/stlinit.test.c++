#include <iostream>

#include <linc/stl.h++>

#define WRONG_NUMBER_OF_FACETS(x)                                              \
  std::cerr << __LINE__ << ": Wrong number of facets: " << (x) << '\n';        \
  return 1

#define WRONG_STL_TYPE(x)                                                      \
  std::cerr << __LINE__ << ": Wrong stl type: " << (x) << '\n';                \
  return 1

static std::string getPath(std::string const &newFile) {
  std::string const thisFile{__FILE__};
  return thisFile.substr(0, thisFile.find("stlinit.test.c++")) + newFile;
}

auto main() -> int {
  {
    Stl const stl{getPath("test-models/small-cube-ascii.stl")};

    if (not(stl.m_stats.number_of_facets == 12)) {
      WRONG_NUMBER_OF_FACETS(stl.m_stats.number_of_facets);
    }

    if (not(stl.m_stats.type == Stl::Type::ASCII)) {
      WRONG_STL_TYPE(stl.m_stats.type);
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
  }

  {
    Stl const stl{getPath("test-models/3DBenchy.stl")};

    if (not(stl.m_stats.number_of_facets == 225706)) {
      WRONG_NUMBER_OF_FACETS(stl.m_stats.number_of_facets);
    }

    if (not(stl.m_stats.type == Stl::Type::BINARY)) {
      WRONG_STL_TYPE(stl.m_stats.type);
    }
  }

  return 0;
}
