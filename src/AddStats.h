#pragma once
#include "StoreVisitor.h"

struct Stats
{
  unsigned group_n = 0;

  unsigned geometry_n = 0;
  unsigned pyramid_n = 0;
  unsigned box_n = 0;
  unsigned rectangular_torus_n = 0;
  unsigned circular_torus_n = 0;
  unsigned elliptical_dish_n = 0;
  unsigned spherical_dish_n = 0;
  unsigned snout_n = 0;
  unsigned cylinder_n = 0;
  unsigned sphere_n = 0;
  unsigned facetgroup_n = 0;
  unsigned facetgroup_triangles_n = 0;
  unsigned facetgroup_quads_n = 0;
  unsigned facetgroup_polygon_n = 0;
  unsigned facetgroup_polygon_n_contours_n = 0;
  unsigned facetgroup_polygon_n_vertices_n = 0;
  unsigned line_n = 0;
};


class AddStats : public StoreVisitor
{
public:

  void init(class Store& store) override;

  void beginGroup(struct Group* group) override;

  void geometry(struct Geometry* geometry) override;

  bool done() override;

private:
  struct Stats* stats = nullptr;

};