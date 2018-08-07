#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iostream>
#include <string>
#include <cstdio>
#include <cctype>

#include <cassert>

class RVMVisitor
{

  virtual bool pyramid(float* M_affine, float* bbox, float* bottom_xy, float* top_xy, float* offset_xy, float height) = 0;

  virtual bool rectangularTorus(float* M_affine, float* bbox, float inner_radius, float outer_radius, float height, float angle) = 0;

  virtual bool circularTorus(float* M_affine, float* bbox, float offset, float radius, float angle) = 0;

  virtual bool ellipticalDish(float* M_affine, float* bbox, float diameter, float radius) = 0;

};



namespace {

  class DummyVisitor : public RVMVisitor
  {
    bool pyramid(float* M_affine, float* bbox, float* bottom_xy, float* top_xy, float* offset_xy, float height) override
    {
      fprintf(stderr, "  +- pyramid: b=[%f, %f], t=[%f, %f], h=%f, o=[%f,%f]\n",
              bottom_xy[0], bottom_xy[1], top_xy[0], top_xy[1], height, offset_xy[0], offset_xy[1]);
      return true;
    }

    bool rectangularTorus(float* M_affine, float* bbox, float inner_radius, float outer_radius, float height, float angle) override
    {
      fprintf(stderr, "  +- recTorus: r_i=%f, r_o=%f, h=%f, a=%f\n", inner_radius, outer_radius, height, angle);
      return true;
    }

    bool circularTorus(float* M_affine, float* bbox, float offset, float radius, float angle) override
    {
      fprintf(stderr, "  +- circTorus: o=%f, r=%f, a=%f\n", offset, radius, angle);
      return true;
    }

    bool ellipticalDish(float* M_affine, float* bbox, float diameter, float radius) override
    {
      fprintf(stderr, "  +- eliDish: d=%f, r=%f\n", diameter, radius);
      return true;
    }


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


}


const char* parse_chunk_header(char* id, const char* p, const char* e)
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
    uint32_t a, b;
    p = read_uint32_be(a, p, e);
    p = read_uint32_be(b, p, e);
    fprintf(stderr, "Chunk '%s' a=%d, b=%d\n", id, a, b);
  }
  else {
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



const char* parse_head(const char* p, const char* e)
{
  uint32_t version;
  p = read_uint32_be(version, p, e);
  std::string banner, note, date, user, encoding;
  p = parse_string(banner, p, e);
  p = parse_string(note, p, e);
  p = parse_string(date, p, e);
  p = parse_string(user, p, e);
  if (2 <= version) {
    p = parse_string(encoding, p, e);
  }
  fprintf(stderr, "  +- version  '%d'\n", version);
  fprintf(stderr, "  +- banner   '%s'\n", banner.c_str());
  fprintf(stderr, "  +- note     '%s'\n", note.c_str());
  fprintf(stderr, "  +- date     '%s'\n", date.c_str());
  fprintf(stderr, "  +- user     '%s'\n", user.c_str());
  fprintf(stderr, "  +- encoding '%s'\n", encoding.c_str());
  return p;
}

const char* parse_modl(const char* p, const char* e)
{
  uint32_t version;
  p = read_uint32_be(version, p, e);
  std::string project, name;
  p = parse_string(project, p, e);
  p = parse_string(name, p, e);
  fprintf(stderr, "  +- project  '%s'\n", project.c_str());
  fprintf(stderr, "  +- name     '%s'\n", name.c_str());
  return p;
}

const char* parse_prim(const char* p, const char* e)
{
  uint32_t version, kind;
  p = read_uint32_be(version, p, e);
  p = read_uint32_be(kind, p, e);

  float M[12];
  for (unsigned i = 0; i < 12; i++) {
    p = read_float32_be(M[i], p, e);
  }

  float BBox[6];
  for (unsigned i = 0; i < 6; i++) {
    p = read_float32_be(BBox[i], p, e);
  }

  switch (kind) {
  case 1:
    fprintf(stderr, "  +- pyramid\n");
    p += 4 * 7;
    break;
  case 2:
    fprintf(stderr, "  +- box\n");
    p += 4 * 3;
    break;
  case 3:
    fprintf(stderr, "  +- rectangular torus\n");
    p += 4 * 4;
    break;
  case 4:
    fprintf(stderr, "  +- circular torus\n");
    p += 4 * 3;
    break;
  case 5:
    fprintf(stderr, "  +- elliptical dish\n");
    p += 4 * 2;
    break;
  case 6:
    fprintf(stderr, "  +- spherical dish\n");
    p += 4 * 2;
    break;
  case 7:
    fprintf(stderr, "  +- snout\n");
    p += 4 * 9;
    break;
  case 8:
    fprintf(stderr, "  +- cylinder\n");
    p += 4 * 2;
    break;
  case 9:
    fprintf(stderr, "  +- sphere\n");
    p += 4;
    break;
  case 10:
    fprintf(stderr, "  +- line\n");
    p += 4 * 2;
    break;
  case 11:
  {
    fprintf(stderr, "  +- facet group\n");
    uint32_t pn;
    p = read_uint32_be(pn, p, e);
    for (unsigned pi = 0; pi < pn; pi++) {
      uint32_t gn;
      p = read_uint32_be(gn, p, e);
      for (unsigned gi = 0; gi < gn; gi++) {
        uint32_t vn;
        p = read_uint32_be(vn, p, e);
        for (unsigned vi = 0; vi < vn; vi++) {
          float ab[6]; // p0 xyz, p1 xyz
          for (unsigned i = 0; i < 6; i++) {
            p = read_float32_be(ab[i], p, e);
          }
        }
      }
    }
    break;
  }
  default:
    fprintf(stderr, "Unknown primitive kind %d\n", kind);
    assert(false);
    break;
  }
  return p;
}

const char* parse_cntb(const char* p, const char* e)
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

  fprintf(stderr, "Group '%s' @ [%f, %f, %f] mat=%d {\n",
          name.c_str(), translation[0], translation[1], translation[2], material_id);

  // todo: process attributes in txt-file that starts with "NEW <name>"

  // process children
  char chunk_id[5] = { 0, 0, 0, 0, 0 };
  auto l = p;
  p = parse_chunk_header(chunk_id, p, e);
  auto id_chunk_id = id(chunk_id);
  while (p < e && id_chunk_id != id("CNTE")) {
    switch (id_chunk_id) {
    case id("CNTB"):
      p = parse_cntb(p, e);
      break;
    case id("PRIM"):
      p = parse_prim(p, e);
      break;
    default:
      fprintf(stderr, "Unknown chunk id '%s", chunk_id);
      assert(false);
    }
    fprintf(stderr, "  => %zd bytes\n", (p - l));
    l = p;
    p = parse_chunk_header(chunk_id, p, e);
    id_chunk_id = id(chunk_id);
  }

  if (id_chunk_id == id("CNTE")) {
    uint32_t a;
    p = read_uint32_be(a, p, e);
    fprintf(stderr, "} of group '%s' a=%dn", name.c_str(), a);
  }

  return p;
}


void parse(const void * ptr, size_t size)
{
  auto * p = reinterpret_cast<const char*>(ptr);
  auto * e = p + size;

  char chunk_id[5] = { 0, 0, 0, 0, 0 };
  p = parse_chunk_header(chunk_id, p, e);
  assert(id(chunk_id) == id("HEAD"));
  p = parse_head(p, e);

  p = parse_chunk_header(chunk_id, p, e);
  assert(id(chunk_id) == id("MODL"));
  p = parse_modl(p, e);

  auto l = p;
  p = parse_chunk_header(chunk_id, p, e);
  auto id_chunk_id = id(chunk_id);
  while (p < e && id_chunk_id != id("END:")) {
    switch (id_chunk_id) {
    case id("CNTB"):
      p = parse_cntb(p, e);
      break;
    case id("PRIM"):
      p = parse_prim(p, e);
      break;
    default:
      fprintf(stderr, "Unknown chunk id '%s", chunk_id);
      assert(false);
    }
    fprintf(stderr, "  => %zd bytes\n", (p - l));
    l = p;
    p = parse_chunk_header(chunk_id, p, e);
    id_chunk_id = id(chunk_id);
  }
}


int main(int argc, char** argv)
{
  assert(argc == 2);

  HANDLE h = CreateFileA(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  assert(h != INVALID_HANDLE_VALUE);

  DWORD hiSize;
  DWORD loSize = GetFileSize(h, &hiSize);
  size_t fileSize =  (size_t(hiSize) << 32u) + loSize;

  HANDLE m = CreateFileMappingA(h, 0, PAGE_READONLY, 0, 0, NULL);
  assert(m != INVALID_HANDLE_VALUE);

  const void * ptr = MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
  assert(ptr != nullptr);

  parse(ptr, fileSize);

  UnmapViewOfFile(ptr);

  CloseHandle(m);

  CloseHandle(h);

  char q;
  std::cin >> q;

  return 0;
}

