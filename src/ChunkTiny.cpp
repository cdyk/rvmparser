#include <cassert>
#include "Store.h"
#include "Flatten.h"
#include "ChunkTiny.h"

ChunkTiny::ChunkTiny(Flatten& flatten, unsigned vertexThreshold) : flatten(flatten), vertexThreshold(vertexThreshold)
{
}

void ChunkTiny::init(class Store& store)
{
  stack = (StackItem*)arena.alloc(sizeof(StackItem)*store.groupCountAllocated());
  stack_p = 0;
}

void ChunkTiny::beginGroup(struct Group* group)
{
  auto & item = stack[stack_p++];
  item.group = group;
  item.vertices = 0;
  item.keep = false;
}

void ChunkTiny::geometry(struct Geometry* geometry)
{
  assert(stack_p);
  auto & item = stack[stack_p - 1];

  if (geometry->kind == Geometry::Kind::Line) {
    item.vertices += 2;
  }
  else {
    assert(geometry->triangulation);
    item.vertices += geometry->triangulation->vertices_n;
  }
}

void ChunkTiny::EndGroup()
{
  assert(stack_p);

  auto & item = stack[stack_p - 1];
  if (stack_p == 1 || (stack[stack_p - 2].keep) || vertexThreshold < item.vertices) {
    flatten.keepTag(item.group->group.name);
    for (unsigned i = 0; i < stack_p; i++) {
      stack[i].keep = true;
    }
  }
  else if(1 < stack_p) {
    stack[stack_p - 2].vertices += item.vertices;
  }

  stack_p--;
}
