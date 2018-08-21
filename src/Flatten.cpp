#include <cstdio>
#include <cassert>
#include <unordered_set>
#include "Store.h"
#include "Flatten.h"
#include "Arena.h"

struct Context
{
  std::unordered_set<std::string> tags;
  std::unordered_set<Group*> groups;
  Arena arena;
  unsigned pass = 0;

  Group** stack = nullptr;
  unsigned stack_p = 0;
  unsigned ignore_n = 0;

  Store* store = nullptr;
};

Flatten::Flatten()
{
  ctx = new Context();
}

Flatten::~Flatten()
{
  if (ctx->store != nullptr) delete ctx->store;
  delete ctx;
}

Store* Flatten::result()
{
  auto rv = ctx->store;
  ctx->store = nullptr;
  return rv;
}



void Flatten::setKeep(const void * ptr, size_t size)
{
  auto * a = (const char*)ptr;
  auto * b = a + size;

  while (true) {
    while (a < b && (*a == '\n' || *a == '\r')) a++;
    auto * c = a;
    while (a < b && (*a != '\n' && *a != '\r')) a++;

    if (c < a) {
      auto * d = a - 1;
      while (c < d && (d[-1] != '\t')) --d;
      std::string tag(d, a - d);
      ctx->tags.insert(tag);
    }
    else {
      break;
    }
  }
}


void Flatten::init(class Store& store)
{
  ctx->stack = (Group**)ctx->arena.alloc(sizeof(Group*)*store.groupCount());
  fprintf(stderr, "Initial number of tags: %zd\n", ctx->tags.size());
  ctx->store = new Store();
}


void Flatten::beginFile(Group* group)
{
  if (ctx->pass == 1) {
    assert(ctx->stack_p == 0);
    ctx->stack[ctx->stack_p] = ctx->store->cloneGroup(nullptr, group);
    ctx->stack_p++;
  }
}

void Flatten::endFile()
{
  if (ctx->pass == 1) {
    assert(ctx->stack_p == 1);
    ctx->stack_p--;
  }
}

void Flatten::beginModel(Group* group)
{
  if (ctx->pass == 1) {
    assert(ctx->stack_p == 1);
    ctx->stack[ctx->stack_p] = ctx->store->cloneGroup(ctx->stack[ctx->stack_p - 1], group);
    ctx->stack_p++;
  }
}

void Flatten::endModel()
{
  if (ctx->pass == 1) {
    assert(ctx->stack_p == 2);
    ctx->stack_p--;
  }
}


void Flatten::beginGroup(Group* group)
{
  if (ctx->pass == 0) {
    // Add parents to groups to keep

    ctx->stack[ctx->stack_p++] = group;
    auto * name = group->group.name;
    if (*name == '/') name++;

    auto it = ctx->tags.find(name);
    if (ctx->stack_p == 1 || it != ctx->tags.end()) {   // Tag found or is root group, include.
      for (unsigned i = 0; i < ctx->stack_p; i++) {
        ctx->groups.insert(ctx->stack[i]);
      }
    }
  }

  else if (ctx->pass == 1) {
    assert(1 < ctx->stack_p);
    auto it = ctx->groups.find(group);
    if (it == ctx->groups.end()) {
      ctx->ignore_n++;
    }
    else {
      assert(ctx->ignore_n == 0);
      ctx->stack[ctx->stack_p] = ctx->store->cloneGroup(ctx->stack[ctx->stack_p - 1], group);
      ctx->stack_p++;
    }
  }

}

void Flatten::EndGroup()
{
  if (ctx->pass == 0) {
    ctx->stack_p--;
  }

  else if(ctx->pass == 1) {

    if (ctx->ignore_n) {
      ctx->ignore_n--;
    }
    else {
      assert(ctx->stack_p);
      ctx->stack_p--;
    }
  }
}


void Flatten::geometry(struct Geometry* geometry)
{
  if (ctx->pass == 1) {
    assert(2 < ctx->stack_p);
    ctx->store->cloneGeometry(ctx->stack[ctx->stack_p - 1], geometry);
  }
}

bool Flatten::done()
{
  if (ctx->pass == 0) {
    fprintf(stderr, "Number of groups to keep: %zd\n", ctx->groups.size());
  }

  else if (ctx->pass == 1) {
    fprintf(stderr, "second pass done.\n");

  }

  return ctx->pass++ == 1;
}
