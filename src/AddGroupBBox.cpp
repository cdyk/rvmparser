#include <climits>
#include <algorithm>
#include <cassert>
#include "Store.h"
#include "AddGroupBBox.h"

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
    float px = (k & 1) ? geometry->bbox[0] : geometry->bbox[3];
    float py = (k & 2) ? geometry->bbox[1] : geometry->bbox[4];
    float pz = (k & 4) ? geometry->bbox[2] : geometry->bbox[5];

    float P[3] = {
      M[0] * px + M[3] * py + M[6] * pz + M[9],
      M[1] * px + M[4] * py + M[7] * pz + M[10],
      M[2] * px + M[5] * py + M[8] * pz + M[11],
    };
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
