#include <cassert>
#include <cmath>
#include "Common.h"
#include "Store.h"

namespace {

  inline float dot3(const float *a, const float *b)
  {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  }

  inline float length3(const float* v)
  {
    return std::sqrt(dot3(v, v));
  }

  inline void add3(float* dst, const float *a, const float *b)
  {
    for (unsigned k = 0; k < 3; k++) dst[k] = a[k] + b[k];
  }

  inline void transformPos3(float* r, const float *M, const float *p)
  {
    for (unsigned k = 0; k < 3; k++) {
      r[k] = M[k]*p[0] + M[3+k]*p[1] + M[6+k]*p[2] + M[9+k];
    }
  }

  inline void transformDir3(float* r, const float *M, const float *d)
  {
    for (unsigned k = 0; k < 3; k++) {
      r[k] = M[k]*d[0] + M[3+k]*d[1] + M[6+k]*d[2];
    }
  }


  struct Anchor
  {
    Geometry* geo;
    float p[3]; // origin
    float r;    // bounding sphere radius
    float d[3]; // direction
    unsigned o;
  };

  struct Context
  {
    Store* store;
    Logger logger;
    Buffer<Anchor> anchors;
    unsigned anchors_n = 0;
  };


  void connect(Context* context)
  {
    for (unsigned i = 0; i < context->anchors_n; i++) {
      auto & a = context->anchors[i];

      float b[3];
      add3(b, a.p, a.d);

      context->store->addDebugLine(a.p, b, 0xff0000);
    }
    context->anchors_n = 0;
  }

  void addAnchor(Context* context, Geometry* geo, float* p, float* d, unsigned o)
  {
    const auto & M = geo->M_3x4;

    Anchor a;
    a.geo = geo;
    transformPos3(a.p, geo->M_3x4, p);
    transformDir3(a.d, geo->M_3x4, d);
    a.r = length3(a.d);
    a.o = o;

    context->anchors[context->anchors_n++] = a;
  }

  void recurse(Context* context, Group* group)
  {
    for (auto * child = group->groups.first; child != nullptr; child = child->next) {
      recurse(context, child);
    }
    for (auto * geo = group->group.geometries.first; geo != nullptr; geo = geo->next) {
      switch (geo->kind) {
      case Geometry::Kind::Cylinder: {
        float d[2][3] = { { 0, 0, -geo->cylinder.radius }, { 0, 0, geo->cylinder.radius } };
        float p[2][3] = { { 0, 0, -0.5f * geo->cylinder.height }, { 0, 0, 0.5f * geo->cylinder.height } };
        for (unsigned i = 0; i < 2; i++) {
          addAnchor(context, geo, d[i], p[i], i);
        }
        break;
      }
      default:
        break;
      }
    }
    connect(context);
  }



}


void connect(Store* store, Logger logger)
{
  Context context;
  context.store = store;
  context.logger = logger;
  context.anchors.accommodate(store->geometryCountAllocated());

  for (auto * root = store->getFirstRoot(); root != nullptr; root = root->next) {
    for (auto * model = root->groups.first; model != nullptr; model = model->next) {
      for (auto * group = model->groups.first; group != nullptr; group = group->next) {
        recurse(&context, group);
      }
    }
  }
}
