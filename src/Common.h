#pragma once
#include <cstdlib>
#include <cstdint>

class Store;

typedef void(*Logger)(unsigned level, const char* msg, ...);

void* xmalloc(size_t size);

void* xcalloc(size_t count, size_t size);

void* xrealloc(void* ptr, size_t size);


struct Arena
{
  ~Arena() { clear(); }

  uint8_t * first = nullptr;
  uint8_t * curr = nullptr;
  size_t fill = 0;
  size_t size = 0;

  void* alloc(size_t bytes);
  void* dup(const void* src, size_t bytes);
  void clear();

  template<typename T> T * alloc() { return new(alloc(sizeof(T))) T(); }
};

struct BufferBase
{
protected:
  char* ptr = nullptr;

  ~BufferBase() { if (ptr) free(ptr - sizeof(size_t)); }

  void _accommodate(size_t typeSize, size_t count)
  {
    if (count == 0) return;
    if (ptr && count <= ((size_t*)ptr)[-1]) return;

    if (ptr) free(ptr - sizeof(size_t));

    ptr = (char*)xmalloc(typeSize * count + sizeof(size_t)) + sizeof(size_t);
    ((size_t*)ptr)[-1] = count;
  }

};

template<typename T>
struct Buffer : public BufferBase
{
  T* data() { return (T*)ptr; }
  T& operator[](size_t ix) { return data()[ix]; }
  const T* data() const { return (T*)ptr; }
  const T& operator[](size_t ix) const { return data()[ix]; }
  void accommodate(size_t count) { _accommodate(sizeof(T), count); }
};


struct Map
{
  ~Map();

  uint64_t* keys = nullptr;
  uint64_t* vals = nullptr;
  size_t fill = 0;
  size_t capacity = 0;

  void clear();

  bool get(uint64_t& val, uint64_t key);
  uint64_t get(uint64_t key);

  void insert(uint64_t key, uint64_t value);
};

struct StringInterning
{
  Arena arena;
  Map map;

  const char* intern(const char* a, const char* b);
  const char* intern(const char* str);  // null terminanted


};


void connect(Store* store, Logger logger);
void align(Store* store, Logger logger);
