#pragma once

#include "RVMVisitor.h"

class TriangulatedVisitor : public RVMVisitor
{
public:

  virtual void triangles(float* affine, float* bbox, std::vector<float>& P, std::vector<float>& N, std::vector<uint32_t>& indices) = 0;

  void pyramid(float* M_affine, float* bbox, float* bottom_xy, float* top_xy, float* offset_xy, float height) override;

  void box(float* affine, float* bbox, float* lengths) override;

  void sphere(float* affine, float* bbox, float diameter) override;

  void rectangularTorus(float* M_affine, float* bbox, float inner_radius, float outer_radius, float height, float angle) override;

  void circularTorus(float* M_affine, float* bbox, float offset, float radius, float angle) override;

  void ellipticalDish(float* affine, float* bbox, float diameter, float radius) override;

  void sphericalDish(float* affine, float* bbox, float diameter, float height) override;

  void cylinder(float* affine, float* bbox, float radius, float height) override;

  void snout(float* affine, float*bbox, float* offset, float* bshear, float* tshear, float bottom, float top, float height) override;

  void facetGroup(float* affine, float* bbox, std::vector<uint32_t>& polygons, std::vector<uint32_t>& contours, std::vector<float>& P, std::vector<float>& N) override;

private:
  std::vector<float> vertices;
  std::vector<float> normals;
  std::vector<uint32_t> indices;

  std::vector<unsigned> u0;
  std::vector<float> t0;
  std::vector<float> t1;
  std::vector<float> t2;

  void sphereBasedShape(float* affine, float* bbox, float radius, float arc, float shift_z, float scale_z);

};