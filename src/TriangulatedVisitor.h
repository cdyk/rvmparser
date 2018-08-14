#pragma once

#include "RVMVisitor.h"

class Triangulator : public RVMVisitor
{
public:

  void init(class Store& store) override;

  void beginFile(const char* info, const char* note, const char* date, const char* user, const char* encoding) {}

  void endFile() {}

  void beginModel(const char* project, const char* name) {}

  void endModel() {}

  void beginGroup(const char* name, const float* translation, const uint32_t material) {}

  void EndGroup() {}

  void geometry(struct Geometry* geometry) override;

private:
  struct Arena * arena = nullptr;

  std::vector<float> vertices;
  std::vector<float> normals;
  std::vector<uint32_t> indices;

  std::vector<unsigned> u0;
  std::vector<float> t0;
  std::vector<float> t1;
  std::vector<float> t2;

  void pyramid(struct Geometry* geometry);

  void box(struct Geometry* geometry);

  void facetGroup(struct Geometry* geometry);

  void sphereBasedShape(float* affine, float* bbox, float radius, float arc, float shift_z, float scale_z);

};