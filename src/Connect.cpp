#include <cassert>
#include <algorithm>
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

  inline float distanceSquared3(const float *a, const float *b)
  {
    auto dx = b[0] - a[0];
    auto dy = b[1] - a[1];
    auto dz = b[2] - a[2];
    return dx * dx + dy * dy + dz * dz;
  }

  inline void add3(float* dst, const float *a, const float *b)
  {
    for (unsigned k = 0; k < 3; k++) dst[k] = a[k] + b[k];
  }

  inline void madd3(float* dst, const float *a, const float *b, const float c)
  {
    for (unsigned k = 0; k < 3; k++) dst[k] = a[k] + c*b[k];
  }

  inline void scale3(float* dst, float s)
  {
    for (unsigned k = 0; k < 3; k++) dst[k] *= s;
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
    const float epsilon = 0.001f;
    unsigned anchors_n = 0;
  };


  void connect(Context* context, unsigned offset)
  {



    for (unsigned j = offset; j < context->anchors_n; j++) {
      auto & aj = context->anchors[j];
      for (unsigned i = j + 1; i < context->anchors_n; i++) {
        auto & ai = context->anchors[i];


        if (distanceSquared3(aj.p, ai.p) < context->epsilon) {
          if (dot3(aj.d, ai.d) < -1.f + 0.01f) {

            float b[3];
            madd3(b, aj.p, aj.d, 0.05f);
            context->store->addDebugLine(aj.p, b, 0x0000ff);

            madd3(b, ai.p, ai.d, 0.05f);
            context->store->addDebugLine(aj.p, b, 0x0000ff);

          }
        }

      }
    }


    /*for (unsigned i = 0; i < context->anchors_n; i++) {
      auto & a = context->anchors[i];

      float b[3];
      add3(b, a.p, a.d);

      context->store->addDebugLine(a.p, b, 0xff0000);
    }*/
    context->anchors_n = offset;
  }

  void addCircularAnchor(Context* context, Geometry* geo, float* p, float* d, float r, unsigned o)
  {
    const auto & M = geo->M_3x4;

    Anchor a;
    a.geo = geo;
    transformPos3(a.p, geo->M_3x4, p);
    transformDir3(a.d, geo->M_3x4, d);
    auto scale = length3(a.d);
    scale3(a.d, 1.f / scale);
    a.r = std::max(0.01f, scale * r);
    a.o = o;

    context->anchors[context->anchors_n++] = a;
  }

  void recurse(Context* context, Group* group)
  {
    auto offset = context->anchors_n;
    for (auto * child = group->groups.first; child != nullptr; child = child->next) {
      recurse(context, child);
    }
    for (auto * geo = group->group.geometries.first; geo != nullptr; geo = geo->next) {
      switch (geo->kind) {
      case Geometry::Kind::Cylinder: {
        float d[2][3] = { { 0, 0, -1.f }, { 0, 0, 1.f } };
        float p[2][3] = { { 0, 0, -0.5f * geo->cylinder.height }, { 0, 0, 0.5f * geo->cylinder.height } };
        for (unsigned i = 0; i < 2; i++) {
          addCircularAnchor(context, geo, p[i], d[i], geo->cylinder.radius, i);
        }
        break;
      }
      default:
        break;
      }
    }
    connect(context, offset);
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
