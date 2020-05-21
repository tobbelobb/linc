#pragma once
#include <linc/params.h++>
#include <linc/triangle-mesh.h++>

[[nodiscard]] bool willCollide(TriangleMesh const &, Pivots const &, float);
