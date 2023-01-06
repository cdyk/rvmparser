#define _USE_MATH_DEFINES
#include <cmath>

#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <algorithm>
#include <span>
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

  struct GeometryItem
  {
    size_t sortKey;   // Bit 0 is line-not-line, bits 1 and up are material index
    const Geometry* geo;
  };

  struct Context {
    Logger logger = nullptr;
    
    const char* path = nullptr; // Path without suffix
    const char* suffix = nullptr;

    std::vector<char> tmpBase64;
    std::vector<Vec3f> tmp3f_1;
    std::vector<Vec3f> tmp3f_2;
    std::vector<uint32_t> tmp32ui;
    std::vector<GeometryItem> tmpGeos;

    struct {
      size_t level = 0;   // Level to do splitting, 0 for no splitting
      size_t choose = 0;  // Keeping track of which split we are processing
      size_t index = 0;   // Which subtree we are to descend
      bool done = false;  // Set to true when there are no more splits
    } split;

    bool centerModel = true;
    bool rotateZToY = true;
    bool includeAttributes = false;
    bool glbContainer = false;
    bool mergeGeometries = true;
  };


  uint32_t addDataItem(Context& /*ctx*/, Model& model, const void* ptr, size_t size, bool copy)
  {
    assert((size % 4) == 0);
    assert(model.dataBytes + size <= std::numeric_limits<uint32_t>::max());

    if (copy) {
      void* copied_ptr = model.arena.alloc(size);
      std::memcpy(copied_ptr, ptr, size);
      ptr = copied_ptr;
    }

    DataItem* item = model.arena.alloc<DataItem>();
    model.dataItems.insert(item);
    item->ptr = ptr;
    item->size = static_cast<uint32_t>(size);

    uint32_t offset = model.dataBytes;
    model.dataBytes += item->size;

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

  uint32_t createBufferView(Context& ctx, Model& model, const void* data, size_t count, size_t byte_stride, uint32_t target, bool copy)
  {
    assert(count);
    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = model.rjAlloc;

    uint32_t bufferIndex = 0;
    uint32_t byteOffset = 0;
    size_t byteLength = byte_stride * count;

    // For GLB, we have one large buffer containing everything that we make later
    if (ctx.glbContainer) {
      byteOffset = addDataItem(ctx, model, data, byteLength, copy);
    }

    // For GLTF, buffer data is base64-encoded in the URI
    else {

      encodeBase64(ctx, static_cast<const uint8_t*>(data), byteLength);
      rj::Value rjData(ctx.tmpBase64.data(), static_cast<rapidjson::SizeType>(ctx.tmpBase64.size()), alloc);

      rj::Value rjBuffer(rj::kObjectType);
      rjBuffer.AddMember("uri", rjData, alloc);
      rjBuffer.AddMember("byteLength", byteLength, alloc);
      bufferIndex = model.rjBufferViews.Size();
      model.rjBuffers.PushBack(rjBuffer, alloc);
    }

    rj::Value rjBufferView(rj::kObjectType);
    rjBufferView.AddMember("buffer", bufferIndex, alloc);
    if (byteOffset) {
      rjBufferView.AddMember("byteOffset", byteOffset, alloc);
    }
    rjBufferView.AddMember("byteLength", byteLength, alloc);

    rjBufferView.AddMember("target", target, alloc);

    uint32_t view_ix = model.rjBufferViews.Size();
    model.rjBufferViews.PushBack(rjBufferView, alloc);
    return view_ix;
  }

  uint32_t createAccessorVec3f(Context& ctx, Model& model, const Vec3f* data, size_t count, bool copy)
  {
    assert(count);
    uint32_t view_ix = createBufferView(ctx, model,
                                        data,
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

    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = model.rjAlloc;
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

    uint32_t accessor_ix = model.rjAccessors.Size();
    model.rjAccessors.PushBack(rjAccessor, alloc);
    return accessor_ix;
  }

  uint32_t createAccessorUint32(Context& ctx, Model& model, const uint32_t* data, size_t count, bool copy)
  {
    assert(count);
    uint32_t view_ix = createBufferView(ctx, model,
                                        data,
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

    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = model.rjAlloc;

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

    uint32_t accessorIndex = model.rjAccessors.Size();
    model.rjAccessors.PushBack(rjAccessor, alloc);
    return accessorIndex;
  }

  uint32_t createOrGetColor(Context& /*ctx*/, Model& model, const Geometry* geo)
  {
    uint32_t color = geo->color;
    uint8_t transparency = static_cast<uint8_t>(geo->transparency);
    // make sure key is never zero
    uint64_t key = (uint64_t(color) << 9) | (uint64_t(transparency) << 1) | 1;
    if (uint64_t val; model.definedMaterials.get(val, key)) {
      return uint32_t(val);
    }

    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = model.rjAlloc;

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
    if (geo->colorName) {
      material.AddMember("name", rj::Value(geo->colorName, alloc), alloc);
      material.AddMember("pbrMetallicRoughness", rjPbrMetallicRoughness, alloc);
    }
    if (transparency != 0) {
      material.AddMember("alphaMode", "BLEND", alloc);
    }

    uint32_t colorIndex = model.rjMaterials.Size();
    model.definedMaterials.insert(key, colorIndex);
    model.rjMaterials.PushBack(material, alloc);

    return colorIndex;
  }

  void addChildNode(Model& model, rj::Value& rjParentChildren, rj::Value& rjChildNode)
  {
    rjParentChildren.PushBack(model.rjNodes.Size(), model.rjAlloc);
    model.rjNodes.PushBack(rjChildNode, model.rjAlloc);
  }

  void addGeometryPrimitive(Context& ctx, Model& model, rj::Value& rjPrimitivesNode, const Geometry* geo)
  {
    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = model.rjAlloc;
    if (geo->kind == Geometry::Kind::Line) {
      float positions[2 * 3] = {
        geo->line.a, 0.f, 0.f,
        geo->line.b, 0.f, 0.f
      };
      rj::Value rjPrimitive(rj::kObjectType);
      rjPrimitive.AddMember("mode", 0x0001 /* GL_LINES */, alloc);

      rj::Value rjAttributes(rj::kObjectType);

      uint32_t accessor_ix = createAccessorVec3f(ctx, model, (Vec3f*)positions, 2, true);
      rjAttributes.AddMember("POSITION", accessor_ix, alloc);

      rjPrimitive.AddMember("attributes", rjAttributes, alloc);

      uint32_t material_ix = createOrGetColor(ctx, model, geo);
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
        uint32_t accessor_ix = createAccessorVec3f(ctx, model, (Vec3f*)tri->vertices, tri->vertices_n, false);
        rjAttributes.AddMember("POSITION", accessor_ix, alloc);
      }

      if (tri->normals) {

        // Make sure that normal vectors are of unit length
        std::vector<Vec3f>& tmpNormals = ctx.tmp3f_1;
        tmpNormals.resize(tri->vertices_n * 3);
        for (size_t i = 0; i < tri->vertices_n; i++) {
          Vec3f n = normalize(makeVec3f(tri->normals + 3 * i));
          if (!std::isfinite(n.x) || !std::isfinite(n.y) || !std::isfinite(n.z)) {
            n = makeVec3f(1.f, 0.f, 0.f);
          }
          tmpNormals[i] = n;
        }

        // And make a copy when setting up the accessor
        uint32_t accessor_ix = createAccessorVec3f(ctx, model, tmpNormals.data(), tri->vertices_n, true);
        rjAttributes.AddMember("NORMAL", accessor_ix, alloc);
      }

      rjPrimitive.AddMember("attributes", rjAttributes, alloc);

      if (tri->indices) {
        uint32_t accessor_ix = createAccessorUint32(ctx, model, tri->indices, 3 * tri->triangles_n, false);
        rjPrimitive.AddMember("indices", accessor_ix, alloc);
      }

      rjPrimitive.AddMember("material", createOrGetColor(ctx, model, geo), alloc);

      rjPrimitivesNode.PushBack(rjPrimitive, alloc);
    }
  }

  bool insertGeometryIntoNode(Context& ctx, Model& model, rj::Value& node, const Geometry* geo)
  {
    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = model.rjAlloc;

    rj::Value rjPrimitives(rj::kArrayType);
    addGeometryPrimitive(ctx, model, rjPrimitives, geo);

    // If no primitives were produced, no point in creating mesh and mesh-holding node
    if (rjPrimitives.Empty()) return false;

    // Create mesh
    rj::Value mesh(rj::kObjectType);
    mesh.AddMember("primitives", rjPrimitives, alloc);
    uint32_t meshIndex = model.rjMeshes.Size();
    model.rjMeshes.PushBack(mesh, alloc);

    node.AddMember("mesh", meshIndex, alloc);

    rj::Value matrix(rj::kArrayType);
    for (size_t c = 0; c < 3; c++) {
      for (size_t r = 0; r < 3; r++) {
        matrix.PushBack(geo->M_3x4.cols[c][r], alloc);
      }
      matrix.PushBack(0.f, alloc);
    }
    for (size_t r = 0; r < 3; r++) {
      matrix.PushBack(geo->M_3x4.cols[3][r] - model.origin[r], alloc);
    }
    matrix.PushBack(1.f, alloc);

    node.AddMember("matrix", matrix, alloc);

    return true;
  }

  bool addPrimitiveForLines(Context& ctx, Model& model, rj::Value& rjPrimitives, const std::span<const GeometryItem>& geos, const Vec3d& localOrigin)
  {
    assert(!geos.empty());
    std::vector<Vec3f>& V = ctx.tmp3f_1;  // No need to clear, they get resized before written to
    size_t vertexOffset = 0;

    for (const GeometryItem& item : geos) {
      const Geometry* geo = item.geo;
      assert(geo->kind == Geometry::Kind::Line);

      // Matrix that transform from local transform to cog
      Mat3x4d M = makeMat3x4d(geo->M_3x4.data);
      M.m03 -= localOrigin.x;
      M.m13 -= localOrigin.y;
      M.m23 -= localOrigin.z;

      V.resize(vertexOffset + 2);
      V[vertexOffset + 0] = makeVec3f(mul(M, makeVec3d(geo->line.a, 0.0, 0.0)));
      V[vertexOffset + 1] = makeVec3f(mul(M, makeVec3d(geo->line.b, 0.0, 0.0)));
      vertexOffset += 2;
    }

    uint32_t positionAccessorIx = createAccessorVec3f(ctx, model, V.data(), vertexOffset, true);

    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = model.rjAlloc;

    rj::Value rjAttributes(rj::kObjectType);
    rjAttributes.AddMember("POSITION", positionAccessorIx, alloc);

    rj::Value rjPrimitive(rj::kObjectType);
    rjPrimitive.AddMember("mode", 0x0001 /* GL_LINES */, alloc);
    rjPrimitive.AddMember("attributes", rjAttributes, alloc);
    rjPrimitive.AddMember("material", geos[0].sortKey >> 1, alloc);

    rjPrimitives.PushBack(rjPrimitive, alloc);

    ctx.logger(2, "exportGLTF: merged %zu lines, vertexCount=%zu", geos.size(), vertexOffset);

    return true;  // We did add geometry
  }

  bool addPrimitiveForTriangulations(Context& ctx, Model& model, rj::Value& rjPrimitives, const std::span<const GeometryItem>& geos, const Vec3d& localOrigin)
  {
    assert(!geos.empty());
    std::vector<Vec3f>& V = ctx.tmp3f_1;  // No need to clear, they get resized before written to
    std::vector<Vec3f>& N = ctx.tmp3f_2;
    std::vector<uint32_t>& I = ctx.tmp32ui;
    size_t vertexOffset = 0;
    size_t indexOffset = 0;

    for (const GeometryItem& item : geos) {
      const Geometry* geo = item.geo;
      assert(geo->kind != Geometry::Kind::Line);
      if (!geo->triangulation) continue;  // Skip missing triangulations

      // Matrix that transform from local transform to cog
      Mat3x4d M = makeMat3x4d(geo->M_3x4.data);
      M.m03 -= localOrigin.x;
      M.m13 -= localOrigin.y;
      M.m23 -= localOrigin.z;

      const Mat3f T = makeMat3f(geo->M_3x4.data);

      size_t vertexCount = geo->triangulation->vertices_n;
      size_t indexCount = 3 * geo->triangulation->triangles_n;

      // Transform vertices and normals into new frame
      V.resize(vertexOffset + vertexCount);
      N.resize(vertexOffset + vertexCount);

      for (size_t i = 0; i < vertexCount; i++) {
        ctx.tmp3f_1[vertexOffset + i] = makeVec3f(mul(M, makeVec3d(geo->triangulation->vertices + 3 * i)));
        Vec3f n = normalize(mul(T, makeVec3f(geo->triangulation->normals + 3 * i)));
        if (!std::isfinite(n.x) || !std::isfinite(n.y) || !std::isfinite(n.z)) {
          n = makeVec3f(1.f, 0.f, 0.f);
        }
        N[vertexOffset + i] = n;
      }

      // Transform indices
      I.resize(indexOffset + indexCount);
      for (size_t i = 0; i < indexCount; i++) {
        I[indexOffset + i] = static_cast<uint32_t>(vertexOffset + geo->triangulation->indices[i]);
      }

      vertexOffset += vertexCount;
      indexOffset += indexCount;
    }

    ctx.logger(2, "exportGLTF: merged %zu meshes, vertexCount=%zu, indexCount=%zu", geos.size(), vertexOffset, indexOffset);
    if (vertexOffset != 0 && indexOffset != 0) {
      uint32_t positionAccessorIx = createAccessorVec3f(ctx, model, V.data(), vertexOffset, true);
      uint32_t normalAccessorIx = createAccessorVec3f(ctx, model, N.data(), vertexOffset, true);
      uint32_t indicesAccesorIx = createAccessorUint32(ctx, model, I.data(), indexOffset, false);

      rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = model.rjAlloc;

      rj::Value rjAttributes(rj::kObjectType);
      rjAttributes.AddMember("POSITION", positionAccessorIx, alloc);
      rjAttributes.AddMember("NORMAL", normalAccessorIx, alloc);

      rj::Value rjPrimitive(rj::kObjectType);
      rjPrimitive.AddMember("mode", 0x0004 /* GL_TRIANGLES */, alloc);
      rjPrimitive.AddMember("attributes", rjAttributes, alloc);
      rjPrimitive.AddMember("indices", indicesAccesorIx, alloc);
      rjPrimitive.AddMember("material", geos[0].sortKey >> 1, alloc);

      rjPrimitives.PushBack(rjPrimitive, alloc);

      return true;  // We did add geometry
    }

    return false; // No geometry added
  }

  bool insertMergedGeometriesIntoNode(Context& ctx, Model& model, rj::Value& node, std::vector<GeometryItem>& geos)
  {
    // Calc average pos and count number of vertices
    Vec3d avg = makeVec3d(0.0, 0.0, 0.0);
    {
      size_t nv = 0;
      for (const GeometryItem& item : geos) {
        const Geometry* geo = item.geo;
        const Mat3x4d M = makeMat3x4d(geo->M_3x4.data);
        if (geo->kind == Geometry::Kind::Line) {
          avg = avg
              + mul(M, makeVec3d(geo->line.a, 0.0, 0.0))
              + mul(M, makeVec3d(geo->line.b, 0.0, 0.0));
          nv += 2;
        }
        else if (geo->triangulation) {
          for (size_t i = 0; i < geo->triangulation->vertices_n; i++) {
            avg = avg + mul(M, makeVec3d(geo->triangulation->vertices + 3 * i));
          }
          nv += geo->triangulation->vertices_n;
        }
      }
      avg = (nv ? 1.0 / static_cast<double>(nv) : 0.0) * avg;
    }

    rj::Value rjPrimitives(rj::kArrayType);

    // Break down into ranges of fixed sort key (fixed material and primitive type)    
    std::sort(geos.begin(), geos.end(), [](const GeometryItem& a, const GeometryItem& b) { return a.sortKey < b.sortKey; });
    for (size_t a = 0, n = geos.size(); a < n; ) {
      size_t b = a + 1;
      while (b < n && geos[a].sortKey == geos[b].sortKey) { b++; }

      // build primitive containing range
      std::span<const GeometryItem> span(geos.data() + a, b - a);
      if (geos[a].geo->kind == Geometry::Kind::Line) {
        addPrimitiveForLines(ctx, model, rjPrimitives, span, avg);
      }
      else {
        addPrimitiveForTriangulations(ctx, model, rjPrimitives, span, avg);
      }
      a = b;
    }

    if (rjPrimitives.Empty()) {
      return false; // No primitives
    }

    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = model.rjAlloc;

    rj::Value mesh(rj::kObjectType);
    mesh.AddMember("primitives", rjPrimitives, alloc);
    uint32_t meshIndex = model.rjMeshes.Size();
    model.rjMeshes.PushBack(mesh, alloc);

    node.AddMember("mesh", meshIndex, alloc);

    rj::Value translation(rj::kArrayType);
    for (size_t r = 0; r < 3; r++) {
      translation.PushBack(avg[r] - model.origin[r], alloc);
    }
    node.AddMember("translation", translation, alloc);

    return true;
  }

  void addAttributes(Context& ctx, Model& model, rj::Value& rjNode, const Node* node)
  {
    // Optionally add all attributes under an "extras" object member.
    if (ctx.includeAttributes && node->attributes.first) {
      rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = model.rjAlloc;
      rj::Value extras(rj::kObjectType);

      rj::Value attributes(rj::kObjectType);
      for (Attribute* att = node->attributes.first; att; att = att->next) {
        if (attributes.HasMember(att->key)) {
          ctx.logger(1, "exportGLTF: Duplicate attribute key, discarding %s=\"%s\"", att->key, att->val);
        }
        attributes.AddMember(rj::Value(att->key, alloc), rj::Value(att->val, alloc), alloc);
      }

      extras.AddMember("rvm-attributes", attributes, alloc);
      rjNode.AddMember("extras", extras, alloc);
    }
  }

  uint32_t processNode(Context& ctx, Model& model, const Node* node, size_t level);

  void processChildren(Context& ctx, Model& model, rj::Value& children, const Node* firstChild, size_t level)
  {
    size_t nextLevel = level + 1;
    if (nextLevel == ctx.split.level) {
      for (const Node* child = firstChild; child; child = child->next) {
        if (ctx.split.index == ctx.split.choose) {
          ctx.logger(0, "exportGLTF: At split level %zu: Restricting to subtree %zu", nextLevel, ctx.split.index);
          children.PushBack(processNode(ctx, model, child, nextLevel), model.rjAlloc);
        }
        ctx.split.index++;
      }
    }
    else {
      for (const Node* child = firstChild; child; child = child->next) {
        children.PushBack(processNode(ctx, model, child, nextLevel), model.rjAlloc);
      }
    }
  }

  void addGeometries(Context& ctx, Model& model, rj::Value& rjNode, rj::Value& rjNodeChildren, std::vector<GeometryItem>& geos, const bool modifyNodeTransform)
  {
    // Handle merging of multiple geometries
    if (ctx.mergeGeometries && 1 < geos.size()) {
      if (modifyNodeTransform) {
        insertMergedGeometriesIntoNode(ctx, model, rjNode, geos);
      }
      else {
        rj::Value geometryNode(rj::kObjectType);
        if (insertMergedGeometriesIntoNode(ctx, model, geometryNode, geos)) {
          addChildNode(model, rjNodeChildren, geometryNode);
        }
      }
    }

    // Handle single geometry when we can modify the node transform
    else if (modifyNodeTransform && geos.size() == 1) {
      insertGeometryIntoNode(ctx, model, rjNode, geos[0].geo);
    }

    // Or we have to create holder geometries for all
    else {
      for (const GeometryItem& item : geos) {
        rj::Value geometryNode(rj::kObjectType);
        if (insertGeometryIntoNode(ctx, model, geometryNode, item.geo)) {
          addChildNode(model, rjNodeChildren, geometryNode);
        }
      }
    }
  }

  uint32_t processNode(Context& ctx, Model& model, const Node* node, size_t level)
  {
    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = model.rjAlloc;

    rj::Value rjNode(rj::kObjectType);
    rj::Value children(rj::kArrayType);

    // If we are splitting, only include attributes and geometries below the split
    // point in the first file
    bool includeContent = true;
    if (level < ctx.split.level && ctx.split.index != 0) {
      includeContent = false;
    }

    switch (node->kind) {
    case Node::Kind::File:
      if (node->file.path) {
        rjNode.AddMember("name", rj::Value(node->file.path, alloc), alloc);
      }
      if (includeContent) {
        addAttributes(ctx, model, rjNode, node);
      }
      break;

    case Node::Kind::Model:
      if (node->model.name) {
        rjNode.AddMember("name", rj::Value(node->model.name, alloc), alloc);
      }
      if (includeContent) {
        addAttributes(ctx, model, rjNode, node);
      }
      break;

    case Node::Kind::Group:
      if (node->group.name) {
        rjNode.AddMember("name", rj::Value(node->group.name, alloc), alloc);
      }
      if (includeContent) {
        addAttributes(ctx, model, rjNode, node);

        if(node->group.geometries.first != nullptr) {

          // Collect all geometries
          std::vector<GeometryItem>& geos = ctx.tmpGeos;
          geos.clear();
          for (Geometry* geo = node->group.geometries.first; geo; geo = geo->next) {
            size_t sortKey = (static_cast<size_t>(createOrGetColor(ctx, model, geo)) << 1) | (geo->kind == Geometry::Kind::Line ? 1 : 0);
            geos.push_back({ .sortKey = sortKey, .geo = geo });
          }

          // Add geometries under node
          addGeometries(ctx, model, rjNode, children, geos, node->children.first == nullptr);

        }
      }
      break;

    default:
      assert(false && "Illegal enum");
      break;
    }

    // And recurse into children
    processChildren(ctx, model, children, node->children.first, level);

    if (!children.Empty()) {
      rjNode.AddMember("children", children, alloc);
    }

    // Add this node to document
    uint32_t nodeIndex = model.rjNodes.Size();
    model.rjNodes.PushBack(rjNode, alloc);
    return nodeIndex;
  }


  void extendBounds(BBox3f& worldBounds, const Node* node)
  {
    for (Node* child = node->children.first; child; child = child->next) {
      extendBounds(worldBounds, child);
    }
    if (node->kind == Node::Kind::Group) {
      for (Geometry* geo = node->group.geometries.first; geo; geo = geo->next) {
        engulf(worldBounds, geo->bboxWorld);
      }
    }
  }

  void calculateOrigin(Context& ctx, Model& model, const Node* firstNode)
  {
    BBox3f worldBounds = createEmptyBBox3f();

    for (const Node* node = firstNode; node; node = node->next) {
      extendBounds(worldBounds, node);
    }

    model.origin = 0.5f * (worldBounds.min + worldBounds.max);
    ctx.logger(0, "exportGLTF: world bounds = [%.2f, %.2f, %.2f]x[%.2f, %.2f, %.2f]",
               worldBounds.min.x, worldBounds.min.y, worldBounds.min.z,
               worldBounds.max.x, worldBounds.max.y, worldBounds.max.z);
    ctx.logger(0, "exportGLTF: setting origin = [%.2f, %.2f, %.2f]",
               model.origin.x, model.origin.y, model.origin.z);
  }

  rj::Document buildGLTF(Context& ctx, Model& model, const Node* firstNode)
  {
    rj::MemoryPoolAllocator<rj::CrtAllocator>& alloc = model.rjAlloc;

    if (ctx.centerModel) {
      calculateOrigin(ctx, model, firstNode);
    }

    rj::Document rjDoc(rj::kObjectType, &alloc);

    // ------- asset -----------------------------------------------------------
    rj::Value rjAsset(rj::kObjectType);
    rjAsset.AddMember("version", "2.0", alloc);
    rjAsset.AddMember("generator", "rvmparser", alloc);
    if (ctx.centerModel) {
      rj::Value rjOrigin(rj::kArrayType);
      rjOrigin.PushBack(model.origin.x, alloc);
      rjOrigin.PushBack(model.origin.y, alloc);
      rjOrigin.PushBack(model.origin.z, alloc);

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
      processChildren(ctx, model, children, firstNode, 0);

      // Add node to document
      rj::Value node(rj::kObjectType);
      node.AddMember("name", "rvmparser-rotate-z-to-y", alloc);
      node.AddMember("rotation", rotation, alloc);
      node.AddMember("children", children, alloc);

      uint32_t nodeIndex = model.rjNodes.Size();
      model.rjNodes.PushBack(node, alloc);

      // Set root
      rjSceneInstanceNodes.PushBack(nodeIndex, alloc);
    }
    else {
      processChildren(ctx, model, rjSceneInstanceNodes, firstNode, 0);
    }

    // If we have a GLB container, add a single buffer that holds all data
    if (ctx.glbContainer) {
      assert(model.rjBuffers.Empty());
      rj::Value rjGlbBuffer(rj::kObjectType);
      rjGlbBuffer.AddMember("byteLength", model.dataBytes, alloc);
      model.rjBuffers.PushBack(rjGlbBuffer, alloc);
    }


    rj::Value rjSceneInstance(rj::kObjectType);
    rjSceneInstance.AddMember("nodes", rjSceneInstanceNodes, alloc);

    rj::Value rjScenes(rj::kArrayType);
    rjScenes.PushBack(rjSceneInstance, alloc);
    rjDoc.AddMember("scenes", rjScenes, alloc);
    rjDoc.AddMember("nodes", model.rjNodes, alloc);
    rjDoc.AddMember("meshes", model.rjMeshes, alloc);
    rjDoc.AddMember("materials", model.rjMaterials, alloc);
    rjDoc.AddMember("accessors", model.rjAccessors, alloc);
    rjDoc.AddMember("bufferViews", model.rjBufferViews, alloc);
    rjDoc.AddMember("buffers", model.rjBuffers, alloc);

    return rjDoc;
  }

  bool writeAsGLB(Context& ctx, Model& model, FILE* out, const char* path, const rj::Document& rjDoc)
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
      8 + model.dataBytes;              // BVIN header and payload

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
      model.dataBytes,  // length of chunk data
      0x004E4942            // chunk type (BIN)
    };

    if (fwrite(binChunkHeader, sizeof(binChunkHeader), 1, out) != 1) {
      ctx.logger(2, "%s: Error writing BIN chunk header", path);
      fclose(out);
      return false;
    }

    uint32_t offset = 0;
    for (DataItem* item = model.dataItems.first; item; item = item->next) {
      if (fwrite(item->ptr, item->size, 1, out) != 1) {
        ctx.logger(2, "%s: Error writing BIN chunk data at offset %u", path, offset);
        fclose(out);
        return false;
      }
      offset += item->size;
    }
    assert(offset == model.dataBytes);

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

  bool processSubtree(Context& ctx, const char* path, const Node* firstNode)
  {
    Model model;

    rj::Document rjDoc = buildGLTF(ctx, model, firstNode);

#ifdef _WIN32
    FILE* out = nullptr;
    auto err = fopen_s(&out, path, "wb");
    if (err != 0) {
      char buf[256];
      if (strerror_s(buf, sizeof(buf), err) != 0) {
        buf[0] = '\0';
      }
      ctx.logger(2, "Failed to open %s for writing: %s", path, buf);
      return false;
    }
    assert(out);
#else
    FILE* out = fopen(path, "w");
    if (out == nullptr) {
      ctx.logger(2, "Failed to open %s for writing.", path);
      return false;
    }
#endif


    bool success = true;
    if (ctx.glbContainer) {
      success = writeAsGLB(ctx, model, out, path, rjDoc);
    }
    else {
      success = writeAsGLTF(ctx, out, path, rjDoc);
    }

    fclose(out);

    return success;
  }

}


bool exportGLTF(Store* store, Logger logger, const char* path, size_t splitLevel, bool rotateZToY, bool centerModel, bool includeAttributes)
{
  Context ctx{
    .logger = logger,
    .centerModel = centerModel,
    .rotateZToY = rotateZToY,
    .includeAttributes = includeAttributes,
  };
  ctx.split.level = splitLevel;

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
    ctx.path = store->strings.intern(path, path + o);
    ctx.suffix = store->strings.intern(path + o);
  }


  ctx.logger(0, "exportGLTF: rotate-z-to-y=%u center=%u attributes=%u",
             ctx.rotateZToY ? 1 : 0,
             ctx.centerModel ? 1 : 0,
             ctx.includeAttributes ? 1 : 0);
  do {
    ctx.split.index = 0;

    std::vector<char> tmp(1);
    const char* currentPath = path;
    if (ctx.split.choose != 0) {
      while (true) {
        int n = snprintf(tmp.data(), tmp.size(), "%s%zu%s", ctx.path, ctx.split.choose, ctx.suffix);
        if (n < 0) {
          ctx.logger(2, "exportGLTF: sprintf error");
          return false;
        }
        if (n < tmp.size()) {
          currentPath = tmp.data();
          break;
        }
        tmp.resize(n + 1);
      }
    }

    if (!processSubtree(ctx, currentPath, store->getFirstRoot())) {
      return false;
    }
    ctx.logger(0, "exportGLTF: Wrote %s", currentPath);
    ctx.split.choose++;
  } while (ctx.split.choose < ctx.split.index);


  return true;
}
