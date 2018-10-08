#pragma once

#include "Common.h"
#include "StoreVisitor.h"
#include "LinAlg.h"

class TriangulationFactory
{
public:
  TriangulationFactory(Store* store, Logger logger, float tolerance, unsigned minSamples, unsigned maxSamples);

  unsigned sagittaBasedSegmentCount(float arc, float radius, float scale);

  float sagittaBasedError(float arc, float radius, float scale, unsigned samples);

  Triangulation* pyramid(Arena* arena, const Geometry* geo, float scale);

  Triangulation* box(Arena* arena, const Geometry* geo, float scale);

  Triangulation* rectangularTorus(Arena* arena, const Geometry* geo, float scale);

  Triangulation* circularTorus(Arena* arena, const Geometry* geo, float scale);

  Triangulation* snout(Arena* arena, const  Geometry* geo, float scale);

  Triangulation* cylinder(Arena* arena, const Geometry* geo, float scale);

  Triangulation* facetGroup(Arena* arena, const Geometry* geo, float scale);

  Triangulation* sphereBasedShape(Arena* arena, const  Geometry* geo, float radius, float arc, float shift_z, float scale_z, float scale);

  unsigned discardedCaps = 0;

private:
  Store* store;
  Logger logger;
  float tolerance = 0.f / 0.f;
  unsigned minSamples = 3;
  unsigned maxSamples = 100;

  std::vector<float> vertices;
  std::vector<Vec3f> vec3;
  std::vector<float> normals;
  std::vector<uint32_t> indices;

  std::vector<unsigned> u0;
  std::vector<uint32_t> u1;
  std::vector<uint32_t> u2;
  std::vector<float> t0;
  std::vector<float> t1;
  std::vector<float> t2;

};

class Tessellator : public StoreVisitor
{
public:
  Tessellator() = delete;
  Tessellator(const Tessellator&) = delete;
  Tessellator(Logger logger, float tolerance, float cullLeafThreshold, float cullGeometryThreshold, unsigned maxSamples);

  Tessellator& operator=(const Tessellator&) = delete;

  ~Tessellator();

  void init(class Store& store) override;

  void beginGroup(struct Group* group)  override;

  void EndGroup() override;

  void geometry(struct Geometry* geometry) override;

  void endModel() override;


  unsigned leafCulled = 0;
  unsigned geometryCulled = 0;
  unsigned tessellated = 0;
  unsigned processed = 0;

  uint64_t vertices = 0;
  uint64_t triangles = 0;

protected:
  struct CacheItem
  {
    struct CacheItem* next;
    struct Geometry* src;
    struct Triangulation* tri;
  };

  struct StackItem
  {
    float groupError;   // Error induced if group is omitted.
  };

  float tolerance = 0.f;
  unsigned maxSamples = 100;
  float cullLeafThresholdScaled = 0.f / 0.f;
  float cullGeometryThresholdScaled = 0.f / 0.f;
  Arena arena;
  TriangulationFactory* factory = nullptr;
  Logger logger;

  Store * store = nullptr;

  struct {
    Map map;
    CacheItem* items;
    unsigned fill;
  } cache;

  StackItem* stack = nullptr;
  unsigned stack_p = 0;

  Triangulation* getTriangulation(Geometry* geo);

  virtual void process(Geometry* geometry) {}
};