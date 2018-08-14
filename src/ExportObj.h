#pragma once
#include <cstdio>

#include "Tessellator.h"

class ExportObj : public RVMVisitor
{
public:

  ExportObj(const char* path);

  ~ExportObj();

  void init(class Store& store) override {}

  void beginFile(const char* info, const char* note, const char* date, const char* user, const char* encoding) override;

  void endFile() override;

  void beginModel(const char* project, const char* name) override;

  void endModel() override;

  void beginGroup(const char* name, const float* translation, const uint32_t material) override;

  void EndGroup() override;

  void geometry(struct Geometry* geometry) override;

private:
  FILE* out = nullptr;
  size_t off = 1;
  float curr_translation[3] = { 0,0,0 };

};