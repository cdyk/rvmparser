#include <cassert>
#include "Store.h"
#include "AddStats.h"

void AddStats::init(class Store& store)
{
  store.stats = store.arena.alloc<Stats>();
  stats = store.stats;
}

void AddStats::beginGroup(const char* name, const float* translation, const uint32_t material)
{
  stats->group_n++;
}

void AddStats::geometry(struct Geometry* geo)
{
  stats->geometry_n++;
  switch (geo->kind) {
  case Geometry::Kind::Pyramid: stats->pyramid_n++; break;
  case Geometry::Kind::Box: stats->box_n++; break;
  case Geometry::Kind::RectangularTorus: stats->rectangular_torus_n++; break;
  case Geometry::Kind::CircularTorus: stats->circular_torus_n++; break;
  case Geometry::Kind::EllipticalDish: stats->elliptical_dish_n++; break;
  case Geometry::Kind::SphericalDish: stats->spherical_dish_n++; break;
  case Geometry::Kind::Snout: stats->snout_n++; break;
  case Geometry::Kind::Cylinder: stats->cylinder_n++; break;
  case Geometry::Kind::Sphere: stats->sphere_n++; break;
  case Geometry::Kind::FacetGroup:
    stats->facetgroup_n++;
    stats->facetgroup_polygon_n += geo->facetGroup.polygons_n;
    for (unsigned p = 0; p < geo->facetGroup.polygons_n; p++) {
      stats->facetgroup_polygon_n_contours_n += geo->facetGroup.polygons[p].contours_n;
      for (unsigned c = 0; c < geo->facetGroup.polygons[p].contours_n; c++) {
        stats->facetgroup_polygon_n_vertices_n += geo->facetGroup.polygons[p].contours[c].vertices_n;
      }
    }
    
    break;
  case Geometry::Kind::Line: stats->line_n++; break;
  default:
    assert(false && "Unhandled primitive type");
    break;
  }

}

bool AddStats::done()
{
  fprintf(stderr, "Stats:\n");
  fprintf(stderr, "    Groups                 %d\n", stats->group_n);
  fprintf(stderr, "    Geometries             %d\n", stats->geometry_n);
  fprintf(stderr, "        Pyramids           %d\n", stats->pyramid_n);
  fprintf(stderr, "        Boxes              %d\n", stats->box_n);
  fprintf(stderr, "        Rectangular tori   %d\n", stats->rectangular_torus_n);
  fprintf(stderr, "        Circular tori      %d\n", stats->circular_torus_n);
  fprintf(stderr, "        Elliptical dish    %d\n", stats->elliptical_dish_n);
  fprintf(stderr, "        Spherical dish     %d\n", stats->spherical_dish_n);
  fprintf(stderr, "        Snouts             %d\n", stats->snout_n);
  fprintf(stderr, "        Cylinders          %d\n", stats->cylinder_n);
  fprintf(stderr, "        Spheres            %d\n", stats->sphere_n);
  fprintf(stderr, "        Facet groups       %d\n", stats->facetgroup_n);
  fprintf(stderr, "            polygons       %d (fgrp avg=%d)\n", stats->facetgroup_polygon_n, unsigned(stats->facetgroup_polygon_n/float(stats->facetgroup_n)));
  fprintf(stderr, "            contours       %d (poly avg=%d)\n", stats->facetgroup_polygon_n_contours_n, unsigned(stats->facetgroup_polygon_n_contours_n / float(stats->facetgroup_polygon_n)));
  fprintf(stderr, "            vertices       %d (cont avg=%d)\n", stats->facetgroup_polygon_n_vertices_n, unsigned(stats->facetgroup_polygon_n_vertices_n / float(stats->facetgroup_polygon_n_contours_n)));
  fprintf(stderr, "        Lines              %d\n", stats->line_n);

  return true;
}
