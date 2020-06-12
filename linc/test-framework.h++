#pragma once
#include <iostream>

#include <experimental/source_location>

using SourceLoc = std::experimental::source_location;

void testPrint(SourceLoc const loc) {
  std::cerr << loc.file_name() << '(' << loc.line() << ") ";
}

void compare(auto const &value1, auto const &value2,
             SourceLoc const loc = SourceLoc::current()) {
  if (not(value1 == value2)) {
    testPrint(loc);
    std::cerr << value1 << " is not equal to " << value2 << '\n';
    throw "Not equal";
  }
}

void check(bool const x, SourceLoc const loc = SourceLoc::current()) {
  if (not x) {
    testPrint(loc);
    std::cerr << "Not true\n";
    throw "Not true";
  }
}
