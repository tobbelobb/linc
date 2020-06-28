#pragma once

using Millimeter = double;

constexpr Millimeter operator"" _mm(long double mm) {
  return static_cast<double>(mm);
}
