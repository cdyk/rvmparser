#pragma once
#include <vector>

class RVMVisitor
{
public:
  virtual void init(class Store& store) = 0;

  virtual void beginFile(const char* info, const char* note, const char* date, const char* user, const char* encoding) = 0;

  virtual void endFile() = 0;

  virtual void beginModel(const char* project, const char* name) = 0;

  virtual void endModel() = 0;

  virtual void beginGroup(const char* name, const float* translation, const uint32_t material) = 0;

  virtual void EndGroup() = 0;

  virtual void geometry(struct Geometry* geometry) = 0;

#if 0

  virtual void line(float* affine, float* bbox, float x0, float x1) = 0;

  virtual void pyramid(float* M_affine, float* bbox, float* bottom_xy, float* top_xy, float* offset_xy, float height) = 0;

  virtual void box(float* affine, float* bbox, float* lengths) = 0;

  virtual void sphere(float* affine, float* bbox, float diameter) = 0;
  
  virtual void rectangularTorus(float* M_affine, float* bbox, float inner_radius, float outer_radius, float height, float angle) = 0;

  virtual void circularTorus(float* M_affine, float* bbox, float offset, float radius, float angle) = 0;

  virtual void ellipticalDish(float* affine, float* bbox, float diameter, float radius) = 0;
  
  virtual void sphericalDish(float* affine, float* bbox, float diameter, float height) = 0;

  virtual void cylinder(float* affine, float* bbox, float radius, float height) = 0;

  virtual void snout(float* affine, float*bbox, float* offset, float* bshear, float* tshear, float bottom, float top, float height) = 0;

  virtual void facetGroup(float* affine, float* bbox, std::vector<uint32_t>& polygons, std::vector<uint32_t>& contours, std::vector<float>& P, std::vector<float>& N) = 0;

#endif

};
