#include <climits>
#include <algorithm>
#include <cassert>
#include "Store.h"
#include "AddGroupBBox.h"
#include "LinAlgOps.h"

void AddGroupBBox::init(class Store& store)
{
  this->store = &store;
  stack = (Group**)arena.alloc(sizeof(Group*)*store.groupCountAllocated());
  stack_p = 0;
}

void AddGroupBBox::geometry(struct Geometry* geometry)
{
  assert(stack_p);
  const auto & M = geometry->M_3x4;
  auto & bbox = stack[stack_p - 1]->group.bbox;
  for (unsigned k = 0; k < 8; k++) {
    Vec3f p((k & 1) ? geometry->bbox.min[0] : geometry->bbox.max[3],
            (k & 2) ? geometry->bbox.min[1] : geometry->bbox.max[4],
            (k & 4) ? geometry->bbox.min[2] : geometry->bbox.max[5]);

    engulf(bbox, mul(geometry->M_3x4, p));
  }
}

void AddGroupBBox::beginGroup(struct Group* group)
{
  group->group.bbox = createEmptyBBox3f();
  stack[stack_p] = group;
  stack_p++;
}

void AddGroupBBox::EndGroup()
{
  assert(stack_p);
  stack_p--;

  auto & bbox = stack[stack_p]->group.bbox;
  if (!isEmpty(bbox) && 0 < stack_p) {
    auto & parentBox = stack[stack_p - 1]->group.bbox;
    engulf(parentBox, bbox);
  }
}
