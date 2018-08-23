#include <cassert>
#include "Store.h"
#include "AddStats.h"

void AddStats::init(class Store& store)
{
  store.stats = store.arena.alloc<Stats>();
  stats = store.stats;
}

void AddStats::beginGroup(Group* group)
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

    for (unsigned p = 0; p < geo->facetGroup.polygons_n; p++) {
      auto & poly = geo->facetGroup.polygons[p];
      if (poly.contours_n == 1 && poly.contours[0].vertices_n == 3) {
        stats->facetgroup_triangles_n++;
      }
      else if (poly.contours_n == 1 && poly.contours[0].vertices_n == 4) {
        stats->facetgroup_quads_n++;
      }
      else {
        stats->facetgroup_polygon_n++;
        stats->facetgroup_polygon_n_contours_n += geo->facetGroup.polygons[p].contours_n;
        for (unsigned c = 0; c < geo->facetGroup.polygons[p].contours_n; c++) {
          stats->facetgroup_polygon_n_vertices_n += geo->facetGroup.polygons[p].contours[c].vertices_n;
        }
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
  return true;
}
