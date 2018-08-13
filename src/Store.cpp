#include <algorithm>
#include <cassert>
#include "Store.h"

namespace {
  
  const size_t pageSize = 4096;

  struct Arena
  {
    uint8_t * first = nullptr;
    uint8_t * curr = nullptr;
    size_t fill = 0;
    size_t size = 0;
  };

  void* arenaAlloc(void* a, size_t bytes)
  {
    if (bytes == 0) return nullptr;
    auto * arena = (Arena*)a;

    auto padded = (bytes + 7) & ~7;

    if (arena->size < arena->fill + padded) {
      arena->fill = sizeof(uint8_t*);
      arena->size = std::max(pageSize, padded + arena->fill);

      auto * page = (uint8_t*)malloc(arena->size);
      *(uint8_t**)page = nullptr;

      if (arena->first == nullptr) {
        arena->first = page;
        arena->curr = page;
      }
      else {
        *(uint8_t**)arena->curr = page; // update next
        arena->curr = page;
      }
    }

    assert(arena->first != nullptr);
    assert(arena->curr != nullptr);
    assert(*(uint8_t**)arena->curr == nullptr);
    assert(arena->fill + padded <= arena->size);

    auto * rv = arena->curr + arena->fill;
    arena->fill += padded;
    return rv;
  }

  void arenaFree(void* a)
  {
    auto * arena = (Arena*)a;

    auto * c = arena->first;
    while (c != nullptr) {
      auto * n = *(uint8_t**)c;
      free(c);
      c = n;
    }
    arena->first = arena->curr = nullptr;
    arena->fill = arena->size = 0;
  }

  template<typename T>
  T * arenaAlloc(void* arena)
  {
    return new(arenaAlloc(arena, sizeof(T))) T();
  }

  template<typename T>
  void insert(ListHeader<T>& list, T* item)
  {
    if (list.first == nullptr) {
      list.first = list.last = item;
    }
    else {
      list.last->next = item;
      list.last = item;
    }
  }

  std::string wrap(const char* p) {
    if (p == nullptr) {
      return "";
    }
    else {
      return p;
    }
  }


}


Store::Store()
{
  arena = new Arena;
  roots.first = nullptr;
  roots.last = nullptr;
}

Store::~Store()
{
  arenaFree(arena);
  delete arena;
}

void* Store::alloc(size_t bytes)
{
  return arenaAlloc(arena, bytes);
}


Geometry* Store::newGeometry(Group* parent)
{
  assert(parent != nullptr);
  assert(parent->kind == Group::Kind::Group);

  auto * geo = arenaAlloc<Geometry>(arena);
  geo->next = nullptr;

  insert(parent->group.geometries, geo);
  return geo;
}

Group* Store::newGroup(Group* parent, Group::Kind kind)
{
  auto grp = arenaAlloc<Group>(arena);
  std::memset(grp, 0, sizeof(Group));

  if (parent == nullptr) {
    insert(roots, grp);
  }
  else {
    insert(parent->groups, grp);
  }

  grp->kind = kind;
  return grp;
}

void Store::apply(RVMVisitor* visitor, Group* group)
{
  assert(group->kind == Group::Kind::Group);
  visitor->beginGroup(wrap(group->group.name),
                      group->group.translation,
                      group->group.material);

  auto * g = group->groups.first;
  while (g != nullptr) {
    apply(visitor, g);
    g = g->next;
  }

  visitor->EndGroup();
}

void Store::apply(RVMVisitor* visitor)
{
  for(auto * file = roots.first; file != nullptr; file = file->next) {
    assert(file->kind == Group::Kind::File);
    visitor->beginFile(wrap(file->file.info),
                       wrap(file->file.note),
                       wrap(file->file.date),
                       wrap(file->file.user),
                       wrap(file->file.encoding));

    for(auto * model = file->groups.first; model != nullptr; model = model->next) {
      assert(model->kind == Group::Kind::Model);
      visitor->beginModel(model->model.project, model->model.name);

      for (auto * group = model->groups.first; group != nullptr; group = group->next) {
        apply(visitor, group);
      }

      visitor->endModel();
    }

    visitor->endFile();
  }


}
