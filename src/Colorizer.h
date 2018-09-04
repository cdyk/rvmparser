#pragma once

#include "Common.h"
#include "StoreVisitor.h"

class Colorizer : public StoreVisitor
{
public:

  Colorizer(Logger logger, const char* colorAttribute = nullptr);

  void init(Store& store) override;

  void beginGroup(Group* group) override;

  void EndGroup() override;

  void geometry(Geometry* geometry) override;

  void attribute(const char* key, const char* val) override;

private:
  struct StackItem
  {
    const char* colorName;
    uint32_t color;
    bool override;
  };

  Arena arena;
  Map colorNameByMaterialId;
  Map colorByName;
  Map naggedMaterialId;
  Map naggedName;
  Logger logger;
  StackItem* stack = nullptr;
  uint32_t stack_p = 0;
  const char* defaultName = nullptr;
  const char* colorAttribute = nullptr;
};