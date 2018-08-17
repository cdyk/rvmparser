#pragma once
#include <cstdio>

#include "StoreVisitor.h"

class ExportObj : public StoreVisitor
{
public:

  ExportObj(const char* path);

  ~ExportObj();

  void init(class Store& store) override;

  void beginFile(const char* info, const char* note, const char* date, const char* user, const char* encoding) override;

  void endFile() override;

  void beginModel(const char* project, const char* name) override;

  void endModel() override;

  void beginGroup(const char* name, const float* translation, const uint32_t material) override;

  void EndGroup() override;

  void geometry(struct Geometry* geometry) override;

private:
  FILE* out = nullptr;
  FILE* mtl = nullptr;
  unsigned off_v = 1;
  unsigned off_n = 1;
  struct Connectivity* conn = nullptr;
  float curr_translation[3] = { 0,0,0 };

  bool primitiveBoundingBoxes = true;

};