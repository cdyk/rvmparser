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

Geometry* Store::cloneGeometry(Group* parent, const Geometry* src)
{
  auto * dst = newGeometry(parent);
  dst->kind = src->kind;
  std::memcpy(dst->M_3x4, src->M_3x4, sizeof(src->M_3x4));
  std::memcpy(dst->bbox, src->bbox, sizeof(src->bbox));
  switch (dst->kind) {
    case Geometry::Kind::Pyramid:
    case Geometry::Kind::Box:
    case Geometry::Kind::RectangularTorus:
    case Geometry::Kind::CircularTorus:
    case Geometry::Kind::EllipticalDish:
    case Geometry::Kind::SphericalDish:
    case Geometry::Kind::Snout:
    case Geometry::Kind::Cylinder:
    case Geometry::Kind::Sphere:
    case Geometry::Kind::Line:
      std::memcpy(&dst->snout, &src->snout, sizeof(src->snout));
      break;
    case Geometry::Kind::FacetGroup:
      dst->facetGroup.polygons_n = src->facetGroup.polygons_n;
      dst->facetGroup.polygons = (Polygon*)arena.alloc(sizeof(Polygon)*dst->facetGroup.polygons_n);
      for (unsigned k = 0; k < dst->facetGroup.polygons_n; k++) {
        auto & dst_poly = dst->facetGroup.polygons[k];
        const auto & src_poly = src->facetGroup.polygons[k];
        dst_poly.contours_n = src_poly.contours_n;
        dst_poly.contours = (Contour*)arena.alloc(sizeof(Contour)*dst_poly.contours_n);
        for (unsigned i = 0; i < dst_poly.contours_n; i++) {
          auto & dst_cont = dst_poly.contours[i];
          const auto & src_cont = src_poly.contours[i];
          dst_cont.vertices_n = src_cont.vertices_n;
          dst_cont.vertices = (float*)arena.dup(src_cont.vertices, 3 * sizeof(float)*dst_cont.vertices_n);
          dst_cont.normals = (float*)arena.dup(src_cont.normals, 3 * sizeof(float)*dst_cont.vertices_n);
        }
      }
      break;
    default:
      assert(false && "Geometry has invalid kind.");
      break;
  }
  return dst;
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

  grp_n++;

  grp->kind = kind;
  grp_n++;
  return grp;
}

Group* Store::cloneGroup(Group* parent, const Group* src)
{
  auto * dst = newGroup(parent, src->kind);
  switch (src->kind) {
  case Group::Kind::File:
    dst->file.info = (char*)arena.dup(src->file.info, strlen(src->file.info) + 1);
    dst->file.note = (char*)arena.dup(src->file.note, strlen(src->file.note) + 1);
    dst->file.date = (char*)arena.dup(src->file.date, strlen(src->file.date) + 1);
    dst->file.user = (char*)arena.dup(src->file.user, strlen(src->file.user) + 1);
    dst->file.encoding = (char*)arena.dup(src->file.encoding, strlen(src->file.encoding) + 1);
    break;
  case Group::Kind::Model:
    dst->model.project = (char*)arena.dup(src->model.project, strlen(src->model.project) + 1);
    dst->model.name = (char*)arena.dup(src->model.name, strlen(src->model.name) + 1);
    break;
  case Group::Kind::Group:
    dst->group.name = (char*)arena.dup(src->group.name, strlen(src->group.name) + 1);
    dst->group.material = src->group.material;
    for (unsigned k = 0; k < 3; k++) dst->group.translation[k] = src->group.translation[k];
    break;
  default:
    assert(false && "Group has invalid kind.");
    break;
  }
  return dst;
}


void Store::apply(StoreVisitor* visitor, Group* group)
{
  assert(group->kind == Group::Kind::Group);
  visitor->beginGroup(group);

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
      visitor->beginFile(file);

      for (auto * model = file->groups.first; model != nullptr; model = model->next) {
        assert(model->kind == Group::Kind::Model);
        visitor->beginModel(model);

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
