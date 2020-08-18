#include <iostream>
#include <set>

#include <gsl/span_ext>

#include <linc/command-line.h++>
#include <linc/linc.h++>
#include <linc/params.h++>
#include <linc/stl.h++>
#include <linc/units.h++>

auto main(int argc, char *argv[]) -> int {
  // These optional variables can be set via the command line.
  std::string outFileName{};
  Millimeter layerHeightMax{1.0};
  bool printHelp{false};
  // Configure optional command line options.
  std::stringstream usage;
  usage << "Usage:\n"
        << *argv
        << " <3d-model> <params> [-l layer-height-max-in-mm] "
           "[-o out-file-name] [-h|--help]\n";
  CommandLine args(usage.str());
  args.addArgument({"-l"}, &layerHeightMax,
                   "Defaults to 1.0. The analysis will cut each layer in half "
                   "until all layers are thinner than this layer height, or "
                   "until a collision is detected in one of the layers.");
  args.addArgument(
      {"-o"}, &outFileName,
      "A debug stl with this name is created upon collision detection."
      "This stl is meant for visual inspection in another program. If no "
      "-o file name is specified, no debug-stl is generated.");
  args.addArgument({"-h", "--help"}, &printHelp, "Print this help.");
  constexpr int MAX_ARGC{7};
  constexpr int MIN_ARGC{3};
  printHelp = (argc < MIN_ARGC or argc > MAX_ARGC);
  if (printHelp) {
    args.printHelp();
    return 0;
  }
  // Parse mandatory arguments
  gsl::span<char *> const mandatoryArgs(&argv[1], static_cast<unsigned int>(2));
  auto *const modelFileName = gsl::at(mandatoryArgs, 0);
  auto *const paramsFileName = gsl::at(mandatoryArgs, 1);

  // Parse optional arguments
  try {
    gsl::span<char *> const optionalArgs(&argv[3],
                                         static_cast<unsigned int>(argc - 3));
    args.parse(optionalArgs);
  } catch (std::runtime_error const &e) {
    std::cerr << e.what() << std::endl;
    return -1;
  }

  if (printHelp) {
    args.printHelp();
    return 0;
  }

  if (layerHeightMax < 0.0_mm) {
    std::cerr << "Negative layer height is not allowed: " << layerHeightMax
              << '\n';
    return -1;
  }

  Stl const stl{modelFileName};
  if (not stl.m_initialized) {
    std::cerr << "Failed to load " << modelFileName << '\n';
    return -1;
  }
  std::vector<Vertex> v{};
  std::vector<Mesh::Edge> e{};
  std::vector<Mesh::Triangle> t{};
  Mesh const meshClipper{stl, v, e, t};

  if (not validateParamsFile(paramsFileName)) {
    std::cerr << "Validation of " << paramsFileName << " failed\n";
    return -1;
  }
  Pivots const pivots{paramsFileName};

  Collision const found{willCollide(meshClipper, pivots, layerHeightMax)};
  if (found) {
    std::cout << "Collision detected at z=" << found.m_height << '\n';
    if (not outFileName.empty()) {
      makeDebugModel(meshClipper, pivots, found, outFileName);
      std::cout << "Wrote " << outFileName << '\n';
    }
    return 1;
  }

  std::cout << "No collision detected\n";
  return 0;
}
