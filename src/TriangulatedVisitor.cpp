#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cassert>
#include "tesselator.h"

#include "TriangulatedVisitor.h"

namespace {

  const float twopi = float(2.0*M_PI);

  unsigned triIndices(std::vector<uint32_t>& indices, unsigned  l, unsigned o, unsigned v0, unsigned v1, unsigned v2)
  {
    indices[l++] = o + v0;
    indices[l++] = o + v1;
    indices[l++] = o + v2;
    return l;
  }


  unsigned quadIndices(std::vector<uint32_t>& indices, unsigned  l, unsigned o, unsigned v0, unsigned v1, unsigned v2, unsigned v3)
  {
    indices[l++] = o + v0;
    indices[l++] = o + v1;
    indices[l++] = o + v2;

    indices[l++] = o + v2;
    indices[l++] = o + v3;
    indices[l++] = o + v0;
    return l;
  }

  unsigned vertex(std::vector<float>& normals, std::vector<float>& vertices, unsigned l, float* n, float* p)
  {
    normals[l] = n[0]; vertices[l++] = p[0];
    normals[l] = n[1]; vertices[l++] = p[1];
    normals[l] = n[2]; vertices[l++] = p[2];
    return l;
  }

  unsigned vertex(std::vector<float>& normals, std::vector<float>& vertices, unsigned l, float nx, float ny, float nz, float px, float py, float pz)
  {
    normals[l] = nx; vertices[l++] = px;
    normals[l] = ny; vertices[l++] = py;
    normals[l] = nz; vertices[l++] = pz;
    return l;
  }

}


void TriangulatedVisitor::pyramid(float* affine, float* bbox, float* bottom_xy, float* top_xy, float* offset_xy, float height) 
{
  bool cap0 = 1e-7f <= std::min(std::abs(bottom_xy[0]), std::abs(bottom_xy[1]));
  bool cap1 = 1e-7f <= std::min(std::abs(top_xy[0]), std::abs(top_xy[1]));

  auto bx = 0.5f*bottom_xy[0];
  auto by = 0.5f*bottom_xy[1];
  auto tx = 0.5f*top_xy[0];
  auto ty = 0.5f*top_xy[1];
  auto ox = 0.5f*offset_xy[0];
  auto oy = 0.5f*offset_xy[1];
  auto h2 = 0.5f*height;

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

  unsigned l = 0;
  vertices.resize(3 * 4 * (4 + (cap0 ? 1 : 0) + (cap1 ? 1 : 0)));
  normals.resize(vertices.size());
  for (unsigned i = 0; i < 4; i++) {
    unsigned ii = (i + 1) & 3;
    l = vertex(normals, vertices, l, n[i], quad[0][i]);
    l = vertex(normals, vertices, l, n[i], quad[0][ii]);
    l = vertex(normals, vertices, l, n[i], quad[1][ii]);
    l = vertex(normals, vertices, l, n[i], quad[1][i]);
  }
  if (cap0) {
    float n[3] = { 0, 0, -1 };
    for (unsigned i = 0; i < 4; i++) {
      l = vertex(normals, vertices, l, n, quad[0][i]);
    }
  }
  if (cap1) {
    float n[3] = { 0, 0, 1 };
    for (unsigned i = 0; i < 4; i++) {
      l = vertex(normals, vertices, l, n, quad[1][i]);
    }
  }
  assert(l == vertices.size());


  l = 0;
  indices.resize(6 * (4 + (cap0 ? 1 : 0) + (cap1 ? 1 : 0)));
  for (unsigned i = 0; i < 4; i++) {
    l = quadIndices(indices, l, 4 * i, 0, 1, 2, 3);
  }
  unsigned o = 4 * 4;
  if (cap0) {
    l = quadIndices(indices, l, o, 3, 2, 1, 0);
    o += 4;
  }
  if (cap1) {
    l = quadIndices(indices, l, o, 0, 1, 2, 3);
    o += 4;
  }
  assert(l == indices.size());
  assert(3*o == vertices.size());

  //triangles(affine, bbox, vertices, normals, indices);
}

void TriangulatedVisitor::box(float* affine, float* bbox, float* lengths)
{
  auto xp = 0.5f * lengths[0]; auto xm = -xp;
  auto yp = 0.5f * lengths[1]; auto ym = -yp;
  auto zp = 0.5f * lengths[2]; auto zm = -zp;

  vertices = {
    xm, ym, zm,  xp, ym, zm,  xp, yp, zm,  xm, yp, zm,
    xm, ym, zp,  xp, ym, zp,  xp, yp, zp,  xm, yp, zp,
    xm, ym, zm,  xm, yp, zm,  xm, yp, zp,  xm, ym, zp,
    xp, ym, zm,  xp, yp, zm,  xp, yp, zp,  xp, ym, zp,
    xm, ym, zm,  xm, ym, zp,  xp, ym, zp,  xp, ym, zm,
    xm, yp, zm,  xm, yp, zp,  xp, yp, zp,  xp, yp, zm,
  };

  normals = {
    0, 0, -1,  0, 0, -1,  0, 0, -1,  0, 0, -1,
    0, 0, 1,  0, 0, 1,  0, 0, 1,  0, 0, 1,
    -1, 0, 0,  -1, 0, 0,  -1, 0, 0,  -1, 0, 0,
    1, 0, 0,  1, 0, 0,  1, 0, 0,  1, 0, 0,
    0, -1, 0,  0, -1, 0,  0, -1, 0,  0, -1, 0,
    0, 1, 0,  0, 1, 0,  0, 1, 0,  0, 1, 0
  };

  indices = {
    4 * 0 + 2, 4 * 0 + 1, 4 * 0 + 0,
    4 * 0 + 3, 4 * 0 + 2, 4 * 0 + 0,
    4 * 1 + 0, 4 * 1 + 1, 4 * 1 + 2,
    4 * 1 + 0, 4 * 1 + 2, 4 * 1 + 3,
    4 * 2 + 2, 4 * 2 + 1, 4 * 2 + 0,
    4 * 2 + 3, 4 * 2 + 2, 4 * 2 + 0,
    4 * 3 + 0, 4 * 3 + 1, 4 * 3 + 2,
    4 * 3 + 0, 4 * 3 + 2, 4 * 3 + 3,
    4 * 4 + 2, 4 * 4 + 1, 4 * 4 + 0,
    4 * 4 + 3, 4 * 4 + 2, 4 * 4 + 0,
    4 * 5 + 0, 4 * 5 + 1, 4 * 5 + 2,
    4 * 5 + 0, 4 * 5 + 2, 4 * 5 + 3,
  };

  //triangles(affine, bbox, vertices, normals, indices);
}

void TriangulatedVisitor::sphere(float* affine, float* bbox, float diameter)
{
  assert(false);

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

  //triangles(affine, bbox, vertices, normals, indices);
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

  //triangles(affine, bbox, vertices, normals, indices);
}

void TriangulatedVisitor::ellipticalDish(float* affine, float* bbox, float diameter, float radius)  { }

void TriangulatedVisitor::sphericalDish(float* affine, float* bbox, float diameter, float height)  { }

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
      l = vertex(normals, vertices, l, t1[2 * i + 0], t0[2 * i + 1], 0, t1[2 * i + 0], t1[2 * i + 1], -h2);
      l = vertex(normals, vertices, l, t1[2 * i + 0], t0[2 * i + 1], 0, t1[2 * i + 0], t1[2 * i + 1], h2);
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

void TriangulatedVisitor::snout(float* affine, float*bbox, float* offset, float* bshear, float* tshear, float bottom, float top, float height)
{
  //assert(false);


}

void TriangulatedVisitor::facetGroup(float* affine, float* bbox, std::vector<uint32_t>& polygons, std::vector<uint32_t>& contours, std::vector<float>& P, std::vector<float>& N)
{
  //fprintf(stderr, "* Group %zd\n", polygons.size() - 1);

  vertices.clear();
  indices.clear();
  for (unsigned p = 0; p + 1 < polygons.size(); p++) {
    unsigned ca = polygons[p];
    unsigned cb = polygons[p + 1];
    //fprintf(stderr, "  * Polygon %d %d .. %d\n", ca, cb, (cb - ca - 1));

    auto tess = tessNewTess(nullptr);
    unsigned va = ~0;
    unsigned vb = ~0;
    for (unsigned c = ca; c + 1 < cb; c++) {
      va = contours[c];
      vb = contours[c + 1];

      tessAddContour(tess, 3, P.data() + va, 3 * sizeof(float), (vb - va) / 3);

     //fprintf(stderr, "    * Contour %d %d .. %f\n", va, vb, (vb - va)/3.0);
    }

    if (tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 3, nullptr)) {
      auto vo = uint32_t(vertices.size())/3;
      auto vn = unsigned(tessGetVertexCount(tess));
      
      vertices.resize(vertices.size() + 3 * vn);
      std::memcpy(vertices.data() + 3 * vo, tessGetVertices(tess), 3 * vn * sizeof(float));

      // Try to remap normals
      auto * remap = tessGetVertexIndices(tess);
      normals.resize(vertices.size());
      for (unsigned i = 0; i < vn; i++) {
        if (remap[i] != TESS_UNDEF) {
          normals[3 * (vo + i) + 0] = N[contours[ca] + 3*remap[i] + 0];
          normals[3 * (vo + i) + 1] = N[contours[ca] + 3*remap[i] + 1];
          normals[3 * (vo + i) + 2] = N[contours[ca] + 3*remap[i] + 2];
        }
      }

      auto io = uint32_t(indices.size());
      auto elements_n = unsigned(tessGetElementCount(tess));
      indices.resize(indices.size() + 3 * elements_n);
      
      auto * elements = tessGetElements(tess);
      for (unsigned e = 0; e < elements_n; e++) {
        auto ix = elements + 3 * e;
        if ((ix[0] != TESS_UNDEF) && (ix[1] != TESS_UNDEF) && (ix[2] != TESS_UNDEF)) {
          indices[io + 3 * e + 0] = ix[0] + vo;
          indices[io + 3 * e + 1] = ix[1] + vo;
          indices[io + 3 * e + 2] = ix[2] + vo;
        }
      }
    }
    tessDeleteTess(tess);

    if (!indices.empty()) {
      //triangles(affine, bbox, vertices, normals, indices);
    }
  }



}
