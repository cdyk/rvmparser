#include <algorithm>
#include <cassert>
#include <cstring>
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
  roots.clear();
  debugLines.clear();
  connections.clear();
  setErrorString("");
}

Color* Store::newColor(Node* parent)
{
  assert(parent != nullptr);
  assert(parent->kind == Node::Kind::Model);
  auto* color = arena.alloc<Color>();
  color->next = nullptr;
  insert(parent->model.colors, color);
  return color;
}

void Store::setErrorString(const char* str)
{
  auto l = strlen(str);
  error_str = strings.intern(str, str + l);
}

Node* Store::findRootGroup(const char* name)
{
  for (auto * file = roots.first; file != nullptr; file = file->next) {
    assert(file->kind == Node::Kind::File);
    for (auto * model = file->children.first; model != nullptr; model = model->next) {
      //fprintf(stderr, "model '%s'\n", model->model.name);
      assert(model->kind == Node::Kind::Model);
      for (auto * group = model->children.first; group != nullptr; group = group->next) {
        //fprintf(stderr, "group '%s' %p\n", group->group.name, (void*)group->group.name);
        if (group->group.name == name) return group;
      }
    }
  }
  return nullptr;
}


void Store::addDebugLine(float* a, float* b, uint32_t color)
{
  auto line = arena.alloc<DebugLine>();
  for (unsigned k = 0; k < 3; k++) {
    line->a[k] = a[k];
    line->b[k] = b[k];
  }
  line->color = color;
  insert(debugLines, line);
}


Connection* Store::newConnection()
{
  auto * connection = arena.alloc<Connection>();
  insert(connections, connection);
  return connection;
}

Geometry* Store::newGeometry(Node* parent)
{
  assert(parent != nullptr);
  assert(parent->kind == Node::Kind::Group);

  auto * geo =  arena.alloc<Geometry>();
  geo->next = nullptr;
  geo->id = numGeometriesAllocated++;

  insert(parent->group.geometries, geo);
  return geo;
}

Geometry* Store::cloneGeometry(Node* parent, const Geometry* src)
{
  auto * dst = newGeometry(parent);
  dst->kind = src->kind;
  dst->M_3x4 = src->M_3x4;
  dst->bboxLocal = src->bboxLocal;
  dst->bboxWorld = src->bboxWorld;
  dst->id = src->id;
  dst->sampleStartAngle = src->sampleStartAngle;
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
  
  if (src->colorName) {
    dst->colorName = strings.intern(src->colorName);
  }
  dst->color = src->color;

  if (src->triangulation) {
    dst->triangulation = arena.alloc<Triangulation>();

    const auto * stri = src->triangulation;
    auto * dtri = dst->triangulation;
    dtri->error = stri->error;
    dtri->id = stri->id;
    if (stri->vertices_n) {
      dtri->vertices_n = stri->vertices_n;
      dtri->vertices = (float*)arena.dup(stri->vertices, 3 * sizeof(float) * dtri->vertices_n);
      dtri->normals = (float*)arena.dup(stri->normals, 3 * sizeof(float) * dtri->vertices_n);
      dtri->texCoords = (float*)arena.dup(stri->texCoords, 2 * sizeof(float) * dtri->vertices_n);
    }
    if (stri->triangles_n) {
      dtri->triangles_n = stri->triangles_n;
      dtri->indices = (uint32_t*)arena.dup(stri->indices, 3 * sizeof(uint32_t) * dtri->triangles_n);
    }
  }

  return dst;
}


Node* Store::newNode(Node* parent, Node::Kind kind)
{
  auto grp = arena.alloc<Node>();
  std::memset(grp, 0, sizeof(Node));

  if (parent == nullptr) {
    insert(roots, grp);
  }
  else {
    insert(parent->children, grp);
  }

  grp->kind = kind;
  numGroupsAllocated++;
  return grp;
}

Attribute* Store::getAttribute(Node* group, const char* key)
{
  for (auto * attribute = group->attributes.first; attribute != nullptr; attribute = attribute->next) {
    if (attribute->key == key) return attribute;
  }
  return nullptr;
}

Attribute* Store::newAttribute(Node* group, const char* key)
{
  auto * attribute = arena.alloc<Attribute>();
  attribute->key = key;
  insert(group->attributes, attribute);
  return attribute;
}


Node* Store::getDefaultModel()
{
  auto * file = roots.first;
  if (file == nullptr) {
    file = newNode(nullptr, Node::Kind::File);
    file->file.info = strings.intern("");
    file->file.note = strings.intern("");
    file->file.date = strings.intern("");
    file->file.user = strings.intern("");
    file->file.encoding = strings.intern("");
  }
  auto * model = file->children.first;
  if (model == nullptr) {
    model = newNode(file, Node::Kind::Model);
    model->model.project = strings.intern("");
    model->model.name = strings.intern("");
  }
  return model;
}


Node* Store::cloneGroup(Node* parent, const Node* src)
{
  auto * dst = newNode(parent, src->kind);
  switch (src->kind) {
  case Node::Kind::File:
    dst->file.info = strings.intern(src->file.info);
    dst->file.note = strings.intern(src->file.note);
    dst->file.date = strings.intern(src->file.date);
    dst->file.user = strings.intern(src->file.user);
    dst->file.encoding = strings.intern(src->file.encoding);
    break;
  case Node::Kind::Model:
    dst->model.project = strings.intern(src->model.project);
    dst->model.name = strings.intern(src->model.name);
    break;
  case Node::Kind::Group:
    dst->group.name = strings.intern(src->group.name);
    dst->group.bboxWorld = src->group.bboxWorld;
    dst->group.material = src->group.material;
    dst->group.id = src->group.id;
    for (unsigned k = 0; k < 3; k++) dst->group.translation[k] = src->group.translation[k];
    break;
  default:
    assert(false && "Group has invalid kind.");
    break;
  }

  for (auto * src_att = src->attributes.first; src_att != nullptr; src_att = src_att->next) {
    auto * dst_att = newAttribute(dst, strings.intern(src_att->key));
    dst_att->val = strings.intern(src_att->val);
  }

  return dst;
}


void Store::apply(StoreVisitor* visitor, Node* group)
{
  assert(group->kind == Node::Kind::Group);
  visitor->beginGroup(group);

  if (group->attributes.first) {
    visitor->beginAttributes(group);
    for (auto * a = group->attributes.first; a != nullptr; a = a->next) {
      visitor->attribute(a->key, a->val);
    }
    visitor->endAttributes(group);
  }

  if (group->kind == Node::Kind::Group && group->group.geometries.first != nullptr) {
    visitor->beginGeometries(group);
    for (auto * geo = group->group.geometries.first; geo != nullptr; geo = geo->next) {
      visitor->geometry(geo);
    }
    visitor->endGeometries();
  }

  visitor->doneGroupContents(group);

  if (group->children.first != nullptr) {
    visitor->beginChildren(group);
    for (auto * g = group->children.first; g != nullptr; g = g->next) {
      apply(visitor, g);
    }
    visitor->endChildren();
  }

  visitor->EndGroup();
}

void Store::apply(StoreVisitor* visitor)
{
  visitor->init(*this);
  do {
    for (auto * file = roots.first; file != nullptr; file = file->next) {
      assert(file->kind == Node::Kind::File);
      visitor->beginFile(file);

      for (auto * model = file->children.first; model != nullptr; model = model->next) {
        assert(model->kind == Node::Kind::Model);
        visitor->beginModel(model);

        for (auto * group = model->children.first; group != nullptr; group = group->next) {
          apply(visitor, group);
        }
        visitor->endModel();
      }

      visitor->endFile();
    }
  } while (visitor->done() == false);
}

void Store::updateCountsRecurse(Node* group)
{

  for (auto * child = group->children.first; child != nullptr; child = child->next) {
    updateCountsRecurse(child);
  }

  numGroups++;
  if (group->children.first == nullptr) {
    numLeaves++;
  }

  if (group->kind == Node::Kind::Group) {
    if (group->children.first == nullptr && group->group.geometries.first == nullptr) {
      numEmptyLeaves++;
    }

    if (group->children.first != nullptr && group->group.geometries.first != nullptr) {
      numNonEmptyNonLeaves++;
    }

    for (auto * geo = group->group.geometries.first; geo != nullptr; geo = geo->next) {
      numGeometries++;
    }
  }

}

void Store::updateCounts()
{
  numGroups = 0;
  numLeaves = 0;
  numEmptyLeaves = 0;
  numNonEmptyNonLeaves = 0;
  numGeometries = 0;
  for (auto * root = roots.first; root != nullptr; root = root->next) {
    updateCountsRecurse(root);
  }

}

namespace {

  void storeGroupIndexInGeometriesRecurse(Node* group)
  {
    for (auto * child = group->children.first; child != nullptr; child = child->next) {
      storeGroupIndexInGeometriesRecurse(child);
    }
    if (group->kind == Node::Kind::Group) {
      for (auto * geo = group->group.geometries.first; geo != nullptr; geo = geo->next) {
        geo->id = group->group.id;
        if (geo->triangulation) {
          geo->triangulation->id = group->group.id;
        }
      }
    }
  }
}


void Store::forwardGroupIdToGeometries()
{
  for (auto * root = roots.first; root != nullptr; root = root->next) {
    storeGroupIndexInGeometriesRecurse(root);
  }
}

