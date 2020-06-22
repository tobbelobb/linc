#pragma once
#include <SI/length.h>

#include <linc/params.h++>
#include <linc/mesh.h++>

[[nodiscard]] bool willCollide(Mesh const &, Pivots const &, Millimeter const &);
