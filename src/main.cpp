#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include <cassert>

#include "RVMParser.h"
#include "FindConnections.h"
#include "Tessellator.h"
#include "ExportObj.h"
#include "Store.h"
#include "AddStats.h"

int main(int argc, char** argv)
{


  Store* store = new Store();

  for (int i = 1; i < argc; i++) {
    auto outfile = std::string(argv[i]) + ".obj";

    fprintf(stderr, "Reading '%s'\n", argv[i]);

    HANDLE h = CreateFileA(argv[i], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(h != INVALID_HANDLE_VALUE);

    DWORD hiSize;
    DWORD loSize = GetFileSize(h, &hiSize);
    size_t fileSize = (size_t(hiSize) << 32u) + loSize;

    HANDLE m = CreateFileMappingA(h, 0, PAGE_READONLY, 0, 0, NULL);
    assert(m != INVALID_HANDLE_VALUE);

    const void * ptr = MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
    assert(ptr != nullptr);

    parseRVM(store, ptr, fileSize);

    UnmapViewOfFile(ptr);
    CloseHandle(m);
    CloseHandle(h);
  }

  AddStats addStats;
  store->apply(&addStats);

  FindConnections findConnections;
  store->apply(&findConnections);

  Tessellator tessellator;
  store->apply(&tessellator);

  ExportObj visitor("output.obj");
  store->apply(&visitor);

  delete store;

  //auto a = getc(stdin);
 
  return 0;
}

