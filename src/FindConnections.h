#pragma once
#include "StoreVisitor.h"
#include "Store.h"

struct Anchor
{
  float p[3];
  float n[3];
  unsigned type;
  float radius;
  struct Geometry* geo;
};

class FindConnections : public StoreVisitor
{
public:

  void init(class Store& store) override;

  void geometry(struct Geometry* geometry) override;

  bool done() override;

private:
  Arena arena;

  Anchor* anchors = nullptr;
  unsigned anchor_n;
  unsigned anchor_i;


  
};