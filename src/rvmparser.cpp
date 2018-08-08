#include "RVMParser.h"
#include "RVMVisitor.h"

#include <iostream>
#include <string>
#include <cstdio>
#include <cctype>
#include <vector>

#include <cassert>




namespace {

  struct Context
  {
    RVMVisitor* v;

    // scratch for reading facet groups
    std::vector<uint32_t> polygons;
    std::vector<uint32_t> contours;
    std::vector<float> P;
    std::vector<float> N;


  };

  



  const char* read_uint32_be(uint32_t& rv, const char* p, const char* e)
  {
    auto * q = reinterpret_cast<const uint8_t*>(p);
    rv = q[0]<<24 | q[1]<<16 | q[2]<<8 | q[3];
    return p + 4;
  }

  const char* read_float32_be(float& rv, const char* p, const char* e)
  {
    union {
      float f;
      uint32_t u;
    };

    auto * q = reinterpret_cast<const uint8_t*>(p);
    u = q[0] << 24 | q[1] << 16 | q[2] << 8 | q[3];
    rv = f;
    return p + 4;
  }



  constexpr uint32_t id(const char* str)
  {
    return str[3] << 24 | str[2] << 16 | str[1] << 8 | str[0];
  }




const char* parse_chunk_header(char* id, uint32_t& len, uint32_t& dunno, const char* p, const char* e)
{
  unsigned i = 0;
  for (i = 0; i < 4 && p + 4 <= e; i++) {
    assert(p[0] == 0);
    assert(p[1] == 0);
    assert(p[2] == 0);
    id[i] = p[3];
    p += 4;
  }
  for (; i < 4; i++) {
    id[i] = ' ';
  }
  if (p + 8 <= e) {
    p = read_uint32_be(len, p, e);
    p = read_uint32_be(dunno, p, e);
  }
  else {
    len = ~0;
    dunno = ~0;
    fprintf(stderr, "Chunk '%s' EOF after %zd bytes\n", id, e-p);
    p = e;
  }
  return p;
}

const char* parse_string(std::string& str, const char* p, const char* e)
{
  uint32_t len;
  p = read_uint32_be(len, p, e);
  str.resize(4 * len);
  unsigned i;
  for (i = 0; i < 4 * len; i++) {
    if (p[i] == 0) break;
    str[i] = p[i];
  }
  str.resize(i);
  return p + 4 * len;
}



const char* parse_head(Context* ctx, const char* p, const char* e)
{
  uint32_t version;
  p = read_uint32_be(version, p, e);
  std::string info, note, date, user, encoding;
  p = parse_string(info, p, e);
  p = parse_string(note, p, e);
  p = parse_string(date, p, e);
  p = parse_string(user, p, e);
  if (2 <= version) {
    p = parse_string(encoding, p, e);
  }
  ctx->v->beginFile(info, note, date, user, encoding);
  return p;
}

const char* parse_modl(Context* ctx, const char* p, const char* e)
{
  uint32_t version;
  p = read_uint32_be(version, p, e);
  std::string project, name;
  p = parse_string(project, p, e);
  p = parse_string(name, p, e);
  ctx->v->beginModel(project, name);
  return p;
}

const char* parse_prim(Context* ctx, const char* p, const char* e)
{
  uint32_t version, kind;
  p = read_uint32_be(version, p, e);
  p = read_uint32_be(kind, p, e);

  float M[12];
  for (unsigned i = 0; i < 12; i++) {
    p = read_float32_be(M[i], p, e);
  }

  float bbox[6];
  for (unsigned i = 0; i < 6; i++) {
    p = read_float32_be(bbox[i], p, e);
  }

  switch (kind) {
  case 1: {
    float bottom[2], top[2], offset[2], height;
    p = read_float32_be(bottom[0], p, e);
    p = read_float32_be(bottom[1], p, e);
    p = read_float32_be(top[0], p, e);
    p = read_float32_be(top[1], p, e);
    p = read_float32_be(height, p, e);
    p = read_float32_be(offset[0], p, e);
    p = read_float32_be(offset[1], p, e);
    ctx->v->pyramid(M, bbox, bottom, top, offset, height);
    break;
  }
  case 2: {
    float lengths[3];
    p = read_float32_be(lengths[0], p, e);
    p = read_float32_be(lengths[1], p, e);
    p = read_float32_be(lengths[2], p, e);
    ctx->v->box(M, bbox, lengths);
    break;
  }
  case 3: {
    float inner_radius, outer_radius, height, angle;
    p = read_float32_be(inner_radius, p, e);
    p = read_float32_be(outer_radius, p, e);
    p = read_float32_be(height, p, e);
    p = read_float32_be(angle, p, e);
    ctx->v->rectangularTorus(M, bbox, inner_radius, outer_radius, height, angle);
    break;
  }
  case 4: {
    float offset, radius, angle;
    p = read_float32_be(offset, p, e);
    p = read_float32_be(radius, p, e);
    p = read_float32_be(angle, p, e);
    ctx->v->circularTorus(M, bbox, offset, radius, angle);
    break;
  }
  case 5: {
    float diameter, radius;
    p = read_float32_be(diameter, p, e);
    p = read_float32_be(radius, p, e);
    ctx->v->ellipticalDish(M, bbox, diameter, radius);
    break;
  }
  case 6: {
    float diameter, height;
    p = read_float32_be(diameter, p, e);
    p = read_float32_be(height, p, e);
    ctx->v->sphericalDish(M, bbox, diameter, height);
    break;
  }
  case 7: {
    float bottom, top, height, offset[2], bshear[2], tshear[2];
    p = read_float32_be(bottom, p, e);
    p = read_float32_be(top, p, e);
    p = read_float32_be(height, p, e);
    p = read_float32_be(offset[0], p, e);
    p = read_float32_be(offset[1], p, e);
    p = read_float32_be(bshear[0], p, e);
    p = read_float32_be(bshear[1], p, e);
    p = read_float32_be(tshear[0], p, e);
    p = read_float32_be(tshear[1], p, e);
    ctx->v->snout(M, bbox, offset, bshear, tshear, bottom, top, height);
    break;
  }
  case 8: {
    float radius, height;
    p = read_float32_be(radius, p, e);
    p = read_float32_be(height, p, e);
    ctx->v->cylinder(M, bbox, radius, height);
    break;
  }
  case 9: {
    float diameter;
    p = read_float32_be(diameter, p, e);
    ctx->v->sphere(M, bbox, diameter);
    break;
  }
  case 10:
    fprintf(stderr, "  +- line\n");
    p += 4 * 2;
    break;
  case 11:
  {
    ctx->P.clear();
    ctx->N.clear();

    ctx->polygons.clear();

    ctx->contours.clear();


    uint32_t polygons_n;
    p = read_uint32_be(polygons_n, p, e);
    //fprintf(stderr, "= Group, polygons_n=%d\n", polygons_n);

    for (unsigned pi = 0; pi < polygons_n; pi++) {
      uint32_t countours_n;
      p = read_uint32_be(countours_n, p, e);
      //fprintf(stderr, "  = Polygon, countours_n=%d\n", countours_n);

      ctx->polygons.push_back(uint32_t(ctx->contours.size()));
      for (unsigned gi = 0; gi < countours_n; gi++) {
        uint32_t vertices_n;
        p = read_uint32_be(vertices_n, p, e);
        //fprintf(stderr, "    = Countour, vertices_n=%d\n", vertices_n);

        ctx->contours.push_back(uint32_t(ctx->P.size()));
        for (unsigned vi = 0; vi < vertices_n; vi++) {
          float t;
          for (unsigned i = 0; i < 3; i++) {
            p = read_float32_be(t, p, e);
            ctx->P.push_back(t);
          }
          for (unsigned i = 0; i < 3; i++) {
            p = read_float32_be(t, p, e);
            ctx->N.push_back(t);
          }
        }
      }
      ctx->contours.push_back(uint32_t(ctx->P.size()));

    }
    ctx->polygons.push_back(uint32_t(ctx->contours.size()));


    ctx->v->facetGroup(M, bbox, ctx->polygons, ctx->contours, ctx->P, ctx->N);
    break;
  }
  default:
    fprintf(stderr, "Unknown primitive kind %d\n", kind);
    assert(false);
    break;
  }
  return p;
}

const char* parse_cntb(Context* ctx, const char* p, const char* e)
{
  uint32_t version;
  p = read_uint32_be(version, p, e);

  std::string name;
  p = parse_string(name, p, e);

  float translation[3];
  p = read_float32_be(translation[0], p, e);
  p = read_float32_be(translation[1], p, e);
  p = read_float32_be(translation[2], p, e);

  uint32_t material_id;
  p = read_uint32_be(material_id, p, e);
  ctx->v->beginGroup(name, translation, material_id);


  // process children
  char chunk_id[5] = { 0, 0, 0, 0, 0 };
  auto l = p;
  uint32_t len, dunno;
  p = parse_chunk_header(chunk_id, len, dunno, p, e);
  auto id_chunk_id = id(chunk_id);
  while (p < e && id_chunk_id != id("CNTE")) {
    switch (id_chunk_id) {
    case id("CNTB"):
      p = parse_cntb(ctx, p, e);
      break;
    case id("PRIM"):
      p = parse_prim(ctx, p, e);
      break;
    default:
      fprintf(stderr, "Unknown chunk id '%s", chunk_id);
      assert(false);
    }
    l = p;
    p = parse_chunk_header(chunk_id, len, dunno, p, e);
    id_chunk_id = id(chunk_id);
  }

  if (id_chunk_id == id("CNTE")) {
    uint32_t a; // no idea of what this flag is.
    p = read_uint32_be(a, p, e);
  }

  ctx->v->EndGroup();
  return p;
}

}

void parseRVM(RVMVisitor* v, const void * ptr, size_t size)
{
  Context ctx = { v };

  auto * p = reinterpret_cast<const char*>(ptr);
  auto * e = p + size;
  uint32_t len, dunno;

  auto l = p;

  char chunk_id[5] = { 0, 0, 0, 0, 0 };
  p = parse_chunk_header(chunk_id, len, dunno, p, e);
  assert(id(chunk_id) == id("HEAD"));
  p = parse_head(&ctx, p, e);
  assert(p - l == len);

  l = p;
  p = parse_chunk_header(chunk_id, len, dunno, p, e);
  assert(id(chunk_id) == id("MODL"));
  p = parse_modl(&ctx, p, e);

  l = p;
  p = parse_chunk_header(chunk_id, len, dunno, p, e);
  auto id_chunk_id = id(chunk_id);
  while (p < e && id_chunk_id != id("END:")) {
    switch (id_chunk_id) {
    case id("CNTB"):
      p = parse_cntb(&ctx, p, e);
      break;
    case id("PRIM"):
      p = parse_prim(&ctx, p, e);
      break;
    default:
      fprintf(stderr, "Unknown chunk id '%s", chunk_id);
      assert(false);
    }
    l = p;
    p = parse_chunk_header(chunk_id, len, dunno, p, e);
    id_chunk_id = id(chunk_id);
  }

  v->endModel();
  v->endFile();

}


