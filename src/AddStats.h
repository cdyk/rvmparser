#pragma once
#include "StoreVisitor.h"

class AddStats : public StoreVisitor
{
public:

  void init(class Store& store) override;

  void beginGroup(const char* name, const float* translation, const uint32_t material) override;

  void geometry(struct Geometry* geometry) override;

  bool done() override;

private:
  struct Stats* stats = nullptr;

};