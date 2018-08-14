#pragma once
#include <cstdint>

#include "RVMVisitor.h"

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
  Contour* coutours;
  uint32_t contours_n;
};

struct Triangulation {
  float* vertices = nullptr;
  float* normals = nullptr;
  uint32_t* indices = 0;
  uint32_t vertices_n = 0;
  uint32_t triangles_n = 0;
};


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
  Geometry* next = nullptr;
  Triangulation* triangulation = nullptr;
  Kind kind;

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

struct Arena
{
  ~Arena() { clear(); }

  uint8_t * first = nullptr;
  uint8_t * curr = nullptr;
  size_t fill = 0;
  size_t size = 0;

  void* alloc(size_t bytes);
  void* dup(void* src, size_t bytes);
  void clear();

  template<typename T> T * alloc() { return new(alloc(sizeof(T))) T(); }
};


class Store
{
public:
  Store();

  Geometry* newGeometry(Group* parent);

  Group* newGroup(Group * parent, Group::Kind kind);


  void apply(RVMVisitor* visitor);

  Arena arena;
  Arena arenaTriangulation;
private:

  void apply(RVMVisitor* visitor, Group* group);

  ListHeader<Group> roots;

  
};