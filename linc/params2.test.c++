#include <iostream>

#include <doctest/doctest.h>

#include <linc/params.h++>

auto tokenizeWellFormedString() -> int {
  std::string const inString{"effector-pivot-A1: ( 220,  -140.0, 130.0 ) "};
  PivotTokens const got = tokenize(inString);

  PivotTokens const expected{"effector-pivot-A1", "220", "-140.0", "130.0"};
  if (not(got == expected)) {
    std::cout << "Expected: " << expected << '\n';
    std::cout << "Got:      " << got << '\n';
    return 1;
  }
  return 0;
}

auto tokenizeTooManyTokens() -> int {
  std::string const inString{"effector-pivot-A1: 220, -140.0, 130.0, 123.4"};
  PivotTokens const got = tokenize(inString);
  PivotTokens const expected{"effector-pivot-A1", "220", "-140.0", "130.0"};
  if (not(got == expected)) {
    std::cout << "Expected: " << expected << '\n';
    std::cout << "Got:      " << got << '\n';
    return 1;
  }
  return 0;
}

TEST_CASE("Tokenize well formed string") {
  REQUIRE(tokenizeWellFormedString() == 0);
}

TEST_CASE("Tokenize too many tokens") { REQUIRE(tokenizeTooManyTokens() == 0); }
