#pragma once
#include "StoreVisitor.h"
#include <cstdio>

class DumpNames : public StoreVisitor
{
public:
  void init(class Store& store) override;

  bool done() override;

  void beginFile(const char* info, const char* note, const char* date, const char* user, const char* encoding) override;

  void endFile() override;

  void beginModel(const char* project, const char* name) override;

  void endModel() override;

  void beginGroup(const char* name, const float* translation, const uint32_t material) override;

  void EndGroup() override;

  void geometry(struct Geometry* geometry) override;

  void composite(struct Composite* composite) override;

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
