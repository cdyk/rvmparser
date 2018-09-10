#include <cstdio>
#include <cassert>

#include "Store.h"
#include "Flatten.h"

Flatten::Flatten(Store* srcStore) :
  srcStore(srcStore)
{
  dstStore = new Store();
}

Flatten::~Flatten()
{
  if (dstStore != nullptr) delete dstStore;
}

Store* Flatten::result()
{
  auto rv = dstStore;
  dstStore = nullptr;
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

      auto * str = srcStore->strings.intern(d, a);
      tags.insert(uint64_t(str), uint64_t(str));
      //std::string tag(d, a - d);
      //ctx->tags.insert(tag);
    }
    else {
      break;
    }
  }
}

void Flatten::keepTag(const char* tag)
{
  auto * str = srcStore->strings.intern(tag);
  tags.insert(uint64_t(str), uint64_t(str));
}


void Flatten::init(class Store& otherStore)
{
  assert(pass == 0);
  stack = (Group**)arena.alloc(sizeof(Group*)*otherStore.groupCount());
  fprintf(stderr, "Initial number of tags: %zd\n", tags.fill);
}


void Flatten::beginFile(Group* group)
{
  if (pass == 1) {
    assert(stack_p == 0);
    stack[stack_p] = dstStore->cloneGroup(nullptr, group);
    stack_p++;
  }
}

void Flatten::endFile()
{
  if (pass == 1) {
    assert(stack_p == 1);
    stack_p--;
  }
}

void Flatten::beginModel(Group* group)
{
  if (pass == 1) {
    assert(stack_p == 1);
    stack[stack_p] = dstStore->cloneGroup(stack[stack_p - 1], group);
    stack_p++;
  }
}

void Flatten::endModel()
{
  if (pass == 1) {
    assert(stack_p == 2);
    stack_p--;
  }
}


void Flatten::beginGroup(Group* group)
{
  if (pass == 0) {

    // Add parents to groups to keep
    stack[stack_p++] = group;
    if (stack_p == 1 || tags.get(uint64_t(group->group.name))) {
      for (unsigned i = 0; i < stack_p; i++) {
        groups.insert(uint64_t(stack[i]), uint64_t(stack[i]));
      }
    }

    //auto * name = group->group.name;
    //auto it = ctx->tags.find(name);
    //if (ctx->stack_p == 1 || it != ctx->tags.end()) {   // Tag found or is root group, include.
    //  for (unsigned i = 0; i < ctx->stack_p; i++) {
    //    ctx->groups.insert(ctx->stack[i]);
    //  }
    //}
  }

  else if (pass == 1) {
    assert(1 < stack_p);

    if (groups.get(uint64_t(group))) {
      assert(ignore_n == 0);
      stack[stack_p] = dstStore->cloneGroup(stack[stack_p - 1], group);
      stack_p++;
    }
    else {
      ignore_n++;
    }

    //auto it = ctx->groups.find(group);
    //if (it == ctx->groups.end()) {
    //  ctx->ignore_n++;
    //}
    //else {
    //  assert(ctx->ignore_n == 0);
    //  ctx->stack[ctx->stack_p] = ctx->store->cloneGroup(ctx->stack[ctx->stack_p - 1], group);
    //  ctx->stack_p++;
    //}
  }

}

void Flatten::EndGroup()
{
  if (pass == 0) {
    stack_p--;
  }

  else if(pass == 1) {

    if (ignore_n) {
      ignore_n--;
    }
    else {
      assert(stack_p);
      stack_p--;
    }
  }
}


void Flatten::geometry(struct Geometry* geometry)
{
  if (pass == 1) {
    assert(2 < stack_p);
    dstStore->cloneGeometry(stack[stack_p - 1], geometry);
  }
}

bool Flatten::done()
{
  if (pass == 0) {
    fprintf(stderr, "Number of groups to keep: %zd\n", groups.fill);
  }

  else if (pass == 1) {
    fprintf(stderr, "second pass done.\n");

  }

  return pass++ == 1;
}
