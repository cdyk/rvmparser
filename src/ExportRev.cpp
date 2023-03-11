#include "Common.h"
#include "Store.h"

#include <cstdio>
#include <cstring>
#include <cassert>

namespace {

  struct Context {
    Store* store = nullptr;
    Logger logger = nullptr;
    FILE* out = nullptr;
  };


  void writeUint(Context* ctx, uint32_t x)
  {
    fprintf(ctx->out, "%6u\n", x);
  }

  void writeUint2(Context* ctx, uint32_t x, uint32_t y)
  {
    fprintf(ctx->out, "%6u%6u\n", x, y);
  }

  void writeVec2f(Context* ctx, float x, float y)
  {
    fprintf(ctx->out, "%14.5f%14.5f\n", x, y);
  }

  void writeVec3f(Context* ctx, float x, float y, float z)
  {
    fprintf(ctx->out, "%14.5f%14.5f%14.5f\n", x, y, z);
  }

  void writeVec3f(Context* ctx, const float* ptr)
  {
    writeVec3f(ctx, ptr[0], ptr[1], ptr[2]);
  }

  void writeVec4f(Context* ctx, float x, float y, float z, float w)
  {
    fprintf(ctx->out, "%14.5f%14.5f%14.5f%14.5f\n", x, y, z, w);
  }

  void writeVec4f(Context* ctx, const float* ptr)
  {
    writeVec4f(ctx, ptr[0], ptr[1], ptr[2], ptr[3]);
  }

  void writeVec5f(Context* ctx, float x, float y, float z, float w, float q)
  {
    fprintf(ctx->out, "%14.5f%14.5f%14.5f%14.5f%14.5f\n", x, y, z, w, q);
  }

  void writeChunkHeader(Context* ctx, const char* id, uint32_t unknown0 = 1, uint32_t unknown1 = 1)
  {
    fputs(id, ctx->out);
    fputc('\n', ctx->out);
    writeUint2(ctx, unknown0, unknown1);
  }

  void reportUnsupportedPrimitive(Context* ctx, uint32_t type)
  {
    ctx->logger(2, "SKIPPING unsupported primitive type %d, if you know the formatting of this primitive type, please tell the program author.", type);
  }

  void writeGeometry(Context* ctx, Geometry* geometry)
  {
    // Skip primitives with unknown formatting
    switch (geometry->kind) {
    case Geometry::Kind::Sphere:
      reportUnsupportedPrimitive(ctx,  9);
      return; break;
    case Geometry::Kind::Pyramid:
    case Geometry::Kind::RectangularTorus:
    case Geometry::Kind::EllipticalDish:
    case Geometry::Kind::Box:
    case Geometry::Kind::CircularTorus:
    case Geometry::Kind::SphericalDish:
    case Geometry::Kind::Snout:
    case Geometry::Kind::Cylinder:
    case Geometry::Kind::Line:
    case Geometry::Kind::FacetGroup:
      // supported
      break;
    default:
      assert(false);
    }

    // write
    writeChunkHeader(ctx, "PRIM");

    uint32_t kind = 0;
    switch (geometry->kind) {
    case Geometry::Kind::Pyramid:           kind = 1; break;
    case Geometry::Kind::Box:               kind = 2; break;
    case Geometry::Kind::RectangularTorus:  kind = 3; break;
    case Geometry::Kind::CircularTorus:     kind = 4; break;
    case Geometry::Kind::EllipticalDish:    kind = 5; break;
    case Geometry::Kind::SphericalDish:     kind = 6; break;
    case Geometry::Kind::Snout:             kind = 7; break;
    case Geometry::Kind::Cylinder:          kind = 8; break;
    case Geometry::Kind::Sphere:            kind = 9; break;
    case Geometry::Kind::Line:              kind = 10; break;
    case Geometry::Kind::FacetGroup:        kind = 11; break;
    default:
      assert(false);
    }
    fprintf(ctx->out, "%6u\n", kind);
    for (size_t k = 0; k < 3; k++) {
      writeVec4f(ctx,
                 geometry->M_3x4.data[k + 0],
                 geometry->M_3x4.data[k + 3],
                 geometry->M_3x4.data[k + 6],
                 geometry->M_3x4.data[k + 9]);
    }
    for (size_t k = 0; k < 2; k++) {
      writeVec3f(ctx, &geometry->bboxLocal.data[3*k]);
    }

    switch (geometry->kind) {
    case Geometry::Kind::Pyramid:
      writeVec4f(ctx,
                 geometry->pyramid.bottom[0], geometry->pyramid.bottom[1],
                 geometry->pyramid.top[0], geometry->pyramid.top[1]);
      writeVec3f(ctx,
                 geometry->pyramid.offset[0], geometry->pyramid.offset[1], geometry->pyramid.height);
      break;
    case Geometry::Kind::Box:
      writeVec3f(ctx, geometry->box.lengths);
      break;
    case Geometry::Kind::RectangularTorus:
      writeVec4f(ctx,
                 geometry->rectangularTorus.inner_radius,
                 geometry->rectangularTorus.outer_radius,
                 geometry->rectangularTorus.height,
                 geometry->rectangularTorus.angle);
      break;
    case Geometry::Kind::CircularTorus:
      writeVec3f(ctx,
                 geometry->circularTorus.offset,
                 geometry->circularTorus.radius,
                 geometry->circularTorus.angle);
      break;
    case Geometry::Kind::EllipticalDish:
      writeVec2f(ctx,
                 geometry->ellipticalDish.baseRadius,
                 geometry->ellipticalDish.height);
      break;
    case Geometry::Kind::SphericalDish:
      writeVec2f(ctx,
                 geometry->sphericalDish.baseRadius,
                 geometry->sphericalDish.height);
      break;
    case Geometry::Kind::Snout:
      writeVec5f(ctx,
                 geometry->snout.radius_b,
                 geometry->snout.radius_t,
                 geometry->snout.height,
                 geometry->snout.offset[0],
                 geometry->snout.offset[1]);
      writeVec4f(ctx,
                 geometry->snout.bshear[0],
                 geometry->snout.bshear[1],
                 geometry->snout.tshear[0],
                 geometry->snout.tshear[1]);
      break;
    case Geometry::Kind::Cylinder:
      writeVec2f(ctx,
                 geometry->cylinder.radius,
                 geometry->cylinder.height);
      break;
    case Geometry::Kind::Sphere:
      fflush(ctx->out);
      assert(false && "Unhandled primitive type 9");
      break;
    case Geometry::Kind::Line:
      writeVec2f(ctx,
                 geometry->line.a,
                 geometry->line.b);
      break;
    case Geometry::Kind::FacetGroup: {
      const auto& facetGroup = geometry->facetGroup;
      writeUint(ctx, facetGroup.polygons_n);
      for (size_t p = 0; p < facetGroup.polygons_n; p++) {
        const Polygon& polygon = facetGroup.polygons[p];
        writeUint(ctx, polygon.contours_n);
        for (size_t c = 0; c < polygon.contours_n; c++) {
          const Contour& contour = polygon.contours[c];
          writeUint(ctx, contour.vertices_n);
          for (unsigned vi = 0; vi < contour.vertices_n; vi++) {
            writeVec3f(ctx, &contour.vertices[3 * vi]);
            writeVec3f(ctx, &contour.normals[3 * vi]);
          }
        }
      }
      break;
    }
    default:
      assert(false);
    }
  }


  void writeGroup(Context* ctx, Node* group)
  {
    assert(group->kind == Node::Kind::Group);
    writeChunkHeader(ctx, "CNTB");
    fprintf(ctx->out, "%s\n", group->group.name);
   
    writeVec3f(ctx,
               1000.f * group->group.translation[0],
               1000.f * group->group.translation[1],
               1000.f * group->group.translation[2]);
    fprintf(ctx->out, "%6u\n", group->group.material);

    for (Node* child = group->children.first; child; child = child->next) {
      writeGroup(ctx, child);
    }

    for (Geometry* geo = group->group.geometries.first; geo; geo = geo->next) {
      writeGeometry(ctx, geo);
    }

    writeChunkHeader(ctx, "CNTE");
  }

  void writeModel(Context* ctx, Node* model)
  {
    assert(model->kind == Node::Kind::Model);
    
    writeChunkHeader(ctx, "MODL");
    fprintf(ctx->out, "%s\n%s\n",
            model->model.project,
            model->model.name);
    for (Node* group = model->children.first; group; group = group->next) {
      writeGroup(ctx, group);
    }
  }

  void writeFile(Context* ctx, Node* file)
  {
    assert(file->kind == Node::Kind::File);

    writeChunkHeader(ctx, "HEAD");
    fprintf(ctx->out, "%s\n%s\n%s\n%s\n",
            file->file.info,
            file->file.note,
            file->file.date,
            file->file.user);
    for (Node* model = file->children.first; model; model = model->next) {
      writeModel(ctx, model);
    }
    writeChunkHeader(ctx, "END:");
  }

}

bool exportRev(Store* store, Logger logger, const char* path)
{
  Context ctx;
  ctx.store = store;
  ctx.logger = logger;
  ctx.out = nullptr;

#ifdef _WIN32
  auto err = fopen_s(&ctx.out, path, "w");
  if (err != 0) {
    char buf[1024];
    if (strerror_s(buf, sizeof(buf), err) != 0) {
      buf[0] = '\0';
    }
    logger(2, "exportRev: Failed to open %s for writing: %s", path, buf);
    return false;
  }
  assert(ctx.out);
#else
  ctx.out = fopen(path, "w");
  if (ctx.out == nullptr) {
    logger(2, "exportRev: Failed to open %s for writing.", path);
    return false;
  }
#endif
  logger(0, "exportRev: Writing %s...", path);
  for (Node* file = store->getFirstRoot(); file; file = file->next) {
    writeFile(&ctx, file);
  }
  logger(0, "exportRev: Writing %s... done", path);
  fclose(ctx.out);
  return true;
}