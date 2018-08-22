#include <cassert>
#include "DumpNames.h"
#include "Arena.h"
#include "Store.h"


void DumpNames::init(class Store& store)
{
  assert(arena == nullptr);
  assert(stack == nullptr);
  assert(stack_p == 0);
  arena = new Arena();
  stack = (const char**)arena->alloc(sizeof(const char*)*store.grp_n);
};

bool DumpNames::done()
{
  assert(stack_p == 0);
  assert(stack);
  delete arena;

  arena = nullptr;
  stack = nullptr;

  return true;
}


void DumpNames::beginFile(const char* info, const char* note, const char* date, const char* user, const char* encoding)
{
  fprintf(out, "File:\n");
  fprintf(out, "    info:     \"%s\"\n", info);
  fprintf(out, "    note:     \"%s\"\n", note);
  fprintf(out, "    date:     \"%s\"\n", date);
  fprintf(out, "    user:     \"%s\"\n", user);
  fprintf(out, "    encoding: \"%s\"\n", encoding);
}

void DumpNames::endFile()
{
}

void DumpNames::beginModel(const char* project, const char* name)
{
  fprintf(out, "Model:\n");
  fprintf(out, "    project:  \"%s\"\n", project);
  fprintf(out, "    name:     \"%s\"\n", name);
}

void DumpNames::endModel()
{

}

void DumpNames::printGroupTail()
{
  if (printed) return;
  printed = true;
  if (geometries || facetgroups) {
    fprintf(out, ":");
  }

  if (geometries) {
    fprintf(out, " paramgeo=%d", geometries);
  }
  if (facetgroups) {
    fprintf(out, " fgroups=%d", facetgroups);
  }
  fprintf(out, "\n");
}

void DumpNames::beginGroup(const char* name, const float* translation, const uint32_t material)
{
  printGroupTail();
  printed = false;
  geometries = 0;
  facetgroups = 0;

  stack[stack_p++] = name;
  if (stack_p) {
    fprintf(out, "%s", stack[0]);
  }
  for (unsigned i = 1; i < stack_p; i++) {
    fprintf(out, "|%s", stack[i]);
  }

}

void DumpNames::EndGroup()
{
  printGroupTail();
  assert(stack_p != 0);
  stack_p--;
}

void DumpNames::geometry(struct Geometry* geometry)
{
  if (geometry->kind == Geometry::Kind::FacetGroup) {
    facetgroups++;
  }
  else {
    geometries++;
  }
}

void DumpNames::composite(struct Composite* composite)
{

}

