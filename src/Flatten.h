#pragma once

#include "Common.h"
#include "StoreVisitor.h"

class Flatten : public StoreVisitor
{
public:

  ~Flatten();

  // newline seperated bufffer of tags to keep
  void setKeep(const void * ptr, size_t size);

  // insert a single tag into keep set
  void keepTag(const char* tag);

  void init(class Store& store) override;

  bool done() override;

  void beginFile(struct Group* group) override;

  void endFile() override;

  void beginModel(struct Group* group) override;

  void endModel() override;

  void beginGroup(struct Group* group) override;

  void EndGroup() override;

  void geometry(struct Geometry* geometry) override;

  unsigned tagsInitial() const { return unsigned(tags.fill); }

  unsigned tagsProcessed() const { return unsigned(groups.fill); }

  Store* result();

private:
  Map tags;
  Map groups;

  Arena arena;
  unsigned pass = 0;

  Store* store = nullptr;

  Group** stack = nullptr;
  unsigned stack_p = 0;
  unsigned ignore_n = 0;
};