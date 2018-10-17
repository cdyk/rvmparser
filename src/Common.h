#pragma once
#include <cstdint>

class Store;

struct Triangulation;

typedef void(*Logger)(unsigned level, const char* msg, ...);

void* xmalloc(size_t size);

void* xcalloc(size_t count, size_t size);

void* xrealloc(void* ptr, size_t size);

uint64_t fnv_1a(const char* bytes, size_t l);


struct Arena
{
  Arena() = default;
  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;

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

  ~BufferBase() { free(); }

  void free();

  void _accommodate(size_t typeSize, size_t count)
  {
    if (count == 0) return;
    if (ptr && count <= ((size_t*)ptr)[-1]) return;

    free();

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
  Map() = default;
  Map(const Map&) = delete;
  Map& operator=(const Map&) = delete;


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

uint64_t fnv_1a(const char* bytes, size_t l);
uint64_t fnv_1a(const char* bytes, size_t l);


void connect(Store* store, Logger logger);
void align(Store* store, Logger logger);
bool exportJson(Store* store, Logger logger, const char* path);
bool discardGroups(Store* store, Logger logger, const void* ptr, size_t size);
