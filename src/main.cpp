#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include <cassert>

#include "RVMParser.h"
#include "RVMVisitor.h"
#include "ExportObj.h"

namespace {

  class DummyVisitor : public RVMVisitor
  {
    unsigned indent = 0;

    const char* indents[10] = {
      "",
      "  ",
      "    ",
      "      ",
      "        ",
      "          ",
      "            ",
      "              ",
      "                ",
      "                  ",
    };

    const char* istr()
    {
      return indents[indent < 10 ? indent : 9];
    }

  public:
    void beginFile(const std::string info, const std::string& note, const std::string& date, const std::string& user, const std::string& encoding) override
    {
      fprintf(stderr, "%s+- info     '%s'\n", istr(), info.c_str());
      fprintf(stderr, "%s+- note     '%s'\n", istr(), note.c_str());
      fprintf(stderr, "%s+- date     '%s'\n", istr(), date.c_str());
      fprintf(stderr, "%s+- user     '%s'\n", istr(), user.c_str());
      fprintf(stderr, "%s+- encoding '%s'\n", istr(), encoding.c_str());
    }

    void endFile() override { }

    void beginModel(const std::string& project, const std::string& name) override
    {
      fprintf(stderr, "%s+- group: project='%s', name='%s'\n", istr(), project.c_str(), name.c_str());
      indent++;
    }

    void endModel() override
    {
      --indent;
    }

    void beginGroup(const std::string& name, const float* translation, const uint32_t material)
    {
      fprintf(stderr, "%s+- Group '%s' @ [%f, %f, %f] mat=%d:\n", istr(),
              name.c_str(), translation[0], translation[1], translation[2], material);
      indent++;
    }

    void EndGroup()
    {
      --indent;
    }

    void line(float* affine, float* bbox, float x0, float x1) override
    {
      fprintf(stderr, "%s+- line: [%f, %f]\n", istr(), x0, x1);
    }

    void pyramid(float* affine, float* bbox, float* bottom_xy, float* top_xy, float* offset_xy, float height) override
    {
      fprintf(stderr, "%s+- pyramid: b=[%f, %f], t=[%f, %f], h=%f, o=[%f,%f]\n", istr(),
              bottom_xy[0], bottom_xy[1], top_xy[0], top_xy[1], height, offset_xy[0], offset_xy[1]);
    }

    void box(float* affine, float* box, float* lengths) override
    {
      fprintf(stderr, "%s+- box l=[%f,%f,%f]\n", istr(), lengths[0], lengths[1], lengths[2]);
    }

    void sphere(float* affine, float* bbox, float diameter)
    {
      fprintf(stderr, "%s+- sphere d=%f\n", istr(), diameter);
    }

    void rectangularTorus(float* M_affine, float* bbox, float inner_radius, float outer_radius, float height, float angle) override
    {
      fprintf(stderr, "%s+- recTorus: r_i=%f, r_o=%f, h=%f, a=%f\n", istr(), inner_radius, outer_radius, height, angle);
    }

    void circularTorus(float* M_affine, float* bbox, float offset, float radius, float angle) override
    {
      fprintf(stderr, "%s+- circular torus: o=%f, r=%f, a=%f\n", istr(), offset, radius, angle);
    }

    void ellipticalDish(float* affine, float* bbox, float diameter, float radius) override
    {
      fprintf(stderr, "%s+- elliptical dish: d=%f, r=%f\n", istr(), diameter, radius);
    }

    void sphericalDish(float* affine, float* bbox, float diameter, float height) override
    {
      fprintf(stderr, "%s+- spherical dish: d=%f, h=%f\n", istr(), diameter, height);

   }

    void cylinder(float* affine, float* bbox, float radius, float height) override
    {
      fprintf(stderr, "%s+- cylinder r=%f, h=%f\n", istr(), radius, height);
    }

    void snout(float* affine, float*bbox, float* offset, float* bshear, float* tshear, float bottom, float top, float height) override
    {
      fprintf(stderr, "%s+- snout o=[%f,%f], bs=[%f,%f], ts=[%f,%f], b=%f, t=%f, h=%f\n",
              istr(), offset[0], offset[1],
              bshear[0], bshear[1],
              tshear[0], tshear[1],
              bottom, top, height);
    }

    void facetGroup(float* affine, float* bbox, std::vector<uint32_t>& polygons, std::vector<uint32_t>& contours, std::vector<float>& P, std::vector<float>& N)
    {
      fprintf(stderr, "%s+- facet group polys=%zd, countours=%zd, vertices=%zd\n", istr(), polygons.size() - 1, contours.size() - 1, P.size() / 3);
    }



  };

}

int main(int argc, char** argv)
{
  for (int i = 1; i < argc; i++) {
    auto outfile = std::string(argv[i]) + ".obj";

    fprintf(stderr, "Converting '%s' -> '%s'\n", argv[i], outfile.c_str());

    HANDLE h = CreateFileA(argv[i], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(h != INVALID_HANDLE_VALUE);

    DWORD hiSize;
    DWORD loSize = GetFileSize(h, &hiSize);
    size_t fileSize = (size_t(hiSize) << 32u) + loSize;

    HANDLE m = CreateFileMappingA(h, 0, PAGE_READONLY, 0, 0, NULL);
    assert(m != INVALID_HANDLE_VALUE);

    const void * ptr = MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
    assert(ptr != nullptr);

    if (false) {
      DummyVisitor visitor;
      parseRVM(&visitor, ptr, fileSize);
    }
    else if (true) {
      ExportObj visitor(outfile);
      parseRVM(&visitor, ptr, fileSize);
    }

    UnmapViewOfFile(ptr);
    CloseHandle(m);
    CloseHandle(h);
  }

  //auto a = getc(stdin);
 
  return 0;
}

