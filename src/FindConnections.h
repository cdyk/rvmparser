#pragma once
#if 0
#include "Common.h"
#include "StoreVisitor.h"

struct Anchor
{
  struct Geometry* geo;
  float n[3];
  unsigned p_ix;
};

struct Connectivity
{
  float* p = nullptr;
  unsigned p_n = 0;

  Anchor* anchors = nullptr;
  unsigned anchor_n = 0;

};

class FindConnections : public StoreVisitor
{
public:

  void init(class Store& store) override;

  void geometry(struct Geometry* geometry) override;

  bool done() override;

private:
  Store* store;
  Arena arena;

  struct Point
  {
    float p[3];
    unsigned ix;
  };
  Point* points = nullptr;

  struct AnchorRef
  {
    struct Geometry* geo;
    float n[3];
    unsigned o;
  };
  AnchorRef* anchors = nullptr;

  Geometry** nodes = nullptr;      // For connected component search.

  unsigned* uscratch = nullptr;

  float epsilon = 1e-3f;

  void addAnchor(Geometry* geo, float*n, float* p, unsigned o);
  void uniquePointsRecurse(Point* range, unsigned N);
  void registerUniquePoint(Point* range, unsigned N, float d);
  void connect(AnchorRef* a0, AnchorRef* a1);
  void findConnectedComponents();

  struct Connectivity* conn = nullptr;
  unsigned anchor_i = 0;

  unsigned unique_points_i = 0;

};
#endif
