dir{.}: dir{linc/} dir{extern/} doc{README.md} manifest

# Include headers for the headers-only eigen library
cxx.poptions =+ "-I$src_root/extern/eigen"

# Follow cpp core guidelines
cxx.poptions =+ "-I$src_root/extern/cppcore"

# Logging header only library
cxx.poptions =+ "-I$src_root/extern/spdlog-1.6.0/include"
