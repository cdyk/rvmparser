#pragma once
#include <cstdint>
#include "Common.h"

struct Group;
struct Geometry;

struct Contour
{
  float* vertices;
  float* normals;
  uint32_t vertices_n;
};

struct Polygon
{
  Contour* contours;
  uint32_t contours_n;
};

struct Triangulation {
  float* vertices = nullptr;
  float* normals = nullptr;
  uint32_t* indices = 0;
  uint32_t vertices_n = 0;
  uint32_t triangles_n = 0;
  float error = 0.f;
};

struct Composite;

struct Geometry
{
  enum struct Kind
  {
    Pyramid,
    Box,
    RectangularTorus,
    CircularTorus,
    EllipticalDish,
    SphericalDish,
    Snout,
    Cylinder,
    Sphere,
    Line,
    FacetGroup
  };
  Geometry* next = nullptr;                 // Next geometry in the list of geometries in group.
  Triangulation* triangulation = nullptr;
  Composite* composite = nullptr;           // Pointer to the composite this geometry belongs to.
  Geometry* next_comp = nullptr;            // Next geometry in list of geometries of this composite

  Geometry* conn_geo[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
  unsigned conn_off[6];

  Kind kind;
  unsigned index;

  float M_3x4[12];
  float bbox[6];

  union {
    struct {
      float bottom[2];
      float top[2];
      float offset[2];
      float height;
    } pyramid;
    struct {
      float lengths[3];
    } box;
    struct {
      float inner_radius;
      float outer_radius;
      float height;
      float angle;
    } rectangularTorus;
    struct {
      float offset;
      float radius;
      float angle;
    } circularTorus;
    struct {
      float diameter;
      float radius;
    } ellipticalDish;
    struct {
      float diameter;
      float height;
    } sphericalDish;
    struct {
      float offset[2];
      float bshear[2];
      float tshear[2];
      float radius_b;
      float radius_t;
      float height;
    } snout;
    struct {
      float radius;
      float height;
    } cylinder;
    struct {
      float diameter;
    } sphere;
    struct {
      float a, b;
    } line;
    struct {
      struct Polygon* polygons;
      uint32_t polygons_n;
    } facetGroup;
  };
};

template<typename T>
struct ListHeader
{
  T* first;
  T* last;
};


struct Composite
{
  Composite* next = nullptr;
  Geometry* first_geo = nullptr;

  float bbox[6];
  float size;
};

struct Group
{
  enum struct Kind
  {
    File,
    Model,
    Group
  };

  Group* next = nullptr;
  ListHeader<Group> groups;

  Kind kind = Kind::Group;
  union {
    struct {
      const char* info;
      const char* note;
      const char* date;
      const char* user;
      const char* encoding;
    } file;
    struct {
      const char* project;
      const char* name;
    } model;
    struct {
      ListHeader<Geometry> geometries;
      const char* name;
      uint32_t material;
      float translation[3];
    } group;
  };

};


class StoreVisitor;

class Store
{
public:
  Store();

  Geometry* newGeometry(Group* parent);

  Geometry* cloneGeometry(Group* parent, const Geometry* src);

  Group* newGroup(Group * parent, Group::Kind kind);

  Group* cloneGroup(Group* parent, const Group* src);

  Group* findRootGroup(const char* name);

  Composite* newComposite();

  void apply(StoreVisitor* visitor);

  unsigned geometryCount() const { return geo_n; }
  unsigned groupCount() const { return grp_n; }
  const char* errorString() const { return error_str; }
  void setErrorString(const char* str);

  Arena arena;
  Arena arenaTriangulation;
  struct Stats* stats = nullptr;
  struct Connectivity* conn = nullptr;

  StringInterning strings;
private:
  unsigned geo_n = 0;
  unsigned grp_n = 0;

  const char* error_str = nullptr;

  void apply(StoreVisitor* visitor, Group* group);

  ListHeader<Group> roots;
  ListHeader<Composite> comps;
  
};