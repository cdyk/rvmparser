#pragma once
#include <cstdio>

#include "Common.h"
#include "StoreVisitor.h"

class ExportObj : public StoreVisitor
{
public:
  bool groupBoundingBoxes = false;

  ~ExportObj();

  bool open(const char* path_obj, const char* path_mtl);

  void init(class Store& store) override;

  void beginFile(Group* group) override;

  void endFile() override;

  void beginModel(Group* group) override;

  void endModel() override;

  void beginGroup(struct Group* group) override;

  void EndGroup() override;

  void geometry(struct Geometry* geometry) override;

private:
  FILE* out = nullptr;
  FILE* mtl = nullptr;
  Map definedColors;
  Store* store = nullptr;
  Buffer<const char*> stack;
  unsigned stack_p = 0;
  unsigned off_v = 1;
  unsigned off_n = 1;
  unsigned off_t = 1;
  struct Connectivity* conn = nullptr;
  float curr_translation[3] = { 0,0,0 };

  bool anchors = false;
  bool primitiveBoundingBoxes = false;
  bool compositeBoundingBoxes = false;

};