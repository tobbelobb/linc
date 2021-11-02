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
  Millimeter layerHeight{1.0};
  Millimeter offset{0.0};
  bool printHelp{false};
  // Configure optional command line options.
  std::stringstream usage;
  usage << "Usage:\n"
        << *argv
        << " <3d-model> <params> [-l layer-height-in-mm] "
           "[-o out-file-name] [-h|--help]\n";
  CommandLine args(usage.str());
  args.addArgument({"-l"}, &layerHeight,
                   "Defaults to 1.0. The analysis will search for collisions "
                   "at z-heights this far apart.");
  args.addArgument(
      {"-m"}, &offset,
      "Defaults to 0.0. Units: millimeter. Positive values create a horizontal "
      "margin around the object where lines are not allowed to enter. If you "
      "want to detect \"near collisions\" then set this parameter to a few "
      "millimeters. If you wan to ignore shallow collisions, then set this "
      "parameter to negative a few millimeters. Note that in an eventual "
      "debug-stl, you will see your anchor points moved closer to the origin "
      "by the specified margin, and the mover will float outside of the print "
      "layer by the specified margin. Line will cross through the "
      "debug-object.");
  args.addArgument(
      {"-o"}, &outFileName,
      "A debug-stl with this name is created upon collision detection. "
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

  if (layerHeight < 0.0_mm) {
    std::cerr << "Negative layer height is not allowed: " << layerHeight
              << '\n';
    return -1;
  }

  Stl const stl{modelFileName};
  if (not stl.m_initialized) {
    std::cerr << "Failed to load " << modelFileName << '\n';
    return -1;
  }
  Mesh const meshClipper{stl};

  if (not validateParamsFile(paramsFileName)) {
    std::cerr << "Validation of " << paramsFileName << " failed\n";
    return -1;
  }
  Pivots const pivots{paramsFileName, offset};

  Collision const found{
      willCollide(meshClipper, pivots, layerHeight, true, offset)};
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
