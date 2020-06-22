#include <linc/linc.h++>

auto willCollide(Mesh const &mesh, Pivots const &pivots,
                 SI::milli_metre_t<double> const &layerHeight) -> bool {
  (void)pivots;
  (void)mesh;
  (void)layerHeight;
  return false;
}
