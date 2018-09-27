#pragma once

#include "ExportJson.h"
#include "Store.h"

namespace {


  void doIndent(FILE* out, unsigned indent)
  {
    const char * spaces = "                                                                                ";
    const auto N = unsigned(sizeof(spaces) - 1);

    while (0 < indent) {
      auto n = indent < N ? indent : N;
      fwrite(spaces, 1, n, out);
      indent -= n;
    }
  }


}



ExportJson::~ExportJson()
{
  if (json) {
    fclose(json);
  }
}

bool ExportJson::open(const char* path_json)
{
  auto err = fopen_s(&json, path_json, "w");
  if (err == 0) return true; // OK

  char buf[1024];
  if (strerror_s(buf, sizeof(buf), err) != 0) {
    buf[0] = '\0';
  }
  fprintf(stderr, "Failed to open %s for writing: %s", path_json, buf);
  return false;
}

void ExportJson::init(class Store& store)
{
  indent = 0;
}

void ExportJson::beginFile(Group* group)
{
}

void ExportJson::endFile()
{
}

void ExportJson::beginModel(Group* group)
{
}

void ExportJson::endModel()
{
}

void ExportJson::beginGroup(struct Group* group)
{
  doIndent(json, indent);
  fprintf(json, "{ \"name\": \"%s\",\n", group->group.name);
  indent += 2;
  doIndent(json, indent); fprintf(json, "\"material\": %u,\n", group->group.material);
  if (group->group.bbox) {
    doIndent(json, indent); fprintf(json, "\"bbox\": [%.3f, %.3f, %.3f, %.3f, %.3f, %.3f],\n",
                                    group->group.bbox[0],
                                    group->group.bbox[1],
                                    group->group.bbox[2],
                                    group->group.bbox[3],
                                    group->group.bbox[4],
                                    group->group.bbox[5]);
  }


}

void ExportJson::EndGroup()
{
  indent -= 2;
  doIndent(json, indent);
  fprintf(json, "},\n");
}

void ExportJson::beginChildren(struct Group* container)
{
  doIndent(json, indent);
  fprintf(json, "\"children\": [\n");
  indent += 2;
}

void ExportJson::endChildren()
{
  indent -= 2;
  doIndent(json, indent);
  fprintf(json, "]\n");
}

void ExportJson::beginAttributes(struct Group* container)
{
  doIndent(json, indent);
  fprintf(json, "\"attributes\": {\n");
  indent += 2;
}

void ExportJson::attribute(const char* key, const char* val)
{
  doIndent(json, indent);
  fprintf(json, "\"%s\": \"%s\",\n", key, val);
}

void ExportJson::endAttributes(struct Group* container)
{
  indent -= 2;
  doIndent(json, indent);
  fprintf(json, "},\n");
}

void ExportJson::geometry(struct Geometry* geometry)
{
}
