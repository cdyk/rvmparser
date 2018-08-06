#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iostream>
#include <string>
#include <cstdio>

#include <cassert>

namespace {

  uint32_t read_uint32_be(const char* p)
  {
    auto * q = reinterpret_cast<const uint8_t*>(p);
    return q[0]<<24 | q[1]<<16 | q[2]<<8 | q[3];
  }

  constexpr uint32_t id(const char* str)
  {
    return str[3] << 24 | str[2] << 16 | str[1] << 8 | str[0];
  }


}


const char* parse_chunk_header(std::string& id, const char* p, const char* e)
{
  if (p + 16 + 8 <= e) {
    id.resize(4);
    for (unsigned i = 0; i < 4; i++) {
      assert(p[4 * i + 0] == 0);
      assert(p[4 * i + 1] == 0);
      assert(p[4 * i + 2] == 0);
      id[i] = p[4 * i + 3];
    }

    fprintf(stderr, "Chunk '%s' a=%d, b=%d\n", id.c_str(), read_uint32_be(p+16), read_uint32_be(p+20));

    return p + 16 + 8;
  }
  else {
    id.clear();
    return p;
  }
}

const char* parse_string(std::string& str, const char* p, const char* e)
{
  auto len = read_uint32_be(p); p += 4;
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
  std::string banner, note, date, user, encoding;

  auto version = read_uint32_be(p); p += 4;
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


void parse(const void * ptr, size_t size)
{
  auto * p = reinterpret_cast<const char*>(ptr);
  auto * e = p + size;

  std::string chunk_id;
  while (p < e) {
    auto l = p;
    p = parse_chunk_header(chunk_id, p, e);
    if (chunk_id.size() == 4) {
      switch (id(chunk_id.c_str())) {
      case id("HEAD"):
        p = parse_head(p, e);
        break;

      default:
        p = e;
        break;
      }
    }
    else {
      p = e;
    }
    fprintf(stderr, "  => %zd bytes\n", (p - l));
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

