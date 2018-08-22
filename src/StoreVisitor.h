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

  virtual void EndGroup() {}

  virtual void beginGeometries(struct Group* container) {}

  virtual void geometry(struct Geometry* geometry) {}

  virtual void endGeometries() {}

  virtual void composite(struct Composite* composite) {}

};
