#pragma once

#include <SI/length.h>

#include <linc/triangle-mesh-cutter.h++>
#include <linc/triangle-mesh.h++>

class Model {
public:
  TriangleMesh m_mesh;

  Model() = delete;
  Model(std::string fileName) : m_mesh(fileName) {}
  Model(TriangleMesh mesh) : m_mesh(mesh){};
  Model(Model const &model) : m_mesh(model.m_mesh){};

  Model cut(SI::milli_metre_t<double> absoluteZ);

  bool isGood() const { return m_mesh.isGood(); }
};
