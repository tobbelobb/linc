#include <linc/util.h++>

std::vector<std::size_t> binarySearchSequence(size_t begin, size_t end) {
  std::vector<std::size_t> indices{};
  if (end < begin) {
    return indices;
  }
  indices.reserve(end - begin + 1);
  std::vector<std::array<std::size_t, 2>> intervals{};
  intervals.emplace_back(std::array<std::size_t, 2>{begin, end});
  while (not intervals.empty()) {
    auto const intervalsCopy{intervals};
    intervals.clear();
    for (auto &interval : intervalsCopy) {
      auto const start = interval[0];
      auto const stop = interval[1];
      if (start == stop) {
        indices.emplace_back(start);
      } else {
        auto const newMid{start + (stop - start) / 2};
        indices.emplace_back(newMid);
        if (newMid != start) {
          intervals.emplace_back(std::array<std::size_t, 2>{start, newMid - 1});
          intervals.emplace_back(std::array<std::size_t, 2>{newMid + 1, stop});
        } else {
          intervals.emplace_back(std::array<std::size_t, 2>{stop, stop});
        }
      }
    }
  }
  return indices;
}
