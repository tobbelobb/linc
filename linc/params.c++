#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <locale>
#include <set>
#include <sstream>
#include <string>

#include <linc/params.h++>

// remove all ' ', '\n', '\r', '\f', '\t', '\v'
// trim from start (in place)
static inline void lTrimWhitespace(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return std::isspace(ch) == 0;
          }));
}

// remove all non-number
// trim from start and end (in place)
static inline void lrTrimNonNumber(std::string &s) {
  auto isNumber = [](unsigned char ch) {
    return (std::isdigit(ch) != 0) or ch == '-' or ch == '.';
  };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), isNumber));
  s.erase(std::find_if(s.rbegin(), s.rend(), isNumber).base(), s.end());
}

static inline auto toDouble(std::string const &s) -> Millimeter {
  std::size_t charCount = 0;
  double res = std::stod(s, &charCount);
  if (s.size() != charCount) {
    throw std::invalid_argument("Could not parse all characters");
  }
  return res;
}

auto tokenize(std::string const &line) -> PivotTokens {
  PivotTokens tokens{};
  std::stringstream ss(line);
  std::getline(ss, tokens.name, ':');
  std::getline(ss, tokens.x, ',');
  std::getline(ss, tokens.y, ',');
  std::getline(ss, tokens.z, ',');

  lrTrimNonNumber(tokens.x);
  lrTrimNonNumber(tokens.y);
  lrTrimNonNumber(tokens.z);

  return tokens;
}

static auto pivotNameToColumn(std::string const &name) -> Pivots::ColumnIndex {
  std::string const lastTwo = name.substr(name.size() - 2);
  if (lastTwo == "A1") {
    return Pivots::ColumnIndex::A1;
  }
  if (lastTwo == "A2") {
    return Pivots::ColumnIndex::A2;
  }
  if (lastTwo == "B1") {
    return Pivots::ColumnIndex::B1;
  }
  if (lastTwo == "B2") {
    return Pivots::ColumnIndex::B2;
  }
  if (lastTwo == "C1") {
    return Pivots::ColumnIndex::C1;
  }
  if (lastTwo == "C2") {
    return Pivots::ColumnIndex::C2;
  }
  return Pivots::ColumnIndex::INVALID;
}

static auto valid(PivotTokens const &tokens) -> bool {
  if (tokens.hasEmptyMember()) {
    return false;
  }
  if (not tokens.isEffector() and not tokens.isAnchor()) {
    std::cerr
        << "Invalid pivot name does not start with 'effector' or 'anchor': "
        << tokens.name << '\n';
    return false;
  }
  if (pivotNameToColumn(tokens.name) == Pivots::ColumnIndex::INVALID) {
    std::cerr << "Invalid pivot name does not end with 'A1', 'A2', 'B1', 'B2', "
                 "'C1', or 'C2': "
              << tokens.name << '\n';
    return false;
  }
  for (auto const &token : {tokens.x, tokens.y, tokens.z}) {
    try {
      double const tmp = toDouble(token);
      (void)tmp;
    } catch (std::exception const &e) {
      std::cerr << "Could not convert to number: " << token << '\n';
      std::cerr << e.what() << '\n';
      return false;
    }
  }
  return true;
}

Pivots::Pivots(std::string const &fileName, double const offset) {
  std::ifstream fileStream{fileName};

  // line by line validation
  for (std::string line; std::getline(fileStream, line);) {
    lTrimWhitespace(line);
    if (line.empty() or line[0] == '#') {
      continue;
    }
    save(tokenize(line));
  }
  if (std::abs(offset) > VertexConstants::eps) {
    Vertex const A =
        (anchors[Pivots::ColumnIndex::A1] + anchors[Pivots::ColumnIndex::A2]) /
        2;
    Vertex const B =
        (anchors[Pivots::ColumnIndex::B1] + anchors[Pivots::ColumnIndex::B2]) /
        2;
    Vertex const C =
        (anchors[Pivots::ColumnIndex::C1] + anchors[Pivots::ColumnIndex::C2]) /
        2;
    Vertex const A_dir = A / A.norm();
    Vertex const B_dir = B / B.norm();
    Vertex const C_dir = C / C.norm();
    anchors[Pivots::ColumnIndex::A1] =
        anchors[Pivots::ColumnIndex::A1] - offset * A_dir;
    anchors[Pivots::ColumnIndex::A2] =
        anchors[Pivots::ColumnIndex::A2] - offset * A_dir;
    anchors[Pivots::ColumnIndex::B1] =
        anchors[Pivots::ColumnIndex::B1] - offset * B_dir;
    anchors[Pivots::ColumnIndex::B2] =
        anchors[Pivots::ColumnIndex::B2] - offset * B_dir;
    anchors[Pivots::ColumnIndex::C1] =
        anchors[Pivots::ColumnIndex::C1] - offset * C_dir;
    anchors[Pivots::ColumnIndex::C2] =
        anchors[Pivots::ColumnIndex::C2] - offset * C_dir;
  }
}

void Pivots::save(PivotTokens const &tokens) {
  auto const col = pivotNameToColumn(tokens.name);
  if (tokens.isEffector()) {
    effector.at(col).x() = toDouble(tokens.x);
    effector.at(col).y() = toDouble(tokens.y);
    effector.at(col).z() = toDouble(tokens.z);
  }
  if (tokens.isAnchor()) {
    anchors.at(col).x() = toDouble(tokens.x);
    anchors.at(col).y() = toDouble(tokens.y);
    anchors.at(col).z() = toDouble(tokens.z);
  }
}

static auto validateFileAsAWhole(std::string const &fileName) -> bool {
  std::ifstream fileStream{fileName};
  if (fileStream.fail()) {
    std::cerr << "Failed to open " << fileName << '\n';
    return false;
  }
  std::set<std::string> const neededPivotNames = {
      "effector-pivot-A1", "effector-pivot-A2", "effector-pivot-B1",
      "effector-pivot-B2", "effector-pivot-C1", "effector-pivot-C2",
      "anchor-pivot-A1",   "anchor-pivot-A2",   "anchor-pivot-B1",
      "anchor-pivot-B2",   "anchor-pivot-C1",   "anchor-pivot-C2",
  };
  auto const lengthOfLongestNeededPivotName =
      (*std::max_element(neededPivotNames.begin(), neededPivotNames.end(),
                         [](std::string const &s, std::string const &s2) {
                           return s.size() < s2.size();
                         }))
          .size();
  auto remainingPivotNames = neededPivotNames;
  for (std::string line; std::getline(fileStream, line);) {
    lTrimWhitespace(line);
    if (line.empty() or line[0] == '#') {
      continue;
    }
    for (auto const &neededName : neededPivotNames) {
      if (line.substr(0, lengthOfLongestNeededPivotName).find(neededName) !=
          std::string::npos) {
        if (remainingPivotNames.find(neededName) == remainingPivotNames.end()) {
          std::cerr << "Found " << neededName << " twice in " << fileName
                    << '\n';
          return false;
        }
        remainingPivotNames.erase(neededName);
      }
    }
  }
  if (not remainingPivotNames.empty()) {
    for (auto const &pivotName : remainingPivotNames) {
      std::cerr << "Did not find " << pivotName << " in " << fileName << '\n';
    }
    return false;
  }
  return true;
}

// Look at each non-empty non-comment line and see
// if we have valid tokens there
static auto validateLineByLine(std::string const &fileName) -> bool {
  std::ifstream fileStream{fileName};
  for (std::string line; std::getline(fileStream, line);) {
    if (line.empty() or line[0] == '#') {
      continue;
    }

    PivotTokens const tokens{tokenize(line)};
    if (not valid(tokens)) {
      std::cerr << "Invalid line:\n" << line << '\n';
      return false;
    }
  }
  return true;
}

auto validateParamsFile(std::string const &fileName) -> bool {
  bool valid = true;
  if (not validateFileAsAWhole(fileName)) {
    valid = false;
  }
  if (not validateLineByLine(fileName)) {
    valid = false;
  }
  return valid;
}
