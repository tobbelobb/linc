#pragma once
#include <SI/length.h>

#include <linc/params.h++>
#include <linc/stl.h++>

[[nodiscard]] bool willCollide(Stl const &, Pivots const &, Millimeter const &);
