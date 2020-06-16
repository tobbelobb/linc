#include <linc/stl.h++>

// Given a z height, return a copy of the model that is cut
// at that height
auto Stl::cut(Millimeter const zCut) const -> Stl {
  (void)zCut;
  //
  //  size_t facetsToRemove = 0;
  //  for (auto const &facet : m_facets) {
  //    std::vector<double
  //    facetsAbove = facet.vertices[0].z() > zCut
  //    if (
  //    if ( and facet.vertices[1].z() > zCut and
  //        facet.vertices[2].z() > zCut) {
  //      ++facetsToRemove;
  //    }
  //
  //    facet.edgeFromBelow(z);
  //  }
  //
  return Stl{"test-models/small-cube.ascii.stl"};
  // return Stl{facets, neighbors, stats, true, Type::INMEMORY};
}
