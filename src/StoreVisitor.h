#pragma once
#include <vector>

class StoreVisitor
{
public:
  virtual void init(class Store& store) {}

  virtual bool done() { return true; }

  virtual void beginFile(struct Group* group) {}

  virtual void endFile() {}

  virtual void beginModel(struct Group* group) {}

  virtual void endModel() {};

  virtual void beginGroup(struct Group* group) {}

  virtual void doneGroupContents(struct Group* group) {}

  virtual void EndGroup() {}

  virtual void beginChildren(struct Group* container) {}

  virtual void endChildren() {}

  virtual void beginAttributes(struct Group* container) {}

  virtual void attribute(const char* key, const char* val) {}

  virtual void endAttributes(struct Group* container) {}

  virtual void beginGeometries(struct Group* container) {}

  virtual void geometry(struct Geometry* geometry) {}

  virtual void endGeometries() {}

};
