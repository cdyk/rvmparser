#pragma once

#include "RVMVisitor.h"

class Tessellator : public RVMVisitor
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
  float tolerance = 0.01f;
  unsigned minSamples = 3;
  unsigned maxSamples = 100;

  struct Arena * arena = nullptr;

  std::vector<float> vertices;
  std::vector<float> normals;
  std::vector<uint32_t> indices;

  std::vector<unsigned> u0;
  std::vector<float> t0;
  std::vector<float> t1;
  std::vector<float> t2;

  unsigned sagittaBasedSampleCount(float arc, float radius, float scale);
 
  float sagittaBasedError(float arc, float radius, float scale, unsigned samples);

  void pyramid(struct Geometry* geo, float scale);

  void box(struct Geometry* geo, float scale);

  void rectangularTorus(struct Geometry* geo, float scale);

  void facetGroup(struct Geometry* geo, float scale);

  void sphereBasedShape(float* affine, float* bbox, float radius, float arc, float shift_z, float scale_z);

};