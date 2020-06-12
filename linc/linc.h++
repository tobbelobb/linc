#pragma once
#include <SI/length.h>

#include <linc/params.h++>
#include <linc/triangle-mesh.h++>

[[nodiscard]] bool willCollide(TriangleMesh const &, Pivots const &,
                               SI::milli_metre_t<double> const &);

