#include <climits>
#include <algorithm>
#include <cassert>
#include "Store.h"
#include "AddGroupBBox.h"
#include "LinAlgOps.h"

void AddGroupBBox::init(class Store& store_)
{
  store = &store_;
  stack = (Node**)arena.alloc(sizeof(Node*)*store->groupCountAllocated());
  stack_p = 0;
}

void AddGroupBBox::geometry(struct Geometry* geometry)
{
  assert(stack_p);
  engulf(stack[stack_p - 1]->group.bboxWorld, geometry->bboxWorld);
}

void AddGroupBBox::beginGroup(struct Node* group)
{
  group->group.bboxWorld = createEmptyBBox3f();
  stack[stack_p] = group;
  stack_p++;
}

void AddGroupBBox::EndGroup()
{
  assert(stack_p);
  stack_p--;

  auto & bbox = stack[stack_p]->group.bboxWorld;
  if (!isEmpty(bbox) && 0 < stack_p) {
    auto & parentBox = stack[stack_p - 1]->group.bboxWorld;
    engulf(parentBox, bbox);
  }
}
