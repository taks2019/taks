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
  SpriteActor2D group[5];
  for(unsigned i=0;i<5;++i)group[i].actorId=i+1;
  assert(group[4].actorId==5);
}
