#include <vector>

#include <linc/model.h++>

Model Model::cut(SI::milli_metre_t<double> absoluteZ) {
  TriangleMeshCutter cutter{m_mesh};

  std::vector<TriangleMeshCutter::IntersectionLine> upper_lines, lower_lines;
  for (size_t facetIdx = 0; facetIdx < m_mesh.m_stl.m_stats.number_of_facets;
       ++facetIdx) {
    Stl::Facet const &facet = m_mesh.m_stl.m_facets[facetIdx];
    // find facet extents
    TriangleMeshCutter::Range const zRange{
        .min = std::min(facet.vertex[0].z(),
                        std::min(facet.vertex[1].z(), facet.vertex[2].z())),
        .max = std::max(facet.vertex[0].z(),
                        std::max(facet.vertex[1].z(), facet.vertex[2].z()))};

    TriangleMeshCutter::FacetMeta facetMeta{facet, facetIdx, zRange};

    cutter.reorderVertices(static_cast<float>(absoluteZ.raw_value()),
                           facetMeta);

    // intersect facet with cutting plane
    TriangleMeshCutter::IntersectionLine const line =
        cutter.sliceFacet(absoluteZ, facetMeta);
    if (line.isFilled) {
      // Save intersection lines for generating correct triangulations.
      if (line.edgeType == TriangleMeshCutter::FacetEdgeType::feTop) {
        lower_lines.emplace_back(line);
      } else if (line.edgeType == TriangleMeshCutter::FacetEdgeType::feBottom) {
        upper_lines.emplace_back(line);
      } else if (line.edgeType !=
                 TriangleMeshCutter::FacetEdgeType::feHorizontal) {
        lower_lines.emplace_back(line);
        upper_lines.emplace_back(line);
      }
    }
  }
  // New mesh is created when?
  return {m_mesh};
}
