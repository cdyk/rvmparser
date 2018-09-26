#include <cstdio>
#include <cassert>
#include <algorithm>
#include "FindConnections.h"
#include "AddStats.h"
#include "Store.h"

void FindConnections::addAnchor(Geometry* geo, float* n, float* p, unsigned o)
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

  points[anchor_i] = { Px, Py, Pz, anchor_i };
  anchors[anchor_i] = { geo, s*Nx, s*Ny, s*Nz, o };

  anchor_i++;
}


void FindConnections::init(class Store& store)
{
  this->store = &store;

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

  points = (Point*)arena.alloc(sizeof(Point)*conn->anchor_n);
  anchors = (AnchorRef*)arena.alloc(sizeof(AnchorRef)*conn->anchor_n);

  nodes = (Geometry**)arena.alloc(sizeof(Geometry*) * store.stats->geometry_n);

  uscratch = (unsigned*)arena.alloc(sizeof(unsigned)*conn->anchor_n);

  anchor_i = 0;
}

void FindConnections::geometry(struct Geometry* geo)
{
  nodes[geo->id] = geo;

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
      addAnchor(geo, n[i], p[i], i);
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
      addAnchor(geo, n[i], p[i], i);
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
      addAnchor(geo, n[i], p[i], i);
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
      addAnchor(geo, n[i], p[i], i);
    }
    break;
  }

  case Geometry::Kind::EllipticalDish:
  case Geometry::Kind::SphericalDish: {
    float n[3] = { 0, 0, -1 };
    float p[3] = { 0, 0, 0.f };
    addAnchor(geo, n, p, 0);
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
      addAnchor(geo, n[i], p[i], i);
    }
    break;
  }

  case Geometry::Kind::Cylinder: {
    float n[2][3] = { { 0, 0, -1 }, { 0, 0, 1 } };
    float p[2][3] = { { 0, 0, -0.5f * geo->cylinder.height }, { 0, 0, 0.5f * geo->cylinder.height } };
    for (unsigned i = 0; i < 2; i++) {
      addAnchor(geo, n[i], p[i], i);
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

void FindConnections::uniquePointsRecurse(Point* range, unsigned N)
{
  if (N == 0) return;

  float cmin[3], cmax[3];
  for (unsigned k = 0; k < 3; k++) {
    cmin[k] = cmax[k] = range[0].p[k];
  }
  for (unsigned i = 1; i < N; i++) {
    for (unsigned k = 0; k < 3; k++) {
      cmin[k] = std::min(cmin[k], range[i].p[k]);
      cmax[k] = std::max(cmax[k], range[i].p[k]);
    }
  }
  float d[3];
  for (unsigned k = 0; k < 3; k++) {
    d[k] = cmax[k] - cmin[k];
  }
  unsigned axis = 0;
  for (unsigned k = 1; k < 3; k++) {
    if (d[axis] < d[k])  axis = k;
  }
  auto split = 0.5f*(cmin[axis] + cmax[axis]);
  assert(cmin[axis] <= split);
  assert(split <= cmax[axis]);

  unsigned a = 0;
  unsigned b = N;

  while (true) {
    bool progress = false;
    while ((a < N) && (range[a].p[axis] < split)) a++;
    while ((0 < b) && (split <= range[b - 1].p[axis])) b--;
    if (a == b) break;

    assert(split <= range[a].p[axis]);
    assert(range[b - 1].p[axis] < split);

    std::swap(range[a], range[b - 1]);

    assert(range[a].p[axis] < split);
    assert(split <= range[b - 1].p[axis]);
  }

  for (unsigned i = 0; i < a; i++) {
    assert(range[i].p[axis] <= split);
  }
  for (unsigned i = a; i < N; i++) {
    assert(split <= range[i].p[axis]);
  }

  if (d[axis] <= epsilon || a == 0 || b == N) {
    registerUniquePoint(range, N, d[axis]);
  }
  else {
    assert(a != 0 && b != N);
    assert(range[a - 1].p[axis] < split);
    assert(split <= range[a].p[axis]);

    uniquePointsRecurse(range, a);
    uniquePointsRecurse(range + a, N - a);
  }
}

void FindConnections::registerUniquePoint(Point* range, unsigned N, float d)
{
  if (1e-7f < d) {
    fprintf(stderr, "%d %f\n", N, d);
  }

  //if (N < 2) return;
  //for (unsigned i = 0; i + 1 < N; i += 2) {
  //  connect(anchors + range[i].ix, anchors + range[i+1].ix);
  //}

  auto * indices = uscratch;
  for (unsigned i = 0; i < N; i++) {
    indices[i] = range[i].ix;
  }

  for (unsigned i = 0; i < N; i++) {
    float best_match = -1.f;
    unsigned best_ix_j;

    auto ix_i = indices[i];

    for (unsigned j = i + 1; j < N; j++) {
      auto ix_j = indices[j];

      auto match = 0.f;
      for (unsigned k = 0; k < 3; k++) {
        match -= anchors[ix_i].n[k] * anchors[ix_j].n[k];
        if (best_match < match) {
          best_match = match;
          best_ix_j = ix_j;
        }
      }
    }
    if (0.f <= best_match) {
      connect(anchors + ix_i, anchors + best_ix_j);
      std::swap(indices[best_ix_j], indices[N - 1]);
      --N;
    }
  }

  unique_points_i++;
}


void FindConnections::connect(AnchorRef* a0, AnchorRef* a1)
{
  a0->geo->conn_geo[a0->o] = a1->geo;
  a0->geo->conn_off[a0->o] = a1->o;

  a1->geo->conn_geo[a1->o] = a0->geo;
  a1->geo->conn_off[a1->o] = a0->o;
}

bool FindConnections::done()
{
  conn->anchor_n = anchor_i;

  assert(conn->anchor_n == anchor_i);

  auto N = conn->anchor_n;
  for (unsigned j = 0; j < anchor_i; j++) {
    auto * p0 = points[j].p;
    auto * n0 = anchors[points[j].ix].n;
    float best_dist = std::numeric_limits<float>::max();
    unsigned best_ix = ~0;
    for (unsigned i = j + 1; i < N; i++) {
      auto * p1 = points[i].p;
      auto * n1 = anchors[points[i].ix].n;
      float dot = n0[0] * n1[0] + n0[1] * n1[1] + n0[2] * n1[2];
      if (0.9f < -dot) {
        float dx = p0[0] - p1[0];
        float dy = p0[1] - p1[1];
        float dz = p0[2] - p1[2];
        float dist = dx * dx + dy * dy + dz * dz;
        if (dist < best_dist) {
          best_dist = dist;
          best_ix = i;
        }
      }
    }

    if (best_dist < epsilon*epsilon) {
      connect(anchors + points[j].ix, anchors + points[best_ix].ix);
      std::swap(points[best_ix], points[N - 1]);
      --N;
    }
  }

  //uniquePointsRecurse(points, anchor_i);
  //fprintf(stderr, "unique points=%d of %d\n", unique_points_i, anchor_i);

  findConnectedComponents();

  return true;
}

void FindConnections::findConnectedComponents()
{
  auto N = store->stats->geometry_n;
  Geometry** stack = (Geometry**)arena.alloc(sizeof(Geometry*) * N);
  uint8_t* visited = (uint8_t*)arena.alloc(sizeof(uint8_t)*N);
  std::memset(visited, 0, sizeof(uint8_t)*N);

  unsigned component_n = 0;
  for (unsigned i = 0; i < N; i++) {
    if (visited[i]) continue;

    auto * comp = store->newComposite();
    comp->bbox[0] = comp->bbox[1] = comp->bbox[2] = std::numeric_limits<float>::max();
    comp->bbox[3] = comp->bbox[4] = comp->bbox[5] = -std::numeric_limits<float>::max();

    stack[0] = nodes[i];
    component_n++;

    unsigned stack_i = 1;
    while (stack_i) {
      auto * g = stack[--stack_i];

      g->composite = comp;
      g->next_comp = comp->first_geo;
      comp->first_geo = g;

      visited[g->id] = 1;

      const auto & M = g->M_3x4;
      for (unsigned k = 0; k < 8; k++) {
        float px = (k & 1) ? g->bbox[0] : g->bbox[3];
        float py = (k & 2) ? g->bbox[1] : g->bbox[4];
        float pz = (k & 4) ? g->bbox[2] : g->bbox[5];

        float Px = M[0] * px + M[3] * py + M[6] * pz + M[9];
        float Py = M[1] * px + M[4] * py + M[7] * pz + M[10];
        float Pz = M[2] * px + M[5] * py + M[8] * pz + M[11];
        comp->bbox[0] = std::min(comp->bbox[0], Px);
        comp->bbox[1] = std::min(comp->bbox[1], Py);
        comp->bbox[2] = std::min(comp->bbox[2], Pz);
        comp->bbox[3] = std::max(comp->bbox[3], Px);
        comp->bbox[4] = std::max(comp->bbox[4], Py);
        comp->bbox[5] = std::max(comp->bbox[5], Pz);
      }

      for (unsigned k = 0; k < 6; k++) {
        if (g->conn_geo[k] && (visited[g->conn_geo[k]->id] == 0)) stack[stack_i++] = g->conn_geo[k];
      }
    }


    auto dx = comp->bbox[3] - comp->bbox[0];
    auto dy = comp->bbox[4] - comp->bbox[1];
    auto dz = comp->bbox[5] - comp->bbox[2];

    comp->size = std::max(std::max(dx, dy), dz);
  }
  fprintf(stderr, "connected components: %d\n", component_n);


}
