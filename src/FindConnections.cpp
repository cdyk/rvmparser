#include <cstdio>
#include <cassert>
#include "FindConnections.h"
#include "AddStats.h"
#include "Store.h"

namespace {

  void addAnchor(Connectivity* conn, unsigned& anchor_i, Geometry* geo, float* n, float* p)
  {

    const auto & M = geo->M_3x4;

    float Px, Py, Pz, Nx, Ny, Nz;
    Px = M[0] * p[0] + M[3] * p[1] + M[6] * p[2] + M[9];
    Py = M[1] * p[0] + M[4] * p[1] + M[7] * p[2] + M[10];
    Pz = M[2] * p[0] + M[5] * p[1] + M[8] * p[2] + M[11];
    Nx = M[0] * n[0] + M[3] * n[1] + M[6] * n[2];
    Ny = M[1] * n[0] + M[4] * n[1] + M[7] * n[2];
    Nz = M[2] * n[0] + M[5] * n[1] + M[8] * n[2];

    auto s = 1.f / std::sqrt(Nx*Nx + Ny * Ny + Nz * Nz);
    assert(std::isfinite(s));

    conn->p[3 * anchor_i + 0] = Px;
    conn->p[3 * anchor_i + 1] = Py;
    conn->p[3 * anchor_i + 2] = Pz;

    conn->anchors[anchor_i].geo = geo;
    conn->anchors[anchor_i].n[0] = s * Nx;
    conn->anchors[anchor_i].n[1] = s * Ny;
    conn->anchors[anchor_i].n[2] = s * Nz;
    conn->anchors[anchor_i].p_ix = anchor_i;

    anchor_i++;
  }

}

void FindConnections::init(class Store& store)
{
  assert(store.stats);
  store.conn = store.arena.alloc<Connectivity>();
  conn = store.conn;

  conn->anchor_n =
    6 * store.stats->pyramid_n +
    6 * store.stats->box_n +
    2 * store.stats->rectangular_torus_n +
    2 * store.stats->circular_torus_n +
    1 * store.stats->elliptical_dish_n +
    1 * store.stats->spherical_dish_n +
    2 * store.stats->snout_n +
    2 * store.stats->cylinder_n;

  conn->anchors = (Anchor*)store.arena.alloc(sizeof(Anchor)*conn->anchor_n);

  conn->p_n = conn->anchor_n;
  conn->p = (float*)store.arena.alloc(3 * sizeof(float) * conn->p_n);

  anchor_i = 0;
}

void FindConnections::geometry(struct Geometry* geo)
{
  switch (geo->kind) {
  case Geometry::Kind::Pyramid: {
    auto bx = 0.5f * geo->pyramid.bottom[0];
    auto by = 0.5f * geo->pyramid.bottom[1];
    auto tx = 0.5f * geo->pyramid.top[0];
    auto ty = 0.5f * geo->pyramid.top[1];
    auto ox = 0.5f * geo->pyramid.offset[0];
    auto oy = 0.5f * geo->pyramid.offset[1];
    auto h2 = 0.5f * geo->pyramid.height;

    float n[6][3] = {
      { 0.f, -h2,  (-ty + oy) - (-by - oy) },
      {  h2, 0.f, -((tx + ox) - (bx - ox)) },
      { 0.f,  h2, -((ty + oy) - (by - oy)) },
      { -h2, 0.f,  (-tx + ox) - (-bx - ox) },
      { 0.f, 0.f, -1.f},
      { 0.f, 0.f, 1.f}
    };
    float p[6][3] = {
      { 0.f, -0.5f*(by + ty), 0.f },
      { 0.5f*(bx + tx), 0.f, 0.f },
      { 0.f, 0.5f*(by + ty), 0.f },
      { -0.5f*(bx + tx), 0.f, 0.f },
      { -ox, -oy, -h2},
      { ox, oy, h2}
    };
    for (unsigned i = 0; i < 6; i++) {
      addAnchor(conn, anchor_i, geo, n[i], p[i]);
    }
    break;
  }

  case Geometry::Kind::Box: {
    auto & box = geo->box;
    float n[6][3] = {
        { -1,  0,  0 }, {  1,  0,  0 },
        {  0, -1,  0 }, {  0,  1,  0 },
        {  0,  0, -1 }, {  0,  0,  1 }
    };
    auto xp = 0.5f * box.lengths[0]; auto xm = -xp;
    auto yp = 0.5f * box.lengths[1]; auto ym = -yp;
    auto zp = 0.5f * box.lengths[2]; auto zm = -zp;
    float p[6][3] = {
      { xm, 0.f, 0.f }, { xp, 0.f, 0.f },
      { 0.f, ym, 0.f }, { 0.f, yp, 0.f },
      { 0.f, 0.f, zm }, { 0.f, 0.f, zp }
    };
    for (unsigned i = 0; i < 6; i++) {
      addAnchor(conn, anchor_i, geo, n[i], p[i]);
    }
    break;
  }

  case Geometry::Kind::RectangularTorus: {
    auto & rt = geo->rectangularTorus;
    auto c = std::cos(rt.angle);
    auto s = std::sin(rt.angle);
    auto m = 0.5f*(rt.inner_radius + rt.outer_radius);
    float n[2][3] = { { 0, -1, 0.f }, { -s, c, 0.f } };
    float p[2][3] = {{ geo->circularTorus.offset, 0, 0.f }, { m * c, m * s, 0.f }};
    for (unsigned i = 0; i < 2; i++) {
      addAnchor(conn, anchor_i, geo, n[i], p[i]);
    }
    break;
  }

  case Geometry::Kind::CircularTorus: {
    auto & ct = geo->circularTorus;
    auto c = std::cos(ct.angle);
    auto s = std::sin(ct.angle);
    float n[2][3] = { { 0, -1, 0.f }, { -s, c, 0.f } };
    float p[2][3] = { { ct.offset, 0, 0.f }, { ct.offset * c, ct.offset * s, 0.f } };
    for (unsigned i = 0; i < 2; i++) {
      addAnchor(conn, anchor_i, geo, n[i], p[i]);
    }
    break;
  }

  case Geometry::Kind::EllipticalDish:
  case Geometry::Kind::SphericalDish: {
    float n[3] = { 0, 0, -1 };
    float p[3] = { 0, 0, 0.f };
    addAnchor(conn, anchor_i, geo, n, p);
    break;
  }

  case Geometry::Kind::Snout: {
    auto & sn = geo->snout;
    float n[2][3] = {
      { std::sin(sn.bshear[0])*std::cos(sn.bshear[1]), std::sin(sn.bshear[1]), -std::cos(sn.bshear[0])*std::cos(sn.bshear[1]) },
      { -std::sin(sn.tshear[0])*std::cos(sn.tshear[1]), -std::sin(sn.tshear[1]), std::cos(sn.tshear[0])*std::cos(sn.tshear[1]) }
    };
    float p[2][3] = {
      { -0.5f*sn.offset[0], -0.5f*sn.offset[1], -0.5f*sn.height },
      { 0.5f*sn.offset[0], 0.5f*sn.offset[1], 0.5f*sn.height }
    };
    for (unsigned i = 0; i < 2; i++) {
      addAnchor(conn, anchor_i, geo, n[i], p[i]);
    }
    break;
  }

  case Geometry::Kind::Cylinder: {
    float n[2][3] = { { 0, 0, -1 }, { 0, 0, 1 } };
    float p[2][3] = { { 0, 0, -0.5f * geo->cylinder.height }, { 0, 0, 0.5f * geo->cylinder.height } };
    for (unsigned i = 0; i < 2; i++) {
      addAnchor(conn, anchor_i, geo, n[i], p[i]);
    }
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



bool FindConnections::done()
{
  assert(conn->anchor_n == anchor_i);
  return true;
}
