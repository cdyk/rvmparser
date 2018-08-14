#include <algorithm>
#include <cassert>
#include "Store.h"


void* Arena::alloc(size_t bytes)
{
  const size_t pageSize = 1024*1024;

  if (bytes == 0) return nullptr;

  auto padded = (bytes + 7) & ~7;

  if (size < fill + padded) {
    fill = sizeof(uint8_t*);
    size = std::max(pageSize, fill + padded);

    auto * page = (uint8_t*)malloc(size);
    *(uint8_t**)page = nullptr;

    if (first == nullptr) {
      first = page;
      curr = page;
    }
    else {
      *(uint8_t**)curr = page; // update next
      curr = page;
    }
  }

  assert(first != nullptr);
  assert(curr != nullptr);
  assert(*(uint8_t**)curr == nullptr);
  assert(fill + padded <= size);

  auto * rv = curr + fill;
  fill += padded;
  return rv;
}

void* Arena::dup(void* src, size_t bytes)
{
  auto * dst = alloc(bytes);
  std::memcpy(dst, src, bytes);
  return dst;
}


void Arena::clear()
{
  auto * c = first;
  while (c != nullptr) {
    auto * n = *(uint8_t**)c;
    free(c);
    c = n;
  }
  first = nullptr;
  curr = nullptr;
  fill = 0;
  size = 0;
}



namespace {

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

}


Store::Store()
{
  roots.first = nullptr;
  roots.last = nullptr;
}


Geometry* Store::newGeometry(Group* parent)
{
  assert(parent != nullptr);
  assert(parent->kind == Group::Kind::Group);

  auto * geo =  arena.alloc<Geometry>();
  geo->next = nullptr;

  insert(parent->group.geometries, geo);
  return geo;
}

Group* Store::newGroup(Group* parent, Group::Kind kind)
{
  auto grp = arena.alloc<Group>();
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
  visitor->beginGroup(group->group.name,
                      group->group.translation,
                      group->group.material);

  auto * g = group->groups.first;
  while (g != nullptr) {
    apply(visitor, g);
    g = g->next;
  }

  if (group->kind == Group::Kind::Group) {
    for (auto * geo = group->group.geometries.first; geo != nullptr; geo = geo->next) {
      visitor->geometry(geo);
    }
  }

  visitor->EndGroup();
}

void Store::apply(RVMVisitor* visitor)
{
  visitor->init(*this);
  for(auto * file = roots.first; file != nullptr; file = file->next) {
    assert(file->kind == Group::Kind::File);
    visitor->beginFile(file->file.info,
                       file->file.note,
                       file->file.date,
                       file->file.user,
                       file->file.encoding);

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
