#include <algorithm>
#include <iomanip>

#include <linc/command-line.h++>

CommandLine::CommandLine(std::string description)
    : mDescription(std::move(description)) {}

void CommandLine::addArgument(std::vector<std::string> const &flags,
                              Value const &value, std::string const &help) {
  mArguments.emplace_back(Argument{flags, value, help});
}

void CommandLine::printHelp(std::ostream &os) const {

  // Print the general description.
  os << mDescription << std::endl;

  // Find the argument with the longest combined flag length (in order
  // to align the help messages).

  std::size_t maxFlagLength = 0;

  for (auto const &argument : mArguments) {
    std::size_t flagLength = 0;
    for (auto const &flag : argument.mFlags) {
      // Plus comma and space.
      flagLength += flag.size() + 2;
    }

    maxFlagLength = std::max(maxFlagLength, flagLength);
  }

  // Now print each argument.
  for (auto const &argument : mArguments) {

    std::string flags;
    for (auto const &flag : argument.mFlags) {
      flags += flag + ", ";
    }

    // Remove last comma and space and add padding according to the
    // longest flags in order to align the help messages.
    std::stringstream sstr;
    sstr << std::left << std::setw(static_cast<int>(maxFlagLength))
         << flags.substr(0, flags.size() - 2);

    // Print the help for each argument. This is a bit more involved
    // since we do line wrapping for long descriptions.
    std::size_t spacePos = 0;
    std::size_t lineWidth = 0;
    while (spacePos != std::string::npos) {
      std::size_t nextspacePos =
          argument.mHelp.find_first_of(' ', spacePos + 1);
      sstr << argument.mHelp.substr(spacePos, nextspacePos - spacePos);
      lineWidth += nextspacePos - spacePos;
      spacePos = nextspacePos;

      constexpr std::size_t MAX_LINE_WIDTH{60};
      if (lineWidth > MAX_LINE_WIDTH) {
        os << sstr.str() << std::endl;
        sstr = std::stringstream();
        sstr << std::left << std::setw(static_cast<int>(maxFlagLength - 1))
             << " ";
        lineWidth = 0;
      }
    }
  }
}

void CommandLine::parse(gsl::span<char *> args) const {

  // Skip the first argument (name of the program).
  gsl::index i{0};
  while (i < static_cast<gsl::index>(args.size())) {
    // First we have to identify wether the value is separated by a space
    // or a '='.
    std::string flag{gsl::at(args, i)};
    std::string value;
    bool valueIsSeparate = false;

    // If there is an '=' in the flag, the part after the '=' is actually
    // the value.
    std::size_t equalPos = flag.find('=');
    if (equalPos != std::string::npos) {
      value = flag.substr(equalPos + 1);
      flag = flag.substr(0, equalPos);
    }
    // Else the following argument is the value.
    else if ((i + 1) < static_cast<gsl::index>(args.size())) {
      value = gsl::at(args, i + 1);
      valueIsSeparate = true;
    }

    // Search for an argument with the provided flag.
    bool foundArgument = false;

    for (auto const &argument : mArguments) {
      if (std::find(argument.mFlags.begin(), argument.mFlags.end(), flag) !=
          std::end(argument.mFlags)) {

        foundArgument = true;

        // In the case of booleans, there must not be a value present.
        // So if the value is neither 'true' nor 'false' it is considered
        // to be the next argument.
        if (std::holds_alternative<bool *>(argument.mValue)) {
          if (!value.empty() && value != "true" && value != "false") {
            valueIsSeparate = false;
          }
          *std::get<bool *>(argument.mValue) = (value != "false");
        }
        // In all other cases there must be a value.
        else if (value.empty()) {
          throw std::runtime_error("Failed to parse command line arguments: "
                                   "Missing value for argument \"" +
                                   flag + "\"!");
        }
        // For a std::string, we take the entire value.
        else if (std::holds_alternative<std::string *>(argument.mValue)) {
          *std::get<std::string *>(argument.mValue) = value;
        }
        // In all other cases we use a std::stringstream to
        // convert the value.
        else {
          std::visit(
              [&value](auto &&arg) {
                std::stringstream sstr(value);
                sstr >> *arg;
              },
              argument.mValue);
        }

        break;
      }
    }

    // Print a warning if there was an unknown argument.
    if (not foundArgument) {
      std::cerr << "Ignoring unknown command line argument \"" << flag << "\"."
                << std::endl;
    }

    // Advance to the next flag.
    ++i;

    // If the value was separated, we have to advance our index once more.
    if (foundArgument && valueIsSeparate) {
      ++i;
    }
  }
}
