#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#else

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#endif

#include <cstdio>
#include <cassert>
#include <cstring>
#include <string>
#include <cctype>
#include <chrono>
#include <algorithm>

#include "Parser.h"
#include "Tessellator.h"
#include "ExportObj.h"
#include "Store.h"
#include "Flatten.h"
#include "AddStats.h"
#include "DumpNames.h"
#include "ChunkTiny.h"
#include "AddGroupBBox.h"
#include "Colorizer.h"


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

template<typename F>
bool
processFile(const std::string& path, F f)
{
  bool rv = false;
#ifdef _WIN32

  HANDLE h = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE) {
    logger(2, "CreateFileA returned INVALID_HANDLE_VALUE");
    rv = false;
  }
  else {
    DWORD hiSize;
    DWORD loSize = GetFileSize(h, &hiSize);
    size_t fileSize = (size_t(hiSize) << 32u) + loSize;

    HANDLE m = CreateFileMappingA(h, 0, PAGE_READONLY, 0, 0, NULL);
    if (m == INVALID_HANDLE_VALUE) {
      logger(2, "CreateFileMappingA returned INVALID_HANDLE_VALUE");
      rv = false;
    }
    else {
      const void * ptr = MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
      if (ptr == nullptr) {
        logger(2, "MapViewOfFile returned INVALID_HANDLE_VALUE");
        rv = false;
      }
      else {
        rv = f(ptr, fileSize);
        UnmapViewOfFile(ptr);
      }
      CloseHandle(m);
    }
    CloseHandle(h);
  }

#else

  int fd = open(path.c_str(), O_RDONLY);
  if(fd == -1) {
    logger(2, "%s: open failed: %s", path.c_str(), strerror(errno));
  }
  else {
    struct stat stat{};
    if(fstat(fd, &stat) != 0) {
      logger(2, "%s: fstat failed: %s", path.c_str(), strerror(errno));
    }
    else {
      void * ptr = mmap(nullptr, stat.st_size, PROT_READ, MAP_PRIVATE|MAP_POPULATE, fd, 0);
      if(ptr == MAP_FAILED) {
        logger(2, "%s: mmap failed: %s", path.c_str(), strerror(errno));
      }
      else {
        if(madvise(ptr, stat.st_size, MADV_SEQUENTIAL) != 0) {
          logger(1, "%s: madvise(MADV_SEQUENTIAL) failed: %s", path.c_str(), strerror(errno));
        }
        rv = f(ptr, stat.st_size);
        if(munmap(ptr, stat.st_size) != 0) {
          logger(2, "%s: munmap failed: %s", path.c_str(), strerror(errno));
          rv = false;
        }
      }
    }
  }

#endif
  return rv;
}

namespace {

  void printHelp(const char* argv0)
  {
    fprintf(stderr, R"help(
Usage: %s [options] files

Files with .rvm-suffix will be interpreted as a geometry files, and files with .txt or .att suffix
will be interpreted as attribute files. A rvm file typically has a matching attribute file.

Options:
  --keep-groups=filename.txt          Provide a list of group names to keep. Groups not itself or
                                      with a child in this list will be merged with the first
                                      parent that should be kept.
  --discard-groups=filename.txt       Provide a list of group names to discard, one name per line.
                                      Groups with its name in this list will be discarded along
                                      with its children. Default is no groups are discarded.
  --output-json=<filename.json>       Write hierarchy with attributes to a json file.
  --output-txt=<filename.txt>         Dump all group names to a text file.
  --output-obj=<filenamestem>         Write geometry to an obj file. The suffices .obj and .mtl are
                                      added to the filenamestem.
  --output-gltf=<filename.glb>        Write geometry into a GLTF file.
  --output-gltf-attributes=<bool>     Include rvm attributes in the extra member of nodes. Default
                                      value is true.
  --output-gltf-center=<bool>         Position the model at the bounding box center and store the
                                      position of this origin in the asset's extra field. This is
                                      useful if the model is so far away from the origin that float
                                      precision becomes an issue. Defaults value is false.
  --output-gltf-rotate-z-to-y=<bool>  Add an extra node below the root that adds a clockwise
                                      rotation of 90 degrees about the X axis such that the +Z axis
                                      will map to the +Y axis, which is the up-direction of GLTF-
                                      files. Default value is true.
  --group-bounding-boxes              Include wireframe of boundingboxes of groups in output.
  --color-attribute=key               Specify which attributes that contain color, empty key
                                      implies that material id of group is used.
  --tolerance=value                   Tessellation tolerance, given in world frame. Default value
                                      is 0.1.
  --cull-scale=value                  Cull objects smaller than cull-scale times tolerance. Set to
                                      a negative value to disable culling. Disabled by default.

Post bug reports or questions at https://github.com/cdyk/rvmparser
)help", argv0);
  }


  bool parseBool(Logger logger, const std::string& arg, const std::string& value)
  {
    std::string lower;
    for (const char c : value) {
      lower.push_back(std::tolower(c));
    }
    if (lower == "true" || lower == "1" || lower == "yes") {
      return true;
    }
    else if (lower == "false" || lower == "0" || lower == "no") {
      return false;
    }
    else {
      logger(2, "Failed to parse bool option '%s'", arg.c_str());
      exit(EXIT_FAILURE);
    }
  }

}


int main(int argc, char** argv)
{
  int rv = 0;
  bool should_tessellate = false;
  bool should_colorize = false;

  float tolerance = 0.1f;
  float cullScale = -10000.1f;

  unsigned chunkTinyVertexThreshold = 0;

  bool groupBoundingBoxes = false;
  std::string discard_groups;
  std::string keep_groups;
  std::string output_json;
  std::string output_txt;
  std::string output_gltf;
  bool output_gltf_rotate_z_to_y = true;
  bool output_gltf_center = false;
  bool output_gltf_attributes = true;

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
        else if (key == "--discard-groups") {
          discard_groups = val;
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
          should_colorize = true;
          continue;
        }
        else if (key == "--output-gltf") {
          output_gltf = val;
          should_tessellate = true;
          should_colorize = true;
          continue;
        }
        else if (key == "--output-gltf-rotate-z-to-y") {
          output_gltf_rotate_z_to_y = parseBool(logger, arg, val);
          continue;
        }
        else if (key == "--output-gltf-center") {
          output_gltf_center = parseBool(logger, arg, val);
          continue;
        }
        else if (key == "--output-gltf-attributes") {
          output_gltf_attributes = parseBool(logger, arg, val);
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
      if (processFile(arg, [store](const void * ptr, size_t size) { return parseRVM(store, ptr, size); }))
      {
        fprintf(stderr, "Successfully parsed %s\n", arg.c_str());
      }
      else {
        fprintf(stderr, "Failed to parse %s: %s\n", arg.c_str(), store->errorString());
        rv = -1;
        break;
      }
      continue;
    }

    // parse attributes file
    l = arg_lc.rfind(".txt");
    if (l != std::string::npos) {
      if (processFile(arg, [store](const void* ptr, size_t size) { return parseAtt(store, logger, ptr, size); })) {
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

  if ((rv == 0) && should_colorize) {
    Colorizer colorizer(logger, color_attribute.empty() ? nullptr : color_attribute.c_str());
    store->apply(&colorizer);
  }

  if (rv == 0 && !discard_groups.empty()) {
    if (processFile(discard_groups, [store](const void * ptr, size_t size) { return discardGroups(store, logger, ptr, size); })) {
      logger(0, "Processed %s", discard_groups.c_str());
    }
    else {
      logger(2, "Failed to parse %s", discard_groups.c_str());
      rv = -1;
    }
  } 


  if (rv == 0) {
    connect(store, logger);
    align(store, logger);
  }

  if (rv == 0 && (should_tessellate || !output_json.empty())) {
    AddGroupBBox addGroupBBox;
    store->apply(&addGroupBBox);
  }

  if (rv == 0 && should_tessellate ) {
    float cullLeafThreshold = -1.f;
    float cullGeometryThreshold = -1.f;
    unsigned maxSamples = 100;

    auto time0 = std::chrono::high_resolution_clock::now();
    Tessellator tessellator(logger, tolerance, cullLeafThreshold, cullGeometryThreshold, maxSamples);
    store->apply(&tessellator);
    auto time1 = std::chrono::high_resolution_clock::now();
    auto e0 = std::chrono::duration_cast<std::chrono::milliseconds>((time1 - time0)).count();
    logger(0, "Tessellated %u items of %u into %llu vertices and %llu triangles (tol=%f, %lluk, %lldms)",
           tessellator.tessellated,
           tessellator.processed,
           tessellator.vertices,
           tessellator.triangles,
           tolerance,
           (4*3*tessellator.vertices + 4*3*tessellator.triangles)/1024,
           e0);
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
    auto * storeNew = flatten.run();
    delete store;
    store = storeNew;
  }


  if (rv == 0 && !output_json.empty()) {
    auto time0 = std::chrono::high_resolution_clock::now();
    if (exportJson(store, logger, output_json.c_str())) {
      auto time1 = std::chrono::high_resolution_clock::now();
      auto e = std::chrono::duration_cast<std::chrono::milliseconds>((time1 - time0)).count();
      logger(0, "Exported json into %s (%lldms)", output_json.c_str(), e);
    }
    else {
      logger(2, "Failed to export obj file.\n");
      rv = -1;
    }
  }

  if (rv == 0 && !output_txt.empty()) {

#ifdef _WIN32
    FILE* out = nullptr;
    if (fopen_s(&out, output_txt.c_str(), "w") == 0) {
#else
    FILE* out = fopen(output_txt.c_str(), "w");
    if(out != nullptr) {
#endif

      DumpNames dumpNames;
      dumpNames.setOutput(out);
      store->apply(&dumpNames);
      fclose(out);
    }
    else {
      logger(2, "Failed to open %s for writing", output_txt.c_str());
      rv = -1;
    }
  }

  if (rv == 0 && !output_obj_stem.empty()) {
    assert(should_tessellate);
 
    auto time0 = std::chrono::high_resolution_clock::now();
    ExportObj exportObj;
    exportObj.groupBoundingBoxes = groupBoundingBoxes;
    if (exportObj.open((output_obj_stem + ".obj").c_str(), (output_obj_stem + ".mtl").c_str())) {
      store->apply(&exportObj);

      auto time1 = std::chrono::high_resolution_clock::now();
      auto e = std::chrono::duration_cast<std::chrono::milliseconds>((time1 - time0)).count();
      logger(0, "Exported obj into %s(.obj|.mtl) (%lldms)", output_obj_stem.c_str(), e);
    }
    else {
      logger(2, "Failed to export obj file.\n");
      rv = -1;
    }
  }

  if (rv == 0 && !output_gltf.empty()) {
    assert(should_tessellate);
    auto time0 = std::chrono::high_resolution_clock::now();
    if (exportGLTF(store, logger,
                   output_gltf.c_str(),
                   output_gltf_rotate_z_to_y,
                   output_gltf_center,
                   output_gltf_attributes))
    {
      long long e = std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::high_resolution_clock::now() - time0)).count();
      logger(0, "Exported gltf into %s (%lldms)", output_gltf.c_str(), e);
    }
    else {
      logger(2, "Failed to export gltf into %s", output_gltf.c_str());
      rv = -1;
    }
  }

  AddStats addStats;
  store->apply(&addStats);
  auto * stats = store->stats;
  if (stats) {
    logger(0, "Stats:");
    logger(0, "    Groups                 %d", stats->group_n);
    logger(0, "    Geometries             %d (grp avg=%.1f)", stats->geometry_n, stats->geometry_n / float(stats->group_n));
    logger(0, "        Pyramids           %d", stats->pyramid_n);
    logger(0, "        Boxes              %d", stats->box_n);
    logger(0, "        Rectangular tori   %d", stats->rectangular_torus_n);
    logger(0, "        Circular tori      %d", stats->circular_torus_n);
    logger(0, "        Elliptical dish    %d", stats->elliptical_dish_n);
    logger(0, "        Spherical dish     %d", stats->spherical_dish_n);
    logger(0, "        Snouts             %d", stats->snout_n);
    logger(0, "        Cylinders          %d", stats->cylinder_n);
    logger(0, "        Spheres            %d", stats->sphere_n);
    logger(0, "        Facet groups       %d", stats->facetgroup_n);
    logger(0, "            triangles      %d", stats->facetgroup_triangles_n);
    logger(0, "            quads          %d", stats->facetgroup_quads_n);
    logger(0, "            polygons       %d (fgrp avg=%.1f)", stats->facetgroup_polygon_n, (stats->facetgroup_polygon_n / float(stats->facetgroup_n)));
    logger(0, "                contours   %d (poly avg=%.1f)", stats->facetgroup_polygon_n_contours_n, (stats->facetgroup_polygon_n_contours_n / float(stats->facetgroup_polygon_n)));
    logger(0, "                vertices   %d (cont avg=%.1f)", stats->facetgroup_polygon_n_vertices_n, (stats->facetgroup_polygon_n_vertices_n / float(stats->facetgroup_polygon_n_contours_n)));
    logger(0, "        Lines              %d", stats->line_n);
  }

  delete store;
 
  return rv;
}

