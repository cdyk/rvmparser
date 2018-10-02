#pragma once

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filewritestream.h>

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

    if (group->attributes.first) {
      rj::Value jAttributes(rj::kObjectType);

      for (auto * att = group->attributes.first; att; att = att->next) {
        jAttributes.AddMember(rj::GenericStringRef(att->key), rj::Value(att->val, alloc), alloc);
      }
      jGroup.AddMember("attributes", jAttributes, alloc);
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
