#pragma once
#include <vector>

class StoreVisitor
{
public:
  virtual void init(class Store& /*store*/) {}

  virtual bool done() { return true; }

  virtual void beginFile(struct Node* /*group*/) {}

  virtual void endFile() {}

  virtual void beginModel(struct Node* /*group*/) {}

  virtual void endModel() {};

  virtual void beginGroup(struct Node* /*group*/) {}

  virtual void doneGroupContents(struct Node* /*group*/) {}

  virtual void EndGroup() {}

  virtual void beginChildren(struct Node* /*container*/) {}

  virtual void endChildren() {}

  virtual void beginAttributes(struct Node* /*container*/) {}

  virtual void attribute(const char* /*key*/, const char* /*val*/) {}

  virtual void endAttributes(struct Node* /*container*/) {}

  virtual void beginGeometries(struct Node* /*container*/) {}

  virtual void geometry(struct Geometry* /*geometry*/) {}

  virtual void endGeometries() {}

};
