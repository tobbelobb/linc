#include <iostream>

#include <linc/params.h++>

auto tokenizeWellFormedString() -> int {
  std::string const inString{"effector-pivot-A1: ( 220,  -140.0, 130.0 ) "};
  PivotTokens const got = tokenize(inString);

  PivotTokens const expected{"effector-pivot-A1", "220", "-140.0", "130.0"};
  if (not(got == expected)) {
    std::cerr << "Expected: " << expected << '\n';
    std::cerr << "Got:      " << got << '\n';
    return 1;
  }
  return 0;
}

auto tokenizeTooManyTokens() -> int {
  std::string const inString{"effector-pivot-A1: 220, -140.0, 130.0, 123.4"};
  PivotTokens const got = tokenize(inString);
  PivotTokens const expected{"effector-pivot-A1", "220", "-140.0", "130.0"};
  if (not(got == expected)) {
    std::cerr << "Expected: " << expected << '\n';
    std::cerr << "Got:      " << got << '\n';
    return 1;
  }
  return 0;
}

auto main() -> int {

  if (not(tokenizeWellFormedString() == 0))
    return 1;

  if (not(tokenizeTooManyTokens() == 0))
    return 1;

  return 0;
}
