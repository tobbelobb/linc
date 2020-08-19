#include <iostream>
#include <linc/test-framework.h++>
#include <linc/util.h++>
#include <vector>

auto main() -> int {
  try {
    {
      auto const indices = binarySearchSequence(1, 1);
      std::vector<std::size_t> expected{1};
      compare(indices.size(), expected.size());
      for (auto const &[i, index] : enumerate(indices)) {
        compare(index, expected[i]);
      }
    }
    {
      auto const indices = binarySearchSequence(1, 2);
      std::vector<std::size_t> expected{1, 2};
      compare(indices.size(), expected.size());
      for (auto const &[i, index] : enumerate(indices)) {
        compare(index, expected[i]);
      }
    }
    {
      auto const indices = binarySearchSequence(1, 3);
      std::vector<std::size_t> expected{2, 1, 3};
      if (indices.size() != expected.size()) {
        for (auto const &index : indices) {
          std::cout << index << ", ";
        }
        std::cout << '\n';
      }
      compare(indices.size(), expected.size());
      for (auto const &[i, index] : enumerate(indices)) {
        compare(index, expected[i]);
      }
    }
    {
      auto const indices = binarySearchSequence(1, 4);
      std::vector<std::size_t> expected{2, 1, 3, 4};
      compare(indices.size(), expected.size());
      for (auto const &[i, index] : enumerate(indices)) {
        compare(index, expected[i]);
      }
    }
    {
      auto const indices = binarySearchSequence(1, 5);
      std::vector<std::size_t> expected{3, 1, 4, 2, 5};
      compare(indices.size(), expected.size());
      for (auto const &[i, index] : enumerate(indices)) {
        compare(index, expected[i]);
      }
    }
    {
      auto const indices = binarySearchSequence(1, 6);
      std::vector<std::size_t> expected{3, 1, 5, 2, 4, 6};
      compare(indices.size(), expected.size());
      for (auto const &[i, index] : enumerate(indices)) {
        compare(index, expected[i]);
      }
    }
    {
      auto const indices = binarySearchSequence(1, 7);
      std::vector<std::size_t> expected{4, 2, 6, 1, 3, 5, 7};
      compare(indices.size(), expected.size());
      for (auto const &[i, index] : enumerate(indices)) {
        compare(index, expected[i]);
      }
    }
  } catch (...) {
    return 1;
  }
  return 0;
}
