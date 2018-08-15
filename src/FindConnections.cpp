#include <cstdio>
#include <cassert>
#include "FindConnections.h"
#include "Store.h"

namespace {

  void addAnchor(Anchor* anchors, unsigned& anchor_i, Geometry* geo, float* n, float* p, unsigned type, float radius)
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

    anchors[anchor_i].p[0] = Px;
    anchors[anchor_i].p[1] = Py;
    anchors[anchor_i].p[2] = Pz;
    anchors[anchor_i].n[0] = s*Nx;
    anchors[anchor_i].n[1] = s*Ny;
    anchors[anchor_i].n[2] = s*Nz;
    anchors[anchor_i].type = type;
    anchors[anchor_i].radius = radius;
    anchors[anchor_i].geo = geo;
  }

}

void FindConnections::init(class Store& store)
{
  assert(store.stats);

  anchor_n =
    2 * store.stats->cylinder_n;
  
  anchors = (Anchor*)arena.alloc(sizeof(Anchor)*anchor_n);
  anchor_i = 0;


}

void FindConnections::geometry(struct Geometry* geo)
{
  switch (geo->kind) {
  case Geometry::Kind::Pyramid:
    break;

  case Geometry::Kind::Box:
    break;

  case Geometry::Kind::RectangularTorus:
    break;

  case Geometry::Kind::CircularTorus: {
    auto c = std::cos(geo->circularTorus.angle);
    auto s = std::sin(geo->circularTorus.angle);
    float na[3] = { 0, -1, 0.f };
    float pa[3] = { geo->circularTorus.offset, 0, 0.f };
    addAnchor(anchors, anchor_i, geo, na, pa, 1, geo->circularTorus.radius);
    float nb[3] = { -s, c, 0.f };
    float pb[3] = { geo->circularTorus.offset * c, geo->circularTorus.offset * s, 0.5f * geo->cylinder.height };
    addAnchor(anchors, anchor_i, geo, na, pb, 1, geo->circularTorus.radius);
    break;
  }
  case Geometry::Kind::EllipticalDish:
    break;

  case Geometry::Kind::SphericalDish:
    break;

  case Geometry::Kind::Snout:
    break;

  case Geometry::Kind::Cylinder: {
    float na[3] = { 0, 0, -1 };
    float pa[3] = { 0, 0, -0.5f * geo->cylinder.height };
    addAnchor(anchors, anchor_i, geo, na, pa, 1, geo->cylinder.radius);
    float nb[3] = { 0, 0, 1 };
    float pb[3] = { 0, 0, 0.5f * geo->cylinder.height };
    addAnchor(anchors, anchor_i, geo, na, pb, 1, geo->cylinder.radius);
    break;
  }

  case Geometry::Kind::Sphere:
    break;

  case Geometry::Kind::FacetGroup:
    break;

  case Geometry::Kind::Line:
    break;

  default:
    assert(false && "Unhandled primitive type");
    break;
  }
}



bool FindConnections::done()
{


  return true;
}
