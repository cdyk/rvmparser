#pragma once

#include "Common.h"

bool parseAtt(Store* store, Logger logger, const void * ptr, size_t size, bool create=false);

bool parseRVM(Store* store, Logger logger, const char* path, const void * ptr, size_t size);
