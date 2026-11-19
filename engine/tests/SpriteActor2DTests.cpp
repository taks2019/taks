#include "../SpriteActor2D.h"
#include <cassert>
using namespace taks;
int main() {
  SpriteActor2D actor;
  assert(actor.direction8()==0);
  actor.headingRadians=3.14159265358979323846/4.0;
  assert(actor.direction8()==1);
  actor.headingRadians=7.0*3.14159265358979323846/4.0;
  assert(actor.direction8()==7);
  actor.headingRadians=2.0*3.14159265358979323846;
  assert(actor.direction8()==0);
  assert(!actor.equip(ClothingSlot::Torso,999));
  assert(actor.addOwned({42,ClothingSlot::Torso,"fur_top",1.0,1.2}));
  assert(!actor.addOwned({42,ClothingSlot::Torso,"duplicate",1.0,1.2}));
  assert(!actor.equip(ClothingSlot::Feet,42));
  assert(actor.equip(ClothingSlot::Torso,42));
  assert(!actor.equip(ClothingSlot::Torso,42));
  assert(actor.renderLayers().size()==2);
  SpriteClimate dry;
  SpriteClimate wet; wet.mud=1.0; wet.rain=1.0;
  assert(actor.groundSpeedFactor(wet)<actor.groundSpeedFactor(dry));
  actor.setAction(SpriteAction::Run);
  double before=actor.needs.stamina;
  for(int i=0;i<50;++i)actor.tick(0.05,wet);
  assert(actor.needs.stamina<before);
  assert(actor.needs.bodyWetness>0.0);
  assert(actor.needs.muddyClothes>0.0);
  assert(actor.owned[0].wetness>0.0 && actor.owned[0].mud>0.0);
  assert(actor.frameIndex()>=0 && actor.frameIndex()<SpriteActor2D::frameCount(actor.action));
  assert(actor.unequip(ClothingSlot::Torso));
  assert(actor.renderLayers().size()==1);
  assert(!actor.unequip(ClothingSlot::Torso));
  // Clothing uses stable IDs and correctly layered sprite slots.
  assert(actor.addOwned({71,ClothingSlot::Bra,"bra_layer",0.14,0.2}));
  assert(actor.addOwned({72,ClothingSlot::UnderwearBottom,"underwear_bottom_layer",0.10,0.1}));
  assert(actor.addOwned({73,ClothingSlot::Feet,"leather_shoes",0.60,0.2}));
  assert(actor.addOwned({74,ClothingSlot::Outerwear,"winter_cape",1.50,2.0}));
  assert(actor.equip(ClothingSlot::Bra,71));
  assert(actor.equip(ClothingSlot::UnderwearBottom,72));
  assert(actor.equip(ClothingSlot::Feet,73));
  assert(actor.equip(ClothingSlot::Outerwear,74));
  auto layers=actor.renderLayers();
  assert(layers.size()==5); // body plus four interchangeable garments
  assert(layers.front().atlasKey=="body");
  assert(layers[1].atlasKey=="underwear_bottom_layer");
  assert(layers[2].atlasKey=="bra_layer");
  assert(layers[3].atlasKey=="leather_shoes");
  assert(layers[4].atlasKey=="winter_cape");
  assert(actor.clothingKg()>2.0);
  assert(actor.unequip(ClothingSlot::Bra));
  assert(actor.renderLayers().size()==4);
  // Content rating and character adulthood are independent. A minor or
  // a character with unspecified age must never enable mature anatomy.
  assert(!actor.mayDisplayAdultAnatomy());
  actor.matureContentEnabled=true;
  assert(!actor.mayDisplayAdultAnatomy());
  actor.ageYears=17;
  assert(!actor.mayDisplayAdultAnatomy());
  actor.ageYears=18;
  assert(actor.mayDisplayAdultAnatomy());
  actor.matureContentEnabled=false;
  assert(!actor.mayDisplayAdultAnatomy());
  SpriteActor2D group[5];
  for(unsigned i=0;i<5;++i)group[i].actorId=i+1;
  assert(group[4].actorId==5);
}
