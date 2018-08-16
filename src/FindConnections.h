#pragma once
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
  struct Connectivity* conn = nullptr;
  unsigned anchor_i = 0;

};