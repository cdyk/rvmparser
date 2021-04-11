#include <cstdio>
#include <cstring>
#include <cassert>

#include "Store.h"


bool exportGLTF(Store* store, Logger logger, const char* path)
{

#ifdef _WIN32
  FILE* out = nullptr;
  auto err = fopen_s(&out, path, "wb");
  if (err != 0) {
    char buf[1024];
    if (strerror_s(buf, sizeof(buf), err) != 0) {
      buf[0] = '\0';
    }
    logger(2, "Failed to open %s for writing: %s", path, buf);
    return false;
  }
  assert(out);
#else
  FILE* out = fopen(path, "w");
  if (out == nullptr) {
    logger(2, "Failed to open %s for writing.", path);
    return false;
  }
#endif

  // write header
  uint32_t header[3] = {
    0x46546C67,       // magic
    2,                // version
    12 + 8 + 8        // total length
  };
  if (fwrite(header, sizeof(header), 1, out) != 1) {
    logger(2, "%s: Error writing header", path);
    fclose(out);
    return false;
  }

  // write JSON chunk
  uint32_t json_chunk_header[2] = {
    0,                // length of chunk data
    0x4E4F534A        // chunk type (JSON)
  };
  if (fwrite(json_chunk_header, sizeof(json_chunk_header), 1, out) != 1) {
    logger(2, "%s: Error writing JSON chunk header", path);
    fclose(out);
    return false;
  }

  // write JSON chunk
  uint32_t bin_chunk_header[2] = {
    0,                // length of chunk data
    0x004E4942        // chunk type (BIN)
  };
  if (fwrite(bin_chunk_header, sizeof(bin_chunk_header), 1, out) != 1) {
    logger(2, "%s: Error writing BIN chunk header", path);
    fclose(out);
    return false;
  }

  fclose(out);

  return true;
}