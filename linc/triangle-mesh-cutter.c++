#include <linc/triangle-mesh-cutter.h++>

void TriangleMeshCutter::reorderVertices(float sliceZ, FacetMeta &facetMeta) {
  (void)sliceZ;
  (void)facetMeta;
  /*
  TriangleVertexIndices const &vertices =
      m_mesh.m_indexedTriangleSet.indices[facetMeta.idx];
  auto const minVertexIdx =
      (facet.vertex[1].z() == facetMeta.z.min)
          ? 1
          : ((facet.vertex[2].z() == facetMeta.z.min) ? 2 : 0);

  for (int j = minVertexIdx; j - minVertexIdx < 3;
       ++j) { // loop through facet edges
    auto const edgeId = m_facetsEdges[facetMeta.idx * 3 + (j % 3)];
    auto const aId = vertices[j % 3];
    auto const bId = vertices[(j + 1) % 3];

    Vertex const &a = m_vScaledShared[a_id];
    Vertex const &b = m_vScaledShared[b_id];

    // Is edge or face aligned with the cutting plane?
    if (a.z() == sliceZ and b->z() == sliceZ) {
      // Edge is horizontal and belongs to the current layer.
      // The following rotation of the three vertices may not be efficient, but
      // this branch happens rarely.
      Vertex const &v0 = this->v_scaled_shared[vertices[0]];
      Vertex const &v1 = this->v_scaled_shared[vertices[1]];
      Vertex const &v2 = this->v_scaled_shared[vertices[2]];
      Normal const &normal = facet.normal;
      if (min_z == max_z) {
        // All three vertices are aligned with slice_z.
        line_out.edgeType = feHorizontal;
        result = Cutting;
        if (normal.z() < 0) {
          // If normal points downwards this is a bottom horizontal facet so we
          // reverse its point order.
          std::swap(a, b);
          std::swap(a_id, b_id);
        }
      } else {
        // Two vertices are aligned with the cutting plane, the third vertex is
        // below or above the cutting plane. Is the third vertex below the
        // cutting plane?
        bool third_below =
            v0.z() < slice_z || v1.z() < slice_z || v2.z() < slice_z;
        // Two vertices on the cutting plane, the third vertex is below the
        // plane. Consider the edge to be part of the slice only if it is the
        // upper edge. (the bottom most edge resp. vertex of a triangle is not
        // owned by the triangle, but the top most edge resp. vertex is part of
        // the triangle in respect to the cutting plane).
        result = third_below ? Slicing : Cutting;
        if (third_below) {
          line_out->edge_type = feTop;
          std::swap(a, b);
          std::swap(a_id, b_id);
        } else
          line_out->edge_type = feBottom;
      }
      line_out->a.x() = a->x();
      line_out->a.y() = a->y();
      line_out->b.x() = b->x();
      line_out->b.y() = b->y();
      line_out->a_id = a_id;
      line_out->b_id = b_id;
      assert(line_out->a != line_out->b);
      return result;
    }

    if (a->z() == slice_z) {
      // Only point a alings with the cutting plane.
      if (point_on_layer == size_t(-1) ||
          points[point_on_layer].point_id != a_id) {
        point_on_layer = num_points;
        IntersectionPoint &point = points[num_points++];
        point.x() = a->x();
        point.y() = a->y();
        point.point_id = a_id;
      }
    } else if (b->z() == slice_z) {
      // Only point b alings with the cutting plane.
      if (point_on_layer == size_t(-1) ||
          points[point_on_layer].point_id != b_id) {
        point_on_layer = num_points;
        IntersectionPoint &point = points[num_points++];
        point.x() = b->x();
        point.y() = b->y();
        point.point_id = b_id;
      }
    } else if ((a->z() < slice_z && b->z() > slice_z) ||
               (b->z() < slice_z && a->z() > slice_z)) {
      // A general case. The face edge intersects the cutting plane. Calculate
      // the intersection point.
      assert(a_id != b_id);
      // Sort the edge to give a consistent answer.
      if (a_id > b_id) {
        std::swap(a_id, b_id);
        std::swap(a, b);
      }
      IntersectionPoint &point = points[num_points];
      double t = (double(slice_z) - double(b->z())) /
                 (double(a->z()) - double(b->z()));
      if (t <= 0.) {
        if (point_on_layer == size_t(-1) ||
            points[point_on_layer].point_id != a_id) {
          point.x() = a->x();
          point.y() = a->y();
          point_on_layer = num_points++;
          point.point_id = a_id;
        }
      } else if (t >= 1.) {
        if (point_on_layer == size_t(-1) ||
            points[point_on_layer].point_id != b_id) {
          point.x() = b->x();
          point.y() = b->y();
          point_on_layer = num_points++;
          point.point_id = b_id;
        }
      } else {
        point.x() = coord_t(floor(double(b->x()) +
                                  (double(a->x()) - double(b->x())) * t + 0.5));
        point.y() = coord_t(floor(double(b->y()) +
                                  (double(a->y()) - double(b->y())) * t + 0.5));
        point.edge_id = edge_id;
        ++num_points;
      }
    }
  }
*/
}

// Return true, if the facet has been sliced and line_out has been filled.
TriangleMeshCutter::IntersectionLine
TriangleMeshCutter::sliceFacet(SI::milli_metre_t<double> const &absoluteZ,
                               FacetMeta &facetMeta) {
  (void)absoluteZ;
  (void)facetMeta;
  /*

  IntersectionPoint points[3];
  size_t num_points = 0;
  size_t point_on_layer = size_t(-1);

  // Facets must intersect each plane 0 or 2 times, or it may touch the plane at
  // a single vertex only.
  assert(num_points < 3);
  if (num_points == 2) {
    line_out->edge_type = feGeneral;
    line_out->a = (Point)points[1];
    line_out->b = (Point)points[0];
    line_out->a_id = points[1].point_id;
    line_out->b_id = points[0].point_id;
    line_out->edge_a_id = points[1].edge_id;
    line_out->edge_b_id = points[0].edge_id;

    // The plane cuts at least one edge in a general position.
    assert(line_out->a_id == -1 || line_out->b_id == -1);
    assert(line_out->edge_a_id != -1 || line_out->edge_b_id != -1);
    facetMeta.intersectionLine.isFilled = true;
  }
  */
  return facetMeta.intersectionLine;
}
