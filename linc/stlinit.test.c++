#include <linc/stl.h++>

#include <doctest/doctest.h>

TEST_CASE("small-cube-ascii.stl looks good") {
  Stl const stl{"./test-models/small-cube-ascii.stl"};
  REQUIRE(stl.m_stats.number_of_facets == 12);
}
