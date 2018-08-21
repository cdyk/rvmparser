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

  bool done() override;

  void beginFile(struct Group* group) override;

  void endFile() override;

  void beginModel(struct Group* group) override;

  void endModel() override;

  void beginGroup(struct Group* group) override;

  void EndGroup() override;

  void geometry(struct Geometry* geometry) override;

  Store* result();

private:
  struct Context* ctx = nullptr;
};