#include <iostream>

#include <linc/params.h++>
#include <linc/test-framework.h++>

static auto tokenizeWellFormedString() -> bool {
  std::string const inString{"effector-pivot-A1: ( 220,  -140.0, 130.0 ) "};
  PivotTokens const got = tokenize(inString);

  PivotTokens const expected{"effector-pivot-A1", "220", "-140.0", "130.0"};
  if (not(got == expected)) {
    std::cerr << "Expected: " << expected << '\n';
    std::cerr << "Got:      " << got << '\n';
    return false;
  }
  return true;
}

static auto tokenizeTooManyTokens() -> bool {
  std::string const inString{"effector-pivot-A1: 220, -140.0, 130.0, 123.4"};
  PivotTokens const got = tokenize(inString);
  PivotTokens const expected{"effector-pivot-A1", "220", "-140.0", "130.0"};
  if (not(got == expected)) {
    std::cerr << "Expected: " << expected << '\n';
    std::cerr << "Got:      " << got << '\n';
    return false;
  }
  return true;
}

auto main() -> int {
  try {
    check(tokenizeWellFormedString());
    check(tokenizeTooManyTokens());
  } catch (...) {
    return 1;
  }
  return 0;
}
