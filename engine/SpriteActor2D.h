#pragma once
// TAKS World: eight-direction, layered PNG actor state. No GLB dependency.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace taks {
enum class SpriteAction : std::uint8_t {
  Idle, Walk, Run, Crouch, Prone, Crawl, Climb, Jump,
  Gather, Carry, Build, Swim, Rest
};
enum class ClothingSlot : std::uint8_t {
  Torso, Legs, Feet, Head, Belt, Back, Hands, Count
};
struct Garment2D {
  unsigned itemId=0;
  ClothingSlot slot=ClothingSlot::Torso;
  std::string atlasKey;
  double massKg=0.0, insulation=0.0, wetness=0.0, mud=0.0;
};
struct SpriteClimate {
  double rain=0.0, wind=0.0, mud=0.0, temperatureC=22.0;
  bool sheltered=false;
};
struct ActorNeeds2D {
  double hunger=10.0, thirst=15.0, stamina=100.0;
  double fatigue=5.0, bodyWetness=0.0, muddyClothes=0.0;
};
struct SpriteRenderLayer {
  std::string atlasKey;
  int direction=0, frame=0, drawOrder=0;
  float footAnchorX=0.5f, footAnchorY=0.98f;
};
struct SpriteActor2D {
  static constexpr int directions=8;
  static constexpr int clothingSlots=static_cast<int>(ClothingSlot::Count);
  unsigned actorId=0;
  double x=0.0, z=0.0, headingRadians=0.0;
  SpriteAction action=SpriteAction::Idle;
  ActorNeeds2D needs{};
  std::vector<Garment2D> owned;
  std::array<unsigned,clothingSlots> worn{};
  double animationSeconds=0.0;

  static double clamp01(double x) { return std::max(0.0,std::min(1.0,x)); }
  int direction8() const {
    constexpr double pi=3.14159265358979323846;
    const double radians=std::remainder(headingRadians,2.0*pi);
    int octant=static_cast<int>(std::floor(radians/(pi/4.0)+0.5));
    return (octant%8+8)%8;
  }
  static const char* actionName(SpriteAction a) {
    switch(a) {
      case SpriteAction::Idle:return "idle";
      case SpriteAction::Walk:return "walk";
      case SpriteAction::Run:return "run";
      case SpriteAction::Crouch:return "crouch";
      case SpriteAction::Prone:return "prone";
      case SpriteAction::Crawl:return "crawl";
      case SpriteAction::Climb:return "climb";
      case SpriteAction::Jump:return "jump";
      case SpriteAction::Gather:return "gather";
      case SpriteAction::Carry:return "carry";
      case SpriteAction::Build:return "build";
      case SpriteAction::Swim:return "swim";
      case SpriteAction::Rest:return "rest";
    }
    return "idle";
  }
  static int frameCount(SpriteAction a) {
    switch(a) {
      case SpriteAction::Idle:return 1;
      case SpriteAction::Walk:return 8;
      case SpriteAction::Run:return 10;
      case SpriteAction::Jump:return 7;
      case SpriteAction::Crouch:return 5;
      case SpriteAction::Prone:return 6;
      default:return 8;
    }
  }
  static double framesPerSecond(SpriteAction a) {
    switch(a) {
      case SpriteAction::Idle:return 1.0;
      case SpriteAction::Walk:return 8.0;
      case SpriteAction::Run:return 12.0;
      case SpriteAction::Jump:return 14.0;
      default:return 7.0;
    }
  }
  int frameIndex() const {
    return static_cast<int>(std::floor(animationSeconds*framesPerSecond(action)))%frameCount(action);
  }
  void setAction(SpriteAction next) {
    if(action!=next) { action=next; animationSeconds=0.0; }
  }
  bool addOwned(const Garment2D& item) {
    if(item.itemId==0||item.atlasKey.empty()||item.massKg<0.0)return false;
    for(const auto& x:owned)if(x.itemId==item.itemId)return false;
    owned.push_back(item);
    return true;
  }
  bool equip(ClothingSlot slot,unsigned itemId) {
    const int s=static_cast<int>(slot);
    if(s<0||s>=clothingSlots||itemId==0)return false;
    for(const auto& item:owned)if(item.itemId==itemId&&item.slot==slot) {
      for(unsigned w:worn)if(w==itemId)return false;
      worn[s]=itemId; return true;
    }
    return false;
  }
  bool unequip(ClothingSlot slot) {
    const int s=static_cast<int>(slot);
    if(s<0||s>=clothingSlots||worn[s]==0)return false;
    worn[s]=0; return true;
  }
  double clothingKg() const {
    double mass=0.0;
    for(unsigned id:worn)if(id)for(const auto& item:owned)
      if(id==item.itemId)mass+=item.massKg;
    return mass;
  }
  double protection() const {
    double value=0.0;
    for(unsigned id:worn)if(id)for(const auto& item:owned)
      if(id==item.itemId)value+=item.insulation*(1.0-0.5*clamp01(item.wetness));
    return value;
  }
  double groundSpeedFactor(const SpriteClimate& climate) const {
    return std::max(0.30,1.0-0.58*clamp01(climate.mud)-0.04*clothingKg());
  }
  void tick(double dtSeconds,const SpriteClimate& climate) {
    if(!std::isfinite(dtSeconds)||dtSeconds<=0.0)return;
    const double dt=std::min(dtSeconds,0.25);
    animationSeconds+=dt;
    const bool work=action!=SpriteAction::Idle&&action!=SpriteAction::Rest;
    const bool strenuous=action==SpriteAction::Run||action==SpriteAction::Climb||
      action==SpriteAction::Jump||action==SpriteAction::Build;
    const double strain=strenuous?1.0:(work?0.5:0.0);
    const double mud=clamp01(climate.mud), rain=clamp01(climate.rain);
    const double heat=std::max(0.0,climate.temperatureC-28.0);
    needs.stamina=std::clamp(needs.stamina+dt*(work?-(0.5+1.8*strain+mud):1.6),0.0,100.0);
    needs.hunger=std::clamp(needs.hunger+dt*(0.0009+0.0012*strain),0.0,100.0);
    needs.thirst=std::clamp(needs.thirst+dt*(0.0015+0.002*strain+heat*0.0004),0.0,100.0);
    needs.fatigue=std::clamp(needs.fatigue+dt*(work?0.04*strain:-0.04),0.0,100.0);
    const double exposure=climate.sheltered?0.03:1.0;
    needs.bodyWetness=std::clamp(needs.bodyWetness+
      dt*(rain*0.13*exposure-0.015*(1+climate.wind*0.1)),0.0,1.0);
    needs.muddyClothes=std::clamp(needs.muddyClothes+
      dt*(work?mud*0.04:0.0),0.0,1.0);
    for(auto& item:owned) {
      bool equipped=false;
      for(unsigned id:worn)if(id==item.itemId)equipped=true;
      if(!equipped)continue;
      item.wetness=std::clamp(item.wetness+dt*(rain*0.08*exposure-0.01),0.0,1.0);
      item.mud=std::clamp(item.mud+dt*(work?mud*0.03:0.0),0.0,1.0);
    }
    if(climate.temperatureC>33.0&&protection()>1.0&&work)
      needs.stamina=std::max(0.0,needs.stamina-dt*protection()*0.10);
  }
  std::vector<SpriteRenderLayer> renderLayers() const {
    std::vector<SpriteRenderLayer> result;
    const int angle=direction8()*45, frame=frameIndex();
    result.push_back({"body",angle,frame,0});
    for(int slot=0;slot<clothingSlots;++slot)if(worn[slot]) {
      for(const auto& item:owned)if(item.itemId==worn[slot]) {
        result.push_back({item.atlasKey,angle,frame,slot+1});
        break;
      }
    }
    return result;
  }
};
}
