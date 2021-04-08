#include "Parser.h"
#include "StoreVisitor.h"
#include "Store.h"

#include <iostream>
#include <string>
#include <cstdio>
#include <cctype>
#include <vector>

#include <cassert>

#include "LinAlgOps.h"

namespace {

  struct Context
  {
    Store* store;
    char* buf;
    size_t buf_size;
    std::vector<Group*> group_stack;
  };

  const char* read_uint8(uint8_t& rv, const char* curr_ptr, const char* end_ptr)
  {
    auto* q = reinterpret_cast<const uint8_t*>(curr_ptr);
    rv = q[0];
    return curr_ptr + 1;
  }

  const char* read_uint32_be(uint32_t& rv, const char* curr_ptr, const char* end_ptr)
  {
    auto * q = reinterpret_cast<const uint8_t*>(curr_ptr);
    rv = q[0] << 24 | q[1] << 16 | q[2] << 8 | q[3];
    return curr_ptr + 4;
  }

  const char* read_float32_be(float& rv, const char* curr_ptr, const char* end_ptr)
  {
    union {
      float f;
      uint32_t u;
    };

    auto * q = reinterpret_cast<const uint8_t*>(curr_ptr);
    u = q[0] << 24 | q[1] << 16 | q[2] << 8 | q[3];
    rv = f;
    return curr_ptr + 4;
  }

  constexpr uint32_t id(const char* str)
  {
    return str[3] << 24 | str[2] << 16 | str[1] << 8 | str[0];
  }

  const char* read_string(const char** dst, Store* store, const char* curr_ptr, const char* end_ptr)
  {
    uint32_t len;
    curr_ptr = read_uint32_be(len, curr_ptr, end_ptr);

    unsigned l = 4 * len;
    for (unsigned i = 0; i < l; i++) {
      if (curr_ptr[i] == 0) {
        l = i;
        break;
      }
    }
    *dst = store->strings.intern(curr_ptr, curr_ptr + l);
    return curr_ptr + 4 * len;
  }

  const char* parse_chunk_header(char* id, uint32_t& next_chunk_offset, uint32_t& dunno, const char* curr_ptr, const char* end_ptr)
  {
    unsigned i = 0;
    for (i = 0; i < 4 && curr_ptr + 4 <= end_ptr; i++) {
      assert(curr_ptr[0] == 0);
      assert(curr_ptr[1] == 0);
      assert(curr_ptr[2] == 0);
      id[i] = curr_ptr[3];
      curr_ptr += 4;
    }
    for (; i < 4; i++) {
      id[i] = ' ';
    }
    if (curr_ptr + 8 <= end_ptr) {
      curr_ptr = read_uint32_be(next_chunk_offset, curr_ptr, end_ptr);
      curr_ptr = read_uint32_be(dunno, curr_ptr, end_ptr);
    }
    else {
      next_chunk_offset = ~0;
      dunno = ~0;
      fprintf(stderr, "Chunk '%s' EOF after %zd bytes\n", id, end_ptr - curr_ptr);
      curr_ptr = end_ptr;
    }
    return curr_ptr;
  }

  bool verifyOffset(Context* ctx, const char* chunk_type, const char* base_ptr, const char* curr_ptr, uint32_t expected_next_chunk_offset)
  {
    size_t current_offset = curr_ptr - base_ptr;
    if (current_offset == expected_next_chunk_offset) {
      return true;
    }
    else {
      snprintf(ctx->buf, ctx->buf_size, "After chunk %s, expected offset %#x, current offset is %#zx",
               chunk_type, expected_next_chunk_offset, current_offset);
      ctx->store->setErrorString(ctx->buf);
      return false;
    }
  }

  const char* parse_head(Context* ctx, const char* base_ptr, const char* curr_ptr, const char* end_ptr, uint32_t expected_next_chunk_offset)
  {
    assert(ctx->group_stack.empty());
    auto * g = ctx->store->newGroup(nullptr, Group::Kind::File);
    ctx->group_stack.push_back(g);

    uint32_t version;
    curr_ptr = read_uint32_be(version, curr_ptr, end_ptr);
    curr_ptr = read_string(&g->file.info, ctx->store, curr_ptr, end_ptr);
    curr_ptr = read_string(&g->file.note, ctx->store, curr_ptr, end_ptr);
    curr_ptr = read_string(&g->file.date, ctx->store, curr_ptr, end_ptr);
    curr_ptr = read_string(&g->file.user, ctx->store, curr_ptr, end_ptr);
    if (2 <= version) {
      curr_ptr = read_string(&g->file.encoding, ctx->store, curr_ptr, end_ptr);
    }
    else {
      g->file.encoding = ctx->store->strings.intern("");
    }

    if (!verifyOffset(ctx, "HEAD", base_ptr, curr_ptr, expected_next_chunk_offset)) return nullptr;

    return curr_ptr;
  }

  const char* parse_modl(Context* ctx, const char* base_ptr, const char* curr_ptr, const char* end_ptr, uint32_t expected_next_chunk_offset)
  {
    assert(!ctx->group_stack.empty());
    auto * g = ctx->store->newGroup(ctx->group_stack.back(), Group::Kind::Model);
    ctx->group_stack.push_back(g);

    uint32_t version;
    curr_ptr = read_uint32_be(version, curr_ptr, end_ptr);

    curr_ptr = read_string(&g->model.project, ctx->store, curr_ptr, end_ptr);
    curr_ptr = read_string(&g->model.name, ctx->store, curr_ptr, end_ptr);

    //fprintf(stderr, "modl project='%s', name='%s'\n", g->model.project, g->model.name);

    if (!verifyOffset(ctx, "MODL", base_ptr, curr_ptr, expected_next_chunk_offset)) return nullptr;

    return curr_ptr;
  }

  const char* parse_prim(Context* ctx, const char* base_ptr, const char* curr_ptr, const char* end_ptr, uint32_t expected_next_chunk_offset)
  {
    assert(!ctx->group_stack.empty());
    if (ctx->group_stack.back()->kind != Group::Kind::Group) {
      ctx->store->setErrorString("In PRIM, parent chunk is not CNTB");
      return nullptr;
    }

    uint32_t version, kind;
    curr_ptr = read_uint32_be(version, curr_ptr, end_ptr);
    curr_ptr = read_uint32_be(kind, curr_ptr, end_ptr);

    auto * g = ctx->store->newGeometry(ctx->group_stack.back());

    for (unsigned i = 0; i < 12; i++) {
      curr_ptr = read_float32_be(g->M_3x4.data[i], curr_ptr, end_ptr);
    }
    for (unsigned i = 0; i < 6; i++) {
      curr_ptr = read_float32_be(g->bboxLocal.data[i], curr_ptr, end_ptr);
    }
    g->bboxWorld = transform(g->M_3x4, g->bboxLocal);

    switch (kind) {
    case 1:
      g->kind = Geometry::Kind::Pyramid;
      curr_ptr = read_float32_be(g->pyramid.bottom[0], curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->pyramid.bottom[1], curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->pyramid.top[0], curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->pyramid.top[1], curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->pyramid.offset[0], curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->pyramid.offset[1], curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->pyramid.height, curr_ptr, end_ptr);
      break;

    case 2:
      g->kind = Geometry::Kind::Box;
      curr_ptr = read_float32_be(g->box.lengths[0], curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->box.lengths[1], curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->box.lengths[2], curr_ptr, end_ptr);
      break;

    case 3:
      g->kind = Geometry::Kind::RectangularTorus;
      curr_ptr = read_float32_be(g->rectangularTorus.inner_radius, curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->rectangularTorus.outer_radius, curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->rectangularTorus.height, curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->rectangularTorus.angle, curr_ptr, end_ptr);
      break;

    case 4:
      g->kind = Geometry::Kind::CircularTorus;
      curr_ptr = read_float32_be(g->circularTorus.offset, curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->circularTorus.radius, curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->circularTorus.angle, curr_ptr, end_ptr);
      break;

    case 5:
      g->kind = Geometry::Kind::EllipticalDish;
      curr_ptr = read_float32_be(g->ellipticalDish.baseRadius, curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->ellipticalDish.height, curr_ptr, end_ptr);
      break;

    case 6:
      g->kind = Geometry::Kind::SphericalDish;
      curr_ptr = read_float32_be(g->sphericalDish.baseRadius, curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->sphericalDish.height, curr_ptr, end_ptr);
      break;

    case 7:
      g->kind = Geometry::Kind::Snout;
      curr_ptr = read_float32_be(g->snout.radius_b, curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->snout.radius_t, curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->snout.height, curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->snout.offset[0], curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->snout.offset[1], curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->snout.bshear[0], curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->snout.bshear[1], curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->snout.tshear[0], curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->snout.tshear[1], curr_ptr, end_ptr);
      break;

    case 8:
      g->kind = Geometry::Kind::Cylinder;
      curr_ptr = read_float32_be(g->cylinder.radius, curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->cylinder.height, curr_ptr, end_ptr);
      break;

    case 9:
      g->kind = Geometry::Kind::Sphere;
      curr_ptr = read_float32_be(g->sphere.diameter, curr_ptr, end_ptr);
      break;

    case 10:
      g->kind = Geometry::Kind::Line;
      curr_ptr = read_float32_be(g->line.a, curr_ptr, end_ptr);
      curr_ptr = read_float32_be(g->line.b, curr_ptr, end_ptr);
      break;

    case 11:
      g->kind = Geometry::Kind::FacetGroup;

      curr_ptr = read_uint32_be(g->facetGroup.polygons_n, curr_ptr, end_ptr);
      g->facetGroup.polygons = (Polygon*)ctx->store->arena.alloc(sizeof(Polygon)*g->facetGroup.polygons_n);

      for (unsigned pi = 0; pi < g->facetGroup.polygons_n; pi++) {
        auto & poly = g->facetGroup.polygons[pi];

        curr_ptr = read_uint32_be(poly.contours_n, curr_ptr, end_ptr);
        poly.contours = (Contour*)ctx->store->arena.alloc(sizeof(Contour)*poly.contours_n);
        for (unsigned gi = 0; gi < poly.contours_n; gi++) {
          auto & cont = poly.contours[gi];

          curr_ptr = read_uint32_be(cont.vertices_n, curr_ptr, end_ptr);
          cont.vertices = (float*)ctx->store->arena.alloc(3 * sizeof(float)*cont.vertices_n);
          cont.normals = (float*)ctx->store->arena.alloc(3 * sizeof(float)*cont.vertices_n);

          for (unsigned vi = 0; vi < cont.vertices_n; vi++) {
            for (unsigned i = 0; i < 3; i++) {
              curr_ptr = read_float32_be(cont.vertices[3 * vi + i], curr_ptr, end_ptr);
            }
            for (unsigned i = 0; i < 3; i++) {
              curr_ptr = read_float32_be(cont.normals[3 * vi + i], curr_ptr, end_ptr);
            }
          }
        }
      }
      break;

    default:
      snprintf(ctx->buf, ctx->buf_size, "In PRIM, unknown primitive kind %d", kind);
      ctx->store->setErrorString(ctx->buf);
      return nullptr;
    }

    if (!verifyOffset(ctx, "PRIM", base_ptr, curr_ptr, expected_next_chunk_offset)) return nullptr;

    return curr_ptr;
  }

  const char* parse_cntb(Context* ctx, const char* base_ptr, const char* curr_ptr, const char* end_ptr, uint32_t expected_next_chunk_offset)
  {
    assert(!ctx->group_stack.empty());
    auto * g = ctx->store->newGroup(ctx->group_stack.back(), Group::Kind::Group);
    ctx->group_stack.push_back(g);

    uint32_t version;
    curr_ptr = read_uint32_be(version, curr_ptr, end_ptr);
    curr_ptr = read_string(&g->group.name, ctx->store, curr_ptr, end_ptr);

    //fprintf(stderr, "group '%s' %p\n", g->group.name, g->group.name);

    // Translation seems to be a reference point that can be used as a local frame for objects in the group.
    // The transform is not relative to this reference point.
    for (unsigned i = 0; i < 3; i++) {
      curr_ptr = read_float32_be(g->group.translation[i], curr_ptr, end_ptr);
      g->group.translation[i] *= 0.001f;
    }

    curr_ptr = read_uint32_be(g->group.material, curr_ptr, end_ptr);

    if (!verifyOffset(ctx, "CNTB", base_ptr, curr_ptr, expected_next_chunk_offset)) return nullptr;

    // process children
    char chunk_id[5] = { 0, 0, 0, 0, 0 };
    auto l = curr_ptr;
    uint32_t dunno;
    curr_ptr = parse_chunk_header(chunk_id, expected_next_chunk_offset, dunno, curr_ptr, end_ptr);
    auto id_chunk_id = id(chunk_id);
    while (curr_ptr < end_ptr && id_chunk_id != id("CNTE")) {
      switch (id_chunk_id) {
      case id("CNTB"):
        curr_ptr = parse_cntb(ctx, base_ptr, curr_ptr, end_ptr, expected_next_chunk_offset);
        if (curr_ptr == nullptr) return curr_ptr;
        break;
      case id("PRIM"):
        curr_ptr = parse_prim(ctx, base_ptr, curr_ptr, end_ptr, expected_next_chunk_offset);
        if (curr_ptr == nullptr) return curr_ptr;
        break;
      default:
        snprintf(ctx->buf, ctx->buf_size, "In CNTB, unknown chunk id %s", chunk_id);
        ctx->store->setErrorString(ctx->buf);
        return nullptr;
      }
      l = curr_ptr;
      curr_ptr = parse_chunk_header(chunk_id, expected_next_chunk_offset, dunno, curr_ptr, end_ptr);
      id_chunk_id = id(chunk_id);
    }

    if (id_chunk_id == id("CNTE")) {
      uint32_t version;
      curr_ptr = read_uint32_be(version, curr_ptr, end_ptr);
    }

    ctx->group_stack.pop_back();

    //ctx->v->EndGroup();
    return curr_ptr;
  }

  const char* parse_colr(Context* ctx, const char* base_ptr, const char* curr_ptr, const char* end_ptr, uint32_t expected_next_chunk_offset) {
    assert(!ctx->group_stack.empty());
    if (ctx->group_stack.back()->kind != Group::Kind::Model) {
      ctx->store->setErrorString("Model chunk unfinished.");
      return nullptr;
    }
    auto* g = ctx->store->newColor(ctx->group_stack.back());
    curr_ptr = read_uint32_be(g->colorKind, curr_ptr, end_ptr);
    curr_ptr = read_uint32_be(g->colorIndex, curr_ptr, end_ptr);
    for (unsigned i = 0; i < 3; i++) {
      curr_ptr = read_uint8(g->rgb[i], curr_ptr, end_ptr);
    }
    curr_ptr += 1;

    if (!verifyOffset(ctx, "COLR", base_ptr, curr_ptr, expected_next_chunk_offset)) return nullptr;

    return curr_ptr;
  }

}

bool parseRVM(class Store* store, const void * ptr, size_t size)
{
  char buf[1024];
  Context ctx = { store,  buf, sizeof(buf) };

  const char* base_ptr = reinterpret_cast<const char*>(ptr);
  const char* curr_ptr = base_ptr;
  const char* end_ptr = curr_ptr + size;

  uint32_t expected_next_chunk_offset, dunno;


  char chunk_id[5] = { 0, 0, 0, 0, 0 };
  curr_ptr = parse_chunk_header(chunk_id, expected_next_chunk_offset, dunno, curr_ptr, end_ptr);
  if (id(chunk_id) != id("HEAD")) {
    snprintf(ctx.buf, ctx.buf_size, "Expected chunk HEAD, got %s", chunk_id);
    store->setErrorString(buf);
    return false;
  }
  curr_ptr = parse_head(&ctx, base_ptr, curr_ptr, end_ptr, expected_next_chunk_offset);
  if (curr_ptr == nullptr) return false;

  curr_ptr = parse_chunk_header(chunk_id, expected_next_chunk_offset, dunno, curr_ptr, end_ptr);
  if (id(chunk_id) != id("MODL")) {
    snprintf(ctx.buf, ctx.buf_size, "Expected chunk MODL, got %s",chunk_id);
    store->setErrorString(buf);
    return false;
  }
  curr_ptr = parse_modl(&ctx, base_ptr, curr_ptr, end_ptr, expected_next_chunk_offset);
  if (curr_ptr == nullptr) return false;

  curr_ptr = parse_chunk_header(chunk_id, expected_next_chunk_offset, dunno, curr_ptr, end_ptr);
  auto id_chunk_id = id(chunk_id);
  while (curr_ptr < end_ptr && id_chunk_id != id("END:")) {
    switch (id_chunk_id) {
    case id("CNTB"):
      curr_ptr = parse_cntb(&ctx, base_ptr, curr_ptr, end_ptr, expected_next_chunk_offset);
      if (curr_ptr == nullptr) return false;
      break;
    case id("PRIM"):
      curr_ptr = parse_prim(&ctx, base_ptr, curr_ptr, end_ptr, expected_next_chunk_offset);
      if (curr_ptr == nullptr) return false;
      break;
    case id("COLR"):
      curr_ptr = parse_colr(&ctx, base_ptr, curr_ptr, end_ptr, expected_next_chunk_offset);
      if (curr_ptr == nullptr) return false;
      break;
    default:
      snprintf(ctx.buf, ctx.buf_size, "Unrecognized chunk %s", chunk_id);
      store->setErrorString(buf);
      return false;
    }
    if (curr_ptr < end_ptr) {
      curr_ptr = parse_chunk_header(chunk_id, expected_next_chunk_offset, dunno, curr_ptr, end_ptr);
      id_chunk_id = id(chunk_id);
    }
  }

  assert(ctx.group_stack.size() == 2);
  ctx.group_stack.pop_back();
  ctx.group_stack.pop_back();

  store->updateCounts();

  return true;
}


