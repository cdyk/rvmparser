#include <cstdio>
#include <cassert>
#include "tesselator.h"

#include "TriangulatedVisitor.h"


void TriangulatedVisitor::pyramid(float* M_affine, float* bbox, float* bottom_xy, float* top_xy, float* offset_xy, float height)  { }

void TriangulatedVisitor::box(float* affine, float* bbox, float* lengths)  { }

void TriangulatedVisitor::sphere(float* affine, float* bbox, float diameter)  { }

void TriangulatedVisitor::rectangularTorus(float* M_affine, float* bbox, float inner_radius, float outer_radius, float height, float angle)  { }

void TriangulatedVisitor::circularTorus(float* M_affine, float* bbox, float offset, float radius, float angle)  { }

void TriangulatedVisitor::ellipticalDish(float* affine, float* bbox, float diameter, float radius)  { }

void TriangulatedVisitor::sphericalDish(float* affine, float* bbox, float diameter, float height)  { }

void TriangulatedVisitor::cylinder(float* affine, float* bbox, float radius, float height)  { }

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
