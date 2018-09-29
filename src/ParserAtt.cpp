#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include "Parser.h"
#include "Store.h"

namespace {

  struct StackItem
  {
    const char* id;
    Group* group;
  };

  struct Context
  {
    Store* store;
    Logger logger;
    const char* headerInfo = nullptr; //Header Information
    char* buf;
    size_t buf_size;
    unsigned line;
    unsigned spaces;
    unsigned tabs;

    StackItem* stack = nullptr;
    unsigned stack_p = 0;
    unsigned stack_c = 0;

    bool create;
  };

  bool handleNew(Context* ctx, const char* id_a, const char* id_b)
  {
    if (ctx->stack_c <= ctx->stack_p + 1) {
      ctx->stack_c = 2 * ctx->stack_c;
      ctx->stack = (StackItem*)xrealloc(ctx->stack, sizeof(StackItem) * ctx->stack_c);
    }

    auto * id = ctx->store->strings.intern(id_a, id_b);

    Group * group = nullptr;
    if (ctx->stack_p == 0) {

      if (id != ctx->headerInfo) {
        group = ctx->store->findRootGroup(id);
        if (ctx->create && group == nullptr) {
          auto * model = ctx->store->getDefaultModel();
          group = ctx->store->newGroup(model, Group::Kind::Group);
          group->group.name = id;
          //ctx->logger(1, "@%d: Failed to find root group '%s' id=%p", ctx->line, id, id);
        }
      }
    }
    else {

      auto * parent = ctx->stack[ctx->stack_p - 1].group;
      if (parent) {
        for (auto * child = parent->groups.first; child; child = child->next) {
          if (child->group.name == id) {
            group = child;
            break;
          }
        }
      }
      if (ctx->create && group == nullptr) {
        group = ctx->store->newGroup(parent, Group::Kind::Group);
        group->group.name = id;
        //ctx->logger(1, "@%d: Failed to find child group '%s' id=%p", ctx->line, id, id);
      }
    }

    //ctx->logger(0, "@%d: new '%s'", ctx->line, id);
    assert(id);
    ctx->stack[ctx->stack_p++] = { id, group };
    return true;
  }

  bool handleEnd(Context* ctx)
  {
    if (ctx->stack_p == 0) {
      ctx->logger(2, "@%d: More END-tags and than NEW-tags.", ctx->line);
      return false;
    }
    //ctx->logger(0, "@%d: end", ctx->line);
    ctx->stack_p--;
    return true;
  }

  bool handleAttribute(Context* ctx, const char* key_a, const char* key_b, const char* value_a, const char* value_b)
  {
    assert(ctx->stack_p);
    auto * grp = ctx->stack[ctx->stack_p - 1].group;
    if (grp == nullptr) return true; // Inside skipped group like headerinfo

    auto * key = ctx->store->strings.intern(key_a, key_b);
    auto * att = ctx->store->getAttribute(grp, key);
    if (att == nullptr) {
      att = ctx->store->newAttribute(grp, key);
    }
    att->val = ctx->store->strings.intern(value_a, value_b);

    //ctx->logger(0, "@%d: att ('%s', '%s')", ctx->line, key, value);
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


bool parseAtt(class Store* store, Logger logger, const void * ptr, size_t size, bool create)
{
  char buf[1024];
  Context ctx = { store, logger, store->strings.intern("Header Information"), buf, sizeof(buf) };

  ctx.stack_c = 1024;
  ctx.stack = (StackItem*)xmalloc(sizeof(StackItem) * ctx.stack_c);
  ctx.create = create;

  auto * p = (const char*)(ptr);
  auto * end = p + size;
  p = getEndOfLine(p, end);
  p = skipEndOfLine(p, end);

  for (ctx.line = 1; p < end; ctx.line++) {
    p = parseIndentation(ctx.spaces, ctx.tabs, p, end);
    if (matchNew(p, end)) {
      auto * a = skipSpace(p + 4, end);
      p = getEndOfLine(a, end);
      if (!handleNew(&ctx, a, reverseSkipSpace(a, p))) goto error;
    }
    else if (matchEnd(p, end)) {
      if (!handleEnd(&ctx)) goto error;
      p = getEndOfLine(p, end);
    }
    else {
      while (true) {
        auto * key_a = p;
        p = findAssign(p, end);
        if (p == end || p[0] != ':') {
          logger(2, "@%d: Failed to find ':=' token.\n", ctx.line);
          goto error;
        }
        auto * key_b = reverseSkipSpace(key_a, p);
        p = skipSpace(p + 2, end);

        auto * value_a = p;
        p = findSep(p, end);
        auto * value_b = reverseSkipSpace(value_a, p);
        if (2 <= (value_b-value_a) && value_a[0] == '\'' && value_b[-1] == '\'') {
          value_a++;
          value_b--;
        }
        if (!handleAttribute(&ctx, key_a, key_b, value_a, value_b)) goto error;

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
      goto error;
    }
    p = skipEndOfLine(p, end);
  }

  if (ctx.stack_p != 0) {
    logger(2, "@%d: More NEW-tags and than END-tags.", ctx.line);
    return false;
  }


  free(ctx.stack);
  store->updateCounts();
  return true;

error:
  free(ctx.stack);
  store->updateCounts();
  return false;
}
