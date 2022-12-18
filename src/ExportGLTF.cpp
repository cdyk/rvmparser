#define _USE_MATH_DEFINES
#include <cmath>

#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <algorithm>
#include <memory>
#include <cctype>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filewritestream.h>

#include "Store.h"
#include "LinAlgOps.h"

#define RVMPARSER_GLTF_PRETTY_PRINT (0)

namespace rj = rapidjson;

namespace {

  struct DataItem
  {
    DataItem* next = nullptr;
    const void* ptr = nullptr;
    uint32_t size = 0;
  };

  // Temporary state gathered prior to writing a GLTF file
  struct Model
  {
    rj::MemoryPoolAllocator<rj::CrtAllocator> rjAlloc = rj::MemoryPoolAllocator<rj::CrtAllocator>();

    rj::Value rjNodes = rj::Value(rj::kArrayType);
    rj::Value rjMeshes = rj::Value(rj::kArrayType);
    rj::Value rjAccessors = rj::Value(rj::kArrayType);
    rj::Value rjBufferViews = rj::Value(rj::kArrayType);
    rj::Value rjMaterials = rj::Value(rj::kArrayType);
    rj::Value rjBuffers = rj::Value(rj::kArrayType);

    uint32_t dataBytes = 0;
    ListHeader<DataItem> dataItems{};
    Arena arena;

    Map definedMaterials;

    Vec3f origin = makeVec3f(0.f);
  };

  struct Context {
    Model model;
    Logger logger = nullptr;
    

    std::vector<char> tmpBase64;
    std::vector<Vec3f> tmp3f;

    bool centerModel = true;
    bool rotateZToY = true;
    bool includeAttributes = false;
    bool glbContainer = false;
  };


  uint32_t addDataItem(Context& ctx, const void* ptr, size_t size, bool copy)
  {
    assert((size % 4) == 0);
    assert(ctx.model.dataBytes + size <= std::numeric_limits<uint32_t>::max());

    if (copy) {
      void* copied_ptr = ctx.model.arena.alloc(size);
      std::memcpy(copied_ptr, ptr, size);
      ptr = copied_ptr;
    }

    DataItem* item = ctx.model.arena.alloc<DataItem>();
    ctx.model.dataItems.insert(item);
    item->ptr = ptr;
    item->size = static_cast<uint32_t>(size);

    uint32_t offset = ctx.model.dataBytes;
    ctx.model.dataBytes += item->size;

    return offset;
  }

  void encodeBase64(Context& ctx, const uint8_t* data, size_t byteLength)
  {
    // Base64 table from RFC4648
    static const char rfc4648[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static_assert(sizeof(rfc4648) == 65);

    static const char prefix[] = "data:application/octet-stream;base64,";
    static const size_t prefixLength = sizeof(prefix) - 1;

    // Add data prefix
    const size_t totalLength = prefixLength + 4 * ((byteLength + 2) / 3);
    ctx.tmpBase64.resize(totalLength);
    std::memcpy(ctx.tmpBase64.data(), prefix, prefixLength);
    size_t o = prefixLength;

    // Handle range that is a multiple of three
    size_t i = 0;
    while (3 * i + 2 < byteLength) {
      const size_t i3 = 3 * i;
      const size_t i4 = 4 * i;
      const uint8_t d0 = data[i3 + 0];
      const uint8_t d1 = data[i3 + 1];
      const uint8_t d2 = data[i3 + 2];
      ctx.tmpBase64[o + i4 + 0] = rfc4648[(d0 >> 2)];
      ctx.tmpBase64[o + i4 + 1] = rfc4648[((d0 << 4) & 0x30) | (d1 >> 4)];
      ctx.tmpBase64[o + i4 + 2] = rfc4648[((d1 << 2) & 0x3c) | (d2 >> 6)];
      ctx.tmpBase64[o + i4 + 3] = rfc4648[d2 & 0x3f];
      i++;
    }

    // Handle end if byteLength is not a multiple of three
    if (3 * i < byteLength) { // End padding
      const size_t i3 = 3 * i;
      const size_t i4 = 4 * i;
      const bool two = (i3 + 1 < byteLength);  // one or two extra bytes (three would go into loop above)?
      const uint8_t d0 = data[i3 + 0];
      const uint8_t d1 = two ? data[i3 + 1] : 0;
      ctx.tmpBase64[o + i4 + 0] = rfc4648[(d0 >> 2)];
      ctx.tmpBase64[o + i4 + 1] = rfc4648[((d0 << 4) & 0x30) | (d1 >> 4)];
      ctx.tmpBase64[o + i4 + 2] = two ? rfc4648[((d1 << 2) & 0x3c)] : '=';
      ctx.tmpBase64[o + i4 + 3] = '=';
    }
  }

  uint32_t createBufferView(Context& ctx, const void* data, uint32_t count, uint32_t byte_stride, uint32_t target, bool copy)
  {
    assert(count);
    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = ctx.model.rjAlloc;

    uint32_t bufferIndex = 0;
    uint32_t byteOffset = 0;
    uint32_t byteLength = byte_stride * count;

    // For GLB, we have one large buffer containing everything that we make later
    if (ctx.glbContainer) {
      byteOffset = addDataItem(ctx, data, byteLength, copy);
    }

    // For GLTF, buffer data is base64-encoded in the URI
    else {

      encodeBase64(ctx, static_cast<const uint8_t*>(data), byteLength);
      rj::Value rjData(ctx.tmpBase64.data(), static_cast<rapidjson::SizeType>(ctx.tmpBase64.size()), alloc);

      rj::Value rjBuffer(rj::kObjectType);
      rjBuffer.AddMember("uri", rjData, alloc);
      rjBuffer.AddMember("byteLength", byteLength, alloc);
      bufferIndex = ctx.model.rjBufferViews.Size();
      ctx.model.rjBuffers.PushBack(rjBuffer, alloc);
    }

    rj::Value rjBufferView(rj::kObjectType);
    rjBufferView.AddMember("buffer", bufferIndex, alloc);
    if (byteOffset) {
      rjBufferView.AddMember("byteOffset", byteOffset, alloc);
    }
    rjBufferView.AddMember("byteLength", byteLength, alloc);

    rjBufferView.AddMember("target", target, alloc);

    uint32_t view_ix = ctx.model.rjBufferViews.Size();
    ctx.model.rjBufferViews.PushBack(rjBufferView, alloc);
    return view_ix;
  }

  uint32_t createAccessorVec3f(Context& ctx, const Vec3f* data, uint32_t count, bool copy)
  {
    assert(count);
    uint32_t view_ix = createBufferView(ctx, data,
                                        count,
                                        3 * static_cast<uint32_t>(sizeof(float)),
                                        0x8892 /* GL_ARRAY_BUFFER */,
                                        copy);

    Vec3f min_val = makeVec3f(std::numeric_limits<float>::max());
    Vec3f max_val = makeVec3f(-std::numeric_limits<float>::max());

    for (size_t i = 0; i < count; i++) {
      min_val = min(min_val, data[i]);
      max_val = max(max_val, data[i]);
    }

    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = ctx.model.rjAlloc;
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

    uint32_t accessor_ix = ctx.model.rjAccessors.Size();
    ctx.model.rjAccessors.PushBack(rjAccessor, alloc);
    return accessor_ix;
  }

  uint32_t createAccessorUint32(Context& ctx, const uint32_t* data, uint32_t count, bool copy)
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

    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = ctx.model.rjAlloc;

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

    uint32_t accessorIndex = ctx.model.rjAccessors.Size();
    ctx.model.rjAccessors.PushBack(rjAccessor, alloc);
    return accessorIndex;
  }

  uint32_t createOrGetColor(Context& ctx, const char* colorName, uint32_t color, uint8_t transparency)
  {
    // make sure key is never zero
    uint64_t key = (uint64_t(color) << 9) | (uint64_t(transparency) << 1) | 1;
    if (uint64_t val; ctx.model.definedMaterials.get(val, key)) {
      return uint32_t(val);
    }

    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = ctx.model.rjAlloc;

    rj::Value rjColor(rj::kArrayType);
    rjColor.PushBack((1.f / 255.f) * ((color >> 16) & 0xff), alloc);
    rjColor.PushBack((1.f / 255.f) * ((color >>  8) & 0xff), alloc);
    rjColor.PushBack((1.f / 255.f) * ((color      ) & 0xff), alloc);
    rjColor.PushBack(std::min(1.f, std::max(0.f, 1.f - (1.f / 100.f) * transparency)), alloc);

    rj::Value rjPbrMetallicRoughness(rj::kObjectType);
    rjPbrMetallicRoughness.AddMember("baseColorFactor", rjColor, alloc);
    rjPbrMetallicRoughness.AddMember("metallicFactor", 0.5f, alloc);
    rjPbrMetallicRoughness.AddMember("roughnessFactor", 0.5f, alloc);

    rj::Value material(rj::kObjectType);
    if (colorName) {
      material.AddMember("name", rj::Value(colorName, alloc), alloc);
      material.AddMember("pbrMetallicRoughness", rjPbrMetallicRoughness, alloc);
    }
    if (transparency != 0) {
      material.AddMember("alphaMode", "BLEND", alloc);
    }

    uint32_t colorIndex = ctx.model.rjMaterials.Size();
    ctx.model.definedMaterials.insert(key, colorIndex);
    ctx.model.rjMaterials.PushBack(material, alloc);

    return colorIndex;
  }

  void addGeometryPrimitive(Context& ctx, rj::Value& rjPrimitivesNode, Geometry* geo)
  {
    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = ctx.model.rjAlloc;
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

      uint32_t material_ix = createOrGetColor(ctx, geo->colorName, geo->color, static_cast<uint8_t>(geo->transparency));
      rjPrimitive.AddMember("material", material_ix, alloc);
    }
    else {
      Triangulation* tri = geo->triangulation;
      if (tri == nullptr) {
        ctx.logger(1, "exportGLTF: Geometry node missing triangulation, ignoring.");
        return;
      }
      if (tri->vertices_n == 0 || tri->triangles_n == 0) {
        ctx.logger(0, "exportGLTF: Geomtry node tessellation with no triangles, ignoring.");
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
        std::vector<Vec3f>& tmpNormals = ctx.tmp3f;
        tmpNormals.resize(tri->vertices_n * 3);
        for (size_t i = 0; i < tri->vertices_n; i++) {
          Vec3f n = normalize(makeVec3f(tri->normals + 3 * i));
          if (!std::isfinite(n.x) || !std::isfinite(n.y) || !std::isfinite(n.z)) {
            n = makeVec3f(1.f, 0.f, 0.f);
          }
          tmpNormals[i] = n;
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
                                                         geo->color,
                                                         static_cast<uint8_t>(geo->transparency)),
                            alloc);

      rjPrimitivesNode.PushBack(rjPrimitive, alloc);
    }
  }

  void createGeometryNode(Context& ctx, rj::Value& rjChildren, Geometry* geo)
  {
    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = ctx.model.rjAlloc;

    rj::Value rjPrimitives(rj::kArrayType);
    addGeometryPrimitive(ctx, rjPrimitives, geo);

    // If no primitives were produced, no point in creating mesh and mesh-holding node
    if (rjPrimitives.Empty()) return;

    // Create mesh
    rj::Value mesh(rj::kObjectType);
    mesh.AddMember("primitives", rjPrimitives, alloc);
    uint32_t meshIndex = ctx.model.rjMeshes.Size();
    ctx.model.rjMeshes.PushBack(mesh, alloc);

    // Create mesh holding node

    rj::Value node(rj::kObjectType);
    node.AddMember("mesh", meshIndex, alloc);

    rj::Value matrix(rj::kArrayType);
    for (size_t c = 0; c < 3; c++) {
      for (size_t r = 0; r < 3; r++) {
        matrix.PushBack(geo->M_3x4.cols[c][r], alloc);
      }
      matrix.PushBack(0.f, alloc);
    }
    for (size_t r = 0; r < 3; r++) {
      matrix.PushBack(geo->M_3x4.cols[3][r] - ctx.model.origin[r], alloc);
    }
    matrix.PushBack(1.f, alloc);

    node.AddMember("matrix", matrix, alloc);

    // Add this node to document
    uint32_t nodeIndex = ctx.model.rjNodes.Size();
    ctx.model.rjNodes.PushBack(node, alloc);
    rjChildren.PushBack(nodeIndex, alloc);
  }

  uint32_t processGroup(Context& ctx, const Node* group)
  {
    assert(group->kind == Node::Kind::Group);
    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = ctx.model.rjAlloc;

    rj::Value node(rj::kObjectType);
    if (group->group.name) {
      node.AddMember("name", rj::Value(group->group.name, alloc), alloc);
    }

    // Optionally add all attributes under an "extras" object member.
    if (ctx.includeAttributes && group->attributes.first) {
      rj::Value extras(rj::kObjectType);

      rj::Value attributes(rj::kObjectType);
      for (Attribute* att = group->attributes.first; att; att = att->next) {
        if (attributes.HasMember(att->key)) {
          ctx.logger(1, "exportGLTF: Duplicate attribute key, discarding %s=\"%s\"", att->key, att->val);
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
    for (Node* child = group->children.first; child; child = child->next) {
      children.PushBack(processGroup(ctx, child), alloc);
    }

    if (!children.Empty()) {
      node.AddMember("children", children, alloc);
    }

    // Add this node to document
    uint32_t nodeIndex = ctx.model.rjNodes.Size();
    ctx.model.rjNodes.PushBack(node, alloc);
    return nodeIndex;
  }

  void extendBounds(Context& ctx, BBox3f& worldBounds, const Node* node)
  {
    for (Node* child = node->children.first; child; child = child->next) {
      extendBounds(ctx, worldBounds, child);
    }
    if (node->kind == Node::Kind::Group) {
      for (Geometry* geo = node->group.geometries.first; geo; geo = geo->next) {
        engulf(worldBounds, geo->bboxWorld);
      }
    }
  }

  void calculateOrigin(Context& ctx, const Node* firstNode)
  {
    BBox3f worldBounds = createEmptyBBox3f();

    for (const Node* node = firstNode; node; node = node->next) {
      extendBounds(ctx, worldBounds, node);
    }

    ctx.model.origin = 0.5f * (worldBounds.min + worldBounds.max);
    ctx.logger(0, "exportGLTF: world bounds = [%.2f, %.2f, %.2f]x[%.2f, %.2f, %.2f]",
               worldBounds.min.x, worldBounds.min.y, worldBounds.min.z,
               worldBounds.max.x, worldBounds.max.y, worldBounds.max.z);
    ctx.logger(0, "exportGLTF: setting origin = [%.2f, %.2f, %.2f]",
               ctx.model.origin.x, ctx.model.origin.y, ctx.model.origin.z);
  }

  void buildRootNodes(Context& ctx, rj::Value& rootNodes, const Node* firstNode)
  {
    for (const Node* node = firstNode; node; node = node->next) {
      if (node->kind == Node::Kind::Group) {
        rootNodes.PushBack(processGroup(ctx, node), ctx.model.rjAlloc);
      }
      else {
        buildRootNodes(ctx, rootNodes, node->children.first);
      }
    }
  }

  rj::Document buildGLTF(Context& ctx, const Node* firstNode)
  {
    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = ctx.model.rjAlloc;

    if (ctx.centerModel) {
      calculateOrigin(ctx, firstNode);
    }

    rj::Document rjDoc(rj::kObjectType, &alloc);

    // ------- asset -----------------------------------------------------------
    rj::Value rjAsset(rj::kObjectType);
    rjAsset.AddMember("version", "2.0", alloc);
    rjAsset.AddMember("generator", "rvmparser", alloc);
    if (ctx.centerModel) {
      rj::Value rjOrigin(rj::kArrayType);
      rjOrigin.PushBack(ctx.model.origin.x, alloc);
      rjOrigin.PushBack(ctx.model.origin.y, alloc);
      rjOrigin.PushBack(ctx.model.origin.z, alloc);

      rj::Value rjExtras(rj::kObjectType);
      rjExtras.AddMember("rvmparser-origin", rjOrigin, alloc);

      rjAsset.AddMember("extras", rjExtras, alloc);
    }
    rjDoc.AddMember("asset", rjAsset, alloc);

    // ------- scene -----------------------------------------------------------
    rjDoc.AddMember("scene", 0, alloc);

    // ------- scenes ----------------------------------------------------------
    rj::Value rjSceneInstanceNodes(rj::kArrayType);

    if (ctx.rotateZToY) {
      //
      // Rotation +Z to +Y by rotation -90 degrees about the X axis
      // 
      // quaternion is x,y,z = sin(angle/2) * [1,0,0], w=cos(angle/2)
      //
      rj::Value rotation(rj::kArrayType);
      rotation.PushBack(std::sin(-M_PI_4), alloc);
      rotation.PushBack(0.f, alloc);
      rotation.PushBack(0.f, alloc);
      rotation.PushBack(std::cos(-M_PI_4), alloc);

      // Add file hierarchy below rotation node
      rj::Value children(rj::kArrayType);
      buildRootNodes(ctx, children, firstNode);

      // Add node to document
      rj::Value node(rj::kObjectType);
      node.AddMember("name", "rvmparser-rotate-z-to-y", alloc);
      node.AddMember("rotation", rotation, alloc);
      node.AddMember("children", children, alloc);

      uint32_t nodeIndex = ctx.model.rjNodes.Size();
      ctx.model.rjNodes.PushBack(node, alloc);

      // Set root
      rjSceneInstanceNodes.PushBack(nodeIndex, alloc);
    }
    else {
      buildRootNodes(ctx, rjSceneInstanceNodes, firstNode);
    }

    // If we have a GLB container, add a single buffer that holds all data
    if (ctx.glbContainer) {
      assert(ctx.model.rjBuffers.Empty());
      rj::Value rjGlbBuffer(rj::kObjectType);
      rjGlbBuffer.AddMember("byteLength", ctx.model.dataBytes, alloc);
      ctx.model.rjBuffers.PushBack(rjGlbBuffer, alloc);
    }


    rj::Value rjSceneInstance(rj::kObjectType);
    rjSceneInstance.AddMember("nodes", rjSceneInstanceNodes, alloc);

    rj::Value rjScenes(rj::kArrayType);
    rjScenes.PushBack(rjSceneInstance, alloc);
    rjDoc.AddMember("scenes", rjScenes, alloc);
    rjDoc.AddMember("nodes", ctx.model.rjNodes, alloc);
    rjDoc.AddMember("meshes", ctx.model.rjMeshes, alloc);
    rjDoc.AddMember("materials", ctx.model.rjMaterials, alloc);
    rjDoc.AddMember("accessors", ctx.model.rjAccessors, alloc);
    rjDoc.AddMember("bufferViews", ctx.model.rjBufferViews, alloc);
    rjDoc.AddMember("buffers", ctx.model.rjBuffers, alloc);

    return rjDoc;
  }

  bool writeAsGLB(Context& ctx, FILE* out, const char* path, const rj::Document& rjDoc)
  {
    // ------- build json buffer -----------------------------------------------
    rj::StringBuffer buffer;
#if RVMPARSER_GLTF_PRETTY_PRINT == 1
    // Pretty printer for debug purposes
    rj::PrettyWriter<rj::StringBuffer> writer(buffer);
    writer.SetIndent(' ', 2);
#else
    rj::Writer<rj::StringBuffer> writer(buffer);
#endif
    rjDoc.Accept(writer);
    size_t jsonByteSize = buffer.GetSize();
    size_t jsonPaddingSize = (4 - (jsonByteSize % 4)) % 4;

    // ------- write glb header ------------------------------------------------

    size_t total_size =
      12 +                                  // Initial header
      8 + jsonByteSize + jsonPaddingSize +  // JSON header, payload and padding
      8 + ctx.model.dataBytes;              // BVIN header and payload

    if (std::numeric_limits<uint32_t>::max() < total_size) {
      ctx.logger(2, "%s: File would be %zu bytes, a number too large to store in 32 bits in the GLB header.");
      return false;
    }
    uint32_t header[3] = {
      0x46546C67,                       // magic
      2,                                // version
      static_cast<uint32_t>(total_size) // total size
    };
    if (fwrite(header, sizeof(header), 1, out) != 1) {
      ctx.logger(2, "%s: Error writing header", path);
      fclose(out);
      return false;
    }

    // ------- write JSON chunk ------------------------------------------------
    uint32_t jsonhunkHeader[2] = {
      static_cast<uint32_t>(jsonByteSize + jsonPaddingSize),  // length of chunk data
      0x4E4F534A                                              // chunk type (JSON)
    };
    if (fwrite(jsonhunkHeader, sizeof(jsonhunkHeader), 1, out) != 1) {
      ctx.logger(2, "%s: Error writing JSON chunk header", path);
      fclose(out);
      return false;
    }

    if (fwrite(buffer.GetString(), jsonByteSize, 1, out) != 1) {
      ctx.logger(2, "%s: Error writing JSON data", path);
      fclose(out);
      return false;
    }
    if (jsonPaddingSize) {
      assert(jsonPaddingSize < 4);
      const char* padding = "   ";
      if (fwrite(padding, jsonPaddingSize, 1, out) != 1) {
        ctx.logger(2, "%s: Error writing JSON padding", path);
        fclose(out);
        return false;
      }
    }

    // -------- write BIN chunk ------------------------------------------------
    uint32_t binChunkHeader[2] = {
      ctx.model.dataBytes,  // length of chunk data
      0x004E4942            // chunk type (BIN)
    };

    if (fwrite(binChunkHeader, sizeof(binChunkHeader), 1, out) != 1) {
      ctx.logger(2, "%s: Error writing BIN chunk header", path);
      fclose(out);
      return false;
    }

    uint32_t offset = 0;
    for (DataItem* item = ctx.model.dataItems.first; item; item = item->next) {
      if (fwrite(item->ptr, item->size, 1, out) != 1) {
        ctx.logger(2, "%s: Error writing BIN chunk data at offset %u", path, offset);
        fclose(out);
        return false;
      }
      offset += item->size;
    }
    assert(offset == ctx.model.dataBytes);

    // ------- close file and exit ---------------------------------------------

    ctx.logger(0, "exportGLTF: Successfully wrote %s (%zu KB)", path, (total_size + 1023) / 1024);
    return true;
  }

  bool writeAsGLTF(Context& ctx, FILE* out, const char* path, const rj::Document& rjDoc)
  {
    std::vector<char> writeBuffer(0x10000);
    rj::FileWriteStream os(out, writeBuffer.data(), writeBuffer.size());
#if RVMPARSER_GLTF_PRETTY_PRINT == 1
    // Pretty printer for debug purposes
    rj::PrettyWriter<rj::FileWriteStream> writer(os);
    writer.SetIndent(' ', 2);
#else
    rj::Writer<rj::FileWriteStream> writer(os);
#endif
    if (!rjDoc.Accept(writer)) {
      ctx.logger(2, "%s: Failed to write json", path);
      return false;
    }
    return true;
  }

}


bool exportGLTF(Store* store, Logger logger, const char* path, bool rotateZToY, bool centerModel, bool includeAttributes)
{
  Context ctx{
    .logger = logger,
    .centerModel = centerModel,
    .rotateZToY = rotateZToY,
    .includeAttributes = includeAttributes,
  };

  { // Split into stem and suffix
    size_t o = 0; // offset of last dot
    size_t n = 0; // string length of output
    assert(path);
    for (; path[n] != '\0'; n++) {
      if (path[n] == '.') {
        o = n;
      }
    }
    if (path[o] != '.') {
      ctx.logger(2, "exportGLTF: Failed to find path suffix in string '%s'", path);
      return false;
    }
    size_t m = n - o; // length of suffix
    if ((m == 4) || (m == 5)) {
      char suffixLc[4] = { 0, 0, 0, 0 };  // lower-case version
      for (size_t i = 0; i + 1 < m; i++) {
        suffixLc[i] = static_cast<char>(std::tolower(path[o + i + 1]));
      }
      if (std::memcmp(suffixLc, "glb", 3) == 0) {
        ctx.glbContainer = true;
        goto recognized_suffix;
      }
      else if (std::memcmp(suffixLc, "gltf", 4) == 0) {
        ctx.glbContainer = false;
        goto recognized_suffix;
      }
    }
    ctx.logger(2, "exportGLTF: Failed to recognize path suffix (.glb or .gltf) in string '%s'", path);
    return false;

  recognized_suffix:
    ;
  }


  ctx.logger(0, "exportGLTF: rotate-z-to-y=%u center=%u attributes=%u",
             ctx.rotateZToY ? 1 : 0,
             ctx.centerModel ? 1 : 0,
             ctx.includeAttributes ? 1 : 0);

  rj::Document rjDoc = buildGLTF(ctx, store->getFirstRoot());

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

  bool success = true;
  if (ctx.glbContainer) {
    success = writeAsGLB(ctx, out, path, rjDoc);
  }
  else {
    success = writeAsGLTF(ctx, out, path, rjDoc);
  }

  fclose(out);

  return success;
}
