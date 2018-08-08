#include <cassert>
#include <cstdio>
#include "ExportObj.h"

ExportObj::ExportObj(const std::string& path)
{
  auto err = fopen_s(&out, path.c_str(), "w");
  assert(err == 0);

}

ExportObj::~ExportObj()
{
  if (out) {
    fclose(out);
  }
}


void ExportObj::beginFile(const std::string info, const std::string& note, const std::string& date, const std::string& user, const std::string& encoding)
{
  fprintf(out, "# %s\n", info.c_str());
  fprintf(out, "# %s\n", note.c_str());
  fprintf(out, "# %s\n", date.c_str());
  fprintf(out, "# %s\n", user.c_str());
}

void ExportObj::endFile() {
  fprintf(out, "# End of file\n");
}

void ExportObj::beginModel(const std::string& project, const std::string& name)
{
  fprintf(out, "# Model project=%s, name=%s\n", project.c_str(), name.c_str());
}

void ExportObj::endModel() { }

void ExportObj::beginGroup(const std::string& name, const float* translation, const uint32_t material)
{
  for (unsigned i = 0; i < 3; i++) curr_translation[i] = translation[i] - shift[i];

  fprintf(out, "o %s\n", name.c_str());

}

void ExportObj::EndGroup() { }

void ExportObj::triangles(float* affine, float* bbox, std::vector<float>& P, std::vector<float>& N, std::vector<uint32_t>& indices)
{
  if (first) {
    for (unsigned i = 0; i < 3; i++) shift[i] = curr_translation[i];
    for (unsigned i = 0; i < 3; i++) curr_translation[i] = 0.f;
    first = false;
  }

  assert(P.size() == N.size());
  fprintf(out, "g\n");
  for (size_t i = 0; i < P.size(); i += 3) {
    fprintf(out, "v %f %f %f\n",
            (curr_translation[0] + P[i]),
            (curr_translation[1] + P[i + 1]),
            (curr_translation[2] + P[i + 2]));
    fprintf(out, "vn %f %f %f\n", N[i], N[i + 1], N[i + 2]);
  }
  for (size_t i = 0; i < indices.size(); i += 3) {
    fprintf(out, "f %zd//%zd %zd//%zd %zd//%zd\n",
            indices[i + 0] + off, indices[i + 0] + off,
            indices[i + 1] + off, indices[i + 1] + off,
            indices[i + 2] + off, indices[i + 2] + off);
  }
  off += P.size();
}
