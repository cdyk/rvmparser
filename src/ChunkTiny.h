#pragma once

#include "Common.h"
#include "StoreVisitor.h"

class Flatten;

class ChunkTiny : public StoreVisitor
{
public:
  ChunkTiny(Flatten& flatten, unsigned vertexThreshold);

  void init(class Store& store) override;

  void geometry(struct Geometry* geometry) override;

  void beginGroup(struct Node* group)  override;

  void EndGroup() override;


protected:
  Flatten& flatten;
  Arena arena;
  struct StackItem
  {
    unsigned vertices;
    Node * group;
    bool keep;
  };
  StackItem* stack = nullptr;
  unsigned stack_p = 0;

  unsigned vertexThreshold;
};