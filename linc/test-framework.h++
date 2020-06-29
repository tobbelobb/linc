#pragma once
#include <iostream>

#include <experimental/source_location>
#include <filesystem>

using SourceLoc = std::experimental::source_location;

void testPrint(SourceLoc const loc) {
  std::cerr << loc.file_name() << '(' << loc.line() << ") ";
}

auto getPath(std::string const &newFile,
             std::string const &thisFile = SourceLoc::current().file_name())
    -> std::string {
  return std::filesystem::path(thisFile).replace_filename(newFile);
}

void compare(auto const &value1, auto const &value2,
             SourceLoc const loc = SourceLoc::current()) {
  if (not(value1 == value2)) {
    testPrint(loc);
    std::cerr << value1 << " is not == to " << value2 << '\n';
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
