#include <iostream>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

TEST_CASE("Testy test case") {
  REQUIRE((1 + 1) == 2);
}
