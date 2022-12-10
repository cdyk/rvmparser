#include <cassert>
#include <cstdio>
#include <string>
#include <algorithm>
#include "ExportObj.h"
#include "Store.h"
#include "LinAlgOps.h"

namespace {

  bool open_w(FILE** f, const char* path)
  {
#ifdef _WIN32
    auto err = fopen_s(f, path, "w");
    if (err == 0) return true;

    char buf[1024];
    if (strerror_s(buf, sizeof(buf), err) != 0) {
      buf[0] = '\0';
    }
    fprintf(stderr, "Failed to open %s for writing: %s", path, buf);
#else
    *f = fopen(path, "w");
    if(*f != nullptr) return true;

    fprintf(stderr, "Failed to open %s for writing.", path);
#endif
    return false;
  }

  void wireBoundingBox(FILE* out, unsigned& off_v, const BBox3f& bbox)
  {
    for (unsigned i = 0; i < 8; i++) {
      float px = (i & 1) ? bbox.min[0] : bbox.min[3];
      float py = (i & 2) ? bbox.min[1] : bbox.min[4];
      float pz = (i & 4) ? bbox.min[2] : bbox.min[5];
      fprintf(out, "v %f %f %f\n", px, py, pz);
    }
    fprintf(out, "l %d %d %d %d %d\n",
            off_v + 0, off_v + 1, off_v + 3, off_v + 2, off_v + 0);
    fprintf(out, "l %d %d %d %d %d\n",
            off_v + 4, off_v + 5, off_v + 7, off_v + 6, off_v + 4);
    fprintf(out, "l %d %d\n", off_v + 0, off_v + 4);
    fprintf(out, "l %d %d\n", off_v + 1, off_v + 5);
    fprintf(out, "l %d %d\n", off_v + 2, off_v + 6);
    fprintf(out, "l %d %d\n", off_v + 3, off_v + 7);
    off_v += 8;
  }

}


ExportObj::~ExportObj()
{
  if (out) {
    fclose(out);
  }
  if (mtl) {
    fclose(mtl);
  }
}

bool ExportObj::open(const char* path_obj, const char* path_mtl)
{
  if (!open_w(&out, path_obj)) return false;
  if (!open_w(&mtl, path_mtl)) return false;

  std::string mtllib(path_mtl);
  auto l = mtllib.find_last_of("/\\");
  if (l != std::string::npos) {
    mtllib = mtllib.substr(l + 1);
  }

  fprintf(out, "mtllib %s\n", mtllib.c_str());

  if (groupBoundingBoxes) {
    fprintf(mtl, "newmtl group_bbox\n");
    fprintf(mtl, "Ka 1 0 0\n");
    fprintf(mtl, "Kd 1 0 0\n");
    fprintf(mtl, "Ks 0 0 0\n");
  }

  return true;
}


void ExportObj::init(class Store& store_)
{
  store = &store_;
  assert(out);
  assert(mtl);

  conn = store->conn;

  stack.accommodate(store->groupCountAllocated());

  for (auto * line = store->getFirstDebugLine(); line != nullptr; line = line->next) {

    uint32_t colorId = (line->color << 8);
    if (!definedColors.get((uint64_t(colorId) << 1) | 1)) {
      definedColors.insert(((uint64_t(colorId) << 1) | 1), 1);
      auto r = (1.f / 255.f)*((line->color >> 16) & 0xFF);
      auto g = (1.f / 255.f)*((line->color >> 8) & 0xFF);
      auto b = (1.f / 255.f)*((line->color) & 0xFF);
      fprintf(mtl, "newmtl %#x\n", colorId);
      fprintf(mtl, "Ka %f %f %f\n", (2.0 / 3.0)*r, (2.0 / 3.0)*g, (2.0 / 3.0)*b);
      fprintf(mtl, "Kd %f %f %f\n", r, g, b);
      fprintf(mtl, "Ks 0.5 0.5 0.5\n");
    }
    fprintf(out, "usemtl %#x\n", colorId);

    fprintf(out, "v %f %f %f\n", line->a[0], line->a[1], line->a[2]);
    fprintf(out, "v %f %f %f\n", line->b[0], line->b[1], line->b[2]);
    fprintf(out, "l -1 -2\n");
    off_v += 2;
  }
}

void ExportObj::beginFile(Group* group)
{
  fprintf(out, "# %s\n", group->file.info);
  fprintf(out, "# %s\n", group->file.note);
  fprintf(out, "# %s\n", group->file.date);
  fprintf(out, "# %s\n", group->file.user);
}

void ExportObj::endFile()
{
  fprintf(out, "# End of file\n");
}

void ExportObj::beginModel(Group* group)
{
  fprintf(out, "# Model project=%s, name=%s\n", group->model.project, group->model.name);
}

void ExportObj::endModel() { }

void ExportObj::beginGroup(Group* group)
{
  for (unsigned i = 0; i < 3; i++) curr_translation[i] = group->group.translation[i];

  stack[stack_p++] = group->group.name;

  fprintf(out, "o %s", stack[0]);
  for (unsigned i = 1; i < stack_p; i++) {
    fprintf(out, "/%s", stack[i]);
  }
  fprintf(out, "\n");
 

//  fprintf(out, "o %s\n", group->group.name);


  if (groupBoundingBoxes && !isEmpty(group->group.bboxWorld)) {
    fprintf(out, "usemtl group_bbox\n");
    wireBoundingBox(out, off_v, group->group.bboxWorld);
  }

}

void ExportObj::EndGroup() {
  assert(stack_p);
  stack_p--;
}

namespace {

  void getMidpoint(Vec3f& p, Geometry* geo)
  {
    switch (geo->kind) {
    case Geometry::Kind::CircularTorus: {
      auto & ct = geo->circularTorus;
      auto c = std::cos(0.5f * ct.angle);
      auto s = std::sin(0.5f * ct.angle);
      p.x = ct.offset * c;
      p.y = ct.offset * s;
      p.z = 0.f;
      break;
    }
    default:
      p = makeVec3f(0.f);
      break;
    }
    p = mul(geo->M_3x4, p);
  }

}

void ExportObj::geometry(struct Geometry* geometry)
{
  uint32_t colorId = (geometry->color << 8) | geometry->transparency;
  if (!definedColors.get((uint64_t(colorId) << 1) | 1)) {
    definedColors.insert(((uint64_t(colorId) << 1) | 1), 1);

    double r = (1.0 / 255.0)*((geometry->color >> 16) & 0xFF);
    double g = (1.0 / 255.0)*((geometry->color >> 8) & 0xFF);
    double b = (1.0 / 255.0)*((geometry->color) & 0xFF);
    fprintf(mtl, "newmtl %#x\n", colorId);
    fprintf(mtl, "Ka %f %f %f\n", (2.0 / 3.0)*r, (2.0 / 3.0)*g, (2.0 / 3.0)*b);
    fprintf(mtl, "Kd %f %f %f\n", r,g, b);
    fprintf(mtl, "Ks 0.5 0.5 0.5\n");
    if (geometry->transparency) {
      fprintf(mtl, "d %f\n", std::max(0.0, std::min(1.0, 1.0 - (1.0 / 100.0) * geometry->transparency)));
    }
  }
  fprintf(out, "usemtl %#x\n", colorId);

  float scale = 1.f;
  if (geometry->kind == Geometry::Kind::Line) {
    auto a = scale * mul(geometry->M_3x4, makeVec3f(geometry->line.a, 0, 0));
    auto b = scale * mul(geometry->M_3x4, makeVec3f(geometry->line.b, 0, 0));
    fprintf(out, "v %f %f %f\n", a.x, a.y, a.z);
    fprintf(out, "v %f %f %f\n", b.x, b.y, b.z);
    fprintf(out, "l -1 -2\n");
    off_v += 2;
  }
  else {
    assert(geometry->triangulation);
    auto * tri = geometry->triangulation;

    if (tri->indices != 0) {
      //fprintf(out, "g\n");
      if (geometry->triangulation->error != 0.f) {
        fprintf(out, "# error=%f\n", geometry->triangulation->error);
      }
      for (size_t i = 0; i < 3 * tri->vertices_n; i += 3) {

        auto p = scale * mul(geometry->M_3x4, makeVec3f(tri->vertices + i));
        Vec3f n = normalize(mul(makeMat3f(geometry->M_3x4.data), makeVec3f(tri->normals + i)));
        if (!std::isfinite(n.x) || !std::isfinite(n.y) || !std::isfinite(n.z)) {
          n = makeVec3f(1.f, 0.f, 0.f);
        }
        fprintf(out, "v %f %f %f\n", p.x, p.y, p.z);
        fprintf(out, "vn %f %f %f\n", n.x, n.y, n.z);
      }
      if (tri->texCoords) {
        for (size_t i = 0; i < tri->vertices_n; i++) {
          const Vec2f vt = makeVec2f(tri->texCoords + 2 * i);
          fprintf(out, "vt %f %f\n", vt.x, vt.y);
        }
      }
      else {
        for (size_t i = 0; i < tri->vertices_n; i++) {
          auto p = scale * mul(geometry->M_3x4, makeVec3f(tri->vertices + 3*i));
          fprintf(out, "vt %f %f\n", 0*p.x, 0*p.y);
        }

        for (size_t i = 0; i < 3 * tri->triangles_n; i += 3) {
          auto a = tri->indices[i + 0];
          auto b = tri->indices[i + 1];
          auto c = tri->indices[i + 2];
          fprintf(out, "f %d/%d/%d %d/%d/%d %d/%d/%d\n",
                  a + off_v, a + off_t, a + off_n,
                  b + off_v, b + off_t, b + off_n,
                  c + off_v, c + off_t, c + off_n);
        }
      }

      off_v += tri->vertices_n;
      off_n += tri->vertices_n;
      off_t += tri->vertices_n;
    }
  }

}
