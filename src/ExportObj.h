#pragma once
#include <cstdio>

#include "TriangulatedVisitor.h"

class ExportObj : public TriangulatedVisitor
{
public:

  ExportObj(const std::string& path);

  ~ExportObj();

  void beginFile(const std::string info, const std::string& note, const std::string& date, const std::string& user, const std::string& encoding) override;

  void endFile() override;

  void beginModel(const std::string& project, const std::string& name) override;

  void endModel() override;

  void beginGroup(const std::string& name, const float* translation, const uint32_t material) override;

  void EndGroup() override;

  void triangles(float* affine, float* bbox, std::vector<float>& P, std::vector<float>& N, std::vector<uint32_t>& indices) override;

private:
  FILE* out = nullptr;
  size_t off = 1;
  float curr_translation[3] = { 0,0,0 };

};