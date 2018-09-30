#pragma once
#include "StoreVisitor.h"
#include <cstdio>

class DumpNames : public StoreVisitor
{
public:
  void init(class Store& store) override;

  bool done() override;

  void beginFile(struct Group* group) override;

  void endFile() override;

  void beginModel(struct Group* group) override;

  void endModel() override;

  void beginGroup(struct Group* group) override;

  void EndGroup() override;

  void geometry(struct Geometry* geometry) override;

  void setOutput(FILE* o) { out = o; }

private:
  struct Arena* arena = nullptr;
  FILE* out = stderr;
  const char** stack = nullptr;
  unsigned stack_p = 0;
  unsigned facetgroups = 0;
  unsigned geometries = 0;
  bool printed = true;

  void printGroupTail();
};
