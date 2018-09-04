#pragma once

#include "Common.h"
#include "StoreVisitor.h"

class AddGroupBBox : public StoreVisitor
{
public:
  void init(class Store& store) override;

  void geometry(struct Geometry* geometry) override;

  void beginGroup(struct Group* group)  override;

  void EndGroup() override;

protected:
  Store* store = nullptr;

  Arena arena;
  Group** stack = nullptr;
  unsigned stack_p = 0;

};