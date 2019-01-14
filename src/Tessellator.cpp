#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cassert>
#include "tesselator.h"

#include "Store.h"
#include "Tessellator.h"
#include "LinAlgOps.h"

namespace {

  const float pi = float(M_PI);
  const float half_pi = float(0.5*M_PI);

}

Tessellator::Tessellator(Logger logger, float tolerance, float cullLeafThreshold, float cullGeometryThreshold, unsigned maxSamples) :
  logger(logger),
  tolerance(tolerance),
  maxSamples(maxSamples),
  cullLeafThresholdScaled(tolerance * cullLeafThreshold),
  cullGeometryThresholdScaled(tolerance * cullGeometryThreshold)
{
}

Tessellator::~Tessellator()
{
  delete factory;
}

Triangulation* Tessellator::getTriangulation(Geometry* geo)
{
  auto a = offsetof(Geometry, Geometry::kind);
  auto n = sizeof(Geometry) - a;

  auto hash = fnv_1a((const char*)geo + a, n);
  if (hash == 0) hash = 1;

  auto * firstItem = (CacheItem*)cache.map.get(hash);
  for (auto * item = firstItem; item != nullptr; item = item->next) {
    if (std::memcmp((const char*)geo + a, (const char*)item->src + a, n) == 0) {
      return item->tri;
    }
  }

  // FIXME: create tri.

  auto * item = &cache.items[cache.fill++];
  item->next = firstItem;
  item->src = geo;
  item->tri = nullptr;
  cache.map.insert(hash, uint64_t(item));
  return item->tri;
}


void Tessellator::init(class Store& store)
{
  this->store = &store;

  factory = new TriangulationFactory(&store, logger, tolerance, 3, maxSamples),

  store.arenaTriangulation.clear();

  cache.items = (CacheItem*)arena.alloc(sizeof(CacheItem)*store.geometryCountAllocated());
  cache.fill = 0;

  stack = (StackItem*)arena.alloc(sizeof(StackItem)*store.groupCountAllocated());
  stack_p = 0;
}

void Tessellator::endModel()
{
  logger(0, "Discarded %u caps.", factory->discardedCaps);
}


void Tessellator::beginGroup(struct Group* group)
{
  StackItem item = { 0 };
  if (!isEmpty(group->group.bboxWorld)) {
    item.groupError = diagonal(group->group.bboxWorld);
  }
  stack[stack_p++] = item;
}

void Tessellator::EndGroup()
{
  assert(stack_p);
  stack_p--;
}

void Tessellator::geometry(Geometry* geo)
{
  assert(stack_p);

  // No need to tessellate lines.
  if (geo->kind == Geometry::Kind::Line) {
    geo->triangulation = nullptr;
    return;
  }
  processed++;

  auto scale = getScale(geo->M_3x4);

  // Group error less than threshold, skip tessellation and record error.
  if (stack[stack_p - 1].groupError < cullLeafThresholdScaled) {
    geo->triangulation = store->arenaTriangulation.alloc<Triangulation>();
    geo->triangulation->error = stack[stack_p - 1].groupError;
    return;
  }
  else {
    auto scaledDiagonal = diagonal(geo->bboxWorld);
    if (scaledDiagonal < cullGeometryThresholdScaled) {
      geo->triangulation = store->arenaTriangulation.alloc<Triangulation>();
      geo->triangulation->error = scaledDiagonal;
      geometryCulled++;
      return;
    }
  }


  Triangulation* tri = nullptr;
  switch (geo->kind) {
  case Geometry::Kind::Pyramid:
    tri = factory->pyramid(&store->arenaTriangulation, geo, scale);
    break;

  case Geometry::Kind::Box:
    tri = factory->box(&store->arenaTriangulation, geo, scale);
    break;

  case Geometry::Kind::RectangularTorus:
    tri = factory->rectangularTorus(&store->arenaTriangulation, geo, scale);
    break;
    
  case Geometry::Kind::CircularTorus:
    tri = factory->circularTorus(&store->arenaTriangulation, geo, scale);
    break;

  case Geometry::Kind::EllipticalDish:
    tri = factory->sphereBasedShape(&store->arenaTriangulation, geo, geo->ellipticalDish.baseRadius, half_pi, 0.f, geo->ellipticalDish.height / geo->ellipticalDish.baseRadius, scale);
    break;

  case Geometry::Kind::SphericalDish: {
    float r_circ = geo->sphericalDish.baseRadius;
    auto h = geo->sphericalDish.height;
    float r_sphere = (r_circ*r_circ + h * h) / (2.f*h);
    float arc = asin(r_circ / r_sphere);
    if (r_circ < h) { arc = pi - arc; }
    tri = factory->sphereBasedShape(&store->arenaTriangulation, geo, r_sphere, arc, h - r_sphere, 1.f, scale);
    break;
  }
  case Geometry::Kind::Snout:
    tri = factory->snout(&store->arenaTriangulation, geo, scale);
    break;

  case Geometry::Kind::Cylinder:
    tri = factory->cylinder(&store->arenaTriangulation, geo, scale);
    break;

  case Geometry::Kind::Sphere:
    tri = factory->sphereBasedShape(&store->arenaTriangulation, geo, 0.5f*geo->sphere.diameter, pi, 0.f, 1.f, scale);
    break;

  case Geometry::Kind::FacetGroup:
    tri = factory->facetGroup(&store->arenaTriangulation, geo, scale);
    break;

  case Geometry::Kind::Line:  // Handled at start of function.
  default:
    assert(false && "Unhandled primitive type");
    break;
  }

  geo->triangulation = tri;
  vertices += uint64_t(tri->vertices_n);
  triangles += uint64_t(tri->triangles_n);

  BBox3f box = createEmptyBBox3f();
  for (unsigned i = 0; i < geo->triangulation->vertices_n; i++) {
    engulf(box, Vec3f(geo->triangulation->vertices + 3 * i));
  }
  //assert(geo->bbox_l.min[0] < box.min[0] + 0.1f*box.maxSideLength());
  //assert(geo->bbox_l.min[1] < box.min[1] + 0.1f*box.maxSideLength());
  //assert(geo->bbox_l.min[2] < box.min[2] + 0.1f*box.maxSideLength());

  //assert(box.max[0] - 0.1f*box.maxSideLength() < geo->bbox_l.max[0]);
  //assert(box.max[1] - 0.1f*box.maxSideLength() < geo->bbox_l.max[1]);
  //assert(box.max[2] - 0.1f*box.maxSideLength() < geo->bbox_l.max[2]);

  geo->triangulation->id = geo->id;
  process(geo);

  tessellated++;
}
