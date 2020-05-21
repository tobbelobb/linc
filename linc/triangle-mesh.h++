#pragma once

#include <linc/stl.h++>
#include <string>

struct TriangleMesh {
  stl_file m_stl;

  TriangleMesh() = delete; // You can not initialize based on nothing

  TriangleMesh(std::string const fileName) { m_stl = stl_open(fileName); }

  static bool writeBinaryStl(std::string const &fileName);
  bool isGood() const { return m_stl.m_initialized; }
};
