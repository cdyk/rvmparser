#include <cstdio>
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
};

Flatten::Flatten()
{
  ctx = new Context();
}

Flatten::~Flatten()
{
  delete ctx;
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
  if (ctx->pass == 0) {
    ctx->stack = (Group**)ctx->arena.alloc(sizeof(Group*)*store.groupCount());

    fprintf(stderr, "Initial number of tags: %zd\n", ctx->tags.size());
  }
}

void Flatten::beginGroup(Group* group)
{

  ctx->stack[ctx->stack_p++] = group;

  if (ctx->pass == 0) {
    // Add parents to groups to keep

    auto * name = group->group.name;
    if (*name == '/') name++;

    auto it = ctx->tags.find(name);
    if (it != ctx->tags.end()) {
      for (unsigned i = 0; i < ctx->stack_p; i++) {
        ctx->groups.insert(ctx->stack[i]);
      }
    }
  }
}

void Flatten::EndGroup()
{
  ctx->stack_p--;
}


void Flatten::geometry(struct Geometry* geometry)
{


}

bool Flatten::done()
{
  if (ctx->pass == 0) {
    fprintf(stderr, "Number of groups to keep: %zd\n", ctx->groups.size());
  }

  return ctx->pass++ == 0;
}
