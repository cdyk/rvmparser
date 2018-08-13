#pragma once
#include <cstdint>

#include "RVMVisitor.h"

struct Group;
struct Geometry;


struct Geometry
{
  Geometry* next = nullptr;
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


class Store
{
public:
  Store();
  ~Store();

  Geometry* newGeometry(Group* parent);

  Group* newGroup(Group * parent, Group::Kind kind);

  void* alloc(size_t bytes);

  void apply(RVMVisitor* visitor);

private:
  void* arena;

  void apply(RVMVisitor* visitor, Group* group);

  ListHeader<Group> roots;

  
};