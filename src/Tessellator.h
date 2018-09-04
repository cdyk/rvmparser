#pragma once

#include "StoreVisitor.h"

class Tessellator : public StoreVisitor
{
public:

  void init(class Store& store) override;

  void geometry(struct Geometry* geometry) override;

private:
  float tolerance = 100.01f;
  bool cullTiny = false;
  unsigned minSamples = 3;
  unsigned maxSamples = 100;

  struct Arena * arena = nullptr;


  std::vector<float> vertices;
  std::vector<float> normals;
  std::vector<uint32_t> indices;

  std::vector<unsigned> u0;
  std::vector<uint32_t> u1;
  std::vector<uint32_t> u2;
  std::vector<float> t0;
  std::vector<float> t1;
  std::vector<float> t2;

  unsigned sagittaBasedSampleCount(float arc, float radius, float scale);
 
  float sagittaBasedError(float arc, float radius, float scale, unsigned samples);

  void pyramid(struct Geometry* geo, float scale);

  void box(struct Geometry* geo, float scale);

  void rectangularTorus(struct Geometry* geo, float scale);

  void circularTorus(struct Geometry* geo, float scale);

  void snout(struct Geometry* geo, float scale);

  void cylinder(struct Geometry* geo, float scale);

  void facetGroup(struct Geometry* geo, float scale);

  void sphereBasedShape(struct Geometry* geo, float radius, float arc, float shift_z, float scale_z, float scale);

};