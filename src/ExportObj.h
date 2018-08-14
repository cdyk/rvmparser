#pragma once
#include <cstdio>

#include "TriangulatedVisitor.h"

class ExportObj : public TriangulatedVisitor
{
public:

  ExportObj(const char* path);

  ~ExportObj();

  void beginFile(const char* info, const char* note, const char* date, const char* user, const char* encoding) override;

  void endFile() override;

  void beginModel(const char* project, const char* name) override;

  void endModel() override;

  void beginGroup(const char* name, const float* translation, const uint32_t material) override;

  void EndGroup() override;

  void line(float* affine, float* bbox, float x0, float x1) override;

  void triangles(float* affine, float* bbox, std::vector<float>& P, std::vector<float>& N, std::vector<uint32_t>& indices) override;

private:
  FILE* out = nullptr;
  size_t off = 1;
  float curr_translation[3] = { 0,0,0 };

};