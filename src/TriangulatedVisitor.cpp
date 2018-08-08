#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdio>
#include <cassert>
#include "tesselator.h"

#include "TriangulatedVisitor.h"

namespace {

  const float twopi = float(2.0*M_PI);

}


void TriangulatedVisitor::pyramid(float* M_affine, float* bbox, float* bottom_xy, float* top_xy, float* offset_xy, float height)  { }

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

  triangles(affine, bbox, vertices, normals, indices);
}

void TriangulatedVisitor::sphere(float* affine, float* bbox, float diameter)
{
  assert(false);

}

void TriangulatedVisitor::rectangularTorus(float* M_affine, float* bbox, float inner_radius, float outer_radius, float height, float angle)  { }

void TriangulatedVisitor::circularTorus(float* affine, float* bbox, float offset, float radius, float angle)
{
  unsigned samples_u = 10;
  unsigned samples_v = 10;

  t0.resize(2 * samples_u);
  for (unsigned i = 0; i < samples_u; i++) {
    t0[2 * i + 0] = std::cos((angle / (samples_u - 1.f))*i);
    t0[2 * i + 1] = std::sin((angle / (samples_u - 1.f))*i);
  }

  t1.resize(2 * samples_v);
  for (unsigned i = 0; i < samples_v; i++) {
    t1[2 * i + 0] = std::cos((twopi / samples_v)*i);
    t1[2 * i + 1] = std::sin((twopi / samples_v)*i);
  }

  vertices.resize(3 * (samples_u + 2) * samples_v);
  normals.resize(vertices.size());

  unsigned k = 0;
  for (unsigned u = 0; u < samples_u; u++) {
    for (unsigned v = 0; v < samples_v; v++) {
      vertices[k++] = ((radius * t1[2 * v + 0] + offset) * t0[2 * u + 0]);
      vertices[k++] = ((radius * t1[2 * v + 0] + offset) * t0[2 * u + 1]);
      vertices[k++] = radius * t1[2 * v + 1];
    }
  }
  for (unsigned e = 0; e < 2; e++) {
    auto u = e == 0 ? 0 : samples_v - 1;
    for (unsigned v = 0; v < samples_v; v++) {
      vertices[k++] = ((radius * t1[2 * v + 0] + offset) * t0[2 * u + 0]);
      vertices[k++] = ((radius * t1[2 * v + 0] + offset) * t0[2 * u + 1]);
      vertices[k++] = radius * t1[2 * v + 1];
    }
  }

  k = 0;
  for (unsigned u = 0; u < samples_u; u++) {
    for (unsigned v = 0; v < samples_v; v++) {
      normals[k++] = t1[2 * v + 0] * t0[2 * u + 0];
      normals[k++] = t1[2 * v + 0] * t0[2 * u + 1];
      normals[k++] = t1[2 * v + 1];
    }
  }
  for (unsigned v = 0; v < samples_v; v++) {
    normals[k++] = 0.f;
    normals[k++] = -1.f;
    normals[k++] = 0.f;
  }
  for (unsigned v = 0; v < samples_v; v++) {
    normals[k++] = -t0[2 * (samples_u - 1) + 1];
    normals[k++] = t0[2 * (samples_u - 1) + 0];
    normals[k++] = 0.f;
  }
  k = 0;
  indices.resize(6 * (samples_u - 1)*samples_v + 2 * 3 * (samples_v - 2));
  for (unsigned u = 0; u + 1 < samples_u; u++) {
    for (unsigned v = 0; v + 1 < samples_v; v++) {
      indices[k++] = samples_v * (u + 0) + (v + 0);
      indices[k++] = samples_v * (u + 1) + (v + 0);
      indices[k++] = samples_v * (u + 1) + (v + 1);

      indices[k++] = samples_v * (u + 1) + (v + 1);
      indices[k++] = samples_v * (u + 0) + (v + 1);
      indices[k++] = samples_v * (u + 0) + (v + 0);
    }
    indices[k++] = samples_v * (u + 0) + (samples_v - 1);
    indices[k++] = samples_v * (u + 1) + (samples_v - 1);
    indices[k++] = samples_v * (u + 1) + 0;
    indices[k++] = samples_v * (u + 1) + 0;
    indices[k++] = samples_v * (u + 0) + 0;
    indices[k++] = samples_v * (u + 0) + (samples_v - 1);
  }
  unsigned t = samples_u * samples_v;
  for (unsigned i = 1; i + 1 < samples_v; i++) {
    indices[k++] = t + 0;
    indices[k++] = t + i;
    indices[k++] = t + i + 1;
  }
  t = (samples_u+1) * samples_v;
  for (unsigned i = 1; i + 1 < samples_v; i++) {
    indices[k++] = t + 0;
    indices[k++] = t + i + 1;
    indices[k++] = t + i;
  }
  triangles(affine, bbox, vertices, normals, indices);
}

void TriangulatedVisitor::ellipticalDish(float* affine, float* bbox, float diameter, float radius)  { }

void TriangulatedVisitor::sphericalDish(float* affine, float* bbox, float diameter, float height)  { }

void TriangulatedVisitor::cylinder(float* affine, float* bbox, float radius, float height)
{ 
  unsigned samples = 20;

  vertices.resize(4 * samples * 3);
  normals.resize(vertices.size());
  indices.resize(6 * samples + 6 * (samples - 2));

  float h2 = 0.5f*height;
  for (unsigned i = 0; i < samples; i++) {
    auto x = std::cos((twopi / samples)*i);
    auto y = std::sin((twopi / samples)*i);
    for (unsigned k = 0; k < 4; k++) {
      vertices[3 * (k*samples + i) + 0] = radius * x;
      vertices[3 * (k*samples + i) + 1] = radius * y;
      vertices[3 * (k*samples + i) + 2] = (k & 1) == 0 ? -h2 : h2;
    }
    for (unsigned k = 0; k < 2; k++) {
      normals[3 * (k*samples + i) + 0] = x;
      normals[3 * (k*samples + i) + 1] = y;
      normals[3 * (k*samples + i) + 2] = 0.f;
    }
  }
  auto k = 3 * 2 * samples + 0;
  for (unsigned i = 0; i < samples; i++) {
    normals[k++] =  0.f;
    normals[k++] =  0.f;
    normals[k++] = -1.f;
  }
  for (unsigned i = 0; i < samples; i++) {
    normals[k++] = 0.f;
    normals[k++] = 0.f;
    normals[k++] = 1.f;
  }

  k = 0;
  for (unsigned i = 0; i < samples; i++) {
    unsigned j = (i + 1) % samples;
    indices[k++] = i;
    indices[k++] = j;
    indices[k++] = j + samples;
    indices[k++] = j + samples;
    indices[k++] = i + samples;
    indices[k++] = i;
  }
  for (unsigned i = 1; i + 1 < samples; i++) {
    indices[k++] = 2 * samples + 0;
    indices[k++] = 2 * samples + i + 1;
    indices[k++] = 2 * samples + i;
  }
  for (unsigned i = 1; i + 1 < samples; i++) {
    indices[k++] = 3 * samples + 0;
    indices[k++] = 3 * samples + i;
    indices[k++] = 3 * samples + i + 1;
  }

  triangles(affine, bbox, vertices, normals, indices);
}

void TriangulatedVisitor::snout(float* affine, float*bbox, float* offset, float* bshear, float* tshear, float bottom, float top, float height)  { }

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
      triangles(affine, bbox, vertices, normals, indices);
    }
  }



}
