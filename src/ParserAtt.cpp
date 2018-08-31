#include <cstring>
#include <cstdio>

#include "Parser.h"

namespace {

  struct Context
  {
    Store* store;
    Logger logger;
    char* buf;
    size_t buf_size;
    unsigned line;
    unsigned spaces;
    unsigned tabs;
  };

  bool handleNew(Context* ctx, const char* id, size_t id_N)
  {
    if (ctx->buf_size <= id_N + 1) {
      ctx->logger(2, "@%d: Insufficient buffer size for string", ctx->line);
      return false;
    }
    std::memcpy(ctx->buf, id, id_N);
    ctx->buf[id_N] = '\0';
    ctx->logger(0, "@%d: new '%s'", ctx->line, ctx->buf);
    return true;
  }

  bool handleEnd(Context* ctx)
  {
    ctx->logger(0, "@%d: end", ctx->line);
    return true;
  }

  bool handleAttribute(Context* ctx, const char* key, size_t key_N, const char* value, size_t value_N)
  {
    if (ctx->buf_size <= key_N + value_N + 2) {
      ctx->logger(2, "@%d: Insufficient buffer size for string", ctx->line);
      return false;
    }

    auto * k = ctx->buf;
    std::memcpy(k, key, key_N);
    k[key_N] = '\0';

    auto * v = k + key_N + 1;
    std::memcpy(v, value, value_N);
    v[value_N] = '\0';

    ctx->logger(0, "@%d: att ('%s', '%s')", ctx->line, k, v);

    return true;
  }


  const char* getEndOfLine(const char* p, const char* end)
  {
    while (p < end && (*p != '\n' && *p != '\r'))  p++;
    return p;
  }

  const char* skipEndOfLine(const char* p, const char* end)
  {
    while (p < end && (*p == '\n' || *p == '\r'))  p++;
    return p;
  }

  const char* skipSpace(const char* p, const char* end)
  {
    while (p < end && (*p == ' ' || *p == '\t'))  p++;
    return p;
  }

  const char * reverseSkipSpace(const char* start, const char* p)
  {
    while (start < p && (p[-1] == ' ' || p[-1] == '\t')) p--;
    return p;
  }

  const char* parseIndentation(unsigned & spaces, unsigned & tabs, const char* p, const char* end)
  {
    spaces = 0;
    tabs = 0;
    for (; p < end; p++) {
      if (*p == ' ') spaces++;
      else if (*p == '\t') tabs++;
      else break;
    }
    return p;
  }

  const char* findAssign(const char* p, const char* end)
  {
    for (; p < end; p++) {
      if (*p == '\r' || *p == '\n') return p;
      if (p+1 < end && (p[0] == ':' && p[1] == '=')) return p; 
    }
    return p;
  }

  const char* findSep(const char* p, const char* end)
  {
    for (; p < end; p++) {
      if (*p == '\r' || *p == '\n') return p;
      if (p + 4 < end && (p[0] == '&' && p[1] == 'e' && p[2] == 'n' && p[3] == 'd' && p[4] == '&')) return p;
    }
    return p;
  }

  bool matchNew(const char* p, const char* end)
  {
    return (p + 3 < end) && p[0] == 'N' && p[1] == 'E' && p[2] == 'W' && (p[3] == ' ' || p[3] == '\t');
  }

  bool matchEnd(const char* p, const char* end)
  {
    return (p + 2 < end) && p[0] == 'E' && p[1] == 'N' && p[2] == 'D';
  }


}


bool parseAtt(class Store* store, Logger logger, const void * ptr, size_t size)
{
  char buf[1024];
  Context ctx = { store, logger, buf, sizeof(buf) };

  auto * p = (const char*)(ptr);
  auto * end = p + size;
  p = getEndOfLine(p, end);
  p = skipEndOfLine(p, end);

  for (ctx.line = 1; p < end; ctx.line++) {
    p = parseIndentation(ctx.spaces, ctx.tabs, p, end);
    if (matchNew(p, end)) {
      auto * a = skipSpace(p + 4, end);
      p = getEndOfLine(a, end);
      if (!handleNew(&ctx, a, reverseSkipSpace(a, p) - a)) return false;
    }
    else if (matchEnd(p, end)) {
      if (!handleEnd(&ctx)) return false;
      p = getEndOfLine(p, end);
    }
    else {
      while (true) {
        auto * keyStart = p;
        p = findAssign(p, end);
        if (p == end || p[0] != ':') {
          logger(2, "@%d: Failed to find ':=' token.\n", ctx.line);
          return false;
        }
        auto keyN = reverseSkipSpace(keyStart, p) - keyStart;
        p = skipSpace(p + 2, end);

        auto * valueStart = p;
        p = findSep(p, end);
        auto valueN = reverseSkipSpace(valueStart, p) - valueStart;
        if (2 <= valueN && valueStart[0] == '\'' && valueStart[valueN - 1] == '\'') {
          valueStart++;
          valueN -= 2;
        }
        if (!handleAttribute(&ctx, keyStart, keyN, valueStart, valueN)) return false;

        if (p + 5 < end && p[0] == '&') {
          p = skipSpace(p + 5, end);
        }
        else {
          break;
        }
      }
    }
    if ((p < end) && (*p != '\n' && *p != '\r')) {
      logger(2, "@%d: Line scanning did not terminate at end of line", ctx.line);
      return false;
    }
    p = skipEndOfLine(p, end);
  }

  // skip first line

  return true;
}
