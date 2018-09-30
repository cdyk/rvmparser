#pragma once

#include "Common.h"
#include "StoreVisitor.h"

class Flatten
{
public:
  Flatten(Store* srcStore);

  // newline seperated bufffer of tags to keep
  void setKeep(const void * ptr, size_t size);

  // insert a single tag into keep set
  void keepTag(const char* tag);

  // Tags submitted as selected, may be way more than actual number of tags. But consistent between different stores.
  unsigned selectedTagsCount() const { return currentIndex; }

  unsigned activeTagsCount() const { return activeTags; }

  Store* run();

private:
  Map srcTags;  // All tags in source store
  Map tags;

  Arena arena;
  unsigned pass = 0;

  uint32_t currentIndex = 0;
  uint32_t activeTags = 0;

  Store* srcStore = nullptr;
  Store* dstStore = nullptr;

  Group** stack = nullptr;
  unsigned stack_p = 0;
  unsigned ignore_n = 0;

  void populateSrcTagsRecurse(Group* srcGroup);

  void populateSrcTags();

  bool anyChildrenSelectedAndTagRecurse(Group* srcGroup, int32_t id = -1);

  void buildPrunedCopyRecurse(Group* dstParent, Group* srcGroup, unsigned level);
};