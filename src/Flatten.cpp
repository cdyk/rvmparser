#include <cassert>

#include "Store.h"
#include "Flatten.h"


Flatten::Flatten(Store* srcStore) :
  srcStore(srcStore)
{
  populateSrcTags();
}

void Flatten::setKeep(const void * ptr, size_t size)
{
  auto * a = (const char*)ptr;
  auto * b = a + size;

  while (true) {
    while (a < b && (*a == '\n' || *a == '\r')) a++;
    auto * c = a;
    while (a < b && (*a != '\n' && *a != '\r')) a++;

    if (c < a) {
      auto * d = a - 1;
      while (c < d && (d[-1] != '\t')) --d;

      auto * str = srcStore->strings.intern(d, a);
      uint64_t val;
      if (srcTags.get(val, uint64_t(str))) {
        tags.insert(uint64_t(str), uint64_t(currentIndex));
        activeTags++;
      }
      currentIndex++;
    }
    else {
      break;
    }
  }
}

void Flatten::keepTag(const char* tag)
{
  auto * str = srcStore->strings.intern(tag);
  uint64_t val;
  if (srcTags.get(val, uint64_t(str))) {
    tags.insert(uint64_t(str), uint64_t(currentIndex));
    ((Group*)val)->group.id = int32_t(currentIndex);
    activeTags++;
  }
  currentIndex++;
}


void Flatten::populateSrcTagsRecurse(Group* srcGroup)
{
  srcGroup->group.id = -1;
  srcTags.insert(uint64_t(srcGroup->group.name), uint64_t(srcGroup));
  for (auto * srcChild = srcGroup->groups.first; srcChild != nullptr; srcChild = srcChild->next) {
    populateSrcTagsRecurse(srcChild);
  }
}

void Flatten::populateSrcTags()
{
  // Sets all group.index to ~0u, and records the tag-names in srcTags. Nothing is selected yet.
  // name of groups are already interned in srcGroup
  for (auto * srcRoot = srcStore->getFirstRoot(); srcRoot != nullptr; srcRoot = srcRoot->next) {
    for (auto * srcModel = srcRoot->groups.first; srcModel != nullptr; srcModel = srcModel->next) {
      for (auto * srcGroup = srcModel->groups.first; srcGroup != nullptr; srcGroup = srcGroup->next) {
        populateSrcTagsRecurse(srcGroup);
      }
    }
  }
}

bool Flatten::anyChildrenSelectedAndTagRecurse(Group* srcGroup, int32_t id)
{
  uint64_t val;
  if (tags.get(val, uint64_t(srcGroup->group.name))) {
    srcGroup->group.id = int32_t(val);
  }
  else {
    srcGroup->group.id = id;
  }

  bool anyChildrenSelected = false;
  for (auto * srcChild = srcGroup->groups.first; srcChild != nullptr; srcChild = srcChild->next) {
    anyChildrenSelected = anyChildrenSelectedAndTagRecurse(srcChild, srcGroup->group.id) || anyChildrenSelected;
  }

  return srcGroup->group.id != -1;
}

void Flatten::buildPrunedCopyRecurse(Group* dstParent, Group* srcGroup, unsigned level)
{
  assert(srcGroup->kind == Group::Kind::Group);

  // Only groups can contain geometry, so we must make sure that we have at least one group even when none is selected.
  // Also, some subsequent stages require that we do not have geometry in the first level of groups.
  if (srcGroup->group.id == -1 && level < 2) {
    srcGroup->group.id = -2;
  }

  if (srcGroup->group.id != -1) {
    dstParent = dstStore->cloneGroup(dstParent, srcGroup);
    dstParent->group.id = srcGroup->group.id;
  }

  for (auto * srcGeo = srcGroup->group.geometries.first; srcGeo != nullptr; srcGeo = srcGeo->next) {
    dstStore->cloneGeometry(dstParent, srcGeo);
  }

  for (auto * srcChild = srcGroup->groups.first; srcChild != nullptr; srcChild = srcChild->next) {
    buildPrunedCopyRecurse(dstParent, srcChild, level + 1);
  }
}

Store* Flatten::run()
{
  dstStore = new Store();

  // populateSrcTags was run by the constructor, and setKeep and keepTags has changed some group.index from ~0u.
  // set group.index of parents of selected nodes to ~1u so we can retain them in the culling pass.
  for (auto * srcRoot = srcStore->getFirstRoot(); srcRoot != nullptr; srcRoot = srcRoot->next) {
    assert(srcRoot->kind == Group::Kind::File);
    for (auto * srcModel = srcRoot->groups.first; srcModel != nullptr; srcModel = srcModel->next) {
      assert(srcModel->kind == Group::Kind::Model);
      for (auto * srcGroup = srcModel->groups.first; srcGroup != nullptr; srcGroup = srcGroup->next) {
        anyChildrenSelectedAndTagRecurse(srcGroup);
      }
    }
  }

  // Create a fresh copy
  for (auto * srcRoot = srcStore->getFirstRoot(); srcRoot != nullptr; srcRoot = srcRoot->next) {

    assert(srcRoot->kind == Group::Kind::File);
    auto * dstRoot = dstStore->cloneGroup(nullptr, srcRoot);

    for (auto * srcModel = srcRoot->groups.first; srcModel != nullptr; srcModel = srcModel->next) {

      assert(srcModel->kind == Group::Kind::Model);
      auto * dstModel = dstStore->cloneGroup(dstRoot, srcModel);

      for (auto * srcGroup = srcModel->groups.first; srcGroup != nullptr; srcGroup = srcGroup->next) {
        buildPrunedCopyRecurse(dstModel, srcGroup, 0);
      }
    }
  }

  dstStore->updateCounts();
  auto rv = dstStore;
  dstStore = nullptr;
  return rv;
}

