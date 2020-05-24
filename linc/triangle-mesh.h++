#pragma once

#include <linc/stl.h++>
#include <string>

struct TriangleMesh {
private:
  Stl m_stl;

public:
  TriangleMesh() = delete;

  TriangleMesh(std::string fileName) : m_stl(fileName) {}

  static bool writeBinaryStl(std::string const &fileName);
  bool isGood() const { return m_stl.m_initialized; }
};
