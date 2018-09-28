#pragma once
#include <cstdint>
#include "Common.h"
#include "LinAlg.h"

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

struct Connection
{
  Connection* next = nullptr;
  Geometry* geo[2] = { nullptr, nullptr };
  unsigned offset[2];
  Vec3f p;
  Vec3f d;
  unsigned temp;
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
  Geometry* next = nullptr;                 // Next geometry in the list of geometries in group.
  Triangulation* triangulation = nullptr;
  Geometry* next_comp = nullptr;            // Next geometry in list of geometries of this composite

  Connection* connections[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
  const char* colorName = nullptr;
  uint32_t color = 0x505050u;

  Kind kind;
  unsigned id;

  Mat3x4f M_3x4;
  BBox3f bbox;

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

struct Attribute
{
  Attribute* next = nullptr;
  const char* key = nullptr;
  const char* val = nullptr;
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
  ListHeader<Attribute> attributes;

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
      float* bbox;
      const char* name;
      uint32_t material;
      float translation[3];
    } group;
  };

};


struct DebugLine
{
  DebugLine* next = nullptr;
  float a[3];
  float b[3];
  uint32_t color = 0xff0000u;
};


class StoreVisitor;

class Store
{
public:
  Store();

  Geometry* newGeometry(Group* parent);

  Geometry* cloneGeometry(Group* parent, const Geometry* src);

  Group* getDefaultModel();

  Group* newGroup(Group * parent, Group::Kind kind);

  Group* cloneGroup(Group* parent, const Group* src);

  Group* findRootGroup(const char* name);

  Attribute* getAttribute(Group* group, const char* key);

  Attribute* newAttribute(Group* group, const char* key);

  void addDebugLine(float* a, float* b, uint32_t color);

  Connection* newConnection();

  void apply(StoreVisitor* visitor);

  unsigned groupCountAllocated() const { return numGroupsAllocated; }
  unsigned geometryCountAllocated() const { return numGeometriesAllocated; }

  const char* errorString() const { return error_str; }
  void setErrorString(const char* str);

  Group* getFirstRoot() { return roots.first; }
  Connection* getFirstConnection() { return connections.first; }
  DebugLine* getFirstDebugLine() { return debugLines.first; }

  Arena arena;
  Arena arenaTriangulation;
  struct Stats* stats = nullptr;
  struct Connectivity* conn = nullptr;

  StringInterning strings;
private:
  unsigned numGroupsAllocated = 0;
  unsigned numGeometriesAllocated = 0;

  const char* error_str = nullptr;

  void apply(StoreVisitor* visitor, Group* group);

  ListHeader<Group> roots;
  ListHeader<DebugLine> debugLines;
  ListHeader<Connection> connections;
  
};