#pragma once

#include "Common.h"
#include "StoreVisitor.h"

class Tessellator : public StoreVisitor
{
public:
  Tessellator() = delete;
  Tessellator(const Tessellator&) = delete;
  Tessellator& operator=(const Tessellator&) = delete;

  Tessellator(Logger logger, float tolerance, float cullthreshold) : logger(logger), tolerance(tolerance), cullThreshold(cullthreshold) {}

  void init(class Store& store) override;

  void beginGroup(struct Group* group)  override;

  void EndGroup() override;

  void geometry(struct Geometry* geometry) override;

  void endModel() override;

private:
  Logger logger;
  Arena arena;
  Store * store = nullptr;
  float tolerance = 0.f / 0.f;
  float cullThreshold = 0.f / 0.f;
  unsigned minSamples = 3;
  unsigned maxSamples = 100;

  struct StackItem
  {
    float groupError;   // Error induced if group is omitted.
  };

  StackItem* stack = nullptr;
  unsigned stack_p = 0;

  unsigned discardedCaps = 0;

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