#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include <cassert>
#include <string>
#include <cctype>

#include "RVMParser.h"
#include "FindConnections.h"
#include "Tessellator.h"
#include "ExportObj.h"
#include "Store.h"
#include "Flatten.h"
#include "AddStats.h"
#include "DumpNames.h"


template<typename F>
bool
processFile(const std::string& path, F f)
{
  bool rv = true;
  HANDLE h = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE) rv = false;
  else {
    DWORD hiSize;
    DWORD loSize = GetFileSize(h, &hiSize);
    size_t fileSize = (size_t(hiSize) << 32u) + loSize;

    HANDLE m = CreateFileMappingA(h, 0, PAGE_READONLY, 0, 0, NULL);
    if (m == INVALID_HANDLE_VALUE)  rv = false;
    else {
      const void * ptr = MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
      if (ptr == nullptr) rv = false;
      else {
        rv = f(ptr, fileSize);
        UnmapViewOfFile(ptr);
      }
      CloseHandle(m);
    }
    CloseHandle(h);
  }
  return rv;

}


int main(int argc, char** argv)
{
  int rv = 0;
  bool run_flatten = true;
  bool dump_names = false;


  Store* store = new Store();

  Flatten flatten;

  std::string stem;
  for (int i = 1; i < argc; i++) {
    auto arg = std::string(argv[i]);

    if (arg == "--dump-names") {
      dump_names = true;
      continue;
    }

    auto arg_lc = arg;
    for (auto & c : arg_lc) c = std::tolower(c);
    auto l = arg_lc.rfind(".rvm");
    if (l == std::string::npos) {
      fprintf(stderr, "Failed to find suffix '.rvm' in path %s\n", arg.c_str());
      rv = -1;
      break;
    }
    stem = arg.substr(0, l);

    if (!processFile(arg, [store](const void * ptr, size_t size) { return parseRVM(store, ptr, size); }))
    {
      fprintf(stderr, "Failed to parse %s: %s\n", arg.c_str(), store->errorString());
      rv = -1;
      break;
    }
    fprintf(stderr, "Successfully parsed %s\n", arg.c_str());

    if (run_flatten) {
      auto tagFile = stem + ".tag";

      if (processFile(tagFile, [f = &flatten](const void * ptr, size_t size) {f->setKeep(ptr, size); return true; })) {
        fprintf(stderr, "Processed %s\n", tagFile.c_str());
      }
      else {
        run_flatten = false;
      }
    }

  }



  if (rv == 0 && !stem.empty()) {
    if (run_flatten) {
      store->apply(&flatten);

      delete store;
      store = flatten.result();
    }

    if (dump_names) {
      FILE* out;
      if (fopen_s(&out, "names.txt", "w") == 0) {
        DumpNames dumpNames;
        dumpNames.setOutput(out);
        store->apply(&dumpNames);
        fclose(out);
      }
    }

    AddStats addStats;
    store->apply(&addStats);
    auto * stats = store->stats;
    if (stats) {
      fprintf(stderr, "Stats:\n");
      fprintf(stderr, "    Groups                 %d\n", stats->group_n);
      fprintf(stderr, "    Geometries             %d (grp avg=%.1f)\n", stats->geometry_n, stats->geometry_n / float(stats->group_n));
      fprintf(stderr, "        Pyramids           %d\n", stats->pyramid_n);
      fprintf(stderr, "        Boxes              %d\n", stats->box_n);
      fprintf(stderr, "        Rectangular tori   %d\n", stats->rectangular_torus_n);
      fprintf(stderr, "        Circular tori      %d\n", stats->circular_torus_n);
      fprintf(stderr, "        Elliptical dish    %d\n", stats->elliptical_dish_n);
      fprintf(stderr, "        Spherical dish     %d\n", stats->spherical_dish_n);
      fprintf(stderr, "        Snouts             %d\n", stats->snout_n);
      fprintf(stderr, "        Cylinders          %d\n", stats->cylinder_n);
      fprintf(stderr, "        Spheres            %d\n", stats->sphere_n);
      fprintf(stderr, "        Facet groups       %d\n", stats->facetgroup_n);
      fprintf(stderr, "            triangles      %d\n", stats->facetgroup_triangles_n);
      fprintf(stderr, "            quads          %d\n", stats->facetgroup_quads_n);
      fprintf(stderr, "            polygons       %d (fgrp avg=%.1f)\n", stats->facetgroup_polygon_n, (stats->facetgroup_polygon_n / float(stats->facetgroup_n)));
      fprintf(stderr, "                contours   %d (poly avg=%.1f)\n", stats->facetgroup_polygon_n_contours_n, (stats->facetgroup_polygon_n_contours_n / float(stats->facetgroup_polygon_n)));
      fprintf(stderr, "                vertices   %d (cont avg=%.1f)\n", stats->facetgroup_polygon_n_vertices_n, (stats->facetgroup_polygon_n_vertices_n / float(stats->facetgroup_polygon_n_contours_n)));
      fprintf(stderr, "        Lines              %d\n", stats->line_n);
    }


    //FindConnections findConnections;
    //store->apply(&findConnections);

#if 1
    Tessellator tessellator;
    store->apply(&tessellator);

    ExportObj exportObj;
    if (exportObj.open((stem + ".obj").c_str(), (stem + ".mtl").c_str())) {
      store->apply(&exportObj);
    }
    else {
      fprintf(stderr, "Failed to export obj file.\n");
      rv = -1;
    }
#endif
  }

  delete store;

  //auto a = getc(stdin);
 
  return rv;
}

