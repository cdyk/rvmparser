#include <cassert>
#include "Common.h"
#include "DumpNames.h"
#include "Store.h"


void DumpNames::init(class Store& store)
{
  assert(arena == nullptr);
  assert(stack == nullptr);
  assert(stack_p == 0);
  arena = new Arena();
  stack = (const char**)arena->alloc(sizeof(const char*)*store.groupCountAllocated());
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


void DumpNames::beginFile(struct Group* group)
{
  fprintf(out, "File:\n");
  fprintf(out, "    info:     \"%s\"\n", group->file.info);
  fprintf(out, "    note:     \"%s\"\n", group->file.note);
  fprintf(out, "    date:     \"%s\"\n", group->file.date);
  fprintf(out, "    user:     \"%s\"\n", group->file.user);
  fprintf(out, "    encoding: \"%s\"\n", group->file.encoding);
}

void DumpNames::endFile()
{
}

void DumpNames::beginModel(struct Group* group)
{
  fprintf(out, "Model:\n");
  fprintf(out, "    project:  \"%s\"\n", group->model.project);
  fprintf(out, "    name:     \"%s\"\n", group->model.name);
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

void DumpNames::beginGroup(struct Group* group)
{
  printGroupTail();
  printed = false;
  geometries = 0;
  facetgroups = 0;

  stack[stack_p++] = group->group.name;
  for (unsigned i = 0; i + 1 < stack_p; i++) {
    fprintf(out, "    ");
  }
  fprintf(out, "%s", group->group.name);

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

