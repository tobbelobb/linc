#pragma once
#include <linc/units.h++>

#include <linc/mesh.h++>
#include <linc/params.h++>

[[nodiscard]] bool willCollide(Mesh const &, Pivots const &, Millimeter);
