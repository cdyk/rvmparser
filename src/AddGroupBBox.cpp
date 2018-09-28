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
  auto * bbox = stack[stack_p - 1]->group.bbox;
  for (unsigned k = 0; k < 8; k++) {
    Vec3f p((k & 1) ? geometry->bbox.min[0] : geometry->bbox.max[3],
            (k & 2) ? geometry->bbox.min[1] : geometry->bbox.max[4],
            (k & 4) ? geometry->bbox.min[2] : geometry->bbox.max[5]);

    auto P = mul(geometry->M_3x4, p);

    for (unsigned i = 0; i < 3; i++) {
      bbox[0 + i] = std::min(bbox[0 + i], P[i]);
      bbox[3 + i] = std::max(bbox[3 + i], P[i]);
    }
  }
}

void AddGroupBBox::beginGroup(struct Group* group)
{
  group->group.bbox = (float*)store->arena.alloc(sizeof(float) * 6);
  for (unsigned i = 0; i < 3; i++) {
    group->group.bbox[0 + i] = FLT_MAX;
    group->group.bbox[3 + i] = -FLT_MAX;
  }

  stack[stack_p] = group;
  stack_p++;
}

void AddGroupBBox::EndGroup()
{
  assert(stack_p);
  stack_p--;

  auto * bbox = stack[stack_p]->group.bbox;
  if (bbox[3] < bbox[0]) {
    // no geometry
    stack[stack_p]->group.bbox = nullptr;
  }
  else if (0 < stack_p) {
    auto * parentBox = stack[stack_p - 1]->group.bbox;

    for (unsigned i = 0; i < 3; i++) {
      parentBox[0 + i] = std::min(parentBox[0 + i], bbox[0 + i]);
      parentBox[3 + i] = std::max(parentBox[3 + i], bbox[3 + i]);
    }
  }

}
