#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <cassert>
#include <string>
#include <cctype>
#include <algorithm>

#include "Parser.h"
#include "FindConnections.h"
#include "Tessellator.h"
#include "ExportObj.h"
#include "ExportJson.h"
#include "Store.h"
#include "Flatten.h"
#include "AddStats.h"
#include "DumpNames.h"
#include "ChunkTiny.h"
#include "AddGroupBBox.h"
#include "Colorizer.h"

template<typename F>
int
processFile(const std::string& path, F f)
{
  int rv = 2;
  HANDLE h = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE) {
    return 1;
  }
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
        rv = f(ptr, fileSize) ? 0 : 2;
        UnmapViewOfFile(ptr);
      }
      CloseHandle(m);
    }
    CloseHandle(h);
  }
  return rv;

}


void logger(unsigned level, const char* msg, ...)
{
  switch (level) {
  case 0: fprintf(stderr, "[I] "); break;
  case 1: fprintf(stderr, "[W] "); break;
  case 2: fprintf(stderr, "[E] "); break;
  }

  va_list argptr;
  va_start(argptr, msg);
  vfprintf(stderr, msg, argptr);
  va_end(argptr);
  fprintf(stderr, "\n");
}

void printHelp(const char* argv0)
{
  fprintf(stderr, "\nUsage: %s [options] files\n\n", argv0);
  fprintf(stderr, "Files with .rvm-suffix will be interpreted as a geometry files, and files with .txt\n");
  fprintf(stderr, "suffix will be interpreted as attribute files.\n");
  fprintf(stderr, "You typically want to pass one of each.\n\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "    --keep-groups=filename.txt   Provide a list of group names to keep. Groups not\n");
  fprintf(stderr, "                                 itself or with a child in this list will be merged\n");
  fprintf(stderr, "                                 with the first parent that should be kept.\n");
  fprintf(stderr, "    --output-json=filename.json  Write hierarchy with attributes to a json file.\n");
  fprintf(stderr, "    --output-txt=filename.txt    Dump all group names to a text file.\n");
  fprintf(stderr, "    --output-obj=filenamestem    Write geometry to an obj file, .obj and .mtl\n");
  fprintf(stderr, "                                 are added to filenamestem.\n");
  fprintf(stderr, "    --group-bounding-boxes       Include wireframe of boundingboxes of groups in output.\n");
  fprintf(stderr, "    --color-attribute=key        Specify which attributes that contain color, empty\n");
  fprintf(stderr, "                                 imply that material id of group is used.\n");
  fprintf(stderr, "    --tolerance=value            Tessellation tolerance, given in world frame.\n");
  fprintf(stderr, "    --cull-scale=value           Cull objects that are smaller than cull-scale times\n");
  fprintf(stderr, "                                 tolerance. Set to a negative value to disable culling.\n");
}


int main(int argc, char** argv)
{
  int rv = 0;
  bool should_tessellate = false;

  float tolerance = 0.1f;
  float cullScale = -10000.1f;

  unsigned chunkTinyVertexThreshold = 0;

  bool groupBoundingBoxes = false;
  std::string keep_groups;
  std::string output_json;
  std::string output_txt;
  std::string output_obj_stem;
  std::string color_attribute;
  

  std::vector<std::string> attributeSuffices = { ".txt",  ".att" };

  Store* store = new Store();


  std::string stem;
  for (int i = 1; i < argc; i++) {
    auto arg = std::string(argv[i]);

    if (arg.substr(0, 2) == "--") {

      if (arg == "--help") {
        printHelp(argv[0]);
        return 0;
      }
      else if (arg == "--group-bounding-boxes") {
        groupBoundingBoxes = true;
        continue;
      }

      auto e = arg.find('=');
      if (e != std::string::npos) {
        auto key = arg.substr(0, e);
        auto val = arg.substr(e + 1);
        if (key == "--keep-groups") {
          keep_groups = val;
          continue;
        }
        else  if (key == "--output-json") {
          output_json = val;
          continue;
        }
        else if (key == "--output-txt") {
          output_txt = val;
          continue;
        }
        else if (key == "--output-obj") {
          output_obj_stem = val;
          should_tessellate = true;
          continue;
        }
        else if (key == "--color-attribute") {
          color_attribute = val;
          continue;
        }
        else if (key == "--tolerance") {
          tolerance = std::max(1e-6f, std::stof(val));
          continue;
        }
        else if (key == "--cull-scale") {
          cullScale = std::stof(val); // set to negative to disable culling.
          continue;
        }
        else if (key == "--chunk-tiny") {
          chunkTinyVertexThreshold = std::stoul(val);
          should_tessellate = true;
          continue;
        }
      }

      fprintf(stderr, "Unrecognized argument '%s'", arg.c_str());
      printHelp(argv[0]);
      return -1;
    }

    auto arg_lc = arg;
    for (auto & c : arg_lc) c = std::tolower(c);

    // parse rvm file
    auto l = arg_lc.rfind(".rvm");
    if (l != std::string::npos) {
      if (processFile(arg, [store](const void * ptr, size_t size) { return parseRVM(store, ptr, size); }) != 0)
      {
        fprintf(stderr, "Failed to parse %s: %s\n", arg.c_str(), store->errorString());
        rv = -1;
        break;
      }
      fprintf(stderr, "Successfully parsed %s\n", arg.c_str());
      continue;
    }

    // parse attributes file
    l = arg_lc.rfind(".txt");
    if (l != std::string::npos) {
      auto rv = processFile(arg, [store](const void* ptr, size_t size) { return parseAtt(store, logger, ptr, size); });
      if (rv == 0) {
        fprintf(stderr, "Successfully parsed %s\n", arg.c_str());
      }
      else {
        fprintf(stderr, "Failed to parse %s\n", arg.c_str());
        rv = -1;
        break;
      }
      continue;
    }
  }

  if (rv == 0) {
    Colorizer colorizer(logger, color_attribute.empty() ? nullptr : color_attribute.c_str());
    store->apply(&colorizer);
  }

  if (rv == 0 && (should_tessellate || !output_json.empty())) {
    AddGroupBBox addGroupBBox;
    store->apply(&addGroupBBox);

    Tessellator tessellator(tolerance, tolerance*cullScale);
    store->apply(&tessellator);
  }


  if (rv == 0 && should_tessellate ) {
    AddGroupBBox addGroupBBox;
    store->apply(&addGroupBBox);

    Tessellator tessellator(tolerance, tolerance*cullScale);
    store->apply(&tessellator);
  }

  bool do_flatten = false;
  Flatten flatten(store);
  if (rv == 0 && !keep_groups.empty()) {
    if (processFile(keep_groups, [f = &flatten](const void * ptr, size_t size) {f->setKeep(ptr, size); return true; })) {
      fprintf(stderr, "Processed %s\n", keep_groups.c_str());
      do_flatten = true;

    }
    else {
      fprintf(stderr, "Failed to parse %s\n", keep_groups.c_str());
      rv = -1;
    }
  }

  if (rv == 0 && chunkTinyVertexThreshold) {
    ChunkTiny chunkTiny(flatten, chunkTinyVertexThreshold);
    store->apply(&chunkTiny);
    do_flatten = true;
  }

  if (do_flatten) {
    store->apply(&flatten);
    delete store;
    store = flatten.result();
  }


  if (rv == 0 && !output_json.empty()) {
    ExportJson exportJson;
    if (exportJson.open(output_json.c_str())) {
      store->apply(&exportJson);
    }
    else {
      fprintf(stderr, "Failed to export obj file.\n");
      rv = -1;
    }
  }


  if (rv == 0 && !output_txt.empty()) {
    FILE* out;
    if (fopen_s(&out, output_txt.c_str(), "w") == 0) {
      DumpNames dumpNames;
      dumpNames.setOutput(out);
      store->apply(&dumpNames);
      fclose(out);
    }
  }

  if (rv == 0 && !output_obj_stem.empty()) {
    assert(should_tessellate);
    ExportObj exportObj;
    exportObj.groupBoundingBoxes = groupBoundingBoxes;
    if (exportObj.open((output_obj_stem + ".obj").c_str(), (output_obj_stem + ".mtl").c_str())) {
      store->apply(&exportObj);
    }
    else {
      fprintf(stderr, "Failed to export obj file.\n");
      rv = -1;
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

  delete store;
 
  return rv;
}

