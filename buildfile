dir{.}: dir{linc/} doc{README.md} manifest

# Include headers for the headers-only eigen library
cxx.poptions =+ "-I$src_root/extern/eigen"

# Follow cpp core guidelines
cxx.poptions =+ "-I$src_root/extern/cppcore"
