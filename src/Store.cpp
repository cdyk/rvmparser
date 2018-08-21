#include <algorithm>
#include <cassert>
#include "Store.h"
#include "StoreVisitor.h"



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

Composite* Store::newComposite()
{
  auto * comp = arena.alloc<Composite>();
  insert(comps, comp);
  return comp;
}

Geometry* Store::newGeometry(Group* parent)
{
  assert(parent != nullptr);
  assert(parent->kind == Group::Kind::Group);

  auto * geo =  arena.alloc<Geometry>();
  geo->next = nullptr;
  geo->index = geo_n++;

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

void Store::apply(StoreVisitor* visitor, Group* group)
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

void Store::apply(StoreVisitor* visitor)
{
  visitor->init(*this);
  do {
    for (auto * file = roots.first; file != nullptr; file = file->next) {
      assert(file->kind == Group::Kind::File);
      visitor->beginFile(file->file.info,
                         file->file.note,
                         file->file.date,
                         file->file.user,
                         file->file.encoding);

      for (auto * model = file->groups.first; model != nullptr; model = model->next) {
        assert(model->kind == Group::Kind::Model);
        visitor->beginModel(model->model.project, model->model.name);

        for (auto * group = model->groups.first; group != nullptr; group = group->next) {
          apply(visitor, group);
        }
        visitor->endModel();
      }

      visitor->endFile();
    }

    for (auto * comp = comps.first; comp != nullptr; comp = comp->next) {
      visitor->composite(comp);
    }

  } while (visitor->done() == false);


}
