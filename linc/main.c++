#include <iostream>
#include <set>

#include <gsl/span_ext>

#include <linc/linc.h++>
#include <linc/params.h++>
#include <linc/stl.h++>
#include <linc/units.h++>

auto main(int argc, char *argv[]) -> int {

  // Parse the arguments...
  if (not(argc == 3 or argc == 4)) {
    std::cerr << "Usage:\n"
              << *argv << " <3d-model> <params> [layer-height (mm)]\n";
    return -1;
  }
  gsl::span<char *> const args(argv, static_cast<unsigned int>(argc));
  auto *const modelFileName = gsl::at(args, 1);
  auto *const paramsFileName = gsl::at(args, 2);

  auto const layerHeight =
      Millimeter((argc > 3) ? std::stod(gsl::at(args, 3)) : 1.0);
  if (layerHeight < 0.0) {
    std::cerr << "Negative layer height is not allowed: " << layerHeight
              << '\n';
    return -1;
  }

  Stl const stl{modelFileName};
  if (not stl.m_initialized) {
    std::cerr << "Failed to load " << modelFileName << '\n';
    return -1;
  }
  Mesh const mesh{stl};

  if (not validateParamsFile(paramsFileName)) {
    std::cerr << "Validation of " << paramsFileName << " failed\n";
    return -1;
  }
  Pivots pivots{paramsFileName};

  Collision const found{willCollide(mesh, pivots, layerHeight)};
  if (found.m_isCollision) {
    std::cout << "Collision detected at z=" << found.m_height << '\n';
    return 1;
  }

  std::cout << "No collision detected\n";
  return 0;
}
