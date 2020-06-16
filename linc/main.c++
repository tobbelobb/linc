#include <iostream>
#include <set>

#include <SI/length.h>
#include <gsl/span_ext>

#include <linc/linc.h++>
#include <linc/params.h++>
#include <linc/triangle-mesh.h++>

using namespace SI::literals;

auto main(int argc, char *argv[]) -> int {

  // Parse the arguments...
  if (not(std::set<int>{{3, 4}}.contains(argc))) {
    std::cerr << "Usage:\n"
              << *argv << " <3d-model> <params> [layer-height (mm)]\n";
    return 1;
  }
  gsl::span<char *> const args(argv, static_cast<unsigned int>(argc));
  auto *const modelFileName = gsl::at(args, 1);
  auto *const paramsFileName = gsl::at(args, 2);

  auto const layerHeight =
      Millimeter((argc > 3) ? std::stod(gsl::at(args, 3)) : 1.0);

  TriangleMesh mesh{modelFileName};
  if (not mesh.isGood()) {
    std::cerr << "Failed to load " << modelFileName << '\n';
    return 1;
  }

  if (not validateParamsFile(paramsFileName)) {
    std::cerr << "Validation of " << paramsFileName << " failed\n";
    return 1;
  }
  Pivots pivots{paramsFileName};

  if (willCollide(mesh, pivots, layerHeight)) {
    std::cout << "Collision detected\n";
  } else {
    std::cout << "No collision detected\n";
  }

  return 0;
}
