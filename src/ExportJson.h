#pragma once
#include <cstdio>

#include "StoreVisitor.h"

class ExportJson : public StoreVisitor
{
public:
  ~ExportJson();

  bool open(const char* path_json);

  void init(class Store& store) override;

  void beginFile(Group* group) override;

  void endFile() override;

  void beginModel(Group* group) override;

  void endModel() override;

  void beginGroup(struct Group* group) override;

  void EndGroup() override;

  void beginChildren(struct Group* container) override;

  void endChildren() override;

  void beginAttributes(struct Group* container) override;

  void attribute(const char* key, const char* val) override;

  void endAttributes(struct Group* container) override;

  void geometry(struct Geometry* geometry) override;


private:
  FILE* json = nullptr;
  unsigned indent = 0;

};