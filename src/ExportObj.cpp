#include <cassert>
#include <cstdio>
#include "ExportObj.h"
#include "Store.h"

ExportObj::ExportObj(const char* path)
{
  auto err = fopen_s(&out, path, "w");
  assert(err == 0);

}

ExportObj::~ExportObj()
{
  if (out) {
    fclose(out);
  }
}


void ExportObj::beginFile(const char* info, const char* note, const char* date, const char* user, const char* encoding)
{
  fprintf(out, "# %s\n", info);
  fprintf(out, "# %s\n", note);
  fprintf(out, "# %s\n", date);
  fprintf(out, "# %s\n", user);
}

void ExportObj::endFile() {
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

      if (true) {
        fprintf(out, "v %f %f %f\n", Px, Py, Pz);
        fprintf(out, "vn %f %f %f\n", Nx, Ny, Nz);
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
