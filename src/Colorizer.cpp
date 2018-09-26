#include <cassert>
#include "Store.h"
#include "Colorizer.h"


Colorizer::Colorizer(Logger logger, const char* colorAttribute) :
  logger(logger),
  colorAttribute(colorAttribute)
{
}

void Colorizer::init(Store& store)
{
  colorNameByMaterialId.insert(1, uint64_t(store.strings.intern("Black")));
  colorNameByMaterialId.insert(2, uint64_t(store.strings.intern("Red")));
  colorNameByMaterialId.insert(3, uint64_t(store.strings.intern("Orange")));
  colorNameByMaterialId.insert(4, uint64_t(store.strings.intern("Yellow")));
  colorNameByMaterialId.insert(5, uint64_t(store.strings.intern("Green")));
  colorNameByMaterialId.insert(6, uint64_t(store.strings.intern("Cyan")));
  colorNameByMaterialId.insert(7, uint64_t(store.strings.intern("Blue")));
  colorNameByMaterialId.insert(8, uint64_t(store.strings.intern("Magenta")));
  colorNameByMaterialId.insert(9, uint64_t(store.strings.intern("Brown")));
  colorNameByMaterialId.insert(10, uint64_t(store.strings.intern("White")));
  colorNameByMaterialId.insert(11, uint64_t(store.strings.intern("Salmon")));
  colorNameByMaterialId.insert(12, uint64_t(store.strings.intern("LightGrey")));
  colorNameByMaterialId.insert(13, uint64_t(store.strings.intern("Grey")));
  colorNameByMaterialId.insert(14, uint64_t(store.strings.intern("Plum")));
  colorNameByMaterialId.insert(15, uint64_t(store.strings.intern("WhiteSmoke")));
  colorNameByMaterialId.insert(16, uint64_t(store.strings.intern("Maroon")));
  colorNameByMaterialId.insert(17, uint64_t(store.strings.intern("SpringGreen")));
  colorNameByMaterialId.insert(18, uint64_t(store.strings.intern("Wheat")));
  colorNameByMaterialId.insert(19, uint64_t(store.strings.intern("Gold")));
  colorNameByMaterialId.insert(20, uint64_t(store.strings.intern("RoyalBlue")));
  colorNameByMaterialId.insert(21, uint64_t(store.strings.intern("LightGold")));
  colorNameByMaterialId.insert(22, uint64_t(store.strings.intern("DeepPink")));
  colorNameByMaterialId.insert(23, uint64_t(store.strings.intern("ForestGreen")));
  colorNameByMaterialId.insert(24, uint64_t(store.strings.intern("BrightOrange")));
  colorNameByMaterialId.insert(25, uint64_t(store.strings.intern("Ivory")));
  colorNameByMaterialId.insert(26, uint64_t(store.strings.intern("Chocolate")));
  colorNameByMaterialId.insert(27, uint64_t(store.strings.intern("SteelBlue")));
  colorNameByMaterialId.insert(28, uint64_t(store.strings.intern("White")));
  colorNameByMaterialId.insert(29, uint64_t(store.strings.intern("Midnight")));
  colorNameByMaterialId.insert(30, uint64_t(store.strings.intern("NavyBlue")));
  colorNameByMaterialId.insert(31, uint64_t(store.strings.intern("Pink")));
  colorNameByMaterialId.insert(32, uint64_t(store.strings.intern("CoralRed")));
  colorNameByMaterialId.insert(33, uint64_t(store.strings.intern("Black")));
  colorNameByMaterialId.insert(34, uint64_t(store.strings.intern("Red")));
  colorNameByMaterialId.insert(35, uint64_t(store.strings.intern("Orange")));
  colorNameByMaterialId.insert(36, uint64_t(store.strings.intern("Yellow")));
  colorNameByMaterialId.insert(37, uint64_t(store.strings.intern("Green")));
  colorNameByMaterialId.insert(38, uint64_t(store.strings.intern("Cyan")));
  colorNameByMaterialId.insert(39, uint64_t(store.strings.intern("Blue")));
  colorNameByMaterialId.insert(40, uint64_t(store.strings.intern("Magenta")));
  colorNameByMaterialId.insert(41, uint64_t(store.strings.intern("Brown")));
  colorNameByMaterialId.insert(42, uint64_t(store.strings.intern("White")));
  colorNameByMaterialId.insert(43, uint64_t(store.strings.intern("Salmon")));
  colorNameByMaterialId.insert(44, uint64_t(store.strings.intern("LightGrey")));
  colorNameByMaterialId.insert(45, uint64_t(store.strings.intern("Grey")));
  colorNameByMaterialId.insert(46, uint64_t(store.strings.intern("Plum")));
  colorNameByMaterialId.insert(47, uint64_t(store.strings.intern("WhiteSmoke")));
  colorNameByMaterialId.insert(48, uint64_t(store.strings.intern("Maroon")));
  colorNameByMaterialId.insert(49, uint64_t(store.strings.intern("SpringGreen")));
  colorNameByMaterialId.insert(50, uint64_t(store.strings.intern("Wheat")));
  colorNameByMaterialId.insert(51, uint64_t(store.strings.intern("Gold")));
  colorNameByMaterialId.insert(52, uint64_t(store.strings.intern("RoyalBlue")));
  colorNameByMaterialId.insert(53, uint64_t(store.strings.intern("LightGold")));
  colorNameByMaterialId.insert(54, uint64_t(store.strings.intern("DeepPink")));
  colorNameByMaterialId.insert(55, uint64_t(store.strings.intern("ForestGreen")));
  colorNameByMaterialId.insert(56, uint64_t(store.strings.intern("BrightOrange")));
  colorNameByMaterialId.insert(57, uint64_t(store.strings.intern("Ivory")));
  colorNameByMaterialId.insert(58, uint64_t(store.strings.intern("Chocolate")));
  colorNameByMaterialId.insert(59, uint64_t(store.strings.intern("SteelBlue")));
  colorNameByMaterialId.insert(60, uint64_t(store.strings.intern("White")));
  colorNameByMaterialId.insert(61, uint64_t(store.strings.intern("Midnight")));
  colorNameByMaterialId.insert(62, uint64_t(store.strings.intern("NavyBlue")));
  colorNameByMaterialId.insert(63, uint64_t(store.strings.intern("Pink")));
  colorNameByMaterialId.insert(64, uint64_t(store.strings.intern("CoralRed")));
  colorNameByMaterialId.insert(206, uint64_t(store.strings.intern("Black")));
  colorNameByMaterialId.insert(207, uint64_t(store.strings.intern("White")));
  colorNameByMaterialId.insert(208, uint64_t(store.strings.intern("WhiteSmoke")));
  colorNameByMaterialId.insert(209, uint64_t(store.strings.intern("Ivory")));
  colorNameByMaterialId.insert(210, uint64_t(store.strings.intern("Grey")));
  colorNameByMaterialId.insert(211, uint64_t(store.strings.intern("LightGrey")));
  colorNameByMaterialId.insert(212, uint64_t(store.strings.intern("DarkGrey")));
  colorNameByMaterialId.insert(213, uint64_t(store.strings.intern("DarkSlate")));
  colorNameByMaterialId.insert(214, uint64_t(store.strings.intern("Red")));
  colorNameByMaterialId.insert(215, uint64_t(store.strings.intern("BrightRed")));
  colorNameByMaterialId.insert(216, uint64_t(store.strings.intern("CoralRed")));
  colorNameByMaterialId.insert(217, uint64_t(store.strings.intern("Tomato")));
  colorNameByMaterialId.insert(218, uint64_t(store.strings.intern("Plum")));
  colorNameByMaterialId.insert(219, uint64_t(store.strings.intern("DeepPink")));
  colorNameByMaterialId.insert(220, uint64_t(store.strings.intern("Pink")));
  colorNameByMaterialId.insert(221, uint64_t(store.strings.intern("Salmon")));
  colorNameByMaterialId.insert(222, uint64_t(store.strings.intern("Orange")));
  colorNameByMaterialId.insert(223, uint64_t(store.strings.intern("BrightOrange")));
  colorNameByMaterialId.insert(224, uint64_t(store.strings.intern("OrangeRed")));
  colorNameByMaterialId.insert(225, uint64_t(store.strings.intern("Maroon")));
  colorNameByMaterialId.insert(226, uint64_t(store.strings.intern("Yellow")));
  colorNameByMaterialId.insert(227, uint64_t(store.strings.intern("Gold")));
  colorNameByMaterialId.insert(228, uint64_t(store.strings.intern("LightYellow")));
  colorNameByMaterialId.insert(229, uint64_t(store.strings.intern("LightGold")));
  colorNameByMaterialId.insert(230, uint64_t(store.strings.intern("YellowGreen")));
  colorNameByMaterialId.insert(231, uint64_t(store.strings.intern("SpringGreen")));
  colorNameByMaterialId.insert(232, uint64_t(store.strings.intern("Green")));
  colorNameByMaterialId.insert(233, uint64_t(store.strings.intern("ForestGreen")));
  colorNameByMaterialId.insert(234, uint64_t(store.strings.intern("DarkGreen")));
  colorNameByMaterialId.insert(235, uint64_t(store.strings.intern("Cyan")));
  colorNameByMaterialId.insert(236, uint64_t(store.strings.intern("Turquoise")));
  colorNameByMaterialId.insert(237, uint64_t(store.strings.intern("Aquamarine")));
  colorNameByMaterialId.insert(238, uint64_t(store.strings.intern("Blue")));
  colorNameByMaterialId.insert(239, uint64_t(store.strings.intern("RoyalBlue")));
  colorNameByMaterialId.insert(240, uint64_t(store.strings.intern("NavyBlue")));
  colorNameByMaterialId.insert(241, uint64_t(store.strings.intern("PowderBlue")));
  colorNameByMaterialId.insert(242, uint64_t(store.strings.intern("Midnight")));
  colorNameByMaterialId.insert(243, uint64_t(store.strings.intern("SteelBlue")));
  colorNameByMaterialId.insert(244, uint64_t(store.strings.intern("Indigo")));
  colorNameByMaterialId.insert(245, uint64_t(store.strings.intern("Mauve")));
  colorNameByMaterialId.insert(246, uint64_t(store.strings.intern("Violet")));
  colorNameByMaterialId.insert(247, uint64_t(store.strings.intern("Magenta")));
  colorNameByMaterialId.insert(248, uint64_t(store.strings.intern("Beige")));
  colorNameByMaterialId.insert(249, uint64_t(store.strings.intern("Wheat")));
  colorNameByMaterialId.insert(250, uint64_t(store.strings.intern("Tan")));
  colorNameByMaterialId.insert(251, uint64_t(store.strings.intern("SandyBrown")));
  colorNameByMaterialId.insert(252, uint64_t(store.strings.intern("Brown")));
  colorNameByMaterialId.insert(253, uint64_t(store.strings.intern("Khaki")));
  colorNameByMaterialId.insert(254, uint64_t(store.strings.intern("Chocolate")));
  colorNameByMaterialId.insert(255, uint64_t(store.strings.intern("DarkBrown")));

  colorByName.insert(uint64_t(store.strings.intern("Blue")), 0x0000cc);
  colorByName.insert(uint64_t(store.strings.intern("blue")), 0x0000cc);
  colorByName.insert(uint64_t(store.strings.intern("Pink")), 0xcc919e);
  colorByName.insert(uint64_t(store.strings.intern("pink")), 0xcc919e);
  colorByName.insert(uint64_t(store.strings.intern("SteelBlue")), 0x4782b5);
  colorByName.insert(uint64_t(store.strings.intern("steelblue")), 0x4782b5);
  colorByName.insert(uint64_t(store.strings.intern("SandyBrown")), 0xf4a55e);
  colorByName.insert(uint64_t(store.strings.intern("sandybrown")), 0xf4a55e);
  colorByName.insert(uint64_t(store.strings.intern("Black")), 0x000000);
  colorByName.insert(uint64_t(store.strings.intern("black")), 0x000000);
  colorByName.insert(uint64_t(store.strings.intern("DarkGrey")), 0x518c8c);
  colorByName.insert(uint64_t(store.strings.intern("darkgrey")), 0x518c8c);
  colorByName.insert(uint64_t(store.strings.intern("RoyalBlue")), 0x4775ff);
  colorByName.insert(uint64_t(store.strings.intern("royalblue")), 0x4775ff);
  colorByName.insert(uint64_t(store.strings.intern("White")), 0xffffff);
  colorByName.insert(uint64_t(store.strings.intern("white")), 0xffffff);
  colorByName.insert(uint64_t(store.strings.intern("Brown")), 0xcc2b2b);
  colorByName.insert(uint64_t(store.strings.intern("brown")), 0xcc2b2b);
  colorByName.insert(uint64_t(store.strings.intern("Ivory")), 0xedede0);
  colorByName.insert(uint64_t(store.strings.intern("ivory")), 0xedede0);
  colorByName.insert(uint64_t(store.strings.intern("DarkGreen")), 0x2d4f2d);
  colorByName.insert(uint64_t(store.strings.intern("darkgreen")), 0x2d4f2d);
  colorByName.insert(uint64_t(store.strings.intern("Salmon")), 0xf97f70);
  colorByName.insert(uint64_t(store.strings.intern("salmon")), 0xf97f70);
  colorByName.insert(uint64_t(store.strings.intern("BrightOrange")), 0xffa500);
  colorByName.insert(uint64_t(store.strings.intern("brightorange")), 0xffa500);
  colorByName.insert(uint64_t(store.strings.intern("Chocolate")), 0xed7521);
  colorByName.insert(uint64_t(store.strings.intern("chocolate")), 0xed7521);
  colorByName.insert(uint64_t(store.strings.intern("BrightRed")), 0xff0000);
  colorByName.insert(uint64_t(store.strings.intern("brightred")), 0xff0000);
  colorByName.insert(uint64_t(store.strings.intern("Plum")), 0x8c668c);
  colorByName.insert(uint64_t(store.strings.intern("plum")), 0x8c668c);
  colorByName.insert(uint64_t(store.strings.intern("ForestGreen")), 0x238e23);
  colorByName.insert(uint64_t(store.strings.intern("forestgreen")), 0x238e23);
  colorByName.insert(uint64_t(store.strings.intern("LightGold")), 0xede8aa);
  colorByName.insert(uint64_t(store.strings.intern("lightgold")), 0xede8aa);
  colorByName.insert(uint64_t(store.strings.intern("CoralRed")), 0xcc5b44);
  colorByName.insert(uint64_t(store.strings.intern("coralred")), 0xcc5b44);
  colorByName.insert(uint64_t(store.strings.intern("Indigo")), 0x330066);
  colorByName.insert(uint64_t(store.strings.intern("indigo")), 0x330066);
  colorByName.insert(uint64_t(store.strings.intern("BlueGrey")), 0x687c93);
  colorByName.insert(uint64_t(store.strings.intern("bluegrey")), 0x687c93);
  colorByName.insert(uint64_t(store.strings.intern("Gold")), 0xedc933);
  colorByName.insert(uint64_t(store.strings.intern("gold")), 0xedc933);
  colorByName.insert(uint64_t(store.strings.intern("LightYellow")), 0xededd1);
  colorByName.insert(uint64_t(store.strings.intern("lightyellow")), 0xededd1);
  colorByName.insert(uint64_t(store.strings.intern("PowderBlue")), 0xafe0e5);
  colorByName.insert(uint64_t(store.strings.intern("powderblue")), 0xafe0e5);
  colorByName.insert(uint64_t(store.strings.intern("LightGrey")), 0xbfbfbf);
  colorByName.insert(uint64_t(store.strings.intern("lightgrey")), 0xbfbfbf);
  colorByName.insert(uint64_t(store.strings.intern("Yellow")), 0xcccc00);
  colorByName.insert(uint64_t(store.strings.intern("yellow")), 0xcccc00);
  colorByName.insert(uint64_t(store.strings.intern("DarkBrown")), 0x8c4414);
  colorByName.insert(uint64_t(store.strings.intern("darkbrown")), 0x8c4414);
  colorByName.insert(uint64_t(store.strings.intern("DeepPink")), 0xed1189);
  colorByName.insert(uint64_t(store.strings.intern("deeppink")), 0xed1189);
  colorByName.insert(uint64_t(store.strings.intern("Mauve")), 0x660099);
  colorByName.insert(uint64_t(store.strings.intern("mauve")), 0x660099);
  colorByName.insert(uint64_t(store.strings.intern("Magenta")), 0xdd00dd);
  colorByName.insert(uint64_t(store.strings.intern("magenta")), 0xdd00dd);
  colorByName.insert(uint64_t(store.strings.intern("Tomato")), 0xff6347);
  colorByName.insert(uint64_t(store.strings.intern("tomato")), 0xff6347);
  colorByName.insert(uint64_t(store.strings.intern("Midnight")), 0x2d2d4f);
  colorByName.insert(uint64_t(store.strings.intern("midnight")), 0x2d2d4f);
  colorByName.insert(uint64_t(store.strings.intern("Orange")), 0xed9900);
  colorByName.insert(uint64_t(store.strings.intern("orange")), 0xed9900);
  colorByName.insert(uint64_t(store.strings.intern("YellowGreen")), 0x99cc33);
  colorByName.insert(uint64_t(store.strings.intern("yellowgreen")), 0x99cc33);
  colorByName.insert(uint64_t(store.strings.intern("Aquamarine")), 0x75edc6);
  colorByName.insert(uint64_t(store.strings.intern("aquamarine")), 0x75edc6);
  colorByName.insert(uint64_t(store.strings.intern("DarkSlate")), 0x2d4f4f);
  colorByName.insert(uint64_t(store.strings.intern("darkslate")), 0x2d4f4f);
  colorByName.insert(uint64_t(store.strings.intern("Red")), 0xcc0000);
  colorByName.insert(uint64_t(store.strings.intern("red")), 0xcc0000);
  colorByName.insert(uint64_t(store.strings.intern("Khaki")), 0x9e9e5e);
  colorByName.insert(uint64_t(store.strings.intern("khaki")), 0x9e9e5e);
  colorByName.insert(uint64_t(store.strings.intern("Wheat")), 0xf4ddb2);
  colorByName.insert(uint64_t(store.strings.intern("wheat")), 0xf4ddb2);
  colorByName.insert(uint64_t(store.strings.intern("Cyan")), 0x00eded);
  colorByName.insert(uint64_t(store.strings.intern("cyan")), 0x00eded);
  colorByName.insert(uint64_t(store.strings.intern("Turquoise")), 0x00bfcc);
  colorByName.insert(uint64_t(store.strings.intern("turquoise")), 0x00bfcc);
  colorByName.insert(uint64_t(store.strings.intern("SpringGreen")), 0x00ff7f);
  colorByName.insert(uint64_t(store.strings.intern("springgreen")), 0x00ff7f);
  colorByName.insert(uint64_t(store.strings.intern("Grey")), 0xa8a8a8);
  colorByName.insert(uint64_t(store.strings.intern("grey")), 0xa8a8a8);
  colorByName.insert(uint64_t(store.strings.intern("Green")), 0x00cc00);
  colorByName.insert(uint64_t(store.strings.intern("green")), 0x00cc00);
  colorByName.insert(uint64_t(store.strings.intern("Beige")), 0xf4f4db);
  colorByName.insert(uint64_t(store.strings.intern("beige")), 0xf4f4db);
  colorByName.insert(uint64_t(store.strings.intern("OrangeRed")), 0xff7f00);
  colorByName.insert(uint64_t(store.strings.intern("orangered")), 0xff7f00);
  colorByName.insert(uint64_t(store.strings.intern("Tan")), 0xdb9370);
  colorByName.insert(uint64_t(store.strings.intern("tan")), 0xdb9370);
  colorByName.insert(uint64_t(store.strings.intern("WhiteSmoke")), 0xf4f4f4);
  colorByName.insert(uint64_t(store.strings.intern("whitesmoke")), 0xf4f4f4);
  colorByName.insert(uint64_t(store.strings.intern("Maroon")), 0x8e236b);
  colorByName.insert(uint64_t(store.strings.intern("maroon")), 0x8e236b);
  colorByName.insert(uint64_t(store.strings.intern("NavyBlue")), 0x00007f);
  colorByName.insert(uint64_t(store.strings.intern("navyblue")), 0x00007f);
  colorByName.insert(uint64_t(store.strings.intern("Violet")), 0xed82ed);
  colorByName.insert(uint64_t(store.strings.intern("violet")), 0xed82ed);

  defaultName = store.strings.intern("Default");
  colorByName.insert(uint64_t(defaultName), 0x787878);

  if (colorAttribute) {
    colorAttribute = store.strings.intern(colorAttribute);
  }

  stack = (StackItem*)arena.alloc(sizeof(StackItem) * store.groupCountAllocated());
  stack_p = 0;

}

void Colorizer::beginGroup(Group* group)
{
  StackItem item;
  if (stack_p == 0) {
    item.colorName = defaultName;
    item.color = uint32_t(colorByName.get(uint64_t(defaultName)));
    item.override = false;
  }
  else {
    item = stack[stack_p - 1];
  }

  if (!item.override) {
    uint64_t colorName;
    if (group->group.material == 0) {
      colorName = uint64_t(defaultName);
    }
    else if (colorNameByMaterialId.get(colorName, group->group.material)) {
      uint64_t color;
      if (colorByName.get(color, colorName)) {
        item.colorName = (const char*)colorName;
        item.color = uint32_t(color);
      }
      else if(!naggedName.get(colorName)) {
        naggedName.insert(colorName, 1);
        logger(1, "Unrecognized color name %s", (const char*)colorName);
      }
    }
    else if (!naggedMaterialId.get(group->group.material)) {
      naggedMaterialId.insert(group->group.material, 1);
      logger(1, "Unrecognized material id %d", group->group.material);
    }
  }

  stack[stack_p++] = item;
}

void Colorizer::EndGroup()
{
  assert(stack_p);
  stack_p--;
}

void Colorizer::attribute(const char* key, const char* val)
{
  assert(stack_p);
  if (key == colorAttribute) {
    uint64_t color;
    if (colorByName.get(color, uint64_t(val))) {
      auto & item = stack[stack_p - 1];
      item.colorName = val;
      item.color = uint32_t(color);
      item.override = true;
    }
    else if (!naggedName.get(uint64_t(val))) {
      naggedName.insert(uint64_t(val), 1);
      logger(1, "A Unrecognized color name %s", (const char*)val);
    }
  }
}

void Colorizer::geometry(Geometry* geometry)
{
  assert(stack_p);
  geometry->colorName = stack[stack_p - 1].colorName;
  geometry->color = stack[stack_p - 1].color;
}

