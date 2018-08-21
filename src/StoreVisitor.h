#pragma once
#include <vector>

class StoreVisitor
{
public:
  virtual void init(class Store& store) {}

  virtual bool done() { return true; }

  virtual void beginFile(const char* info, const char* note, const char* date, const char* user, const char* encoding) {}

  virtual void endFile() {}

  virtual void beginModel(const char* project, const char* name) {}

  virtual void endModel() {};

  virtual void beginGroup(struct Group* group) {}

  virtual void EndGroup() {}

  virtual void geometry(struct Geometry* geometry) {}

  virtual void composite(struct Composite* composite) {}

};
