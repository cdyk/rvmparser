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
#include "AddStats.h"


bool parseRVM(Store* store, const std::string& path)
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
        parseRVM(store, ptr, fileSize);
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


  Store* store = new Store();


  std::string stem;
  for (int i = 1; i < argc; i++) {
    auto arg = std::string(argv[i]);
    auto arg_lc = arg;
    for (auto & c : arg_lc) c = std::tolower(c);
    auto l = arg_lc.rfind(".rvm");
    if (l == std::string::npos) {
      fprintf(stderr, "Failed to find suffix '.rvm' in path %s\n", arg.c_str());
      rv = -1;
      break;
    }
    stem = arg.substr(0, l);

    if (!parseRVM(store, arg)) {
      fprintf(stderr, "Failed to parse %s\n", arg.c_str());
      rv = -1;
      break;
    }
    fprintf(stderr, "Successfully parsed %s\n", arg.c_str());

  }

  if (!stem.empty()) {
    AddStats addStats;
    store->apply(&addStats);

    //FindConnections findConnections;
    //store->apply(&findConnections);

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
  }

  delete store;

  //auto a = getc(stdin);
 
  return rv;
}

