#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <algorithm>
#include <memory>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filewritestream.h>

#include "Store.h"
#include "LinAlgOps.h"

namespace rj = rapidjson;

namespace {

  struct DataItem
  {
    DataItem* next = nullptr;
    const void* ptr = nullptr;
    uint32_t size = 0;
  };

  struct Context {
    rj::Document doc;

    rj::Value rjNodes = rj::Value(rj::kArrayType);
    rj::Value rjMeshes = rj::Value(rj::kArrayType);
    rj::Value rjAccessors = rj::Value(rj::kArrayType);
    rj::Value rjBufferViews = rj::Value(rj::kArrayType);
    rj::Value rjMaterials = rj::Value(rj::kArrayType);

    uint32_t dataBytes = 0;
    ListHeader<DataItem> dataItems{};
    Arena arena;

    Map definedMaterials;

    bool includeAttributes = true;

  };


  uint32_t addDataItem(Context* ctx, const void* ptr, size_t size, bool copy = false)
  {
    assert(ctx->dataBytes + size <= std::numeric_limits<uint32_t>::max());

    if (copy) {
      void* copied_ptr = ctx->arena.alloc(size);
      std::memcpy(copied_ptr, ptr, size);
      ptr = copied_ptr;
    }

    DataItem* item = ctx->arena.alloc<DataItem>();
    ctx->dataItems.insert(item);
    item->ptr = ptr;
    item->size = static_cast<uint32_t>(size);

    uint32_t offset = ctx->dataBytes;
    ctx->dataBytes += item->size;

    return offset;
  }


  uint32_t createBufferView(Context* ctx, const void* data, uint32_t count, uint32_t byte_stride, uint32_t target)
  {
    assert(count);
    uint32_t byteLength = byte_stride * count;
    uint32_t byteOffset = addDataItem(ctx, data, byteLength, false);

    auto& alloc = ctx->doc.GetAllocator();
    rj::Value rjBufferView(rj::kObjectType);
    rjBufferView.AddMember("buffer", 0, alloc);
    rjBufferView.AddMember("byteOffset", byteOffset, alloc);
    rjBufferView.AddMember("byteLength", byteLength, alloc);
    rjBufferView.AddMember("byteStride", byte_stride, alloc);
    rjBufferView.AddMember("target", target, alloc);

    uint32_t view_ix = ctx->rjBufferViews.Size();
    ctx->rjBufferViews.PushBack(rjBufferView, alloc);
    return view_ix;
  }


  uint32_t createAccessorVec3f(Context* ctx, const Vec3f* data, uint32_t count)
  {
    assert(count);
    uint32_t view_ix = createBufferView(ctx, data,
                                        count,
                                        3 * static_cast<uint32_t>(sizeof(float)),
                                        0x8892 /* GL_ARRAY_BUFFER */);

    Vec3f min_val(std::numeric_limits<float>::max());
    Vec3f max_val(-std::numeric_limits<float>::max());

    for (size_t i = 0; i < count; i++) {
      min_val = min(min_val, data[i]);
      max_val = max(max_val, data[i]);
    }

    auto& alloc = ctx->doc.GetAllocator();
    rj::Value rjMin(rj::kArrayType);
    rj::Value rjMax(rj::kArrayType);
    for (size_t i = 0; i < 3; i++) {
      rjMin.PushBack(min_val.data[i], alloc);
      rjMax.PushBack(max_val.data[i], alloc);
    }

    rj::Value rjAccessor(rj::kObjectType);
    rjAccessor.AddMember("bufferView", view_ix, alloc);
    rjAccessor.AddMember("byteOffset", 0, alloc);
    rjAccessor.AddMember("type", "VEC3", alloc);
    rjAccessor.AddMember("componentType", 0x1406 /* GL_FLOAT*/, alloc);
    rjAccessor.AddMember("count", count, alloc);
    rjAccessor.AddMember("min", rjMin, alloc);
    rjAccessor.AddMember("max", rjMax, alloc);

    uint32_t accessor_ix = ctx->rjAccessors.Size();
    ctx->rjAccessors.PushBack(rjAccessor, alloc);
    return accessor_ix;
  }


  uint32_t createAccessorUint32(Context* ctx, const uint32_t* data, uint32_t count)
  {
    assert(count);
    uint32_t view_ix = createBufferView(ctx, data,
                                        count,
                                        static_cast<uint32_t>(sizeof(uint32_t)),
                                        0x8893 /* GL_ELEMENT_ARRAY_BUFFER */);

    uint32_t min_val =  std::numeric_limits<uint32_t>::max();
    uint32_t max_val = 0;

    for (size_t i = 0; i < count; i++) {
      min_val = std::min(min_val, data[i]);
      max_val = std::max(max_val, data[i]);
    }

    auto& alloc = ctx->doc.GetAllocator();

    rj::Value rjMin(rj::kArrayType);
    rjMin.PushBack(min_val, alloc);

    rj::Value rjMax(rj::kArrayType);
    rjMax.PushBack(max_val, alloc);

    rj::Value rjAccessor(rj::kObjectType);
    rjAccessor.AddMember("bufferView", view_ix, alloc);
    rjAccessor.AddMember("byteOffset", 0, alloc);
    rjAccessor.AddMember("type", "SCALAR", alloc);
    rjAccessor.AddMember("componentType", 0x1405 /* GL_UNSIGNED_INT*/, alloc);
    rjAccessor.AddMember("count", count, alloc);
    rjAccessor.AddMember("min", rjMin, alloc);
    rjAccessor.AddMember("max", rjMax, alloc);

    uint32_t accessor_ix = ctx->rjAccessors.Size();
    ctx->rjAccessors.PushBack(rjAccessor, alloc);
    return accessor_ix;
  }


  uint32_t createOrGetColor(Context* ctx, const char* colorName, uint32_t color)
  {
    // make sure key is never zero
    uint64_t key = (color << 1) | 1 ;

    uint64_t val;
    if (ctx->definedMaterials.get(val, key)) {
      return uint32_t(val);
    }

    auto& alloc = ctx->doc.GetAllocator();

    rj::Value rjColor(rj::kArrayType);
    rjColor.PushBack((1.f / 255.f) * ((color >> 16) & 0xff), alloc);
    rjColor.PushBack((1.f / 255.f) * ((color >>  8) & 0xff), alloc);
    rjColor.PushBack((1.f / 255.f) * ((color      ) & 0xff), alloc);
    rjColor.PushBack(1.f, alloc);

    rj::Value rjPbrMetallicRoughness(rj::kObjectType);
    rjPbrMetallicRoughness.AddMember("baseColorFactor", rjColor, alloc);
    rjPbrMetallicRoughness.AddMember("metallicFactor", 0.5f, alloc);
    rjPbrMetallicRoughness.AddMember("roughnessFactor", 0.5f, alloc);

    rj::Value material(rj::kObjectType);
    if (colorName) {
      material.AddMember("name", rj::Value(colorName, alloc), alloc);
      material.AddMember("pbrMetallicRoughness", rjPbrMetallicRoughness, alloc);
    }

    uint32_t color_ix = ctx->rjMaterials.Size();
    ctx->definedMaterials.insert(key, color_ix);
    ctx->rjMaterials.PushBack(material, alloc);

    return color_ix;
  }


  uint32_t createGeometriesMesh(Context* ctx, Group* group)
  {
    auto& alloc = ctx->doc.GetAllocator();
    rj::Value primitives(rj::kArrayType);


    for (Geometry* geometry = group->group.geometries.first; geometry; geometry = geometry->next) {
      if (Triangulation* tri = geometry->triangulation; tri) {

        rj::Value primitive(rj::kObjectType);
        primitive.AddMember("mode", 0x0004 /* GL_TRIANGLES */, alloc);


        rj::Value attributes(rj::kObjectType);
        if (tri->vertices) {
          attributes.AddMember("POSITION",
                               createAccessorVec3f(ctx,
                                                   (Vec3f*)tri->vertices,
                                                   tri->vertices_n),
                               alloc);
        }
        if (tri->normals) {
          attributes.AddMember("NORMALS",
                               createAccessorVec3f(ctx,
                                                   (Vec3f*)tri->vertices,
                                                   tri->vertices_n),
                               alloc);
        }
        primitive.AddMember("attributes", attributes, alloc);
        if (tri->indices) {
          primitive.AddMember("indices",
                              createAccessorUint32(ctx,
                                                    tri->indices,
                                                    3 * tri->triangles_n),
                              alloc);
        }

        primitive.AddMember("material", createOrGetColor(ctx,
                                                         geometry->colorName,
                                                         geometry->color),
                            alloc);

        primitives.PushBack(primitive, alloc);
      }
    }
    
    if (primitives.Empty()) {
      return ~0u;
    }

    rj::Value mesh(rj::kObjectType);
    mesh.AddMember("primitives", primitives, alloc);

    uint32_t mesh_ix = ctx->rjMeshes.Size();
    ctx->rjMeshes.PushBack(mesh, alloc);
    return mesh_ix;
  }


  uint32_t processGroup(Context* ctx, Group* group)
  {
    assert(group->kind == Group::Kind::Group);
    auto& alloc = ctx->doc.GetAllocator();

    rj::Value node(rj::kObjectType);
    if (group->group.name) {
      node.AddMember("name", rj::Value(group->group.name, alloc), alloc);
    }

    if (ctx->includeAttributes && group->attributes.first) {
      rj::Value extras(rj::kObjectType);
      
      for (Attribute* att = group->attributes.first; att; att = att->next) {
        extras.AddMember(rj::Value(att->key, alloc), rj::Value(att->val, alloc), alloc);
      }
      node.AddMember("extras", extras, alloc);
    }

    uint32_t mesh_ix = createGeometriesMesh(ctx, group);

    if (group->groups.first) {
      rj::Value children(rj::kArrayType);
      for (Group* child = group->groups.first; child; child = child->next) {
        children.PushBack(processGroup(ctx, child), alloc);
      }
      node.AddMember("children", children, alloc);
    }

    uint32_t index = ctx->rjNodes.Size();
    ctx->rjNodes.PushBack(node, alloc);

    return index;
  }

}


bool exportGLTF(Store* store, Logger logger, const char* path)
{

#ifdef _WIN32
  FILE* out = nullptr;
  auto err = fopen_s(&out, path, "wb");
  if (err != 0) {
    char buf[1024];
    if (strerror_s(buf, sizeof(buf), err) != 0) {
      buf[0] = '\0';
    }
    logger(2, "Failed to open %s for writing: %s", path, buf);
    return false;
  }
  assert(out);
#else
  FILE* out = fopen(path, "w");
  if (out == nullptr) {
    logger(2, "Failed to open %s for writing.", path);
    return false;
  }
#endif

  std::unique_ptr<Context> ctxOwner = std::make_unique<Context>();

  Context* ctx = ctxOwner.get();
  ctx->doc.SetObject();

  // ------- asset -----------------------------------------------------------
  auto& alloc = ctx->doc.GetAllocator();
  rj::Value rjAsset(rj::kObjectType);
  rjAsset.AddMember("version", "2.0", alloc);
  rjAsset.AddMember("generator", "rvmparser", alloc);
  ctx->doc.AddMember("asset", rjAsset, ctx->doc.GetAllocator());

  // ------- scene -----------------------------------------------------------
  ctx->doc.AddMember("scene", 0, ctx->doc.GetAllocator());

  // ------- scenes ----------------------------------------------------------
  rj::Value rjSceneInstanceNodes(rj::kArrayType);

  std::vector<uint32_t> rootNodes;
  for (Group* file = store->getFirstRoot(); file; file = file->next) {
    assert(file->kind == Group::Kind::File);
    for (Group* model = file->groups.first; model; model = model->next) {
      assert(model->kind == Group::Kind::Model);
      for (Group* group = model->groups.first; group; group = group->next) {
        rjSceneInstanceNodes.PushBack(processGroup(ctx, group),
                                      alloc);
      }
    }
  }

  rj::Value rjSceneInstance(rj::kObjectType);
  rjSceneInstance.AddMember("nodes", rjSceneInstanceNodes, alloc);

  rj::Value rjScenes(rj::kArrayType);
  rjScenes.PushBack(rjSceneInstance, alloc);
  ctx->doc.AddMember("scenes", rjScenes, ctx->doc.GetAllocator());

  // ------ nodes ------------------------------------------------------------
  ctx->doc.AddMember("nodes", ctx->rjNodes, ctx->doc.GetAllocator());

  // ------ meshes -----------------------------------------------------------
  ctx->doc.AddMember("meshes", ctx->rjMeshes, ctx->doc.GetAllocator());

  // ------ materials --------------------------------------------------------
  ctx->doc.AddMember("materials", ctx->rjMaterials, ctx->doc.GetAllocator());

  // ------ accessors --------------------------------------------------------
  ctx->doc.AddMember("accessors", ctx->rjAccessors, ctx->doc.GetAllocator());

  // ------ buffer views  ----------------------------------------------------
  ctx->doc.AddMember("bufferViews", ctx->rjBufferViews, ctx->doc.GetAllocator());

  // ------- buffers ---------------------------------------------------------
  rj::Value rjGlbBuffer(rj::kObjectType);
  rjGlbBuffer.AddMember("byteLength", ctx->dataBytes, alloc);

  rj::Value rjBuffers(rj::kArrayType);
  rjBuffers.PushBack(rjGlbBuffer, alloc);

  ctx->doc.AddMember("buffers", rjBuffers, alloc);

  // ------- build json buffer -----------------------------------------------
  rj::StringBuffer buffer;
  rj::Writer<rj::StringBuffer> writer(buffer);
  ctx->doc.Accept(writer);
  uint32_t jsonByteSize = static_cast<uint32_t>(buffer.GetSize());
  
  // ------- pretty-printed JSON to stdout for debugging ---------------------
  if (true) {
    char writeBuffer[0x10000];
    rj::FileWriteStream os(stdout, writeBuffer, sizeof(writeBuffer));
    rj::PrettyWriter<rj::FileWriteStream> writer(os);
    writer.SetIndent(' ', 2);
    writer.SetMaxDecimalPlaces(4);
    ctx->doc.Accept(writer);
    putc('\n', stdout);
    fflush(stdout);
  }

  // ------- write glb header ------------------------------------------------
  uint32_t header[3] = {
    0x46546C67,         // magic
    2,                  // version
    (12 +               // total size: header size
     8 + jsonByteSize + // total size: json header and payload
     8 + ctx->dataBytes) // total size: binary data header and payload
  };
  if (fwrite(header, sizeof(header), 1, out) != 1) {
    logger(2, "%s: Error writing header", path);
    fclose(out);
    return false;
  }

  // ------- write JSON chunk ------------------------------------------------
  uint32_t jsonhunkHeader[2] = {
    jsonByteSize,       // length of chunk data
    0x4E4F534A          // chunk type (JSON)
  };
  if (fwrite(jsonhunkHeader, sizeof(jsonhunkHeader), 1, out) != 1) {
    logger(2, "%s: Error writing JSON chunk header", path);
    fclose(out);
    return false;
  }

  if (fwrite(buffer.GetString(), jsonByteSize, 1, out) != 1) {
    logger(2, "%s: Error writing JSON data", path);
    fclose(out);
    return false;
  }

  // -------- write BIN chunk ------------------------------------------------
  uint32_t binChunkHeader[2] = {
    ctx->dataBytes,  // length of chunk data
    0x004E4942      // chunk type (BIN)
  };

  if (fwrite(binChunkHeader, sizeof(binChunkHeader), 1, out) != 1) {
    logger(2, "%s: Error writing BIN chunk header", path);
    fclose(out);
    return false;
  }
    
  uint32_t offset = 0;
  for (DataItem* item = ctx->dataItems.first; item; item = item->next) {
    if (fwrite(item->ptr, item->size, 1, out) != 1) {
      logger(2, "%s: Error writing BIN chunk data at offset %u", path, offset);
      fclose(out);
      return false;
    }
    offset += item->size;
  }
  assert(offset = ctx->dataBytes);

  // ------- close file and exit ---------------------------------------------
  fclose(out);
  return true;
}