#include <cassert>
#include <cstdio>
#include <string>
#include "ExportObj.h"
#include "FindConnections.h"
#include "Store.h"

ExportObj::ExportObj(const char* path)
{
  auto err = fopen_s(&out, path, "w");
  assert(err == 0);

  std::string mtlpath(path);
  auto l = mtlpath.rfind(".obj");
  assert(l != std::string::npos);

  mtlpath = mtlpath.substr(0, l) + ".mtl";
  err = fopen_s(&mtl, mtlpath.c_str(), "w");
  assert(err == 0);


  fprintf(out, "mtllib %s\n", mtlpath.c_str());

  fprintf(mtl, "newmtl default\n");
  fprintf(mtl, "Kd 0.2 0.2 0.2\n");
  fprintf(mtl, "Kd 1.0 1.0 1.0\n");

  fprintf(mtl, "newmtl red_line\n");
  fprintf(mtl, "Kd 1.0 0.0 0.0\n");
  fprintf(mtl, "Kd 1.0 0.0 0.0\n");

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

void ExportObj::init(class Store& store)
{
  conn = store.conn;
  assert(conn);
}

void ExportObj::beginFile(const char* info, const char* note, const char* date, const char* user, const char* encoding)
{
  fprintf(out, "# %s\n", info);
  fprintf(out, "# %s\n", note);
  fprintf(out, "# %s\n", date);
  fprintf(out, "# %s\n", user);
}

void ExportObj::endFile() {

  fprintf(out, "usemtl red_line\n");

  auto l = 0.05f;
  for (unsigned i = 0; i < conn->anchor_n; i++) {
    auto * p = conn->p + 3 * i;
    auto * n = conn->anchors[i].n;

    fprintf(out, "v %f %f %f\n", p[0], p[1], p[2]);
    fprintf(out, "v %f %f %f\n", p[0] + l * n[0], p[1] + l * n[1], p[2] + l * n[2]);
    fprintf(out, "l %d %d\n", (int)off, (int)off + 1);
    off += 2;
  }

  fprintf(out, "# End of file\n");
}

void ExportObj::beginModel(const char* project, const char* name)
{
  fprintf(out, "# Model project=%s, name=%s\n", project, name);
}

void ExportObj::endModel() { }

void ExportObj::beginGroup(const char* name, const float* translation, const uint32_t material)
{
  for (unsigned i = 0; i < 3; i++) curr_translation[i] = translation[i];

  fprintf(out, "o %s\n", name);

}

void ExportObj::EndGroup() { }

void ExportObj::geometry(struct Geometry* geometry)
{
  const auto & M = geometry->M_3x4;

  fprintf(out, "usemtl default\n");
  if (geometry->kind == Geometry::Kind::Line) {
    auto x0 = geometry->line.a;
    auto x1 = geometry->line.b;

    auto p0_x = M[0] * x0 + M[3] * 0.f + M[6] * 0.f + M[9];
    auto p0_y = M[1] * x0 + M[4] * 0.f + M[7] * 0.f + M[10];
    auto p0_z = M[2] * x0 + M[5] * 0.f + M[8] * 0.f + M[11];

    auto p1_x = M[0] * x1 + M[3] * 0.f + M[6] * 0.f + M[9];
    auto p1_y = M[1] * x1 + M[4] * 0.f + M[7] * 0.f + M[10];
    auto p1_z = M[2] * x1 + M[5] * 0.f + M[8] * 0.f + M[11];

    fprintf(out, "v %f %f %f\n", p0_x, p0_y, p0_z);
    fprintf(out, "v %f %f %f\n", p1_x, p1_y, p1_z);
    fprintf(out, "l -1 -2\n");

    off += 2;
  }
  else if (geometry->triangulation != nullptr) {
    auto * tri = geometry->triangulation;

    fprintf(out, "g\n");
    if(geometry->triangulation->error != 0.f) {
      fprintf(out, "# error=%f\n", geometry->triangulation->error);
    }
    for (size_t i = 0; i < 3 * tri->vertices_n; i += 3) {
      auto px = tri->vertices[i + 0];
      auto py = tri->vertices[i + 1];
      auto pz = tri->vertices[i + 2];
      auto nx = tri->normals[i + 0];
      auto ny = tri->normals[i + 1];
      auto nz = tri->normals[i + 2];

      float Px, Py, Pz, Nx, Ny, Nz;
      Px = M[0] * px + M[3] * py + M[6] * pz + M[9];
      Py = M[1] * px + M[4] * py + M[7] * pz + M[10];
      Pz = M[2] * px + M[5] * py + M[8] * pz + M[11];
      Nx = M[0] * nx + M[3] * ny + M[6] * nz;
      Ny = M[1] * nx + M[4] * ny + M[7] * nz;
      Nz = M[2] * nx + M[5] * ny + M[8] * nz;

      float s = 1.f / std::sqrt(Nx*Nx + Ny * Ny + Nz * Nz);


      if (true) {
        fprintf(out, "v %f %f %f\n", Px, Py, Pz);
        fprintf(out, "vn %f %f %f\n", s*Nx, s*Ny, s*Nz);
      }
      else {
        fprintf(out, "v %f %f %f\n", px, py, pz);
        fprintf(out, "vn %f %f %f\n", nx, ny, nz);
      }
    }
    for (size_t i = 0; i < 3*tri->triangles_n; i += 3) {
      fprintf(out, "f %zd//%zd %zd//%zd %zd//%zd\n",
              tri->indices[i + 0] + off, tri->indices[i + 0] + off,
              tri->indices[i + 1] + off, tri->indices[i + 1] + off,
              tri->indices[i + 2] + off, tri->indices[i + 2] + off);
    }
    off += tri->vertices_n;
  }

}
