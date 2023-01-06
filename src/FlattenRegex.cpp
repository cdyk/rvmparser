#include <cassert>
#include <regex>

#include "Store.h"

// Flatten with regex
// ==================
//
// Simplifies the hierarchy by removing all nodes that doesn't have a name that
// matches a regex. The geometries and attributes of discarded node are moved to
// the neareast ancestor that is kept.

namespace {
  
  struct Context
  {
    Store* store;
    Logger logger;
    std::regex re;
  };

  // Grabs all children from a parent node and checks the children one-by-one.
  // 
  // If the child is to be kept, it is inserted as a child of the nearest kept
  // ancestor (which can be the same node as parent) and is recursed upon using
  // that child as neareast kept ancestor.
  //
  // If the node is to be discarded, all attributes and geometries are moved to
  // the neareast kept ancestor, and the function recurse on the children.
  //
  void handleChildren(Context& ctx, Node* nearestKeptAncestor, Node* parent)
  {
    // Grab children from parent
    ListHeader<Node> children = parent->children;
    parent->children.clear();

    // Process grabbed children one-by-one
    while (Node* child = children.popFront()) {

      // Child should be kept
      if (child->group.name && std::regex_match(child->group.name, ctx.re)) {

        // Set it as child of nearest kept ancestor node. That might be its original
        // parent, but that is OK since we removed all children from the parent
        // before the loop.
        nearestKeptAncestor->children.insert(child);

        // Recurse using child as nearest .
        handleChildren(ctx, child, child);
      }
      
      // node should be discarded, move attributes and geometries
      else {

        // Move attributes from node to be discarded to nearest kept ancestor node.
        ListHeader<Attribute> attributes = child->attributes;
        attributes.clear();
        while (Attribute* att = attributes.popFront()) {
          nearestKeptAncestor->attributes.insert(att);
        }

        // Move geometries from node to be discarded to nearest keeper node.
        ListHeader<Geometry> geometries = child->group.geometries;
        child->group.geometries.clear();
        while (Geometry* geo = geometries.popFront()) {
          nearestKeptAncestor->group.geometries.insert(geo);
        }

        handleChildren(ctx, nearestKeptAncestor, child);
      }
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

  // The three lowest levels, file, model and first group are always kept
  for (Node* root = store->getFirstRoot(); root; root = root->next) {
    for (Node* model = root->children.first; model; model = model->next) {
      for (Node* group = model->children.first; group; group = group->next) {
        handleChildren(ctx, group, group);
      }
    }
  }

  return true;
}
