#include <linc/mesh.h++>

// Given a z height, return a copy of the model that is cut
// at that height
auto cut(Mesh const &mesh, Millimeter const zCut) -> Mesh {
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
  return mesh;
  // return Stl{facets, neighbors, stats, true, Type::INMEMORY};
}
