#pragma once

#include "StoreVisitor.h"

class Flatten : public StoreVisitor
{
public:
  Flatten();

  ~Flatten();

  // newline seperated bufffer of tags to keep
  void setKeep(const void * ptr, size_t size);

  void init(class Store& store) override;

  void geometry(struct Geometry* geometry) override;

  void beginGroup(struct Group* group) override;

  void EndGroup() override;

  bool done() override;

private:
  struct Context* ctx = nullptr;
};