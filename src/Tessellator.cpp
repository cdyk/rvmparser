#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cassert>
#include "tesselator.h"

#include "Store.h"
#include "Tessellator.h"

namespace {

  const float pi = float(M_PI);
  const float half_pi = float(0.5*M_PI);
  const float one_over_pi = float(1.0 / M_PI);
  const float twopi = float(2.0*M_PI);

  unsigned triIndices(uint32_t* indices, unsigned  l, unsigned o, unsigned v0, unsigned v1, unsigned v2)
  {
    indices[l++] = o + v0;
    indices[l++] = o + v1;
    indices[l++] = o + v2;
    return l;
  }


  unsigned quadIndices(uint32_t* indices, unsigned  l, unsigned o, unsigned v0, unsigned v1, unsigned v2, unsigned v3)
  {
    indices[l++] = o + v0;
    indices[l++] = o + v1;
    indices[l++] = o + v2;

    indices[l++] = o + v2;
    indices[l++] = o + v3;
    indices[l++] = o + v0;
    return l;
  }

  unsigned vertex(float* normals, float* vertices, unsigned l, float* n, float* p)
  {
    normals[l] = n[0]; vertices[l++] = p[0];
    normals[l] = n[1]; vertices[l++] = p[1];
    normals[l] = n[2]; vertices[l++] = p[2];
    return l;
  }

  unsigned vertex(float* normals, float* vertices, unsigned l, float nx, float ny, float nz, float px, float py, float pz)
  {
    normals[l] = nx; vertices[l++] = px;
    normals[l] = ny; vertices[l++] = py;
    normals[l] = nz; vertices[l++] = pz;
    return l;
  }

}

void Tessellator::init(class Store& store)
{
  arena = &store.arenaTriangulation;

  arena->clear();
}


void Tessellator::geometry(Geometry* geo)
{
  switch (geo->kind) {
  case Geometry::Kind::Pyramid:
    //pyramid(geometry);
    break;

  case Geometry::Kind::Box:
    box(geo);
    break;

  case Geometry::Kind::FacetGroup:
    //facetGroup(geometry);
    break;

  default:
    geo->triangulation = nullptr;
    break;
  }


}


void Tessellator::pyramid(Geometry* geo)
{
  bool cap0 = 1e-7f <= std::min(std::abs(geo->pyramid.bottom[0]), std::abs(geo->pyramid.bottom[1]));
  bool cap1 = 1e-7f <= std::min(std::abs(geo->pyramid.top[0]), std::abs(geo->pyramid.top[1]));

  auto bx = 0.5f * geo->pyramid.bottom[0];
  auto by = 0.5f * geo->pyramid.bottom[1];
  auto tx = 0.5f * geo->pyramid.top[0];
  auto ty = 0.5f * geo->pyramid.top[1];
  auto ox = 0.5f * geo->pyramid.offset[0];
  auto oy = 0.5f * geo->pyramid.offset[1];
  auto h2 = 0.5f * geo->pyramid.height;

  float quad[2][4][3] =
  {
    {
      { -bx - ox, -by - oy, -h2 },
      {  bx - ox, -by - oy, -h2 },
      {  bx - ox,  by - oy, -h2 },
      { -bx - ox,  by - oy, -h2 }
    },
    {
      { -tx + ox, -ty + oy, h2 },
      {  tx + ox, -ty + oy, h2 },
      {  tx + ox,  ty + oy, h2 },
      { -tx + ox,  ty + oy, h2 }
    },
  };

  float n[4][3] = {
    { 0.f, -h2,  (quad[1][0][1] - quad[0][0][1]) },
    {  h2, 0.f, -(quad[1][1][0] - quad[0][1][0]) },
    { 0.f,  h2, -(quad[1][2][1] - quad[0][2][1]) },
    { -h2, 0.f,  (quad[1][3][0] - quad[0][3][0]) },
  };

  auto * tri = arena->alloc<Triangulation>();
  geo->triangulation = tri;

  tri->vertices_n = 4 * (4 + (cap0 ? 1 : 0) + (cap1 ? 1 : 0));
  tri->triangles_n = 2 * (4 + (cap0 ? 1 : 0) + (cap1 ? 1 : 0));

  tri->vertices = (float*)arena->alloc(3 * sizeof(float)*tri->vertices_n);
  tri->normals = (float*)arena->alloc(3 * sizeof(float)*tri->vertices_n);

  unsigned l = 0;
  for (unsigned i = 0; i < 4; i++) {
    unsigned ii = (i + 1) & 3;
    l = vertex(tri->normals, tri->vertices, l, n[i], quad[0][i]);
    l = vertex(tri->normals, tri->vertices, l, n[i], quad[0][ii]);
    l = vertex(tri->normals, tri->vertices, l, n[i], quad[1][ii]);
    l = vertex(tri->normals, tri->vertices, l, n[i], quad[1][i]);
  }
  if (cap0) {
    float n[3] = { 0, 0, -1 };
    for (unsigned i = 0; i < 4; i++) {
      l = vertex(tri->normals, tri->vertices, l, n, quad[0][i]);
    }
  }
  if (cap1) {
    float n[3] = { 0, 0, 1 };
    for (unsigned i = 0; i < 4; i++) {
      l = vertex(tri->normals, tri->vertices, l, n, quad[1][i]);
    }
  }
  assert(l == 3*tri->vertices_n);

  l = 0;
  tri->indices = (uint32_t*)arena->alloc(3 * sizeof(uint32_t) * tri->triangles_n);
  for (unsigned i = 0; i < 4; i++) {
    l = quadIndices(tri->indices, l, 4 * i, 0, 1, 2, 3);
  }
  unsigned o = 4 * 4;
  if (cap0) {
    l = quadIndices(tri->indices, l, o, 3, 2, 1, 0);
    o += 4;
  }
  if (cap1) {
    l = quadIndices(tri->indices, l, o, 0, 1, 2, 3);
    o += 4;
  }
  assert(l == 3 * tri->triangles_n);
  assert(o == tri->vertices_n);
}


void Tessellator::box(Geometry* geo)
{
  auto & box = geo->box;


  auto xp = 0.5f * box.lengths[0]; auto xm = -xp;
  auto yp = 0.5f * box.lengths[1]; auto ym = -yp;
  auto zp = 0.5f * box.lengths[2]; auto zm = -zp;

  bool faces[6] = {
    1e-5 <= box.lengths[0],
    1e-5 <= box.lengths[0],
    1e-5 <= box.lengths[1],
    1e-5 <= box.lengths[1],
    1e-5 <= box.lengths[2],
    1e-5 <= box.lengths[2],
  };

  float V[6][4][3] = {
    { { xm, ym, zp }, { xm, yp, zp }, { xm, yp, zm }, { xm, ym, zm } },
    { { xp, ym, zm }, { xp, yp, zm }, { xp, yp, zp }, { xp, ym, zp } },
    { { xp, ym, zm }, { xp, ym, zp }, { xm, ym, zp }, { xm, ym, zm } },
    { { xm, yp, zm }, { xm, yp, zp }, { xp, yp, zp }, { xp, yp, zm } },
    { { xm, yp, zm }, { xp, yp, zm }, { xp, ym, zm }, { xm, ym, zm } },
    { { xm, ym, zp }, { xp, ym, zp }, { xp, yp, zp }, { xm, yp, zp } }
  };

  float N[6][3] = {
    { -1,  0,  0 },
    {  1,  0,  0 },
    {  0, -1,  0 },
    {  0,  1,  0 },
    {  0,  0, -1 },
    {  0,  0,  1 }
  };

  unsigned faces_n = 0;
  for(unsigned i=0; i<6; i++) {
    if (faces[i]) faces_n++;
  }
  Triangulation* tri = nullptr;
  if (faces_n) {
    tri = arena->alloc<Triangulation>();
    tri->vertices_n = 4 * faces_n;
    tri->vertices = (float*)arena->alloc(3 * sizeof(float)*tri->vertices_n);
    tri->normals = (float*)arena->alloc(3 * sizeof(float)*tri->vertices_n);

    tri->triangles_n = 2 * faces_n;
    tri->indices = (uint32_t*)arena->alloc(3 * sizeof(uint32_t)*tri->triangles_n);

    unsigned o = 0;
    unsigned i_v = 0;
    unsigned i_p = 0;
    for (unsigned f = 0; f < 6; f++) {
      if (!faces[f]) continue;

      for (unsigned i = 0; i < 4; i++) {
        i_v = vertex(tri->normals, tri->vertices, i_v, N[f], V[f][i]);
      }
      i_p = quadIndices(tri->indices, i_p, o, 0, 1, 2, 3);

      o += 4;
    }
    assert(i_v == 3 * tri->vertices_n);
    assert(i_p == 3 * tri->triangles_n);
    assert(o == tri->vertices_n);
  }


  geo->triangulation = tri;
}

#if 0


void TriangulatedVisitor::sphere(float* affine, float* bbox, float diameter)
{
  sphereBasedShape(affine, bbox, 0.5f*diameter, pi, 0.f, 1.f);
}

void TriangulatedVisitor::rectangularTorus(float* affine, float* bbox, float inner_radius, float outer_radius, float height, float angle)
{
  unsigned samples = 7;
  bool shell = true;
  bool cap0 = true;
  bool cap1 = true;

  auto h2 = 0.5f*height;
  float square[4][2] = {
    { outer_radius, -h2 },
    { inner_radius, -h2 },
    { inner_radius, h2 },
    { outer_radius, h2 },
  };


  t0.resize(2 * samples);
  for (unsigned i = 0; i < samples; i++) {
    t0[2 * i + 0] = std::cos((angle / (samples - 1.f))*i);
    t0[2 * i + 1] = std::sin((angle / (samples - 1.f))*i);
  }

  unsigned l = 0;
  vertices.resize(3 * ((shell ? 4 * 2 * samples : 0) + (cap0 ? 4 : 0) + (cap1 ? 4 : 0)));
  normals.resize(vertices.size());
  if (shell) {
    for (unsigned i = 0; i < samples; i++) {
      float n[4][3] = {
        {0.f, 0.f, -1.f},
        {-t0[2 * i + 0], -t0[2 * i + 1], 0.f},
        {0.f, 0.f, 1.f},
        { t0[2 * i + 0], t0[2 * i + 1], 0.f},
      };

      for (unsigned k = 0; k < 4; k++) {
        unsigned kk = (k + 1) & 3;

        normals[l] = n[k][0]; vertices[l++] = square[k][0] * t0[2 * i + 0];
        normals[l] = n[k][1]; vertices[l++] = square[k][0] * t0[2 * i + 1];
        normals[l] = n[k][2]; vertices[l++] = square[k][1];

        normals[l] = n[k][0]; vertices[l++] = square[kk][0] * t0[2 * i + 0];
        normals[l] = n[k][1]; vertices[l++] = square[kk][0] * t0[2 * i + 1];
        normals[l] = n[k][2]; vertices[l++] = square[kk][1];
      }
    }
  }
  if (cap0) {
    for (unsigned k = 0; k < 4; k++) {
      normals[l] =  0.f; vertices[l++] = square[k][0] * t0[0];
      normals[l] = -1.f; vertices[l++] = square[k][0] * t0[1];
      normals[l] =  0.f; vertices[l++] = square[k][1];
    }
  }
  if (cap1) {
    for (unsigned k = 0; k < 4; k++) {
      normals[l] = -t0[2 * (samples - 1) + 1]; vertices[l++] = square[k][0] * t0[2 * (samples - 1) + 0];
      normals[l] =  t0[2 * (samples - 1) + 0]; vertices[l++] = square[k][0] * t0[2 * (samples - 1) + 1];
      normals[l] = 0.f; vertices[l++] = square[k][1];
    }
  }
  assert(l == vertices.size());

  l = 0;
  unsigned o = 0;
  indices.resize(3 * ((shell ? 4 * 2 * (samples - 1) : 0) + (cap0 ? 2 : 0) + (cap1 ? 2 : 0)));
  if (shell) {
    for (unsigned i = 0; i + 1 < samples; i++) {
      for (unsigned k = 0; k < 4; k++) {
        indices[l++] = 4 * 2 * (i + 0) + 0 + 2 * k;
        indices[l++] = 4 * 2 * (i + 0) + 1 + 2 * k;
        indices[l++] = 4 * 2 * (i + 1) + 0 + 2 * k;

        indices[l++] = 4 * 2 * (i + 1) + 0 + 2 * k;
        indices[l++] = 4 * 2 * (i + 0) + 1 + 2 * k;
        indices[l++] = 4 * 2 * (i + 1) + 1 + 2 * k;
      }
    }
    o += 4 * 2 * samples;
  }
  if (cap0) {
    indices[l++] = o + 0;
    indices[l++] = o + 2;
    indices[l++] = o + 1;
    indices[l++] = o + 2;
    indices[l++] = o + 0;
    indices[l++] = o + 3;
    o += 4;
  }
  if (cap1) {
    indices[l++] = o + 0;
    indices[l++] = o + 1;
    indices[l++] = o + 2;
    indices[l++] = o + 2;
    indices[l++] = o + 3;
    indices[l++] = o + 0;
    o += 4;
  }
  assert(3 * o == vertices.size());
  assert(l == indices.size());

  triangles(affine, bbox, vertices, normals, indices);
}

void TriangulatedVisitor::circularTorus(float* affine, float* bbox, float offset, float radius, float angle)
{
  unsigned samples_l = 7; // large radius, toroidal direction
  unsigned samples_s = 5; // small radius, poloidal direction

  bool shell = true;
  bool cap0 = true;
  bool cap1 = true;


  t0.resize(2 * samples_l);
  for (unsigned i = 0; i < samples_l; i++) {
    t0[2 * i + 0] = std::cos((angle / (samples_l - 1.f))*i);
    t0[2 * i + 1] = std::sin((angle / (samples_l - 1.f))*i);
  }

  t1.resize(2 * samples_s);
  for (unsigned i = 0; i < samples_s; i++) {
    t1[2 * i + 0] = std::cos((twopi / samples_s)*i);
    t1[2 * i + 1] = std::sin((twopi / samples_s)*i);
  }

  // generate vertices
  unsigned l = 0;
  vertices.resize(3 * ((shell ? samples_l : 0) + (cap0 ? 1 : 0) + (cap1 ? 1 : 0)) * samples_s);
  normals.resize(vertices.size());
  if (shell) {
    for (unsigned u = 0; u < samples_l; u++) {
      for (unsigned v = 0; v < samples_s; v++) {
        normals[l] = t1[2 * v + 0] * t0[2 * u + 0]; vertices[l++] = ((radius * t1[2 * v + 0] + offset) * t0[2 * u + 0]);
        normals[l] = t1[2 * v + 0] * t0[2 * u + 1]; vertices[l++] = ((radius * t1[2 * v + 0] + offset) * t0[2 * u + 1]);
        normals[l] = t1[2 * v + 1]; vertices[l++] = radius * t1[2 * v + 1];
      }
    }
  }
  if (cap0) {
    for (unsigned v = 0; v < samples_s; v++) {
      normals[l] = 0.f; vertices[l++] = ((radius * t1[2 * v + 0] + offset) * t0[0]);
      normals[l] = -1.f; vertices[l++] = ((radius * t1[2 * v + 0] + offset) * t0[1]);
      normals[l] = 0.f; vertices[l++] = radius * t1[2 * v + 1];
    }
  }
  if (cap1) {
    unsigned m = 2 * (samples_l - 1);
    for (unsigned v = 0; v < samples_s; v++) {
      normals[l] = -t0[m + 1]; vertices[l++] = ((radius * t1[2 * v + 0] + offset) * t0[m + 0]);
      normals[l] = t0[m + 0]; vertices[l++] = ((radius * t1[2 * v + 0] + offset) * t0[m + 1]);
      normals[l] = 0.f; vertices[l++] = radius * t1[2 * v + 1];
    }
  }
  assert(l == vertices.size());

  // generate indices
  l = 0;
  unsigned o = 0;
  indices.resize((shell ? 6 * (samples_l - 1)*samples_s : 0) + 3 * (samples_s - 2) *((cap0 ? 1 : 0) + (cap1 ? 1 : 0)));
  if (shell) {
    for (unsigned u = 0; u + 1 < samples_l; u++) {
      for (unsigned v = 0; v + 1 < samples_s; v++) {
        indices[l++] = samples_s * (u + 0) + (v + 0);
        indices[l++] = samples_s * (u + 1) + (v + 0);
        indices[l++] = samples_s * (u + 1) + (v + 1);

        indices[l++] = samples_s * (u + 1) + (v + 1);
        indices[l++] = samples_s * (u + 0) + (v + 1);
        indices[l++] = samples_s * (u + 0) + (v + 0);
      }
      indices[l++] = samples_s * (u + 0) + (samples_s - 1);
      indices[l++] = samples_s * (u + 1) + (samples_s - 1);
      indices[l++] = samples_s * (u + 1) + 0;
      indices[l++] = samples_s * (u + 1) + 0;
      indices[l++] = samples_s * (u + 0) + 0;
      indices[l++] = samples_s * (u + 0) + (samples_s - 1);
    }
    o += samples_l * samples_s;
  }
  if (cap0) {
    for (unsigned i = 1; i + 1 < samples_s; i++) {
      indices[l++] = o + 0;
      indices[l++] = o + i;
      indices[l++] = o + i + 1;
    }
    o += samples_s;
  }
  if (cap1) {
    for (unsigned i = 1; i + 1 < samples_s; i++) {
      indices[l++] = o + 0;
      indices[l++] = o + i + 1;
      indices[l++] = o + i;
    }
    o += samples_s;
  }
  assert(l == indices.size());
  assert(3 * o == vertices.size());

  triangles(affine, bbox, vertices, normals, indices);
}

void TriangulatedVisitor::ellipticalDish(float* affine, float* bbox, float diameter, float radius)
{
  sphereBasedShape(affine, bbox, 0.5f*diameter, half_pi, 0.f, 2.f * radius / diameter);
}

void TriangulatedVisitor::sphericalDish(float* affine, float* bbox, float diameter, float height)
{
  float r_circ = 0.5f*diameter;
  float r_sphere = (r_circ*r_circ + height * height) / (2.f*height);
  float arc = asin(r_circ / r_sphere);
  if (r_circ < height) { arc = pi - arc; }

  sphereBasedShape(affine, bbox, r_sphere, arc, height - r_sphere, 1.f);
}


void TriangulatedVisitor::sphereBasedShape(float* affine, float* bbox, float radius, float arc, float shift_z, float scale_z)
{
  unsigned samples = 5;

  bool is_sphere = false;
  if (pi - 1e-3 <= arc) {
    arc = pi;
    is_sphere = true;
  }

  unsigned min_rings = 3;// arc <= half_pi ? 2 : 3;
  unsigned rings = unsigned(std::max(float(min_rings), scale_z * samples*arc*(1.f / twopi)));
  
  u0.resize(rings);
  t0.resize(2 * rings);
  auto theta_scale = arc / (rings - 1);
  for (unsigned r = 0; r < rings; r++) {
    float theta = theta_scale * r;
    t0[2 * r + 0] = std::cos(theta);
    t0[2 * r + 1] = std::sin(theta);
    u0[r] = unsigned(std::max(3.f, t0[2 * r + 1] * samples));  // samples in this ring
  }
  u0[0] = 1;
  if (is_sphere) {
    u0[rings - 1] = 1;
  }

  unsigned s = 0;
  for (unsigned r = 0; r < rings; r++) {
    s += u0[r];
  }

  unsigned l = 0;
  vertices.resize(3 * s);
  normals.resize(vertices.size());
  for (unsigned r = 0; r < rings; r++) {
    auto nz = t0[2 * r + 0];
    auto z = radius * scale_z * nz + shift_z;
    auto w = t0[2 * r + 1];
    auto n = u0[r];

    auto phi_scale = twopi / n;
    for (unsigned i = 0; i < n; i++) {
      auto phi = phi_scale * i;
      auto nx = w * std::cos(phi);
      auto ny = w * std::sin(phi);
      l = vertex(normals, vertices, l, nx, ny, nz / scale_z, radius*nx, radius*ny, z);
    }
  }
  assert(l == 3 * s);

  unsigned o_c = 0;
  indices.clear();
  for (unsigned r = 0; r + 1 < rings; r++) {
    auto n_c = u0[r];
    auto n_n = u0[r + 1];
    auto o_n = o_c + n_c;

    if (n_c < n_n) {
      for (unsigned i_n = 0; i_n < n_n; i_n++) {
        unsigned ii_n = (i_n + 1);
        unsigned i_c = (n_c*(i_n + 1)) / n_n;
        unsigned ii_c = (n_c*(ii_n + 1)) / n_n;

        i_c %= n_c;
        ii_c %= n_c;
        ii_n %= n_n;

        if (i_c != ii_c) {
          indices.push_back(o_c + i_c);
          indices.push_back(o_n + ii_n);
          indices.push_back(o_c + ii_c);
        }
        assert(i_n != ii_n);
        indices.push_back(o_c + i_c);
        indices.push_back(o_n + i_n);
        indices.push_back(o_n + ii_n);
      }
    }
    else {
      for (unsigned i_c = 0; i_c < n_c; i_c++) {
        auto ii_c = (i_c + 1);
        unsigned i_n = (n_n*(i_c + 0)) / n_c;
        unsigned ii_n = (n_n*(ii_c + 0)) / n_c;

        i_n %= n_n;
        ii_n %= n_n;
        ii_c %= n_c;

        assert(i_c != ii_c);
        indices.push_back(o_c + i_c);
        indices.push_back(o_n + ii_n);
        indices.push_back(o_c + ii_c);

        if (i_n != ii_n) {
          indices.push_back(o_c + i_c);
          indices.push_back(o_n + i_n);
          indices.push_back(o_n + ii_n);
        }
      }
    }
    o_c = o_n;
  }
  triangles(affine, bbox, vertices, normals, indices);
}


void TriangulatedVisitor::cylinder(float* affine, float* bbox, float radius, float height)
{ 
  unsigned samples = 5;

  bool shell = true;
  bool cap0 = true;
  bool cap1 = true;

  vertices.resize(3 * ((shell ? 2 * samples : 0) + (cap0 ? samples : 0) + (cap1 ? samples : 0)));
  normals.resize(vertices.size());


  t0.resize(2 * samples);
  for (unsigned i = 0; i < samples; i++) {
    t0[2 * i + 0] = std::cos((twopi / samples)*i);
    t0[2 * i + 1] = std::sin((twopi / samples)*i);
  }
  t1.resize(2 * samples);
  for (unsigned i = 0; i < 2 * samples; i++) {
    t1[i] = radius * t0[i];
  }

  float h2 = 0.5f*height;
  unsigned l = 0;

  if (shell) {
    for (unsigned i = 0; i < samples; i++) {
      l = vertex(normals, vertices, l, t0[2 * i + 0], t0[2 * i + 1], 0, t1[2 * i + 0], t1[2 * i + 1], -h2);
      l = vertex(normals, vertices, l, t0[2 * i + 0], t0[2 * i + 1], 0, t1[2 * i + 0], t1[2 * i + 1], h2);
    }
  }
  if (cap0) {
    for (unsigned i = 0; cap0 && i < samples; i++) {
      l = vertex(normals, vertices, l, 0, 0, -1, t1[2 * i + 0], t1[2 * i + 1], -h2);
    }
  }
  if (cap1) {
    for (unsigned i = 0; i < samples; i++) {
      l = vertex(normals, vertices, l, 0, 0, 1, t1[2 * i + 0], t1[2 * i + 1], h2);
    }
  }

  l = 0;
  unsigned o = 0;
  indices.resize(3 * ((shell ? 2 * samples : 0) + (cap0 ? samples - 2 : 0) + (cap1 ? samples - 2 : 0)));
  if (shell) {
    for (unsigned i = 0; i < samples; i++) {
      unsigned ii = (i + 1) % samples;
      l = quadIndices(indices, l, 0, 2 * i, 2 * ii, 2 * ii + 1, 2 * i + 1);
    }
    o += 2 * samples;
  }
  if (cap0) {
    for (unsigned i = 1; i + 1 < samples; i++) {
      l = triIndices(indices, l, o, 0, i + 1, i);
    }
    o += samples;
  }
  if (cap1) {
    for (unsigned i = 1; i + 1 < samples; i++) {
      l = triIndices(indices, l, o, 0, i, i + 1);
    }
    o += samples;
  }
  assert(l == indices.size());
  assert(3 * o == vertices.size());

  triangles(affine, bbox, vertices, normals, indices);
}

void TriangulatedVisitor::snout(float* affine, float*bbox, float* offset_xy, float* bshear, float* tshear, float radius_b, float radius_t, float height)
{
  unsigned samples = 5;

  bool shell = true;
  bool cap0 = true;
  bool cap1 = true;

  t0.resize(2 * samples);
  for (unsigned i = 0; i < samples; i++) {
    t0[2 * i + 0] = std::cos((twopi / samples)*i);
    t0[2 * i + 1] = std::sin((twopi / samples)*i);
  }
  t1.resize(2 * samples);
  for (unsigned i = 0; i < 2 * samples; i++) {
    t1[i] = radius_b * t0[i];
  }
  t2.resize(2 * samples);
  for (unsigned i = 0; i < 2 * samples; i++) {
    t2[i] = radius_t * t0[i];
  }

  float h2 = height;
  unsigned l = 0;
  auto ox = 0.5f*offset_xy[0];
  auto oy = 0.5f*offset_xy[1];
  float mb[2] = { std::tan(bshear[0]), std::tan(bshear[1]) };
  float mt[2] = { std::tan(tshear[0]), std::tan(tshear[1]) };
  vertices.resize(3 * ((shell ? 2 * samples : 0) + (cap0 ? samples : 0) + (cap1 ? samples : 0)));
  normals.resize(vertices.size());
  if (shell) {
    for (unsigned i = 0; i < samples; i++) {
      float xb = t1[2 * i + 0] - ox;
      float yb = t1[2 * i + 1] - oy;
      float zb = -h2 + mb[0] * t1[2 * i + 0] + mb[1] * t1[2 * i + 1];

      float xt = t2[2 * i + 0] + ox;
      float yt = t2[2 * i + 1] + oy;
      float zt = h2 + mt[0] * t2[2 * i + 0] + mt[1] * t2[2 * i + 1];

      float s = (offset_xy[0] * t0[2 * i + 0] + offset_xy[1] * t0[2 * i + 1]);
      float nx = t0[2 * i + 0];
      float ny = t0[2 * i + 1];
      float nz = -(radius_t - radius_b + s) / height;

      l = vertex(normals, vertices, l, nx, ny, nz, xb, yb, zb);
      l = vertex(normals, vertices, l, nx, ny, nz, xt, yt, zt);
    }
  }
  if (cap0) {
    auto nx = std::sin(bshear[0])*std::cos(bshear[1]);
    auto ny = std::sin(bshear[1]);
    auto nz = -std::cos(bshear[0])*std::cos(bshear[1]);
    for (unsigned i = 0; cap0 && i < samples; i++) {
      l = vertex(normals, vertices, l, nx, ny, nz,
                 t1[2 * i + 0] - ox,
                 t1[2 * i + 1] - oy,
                 -h2 + mb[0] * t1[2 * i + 0] + mb[1] * t1[2 * i + 1]);
    }
  }
  if (cap1) {
    auto nx = -std::sin(tshear[0])*std::cos(tshear[1]);
    auto ny = -std::sin(tshear[1]);
    auto nz = std::cos(tshear[0])*std::cos(tshear[1]);
    for (unsigned i = 0; i < samples; i++) {
      l = vertex(normals, vertices, l, nx, ny, nz,
                 t2[2 * i + 0] + ox,
                 t2[2 * i + 1] + oy,
                 h2 + mt[0] * t2[2 * i + 0] + mt[1] * t2[2 * i + 1]);
    }
  }

  l = 0;
  unsigned o = 0;
  indices.resize(3 * ((shell ? 2 * samples : 0) + (cap0 ? samples - 2 : 0) + (cap1 ? samples - 2 : 0)));
  if (shell) {
    for (unsigned i = 0; i < samples; i++) {
      unsigned ii = (i + 1) % samples;
      l = quadIndices(indices, l, 0, 2 * i, 2 * ii, 2 * ii + 1, 2 * i + 1);
    }
    o += 2 * samples;
  }
  if (cap0) {
    for (unsigned i = 1; i + 1 < samples; i++) {
      l = triIndices(indices, l, o, 0, i + 1, i);
    }
    o += samples;
  }
  if (cap1) {
    for (unsigned i = 1; i + 1 < samples; i++) {
      l = triIndices(indices, l, o, 0, i, i + 1);
    }
    o += samples;
  }
  assert(l == indices.size());
  assert(3 * o == vertices.size());

  triangles(affine, bbox, vertices, normals, indices);
}


#endif

void Tessellator::facetGroup(struct Geometry* geo)
{
  auto & fg = geo->facetGroup;

  vertices.clear();
  indices.clear();
  for (unsigned p = 0; p < fg.polygons_n; p++) {
    auto & poly = fg.polygons[p];

    auto tess = tessNewTess(nullptr);
    for (unsigned c = 0; c < poly.contours_n; c++) {
      auto & cont = poly.coutours[c];
      tessAddContour(tess, 3, cont.vertices, 3 * sizeof(float), cont.vertices_n);
    }

    if (tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 3, nullptr)) {
      auto vo = uint32_t(vertices.size()) / 3;
      auto vn = unsigned(tessGetVertexCount(tess));
 
      vertices.resize(vertices.size() + 3 * vn);
      std::memcpy(vertices.data() + 3 * vo, tessGetVertices(tess), 3 * vn * sizeof(float));

      auto * remap = tessGetVertexIndices(tess);
      normals.resize(vertices.size());
      for (unsigned i = 0; i < vn; i++) {

        if (remap[i] != TESS_UNDEF) {
          unsigned ix = remap[i];
          for (unsigned c = 0; c < poly.contours_n; c++) {
            auto & cont = poly.coutours[c];
            if (ix < cont.vertices_n) {
              normals[3 * (vo + i) + 0] = cont.normals[3 * ix + 0];
              normals[3 * (vo + i) + 1] = cont.normals[3 * ix + 1];
              normals[3 * (vo + i) + 2] = cont.normals[3 * ix + 2];
              break;
            }
            ix -= cont.vertices_n;
          }
        }

        auto io = uint32_t(indices.size());
        auto * elements = tessGetElements(tess);
        auto elements_n = unsigned(tessGetElementCount(tess));
        for (unsigned e = 0; e < elements_n; e++) {
          auto ix = elements + 3 * e;
          if ((ix[0] != TESS_UNDEF) && (ix[1] != TESS_UNDEF) && (ix[2] != TESS_UNDEF)) {
            indices.push_back(ix[0] + vo);
            indices.push_back(ix[1] + vo);
            indices.push_back(ix[2] + vo);
          }
        }
      }
    }
  }

  assert(vertices.size() == normals.size());

  Triangulation* tri = nullptr;
  if (!indices.empty()) {
    tri = arena->alloc<Triangulation>();
    tri->vertices_n = uint32_t(vertices.size() / 3);
    tri->triangles_n =  uint32_t(indices.size() / 3);

    tri->vertices = (float*)arena->dup(vertices.data(), sizeof(float)*vertices.size());
    tri->normals = (float*)arena->dup(normals.data(), sizeof(float)*normals.size());
    tri->indices = (uint32_t*)arena->dup(indices.data(), sizeof(uint32_t)*indices.size());
  }
  geo->triangulation = tri;
}

