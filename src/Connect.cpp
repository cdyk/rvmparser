#include <cassert>
#include <algorithm>
#include <cmath>
#include <chrono>
#include "Common.h"
#include "Store.h"
#include "LinAlgOps.h"

using std::sin;
using std::cos;

namespace {
  uint32_t colors[6] =
  {
    0x0000AA,
    0x00AA00,
    0x00AAAA,
    0xAA0000,
    0xAA00AA,
    0xAA5500
  };

  struct Anchor
  {
    Geometry* geo = nullptr;
    Vec3f p;
    Vec3f d;
    unsigned o;
    Connection::Flags flags;
    uint8_t matched = 0;
  };

  struct Context
  {
    Store* store;
    Logger logger;
    Buffer<Anchor> anchors;
    const float epsilon = 0.001f;
    unsigned anchors_n = 0;

    unsigned anchors_max = 0;

    unsigned anchors_total = 0;
    unsigned anchors_matched = 0;
  };


  void connect(Context* context, unsigned off)
  {
    auto * a = context->anchors.data();
    auto a_n = context->anchors_n;
    auto e = context->epsilon;
    auto ee = e * e;
    assert(off <= a_n);

    std::sort(a + off, a + a_n, [](auto &a, auto& b) { return a.p.x < b.p.x; });

    for (unsigned j = off; j < a_n; j++) {
      if (a[j].matched) continue;

      for (unsigned i = j + 1; i < a_n && a[i].p.x <= a[j].p.x + e; i++) {

        bool canMatch = a[i].matched == false;
        bool close = distanceSquared(a[j].p, a[i].p) <= ee;
        bool aligned = dot(a[j].d, a[i].d) < -0.98f;

        if (canMatch && close && aligned) {

          auto * connection = context->store->newConnection();
          connection->geo[0] = a[j].geo;
          connection->geo[1] = a[i].geo;
          connection->offset[0] = a[j].o;
          connection->offset[1] = a[i].o;
          connection->p = a[j].p;
          connection->d = a[j].d;
          connection->flags = Connection::Flags::None;
          connection->setFlag(a[i].flags);
          connection->setFlag(a[j].flags);

          a[j].geo->connections[a[j].o] = connection;
          a[i].geo->connections[a[i].o] = connection;

          a[j].matched = true;
          a[i].matched = true;
          context->anchors_matched+=2;

          //context->store->addDebugLine((a[j].p + 0.03f*a[j].d).data,
          //                             (a[i].p + 0.03f*a[i].d).data,
          //                             0x0000ff);
        }
      }
    }

    // Remove matched anchors.
    for (unsigned j = off; j < a_n; ) {
      if (a[j].matched) {
        std::swap(a[j], a[--a_n]);
      }
      else {
        j++;
      }
    }
    assert(off <= a_n);

    context->anchors_n = a_n;
  }

  void addAnchor(Context* context, Geometry* geo, const Vec3f& p, const Vec3f& d, unsigned o, Connection::Flags flags)
  {

    Anchor a;
    a.geo = geo;
    a.p = mul(Mat3x4f(geo->M_3x4), p);
    a.d = normalize(mul(Mat3f(geo->M_3x4.data), d));
    a.o = o;
    a.flags = flags;

    //context->store->addDebugLine(a.p.data, (a.p + 0.02*a.d).data, 0x008800);

    assert(context->anchors_n < context->anchors_max);
    context->anchors[context->anchors_n++] = a;
    context->anchors_total++;
  }


  void recurse(Context* context, Group* group)
  {
    auto offset = context->anchors_n;
    for (auto * child = group->groups.first; child != nullptr; child = child->next) {
      recurse(context, child);
    }
    for (auto * geo = group->group.geometries.first; geo != nullptr; geo = geo->next) {
      switch (geo->kind) {

      case Geometry::Kind::Pyramid: {
        auto b = 0.5f * Vec2f(geo->pyramid.bottom);
        auto t = 0.5f * Vec2f(geo->pyramid.top);
        auto m = 0.5f * (b + t);
        auto o = 0.5f * Vec2f(geo->pyramid.offset);

        auto h = 0.5f * geo->pyramid.height;

        auto & M = geo->M_3x4;
        auto N = Mat3f(M.data);

        Vec3f n[6] = {
           Vec3f(0.f, -h,  (-t.y + o.y) - (-b.y - o.y)),
           Vec3f(h, 0.f, -((t.x + o.x) - (b.x - o.x))),
           Vec3f(0.f,  h, -((t.y + o.y) - (b.y - o.y))),
           Vec3f(-h, 0.f,  (-t.x + o.x) - (-b.x - o.x)),
           Vec3f(0.f, 0.f, -1.f),
           Vec3f(0.f, 0.f, 1.f)
        };
        Vec3f p[6] = {
          Vec3f(0.f, -m.y, 0.f),
          Vec3f(m.x, 0.f, 0.f),
          Vec3f(0.f, m.y, 0.f),
          Vec3f(-m.x, 0.f, 0.f),
          Vec3f(-o.x, -o.y, -h),
          Vec3f(o.x, o.y, h)
        };
        for (unsigned i = 0; i < 6; i++) {
          addAnchor(context, geo, p[i], n[i], i, Connection::Flags::HasRectangularSide);
        }
        break;
      }

      case Geometry::Kind::Box: {
        auto & box = geo->box;
        Vec3f n[6] = {
            Vec3f(-1,  0,  0), Vec3f(1,  0,  0),
            Vec3f(0, -1,  0), Vec3f(0,  1,  0),
            Vec3f(0,  0, -1), Vec3f(0,  0,  1)
        };
        auto xp = 0.5f * box.lengths[0]; auto xm = -xp;
        auto yp = 0.5f * box.lengths[1]; auto ym = -yp;
        auto zp = 0.5f * box.lengths[2]; auto zm = -zp;
        Vec3f p[6] = {
          Vec3f(xm, 0.f, 0.f ), Vec3f(xp, 0.f, 0.f ),
          Vec3f(0.f, ym, 0.f ), Vec3f(0.f, yp, 0.f ),
          Vec3f(0.f, 0.f, zm ), Vec3f(0.f, 0.f, zp )
        };
        for (unsigned i = 0; i < 6; i++) addAnchor(context, geo, p[i], n[i], i, Connection::Flags::HasRectangularSide);
        break;
      }

      case Geometry::Kind::RectangularTorus: {
        auto & rt = geo->rectangularTorus;
        auto c = cos(rt.angle);
        auto s = sin(rt.angle);
        auto m = 0.5f*(rt.inner_radius + rt.outer_radius);
        Vec3f n[2] = { Vec3f( 0, -1, 0.f ), Vec3f( -s, c, 0.f ) };
        Vec3f p[2] = { Vec3f( geo->circularTorus.offset, 0, 0.f ), Vec3f( m * c, m * s, 0.f ) };
        for (unsigned i = 0; i < 2; i++) addAnchor(context, geo, p[i], n[i], i, Connection::Flags::HasRectangularSide);
        break;
      }

      case Geometry::Kind::CircularTorus: {
        auto & ct = geo->circularTorus;
        auto c = cos(ct.angle);
        auto s = sin(ct.angle);
        Vec3f n[2] = { Vec3f(0, -1, 0.f ), Vec3f(-s, c, 0.f ) };
        Vec3f p[2] = { Vec3f(ct.offset, 0, 0.f ), Vec3f(ct.offset * c, ct.offset * s, 0.f ) };
        for (unsigned i = 0; i < 2; i++) addAnchor(context, geo, p[i], n[i], i, Connection::Flags::HasCircularSide);
        break;
      }

      case Geometry::Kind::EllipticalDish:
      case Geometry::Kind::SphericalDish: {
        addAnchor(context, geo, Vec3f(0,0,0), Vec3f(0, 0, -1), 0, Connection::Flags::HasCircularSide);
        break;
      }

      case Geometry::Kind::Snout: {
        auto & sn = geo->snout;
        Vec3f n[2] = {
          Vec3f(sin(sn.bshear[0])*cos(sn.bshear[1]), sin(sn.bshear[1]), -cos(sn.bshear[0])*cos(sn.bshear[1]) ),
          Vec3f(-sin(sn.tshear[0])*cos(sn.tshear[1]), -sin(sn.tshear[1]), cos(sn.tshear[0])*cos(sn.tshear[1]))
        };
        Vec3f p[2] = {
          Vec3f(-0.5f*sn.offset[0], -0.5f*sn.offset[1], -0.5f*sn.height ),
          Vec3f(0.5f*sn.offset[0], 0.5f*sn.offset[1], 0.5f*sn.height )
        };
        for (unsigned i = 0; i < 2; i++) addAnchor(context, geo, p[i], n[i], i, Connection::Flags::HasCircularSide);
        break;
      }

      case Geometry::Kind::Cylinder: {
        Vec3f d[2] = { Vec3f(0, 0, -1.f), Vec3f(0, 0, 1.f) };
        Vec3f p[2] = { Vec3f(0, 0, -0.5f * geo->cylinder.height), Vec3f(0, 0, 0.5f * geo->cylinder.height) };
        for (unsigned i = 0; i < 2; i++) addAnchor(context, geo, p[i], d[i], i, Connection::Flags::HasCircularSide);
        break;
      }

      case Geometry::Kind::Sphere:
      case Geometry::Kind::FacetGroup:
      case Geometry::Kind::Line:
        break;

      default:
        assert(false && "Unhandled primitive type");
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

  context.anchors_max = 6*store->geometryCountAllocated();
  context.anchors.accommodate(context.anchors_max);


  auto time0 = std::chrono::high_resolution_clock::now();
  context.anchors_n = 0;
  for (auto * root = store->getFirstRoot(); root != nullptr; root = root->next) {
    for (auto * model = root->groups.first; model != nullptr; model = model->next) {
      for (auto * group = model->groups.first; group != nullptr; group = group->next) {
        recurse(&context, group);
      }
    }
  }
  for (unsigned i = 0; i < context.anchors_n; i++) {
    auto & a = context.anchors[i];
    assert(a.matched == false);

    auto b = a.p + 0.02f*a.d;

    //if (a.geo->kind == Geometry::Kind::Pyramid) {
    //  context.store->addDebugLine(a.p.data, b.data, 0x003300);
    //}
    //else {
    //  context.store->addDebugLine(a.p.data, b.data, 0xff0000);
    //}
  }
  auto time1 = std::chrono::high_resolution_clock::now();
  auto e0 = std::chrono::duration_cast<std::chrono::milliseconds>((time1 - time0)).count();

  logger(0, "Matched %u of %u anchors (%lldms).", context.anchors_matched, context.anchors_total, e0);

}
