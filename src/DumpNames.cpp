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

  fprintf(out, "\n");

  if (geometries) {
    for (unsigned i = 0; i < stack_p; i++) {
      fprintf(out, "    ");
    }
    fprintf(out, " pgeos=%d\n", geometries);
  }

  if (facetgroups) {
    for (unsigned i = 0; i < stack_p; i++) {
      fprintf(out, "    ");
    }
    fprintf(out, " fgrps=%d\n", facetgroups);
  }
}

void DumpNames::beginGroup(const char* name, const float* translation, const uint32_t material)
{
  printGroupTail();
  printed = false;
  geometries = 0;
  facetgroups = 0;

  stack[stack_p++] = name;
  for (unsigned i = 0; i + 1 < stack_p; i++) {
    fprintf(out, "    ");
  }
  fprintf(out, "%s", name);

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

