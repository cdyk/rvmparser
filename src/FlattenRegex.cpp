#include <cassert>
#include <regex>

#include "Store.h"


namespace {
  
  struct Context
  {
    Store* store;
    Logger logger;
    std::regex re;
  };

  void extractKeepList(Context& ctx, const Node* group)
  {
    assert(group->kind == Node::Kind::Group);
    if (group->group.name && std::regex_match(group->group.name, ctx.re)) {
      ctx.logger(1, "Keep: %s", group->group.name);
    }
    else {
      ctx.logger(1, "Discard: %s", group->group.name);
    }

    for (const Node* child = group->children.first; child; child = child->next) {
      extractKeepList(ctx, child);
    }
  }

}

bool flattenRegex(Store* store, Logger logger, const char* regex)
{
  Context ctx{
    .store = store,
    .logger = logger,
  };

  try {
    ctx.re = std::regex(regex);
  }
  catch (std::regex_error& e) {
    ctx.logger(2, "Failed to compile regular expression '%s': %s", "sdf", e.what());
    return false;
  }

  for (const Node* root = store->getFirstRoot(); root; root = root->next) {
    for (const Node* model = root->children.first; model; model = model->next) {
      for (const Node* group = model->children.first; group; group = group->next) {
        extractKeepList(ctx, group);
      }
    }
  }


  return true;
}
