#pragma once
#include <SI/length.h>

#include <linc/mesh.h++>

class MeshClipper : public Mesh {
  struct Point {
    Vertex m_vertex;
    Millimeter distance;
    unsigned short occurs;
    bool visible;
  };

  struct ClipperEdge {
    Edge m_edge;
    bool visible;
  };
};

Mesh cut(Mesh const &mesh, Millimeter const zCut);
