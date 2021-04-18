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
    Store* store = nullptr;
    Logger logger = nullptr;

    rj::Document rjDoc;

    rj::Value rjNodes = rj::Value(rj::kArrayType);
    rj::Value rjMeshes = rj::Value(rj::kArrayType);
    rj::Value rjAccessors = rj::Value(rj::kArrayType);
    rj::Value rjBufferViews = rj::Value(rj::kArrayType);
    rj::Value rjMaterials = rj::Value(rj::kArrayType);

    uint32_t dataBytes = 0;
    ListHeader<DataItem> dataItems{};
    Arena arena;

    Map definedMaterials;

    std::vector<Vec3f> tmp3f;

    bool dumpDebugJson = false;
    bool includeAttributes = false;

  };


  uint32_t addDataItem(Context* ctx, const void* ptr, size_t size, bool copy)
  {
    assert((size % 4) == 0);
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


  uint32_t createBufferView(Context* ctx, const void* data, uint32_t count, uint32_t byte_stride, uint32_t target, bool copy)
  {
    assert(count);
    uint32_t byteLength = byte_stride * count;
    uint32_t byteOffset = addDataItem(ctx, data, byteLength, copy);

    auto& alloc = ctx->rjDoc.GetAllocator();
    rj::Value rjBufferView(rj::kObjectType);
    rjBufferView.AddMember("buffer", 0, alloc);
    rjBufferView.AddMember("byteOffset", byteOffset, alloc);
    rjBufferView.AddMember("byteLength", byteLength, alloc);

    //
    //rjBufferView.AddMember("byteStride", byte_stride, alloc);

    rjBufferView.AddMember("target", target, alloc);

    uint32_t view_ix = ctx->rjBufferViews.Size();
    ctx->rjBufferViews.PushBack(rjBufferView, alloc);
    return view_ix;
  }


  uint32_t createAccessorVec3f(Context* ctx, const Vec3f* data, uint32_t count, bool copy)
  {
    assert(count);
    uint32_t view_ix = createBufferView(ctx, data,
                                        count,
                                        3 * static_cast<uint32_t>(sizeof(float)),
                                        0x8892 /* GL_ARRAY_BUFFER */,
                                        copy);

    Vec3f min_val(std::numeric_limits<float>::max());
    Vec3f max_val(-std::numeric_limits<float>::max());

    for (size_t i = 0; i < count; i++) {
      min_val = min(min_val, data[i]);
      max_val = max(max_val, data[i]);
    }

    auto& alloc = ctx->rjDoc.GetAllocator();
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


  uint32_t createAccessorUint32(Context* ctx, const uint32_t* data, uint32_t count, bool copy)
  {
    assert(count);
    uint32_t view_ix = createBufferView(ctx, data,
                                        count,
                                        static_cast<uint32_t>(sizeof(uint32_t)),
                                        0x8893 /* GL_ELEMENT_ARRAY_BUFFER */,
                                        copy);

    uint32_t min_val =  std::numeric_limits<uint32_t>::max();
    uint32_t max_val = 0;

    for (size_t i = 0; i < count; i++) {
      min_val = std::min(min_val, data[i]);
      max_val = std::max(max_val, data[i]);
    }

    auto& alloc = ctx->rjDoc.GetAllocator();

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

    auto& alloc = ctx->rjDoc.GetAllocator();

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


  void addGeometryPrimitive(Context* ctx, rj::Value& rjPrimitivesNode, Geometry* geo)
  {
    auto& alloc = ctx->rjDoc.GetAllocator();
    if (geo->kind == Geometry::Kind::Line) {
      float positions[2 * 3] = {
        geo->line.a, 0.f, 0.f,
        geo->line.b, 0.f, 0.f
      };
      rj::Value rjPrimitive(rj::kObjectType);
      rjPrimitive.AddMember("mode", 0x0001 /* GL_LINES */, alloc);

      rj::Value rjAttributes(rj::kObjectType);

      uint32_t accessor_ix = createAccessorVec3f(ctx, (Vec3f*)positions, 2, true);
      rjAttributes.AddMember("POSITION", accessor_ix, alloc);

      rjPrimitive.AddMember("attributes", rjAttributes, alloc);

      uint32_t material_ix = createOrGetColor(ctx, geo->colorName, geo->color);
      rjPrimitive.AddMember("material", material_ix, alloc);
    }
    else {
      Triangulation* tri = geo->triangulation;
      if (tri == nullptr) {
        ctx->logger(1, "exportGLTF: Geometry node missing triangulation, ignoring.");
        return;
      }
      if (tri->vertices_n == 0 || tri->triangles_n == 0) {
        ctx->logger(0, "exportGLTF: Geomtry node tessellation with no triangles, ignoring.");
        return;
      }

      rj::Value rjPrimitive(rj::kObjectType);
      rjPrimitive.AddMember("mode", 0x0004 /* GL_TRIANGLES */, alloc);

      rj::Value rjAttributes(rj::kObjectType);

      if (tri->vertices) {
        uint32_t accessor_ix = createAccessorVec3f(ctx, (Vec3f*)tri->vertices, tri->vertices_n, false);
        rjAttributes.AddMember("POSITION", accessor_ix, alloc);
      }

      if (tri->normals) {

        // Make sure that normal vectors are of unit length
        std::vector<Vec3f>& tmpNormals = ctx->tmp3f;
        tmpNormals.resize(tri->vertices_n * 3);
        for (size_t i = 0; i < tri->vertices_n; i++) {
          tmpNormals[i] = normalize(Vec3f(tri->vertices + 3 * i));
        }

        // And make a copy when setting up the accessor
        uint32_t accessor_ix = createAccessorVec3f(ctx, tmpNormals.data(), tri->vertices_n, true);
        rjAttributes.AddMember("NORMAL", accessor_ix, alloc);
      }

      rjPrimitive.AddMember("attributes", rjAttributes, alloc);

      if (tri->indices) {
        uint32_t accessor_ix = createAccessorUint32(ctx, tri->indices, 3 * tri->triangles_n, false);
        rjPrimitive.AddMember("indices", accessor_ix, alloc);
      }

      rjPrimitive.AddMember("material", createOrGetColor(ctx,
                                                         geo->colorName,
                                                         geo->color),
                            alloc);

      rjPrimitivesNode.PushBack(rjPrimitive, alloc);
    }
  }

  void createGeometryNode(Context* ctx, rj::Value& rjChildren, Geometry* geo)
  {
    auto& alloc = ctx->rjDoc.GetAllocator();

    rj::Value rjPrimitives(rj::kArrayType);
    addGeometryPrimitive(ctx, rjPrimitives, geo);

    // If no primitives were produced, no point in creating mesh and mesh-holding node
    if (rjPrimitives.Empty()) return;

    // Create mesh
    rj::Value mesh(rj::kObjectType);
    mesh.AddMember("primitives", rjPrimitives, alloc);
    uint32_t mesh_ix = ctx->rjMeshes.Size();
    ctx->rjMeshes.PushBack(mesh, alloc);

    // Create mesh holding node

    rj::Value node(rj::kObjectType);
    node.AddMember("mesh", mesh_ix, alloc);

    rj::Value matrix(rj::kArrayType);
    for (size_t c = 0; c < 4; c++) {
      for (size_t r = 0; r < 3; r++) {
        matrix.PushBack(geo->M_3x4.cols[c][r], alloc);
      }
      matrix.PushBack(c==3 ? 1.f : 0.f, alloc);
    }
    node.AddMember("matrix", matrix, alloc);

    // Add this node to document
    uint32_t node_ix = ctx->rjNodes.Size();
    ctx->rjNodes.PushBack(node, alloc);
    rjChildren.PushBack(node_ix, alloc);
  }

  uint32_t processGroup(Context* ctx, Group* group)
  {
    assert(group->kind == Group::Kind::Group);
    auto& alloc = ctx->rjDoc.GetAllocator();

    rj::Value node(rj::kObjectType);
    if (group->group.name) {
      node.AddMember("name", rj::Value(group->group.name, alloc), alloc);
    }

    // Optionally add all attributes under an "extras" object member.
    if (ctx->includeAttributes && group->attributes.first) {
      rj::Value extras(rj::kObjectType);

      rj::Value attributes(rj::kObjectType);
      for (Attribute* att = group->attributes.first; att; att = att->next) {
        if (attributes.HasMember(att->key)) {
          ctx->logger(1, "exportGLTF: Duplicate attribute key, discarding %s=\"%s\"", att->key, att->val);
        }
        attributes.AddMember(rj::Value(att->key, alloc), rj::Value(att->val, alloc), alloc);
      }

      extras.AddMember("rvm-attributes", attributes, alloc);
      node.AddMember("extras", extras, alloc);
    }


    rj::Value children(rj::kArrayType);

    // Create a child node for each geometry since transforms are per-geomtry
    for (Geometry* geo = group->group.geometries.first; geo; geo = geo->next) {
      createGeometryNode(ctx, children, geo);
    }

    // And recurse into children
    for (Group* child = group->groups.first; child; child = child->next) {
      children.PushBack(processGroup(ctx, child), alloc);
    }

    if (!children.Empty()) {
      node.AddMember("children", children, alloc);
    }

    // Add this node to document
    uint32_t node_ix = ctx->rjNodes.Size();
    ctx->rjNodes.PushBack(node, alloc);
    return node_ix;
  }

}


bool exportGLTF(Store* store, Logger logger, const char* path)
{

#ifdef _WIN32
  FILE* out = nullptr;
  auto err = fopen_s(&out, path, "wb");
  if (err != 0) {
    char buf[256];
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
  ctx->store = store;
  ctx->logger = logger;

  ctx->rjDoc.SetObject();

  // ------- asset -----------------------------------------------------------
  auto& alloc = ctx->rjDoc.GetAllocator();
  rj::Value rjAsset(rj::kObjectType);
  rjAsset.AddMember("version", "2.0", alloc);
  rjAsset.AddMember("generator", "rvmparser", alloc);
  ctx->rjDoc.AddMember("asset", rjAsset, ctx->rjDoc.GetAllocator());

  // ------- scene -----------------------------------------------------------
  ctx->rjDoc.AddMember("scene", 0, ctx->rjDoc.GetAllocator());

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
  ctx->rjDoc.AddMember("scenes", rjScenes, ctx->rjDoc.GetAllocator());

  // ------ nodes ------------------------------------------------------------
  ctx->rjDoc.AddMember("nodes", ctx->rjNodes, ctx->rjDoc.GetAllocator());

  // ------ meshes -----------------------------------------------------------
  ctx->rjDoc.AddMember("meshes", ctx->rjMeshes, ctx->rjDoc.GetAllocator());

  // ------ materials --------------------------------------------------------
  ctx->rjDoc.AddMember("materials", ctx->rjMaterials, ctx->rjDoc.GetAllocator());

  // ------ accessors --------------------------------------------------------
  ctx->rjDoc.AddMember("accessors", ctx->rjAccessors, ctx->rjDoc.GetAllocator());

  // ------ buffer views  ----------------------------------------------------
  ctx->rjDoc.AddMember("bufferViews", ctx->rjBufferViews, ctx->rjDoc.GetAllocator());

  // ------- buffers ---------------------------------------------------------
  rj::Value rjGlbBuffer(rj::kObjectType);
  rjGlbBuffer.AddMember("byteLength", ctx->dataBytes, alloc);

  rj::Value rjBuffers(rj::kArrayType);
  rjBuffers.PushBack(rjGlbBuffer, alloc);

  ctx->rjDoc.AddMember("buffers", rjBuffers, alloc);

  // ------- build json buffer -----------------------------------------------
  rj::StringBuffer buffer;
  rj::Writer<rj::StringBuffer> writer(buffer);
  ctx->rjDoc.Accept(writer);
  uint32_t jsonByteSize = static_cast<uint32_t>(buffer.GetSize());
  uint32_t jsonPaddingSize = (4 - (jsonByteSize % 4));
  
  // ------- pretty-printed JSON to stdout for debugging ---------------------
  if (ctx->dumpDebugJson) {
    char writeBuffer[0x10000];
    rj::FileWriteStream os(stdout, writeBuffer, sizeof(writeBuffer));
    rj::PrettyWriter<rj::FileWriteStream> writer(os);
    writer.SetIndent(' ', 2);
    writer.SetMaxDecimalPlaces(4);
    ctx->rjDoc.Accept(writer);
    putc('\n', stdout);
    fflush(stdout);
  }

  // ------- write glb header ------------------------------------------------

  uint32_t total_size =
    12 +                                  // Initial header
    8 + jsonByteSize + jsonPaddingSize +  // JSON header, payload and padding
    8 + ctx->dataBytes;                   // BVIN header and payload

  uint32_t header[3] = {
    0x46546C67,         // magic
    2,                  // version
    total_size          // total size
  };
  if (fwrite(header, sizeof(header), 1, out) != 1) {
    logger(2, "%s: Error writing header", path);
    fclose(out);
    return false;
  }

  // ------- write JSON chunk ------------------------------------------------
  uint32_t jsonhunkHeader[2] = {
    jsonByteSize + jsonPaddingSize,       // length of chunk data
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
  if (jsonPaddingSize) {
    assert(jsonPaddingSize < 4);
    const char* padding = "   ";
    if (fwrite(padding, jsonPaddingSize, 1, out) != 1) {
      logger(2, "%s: Error writing JSON padding", path);
      fclose(out);
      return false;
    }
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
  assert(offset == ctx->dataBytes);

  // ------- close file and exit ---------------------------------------------
  fclose(out);

  ctx->logger(0, "exportGLTF: Successfully wrote %s (%zu KB)", path, (total_size + 1023) / 1024);

  return true;
}