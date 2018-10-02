#pragma once

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filewritestream.h>

#include "ExportJson.h"
#include "Store.h"
#include "LinAlgOps.h"



namespace rj = rapidjson;

namespace {


  void process(rj::MemoryPoolAllocator<>& alloc, rj::Value& jParentArray, Logger logger, Group* group)
  {
    assert(group->kind == Group::Kind::Group);

    rj::Value jGroup(rj::kObjectType);
    jGroup.AddMember("name", rj::Value(group->group.name, alloc), alloc);
    jGroup.AddMember("material", group->group.material, alloc);

    if (isNotEmpty(group->group.bboxWorld)) {
      rj::Value bbox(rj::kArrayType);
      for (unsigned k = 0; k < 6; k++) {
        bbox.PushBack(rj::Value(group->group.bboxWorld.data[k]), alloc);
      }
      jGroup.AddMember("bbox", bbox, alloc);
    }
    if (group->groups.first) {
      rj::Value jChildren(rj::kArrayType);
      for (auto * child = group->groups.first; child; child = child->next) {
        process(alloc, jChildren, logger, child);
      }
      jGroup.AddMember("children", jChildren, alloc);
    }
    jParentArray.PushBack(jGroup, alloc);
  }

}



bool exportJson(Store* store, Logger logger, const char* path)
{

  rj::Document d;
  d.SetArray();
  auto & alloc = d.GetAllocator();

  for (auto * root = store->getFirstRoot(); root != nullptr; root = root->next) {
    assert(root->kind == Group::Kind::File);
    rj::Value jRoot(rj::kObjectType);
    jRoot.AddMember("info", rj::Value(root->file.info, alloc), alloc);
    jRoot.AddMember("note", rj::Value(root->file.note, alloc), alloc);
    jRoot.AddMember("date", rj::Value(root->file.date, alloc), alloc);
    jRoot.AddMember("user", rj::Value(root->file.user, alloc), alloc);

    if (root->groups.first) {
      rj::Value jRootChildren(rj::kArrayType);
      for (auto * model = root->groups.first; model != nullptr; model = model->next) {

        assert(model->kind == Group::Kind::Model);
        rj::Value jModel(rj::kObjectType);
        jModel.AddMember("project", rj::Value(model->model.project, alloc), alloc);
        jModel.AddMember("name", rj::Value(model->model.name, alloc), alloc);

        if (model->groups.first) {
          rj::Value jModelChildren(rj::kArrayType);
          for (auto * group = model->groups.first; group != nullptr; group = group->next) {
            process(alloc, jModelChildren, logger, group);
          }
          jModel.AddMember("children", jModelChildren, alloc);
        }

        jRootChildren.PushBack(jModel, alloc);
      }
      jRoot.AddMember("children", jRootChildren, alloc);
    }

    d.GetArray().PushBack(jRoot, alloc);
  }


  FILE* out = nullptr;
  auto err = fopen_s(&out, path, "w");
  if (err != 0) {
    char buf[1024];
    if (strerror_s(buf, sizeof(buf), err) != 0) {
      buf[0] = '\0';
    }
    logger(2, "Failed to open %s for writing: %s", path, buf);
    return false;
  }

  char writeBuffer[0x10000];
  rj::FileWriteStream os(out, writeBuffer, sizeof(writeBuffer));
  rj::PrettyWriter<rj::FileWriteStream> writer(os);
  writer.SetIndent(' ', 2);
  writer.SetMaxDecimalPlaces(4);
  d.Accept(writer);
  fclose(out);
  return true;
}


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
  if (isNotEmpty(group->group.bboxWorld)) {
    doIndent(json, indent); fprintf(json, "\"bbox\": [%.3f, %.3f, %.3f, %.3f, %.3f, %.3f],\n",
                                    group->group.bboxWorld.min.x,
                                    group->group.bboxWorld.min.y,
                                    group->group.bboxWorld.min.z,
                                    group->group.bboxWorld.max.x,
                                    group->group.bboxWorld.max.y,
                                    group->group.bboxWorld.max.z);
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
