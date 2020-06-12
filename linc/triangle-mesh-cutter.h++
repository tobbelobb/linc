#pragma once

#include <vector>

#include <SI/length.h>

#include <linc/triangle-mesh.h++>

class TriangleMeshCutter {
private:
  TriangleMesh m_mesh;
  std::vector<int> m_facetsEdges;
  std::vector<Vertex> m_vertexScaledShared;

  struct IntersectionPoint {};

  enum class FacetSliceType { NoSlice, Slicing, Cutting };

public:
  enum class FacetEdgeType { feGeneral, feTop, feBottom, feHorizontal };

  struct Range {
    float min = 0.0F;
    float max = 0.0F;
  };

  struct IntersectionLine {
    bool isFilled = false;
    FacetEdgeType edgeType = FacetEdgeType::feHorizontal;
  };

  struct FacetMeta {
    Stl::Facet facet;
    size_t idx;
    Range z;
    IntersectionLine intersectionLine{};

    FacetMeta() = delete;
    explicit FacetMeta(Stl::Facet facet_in, size_t idx_in, Range z_in)
        : facet(facet_in), idx(idx_in), z(z_in) {}
  };

  TriangleMeshCutter(TriangleMesh mesh) : m_mesh(mesh) {}

  // Reorder vertices so that the first one is the one with lowest Z.
  // This is needed to get all intersection lines in a consistent order
  // (external on the right of the line)
  void reorderVertices(float sliceZ, FacetMeta &facetMeta);

  TriangleMeshCutter::IntersectionLine
  sliceFacet(SI::milli_metre_t<double> const &absoluteZ, FacetMeta &facetMeta);
};
