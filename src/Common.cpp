#include "Common.h"
#include <cstdlib>
#include <algorithm>
#include <cassert>

namespace {

  template<typename T>
  constexpr bool isPow2(T x)
  {
    return x != 0 && (x & (x - 1)) == 0;
  }

  uint64_t fnv_1a(const char* bytes, size_t l)
  {
    uint64_t hash = 0xcbf29ce484222325;
    for (size_t i = 0; i < l; i++) {
      hash = hash ^ bytes[i];
      hash = hash * 0x100000001B3;
    }
    return hash;
  }

  uint64_t hash_uint64(uint64_t x)
  {
    x *= 0xff51afd7ed558ccd;
    x ^= x >> 32;
    return x;
  }

  void* xmalloc(size_t size)
  {
    auto rv = malloc(size);
    if (rv != nullptr) return rv;

    fprintf(stderr, "Failed to allocate memory.");
    exit(-1);
  }

  void* xcalloc(size_t count, size_t size)
  {
    auto rv = calloc(count, size);
    if (rv != nullptr) return rv;

    fprintf(stderr, "Failed to allocate memory.");
    exit(-1);
  }



}

void* Arena::alloc(size_t bytes)
{
  const size_t pageSize = 1024 * 1024;

  if (bytes == 0) return nullptr;

  auto padded = (bytes + 7) & ~7;

  if (size < fill + padded) {
    fill = sizeof(uint8_t*);
    size = std::max(pageSize, fill + padded);

    auto * page = (uint8_t*)xmalloc(size);
    *(uint8_t**)page = nullptr;

    if (first == nullptr) {
      first = page;
      curr = page;
    }
    else {
      *(uint8_t**)curr = page; // update next
      curr = page;
    }
  }

  assert(first != nullptr);
  assert(curr != nullptr);
  assert(*(uint8_t**)curr == nullptr);
  assert(fill + padded <= size);

  auto * rv = curr + fill;
  fill += padded;
  return rv;
}

void* Arena::dup(const void* src, size_t bytes)
{
  auto * dst = alloc(bytes);
  std::memcpy(dst, src, bytes);
  return dst;
}


void Arena::clear()
{
  auto * c = first;
  while (c != nullptr) {
    auto * n = *(uint8_t**)c;
    free(c);
    c = n;
  }
  first = nullptr;
  curr = nullptr;
  fill = 0;
  size = 0;
}

Map::~Map()
{
  if (keys) free(keys);
  if (vals) free(vals);
}

uint64_t Map::get(uint64_t key)
{
  if (fill == 0) return 0;

  auto mask = capacity - 1;
  for (auto i = size_t(hash_uint64(key)); true ; i++) { // linear probing
    i = i & mask;
    if (keys[i] == key) {
      return vals[i];
    }
    else if (keys[i] == 0) {
      return 0;
    }
  }
}

void Map::insert(uint64_t key, uint64_t value)
{
  assert(key != 0);     // null is used to denote no-key
  assert(value != 0);   // null value is used to denote not found

  if (capacity <= 2 * fill) {
    auto old_fill = fill;
    auto old_keys = keys;
    auto old_vals = vals;
    
    fill = 0;
    capacity = capacity ? 2 * capacity : 16;
    keys = (uint64_t*)xcalloc(capacity, sizeof(uint64_t));
    vals = (uint64_t*)malloc(capacity * sizeof(uint64_t));

    for (size_t i = 0; i < old_fill; i++) {
      if (old_keys[i]) insert(old_keys[i], old_vals[i]);
    }

    free(old_keys);
    free(old_vals);
  }

  assert(isPow2(capacity));
  auto mask = capacity - 1;
  for (auto i = size_t(hash_uint64(key)); true; i++) { // linear probing
    i = i & mask;
    if (keys[i] == key) {
      vals[i] = value;
      break;
    }
    else if (keys[i] == 0) {
      keys[i] = key;
      vals[i] = value;
      fill++;
      break;
    }
  }

}

namespace {

  struct StringHeader
  {
    StringHeader* next;
    size_t length;
    char string[1];
  };

}

const char* StringInterning::intern(const char* str)
{
  return intern(str, str + strlen(str));
}

const char* StringInterning::intern(const char* a, const char* b)
{
  auto length = b - a;
  auto hash = fnv_1a(a, length);
  hash = hash ? hash : 1;

  auto * intern = (StringHeader*)map.get(hash);
  for (auto * it = intern; it != nullptr; it = it->next) {
    if (it->length == length) {
      if (strncmp(it->string, a, length) == 0) {
        return it->string;
      }
    }
  }

  auto * newIntern = (StringHeader*)arena.alloc(sizeof(StringHeader) + length);
  newIntern->next = intern;
  newIntern->length = length;
  std::memcpy(newIntern->string, a, length);
  newIntern->string[length] = '\0';
  //fprintf(stderr, "new string '%s'\n", newIntern->string);
  map.insert(hash, uint64_t(newIntern));
  return newIntern->string;
}
