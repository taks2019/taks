#pragma once
// TAKS World – isolated native C++17 cause/effect layer.
// Integration into the existing Android renderer and saves is deliberately pending.
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>
namespace taks {
inline float clamp01(float v) { return std::clamp(v,0.f,1.f); }
struct Weather {
  float wind_mps=1.f;
  float rain_mm_per_hour=0.f;
  float air_temp_C=27.f;
  float relative_humidity=.3f;
};
struct Soil {
  float water_saturation=.3f;
  float mud_depth_m=0.f;
  float ground_friction=.85f;
  float footprint_depth_m=0.f;
};
struct Water {
  float level_m=.25f;
  float ripple_amplitude_m=0.f;
  float surface_current_mps=0.f;
};
struct Vegetation { float bend_angle_deg=0.f; float root_integrity=1.f; };
struct Campfire { float fuel_kg=1.f; float wetness=0.f; bool burning=true; };
struct BodyNeeds {
  float hunger=0.f, thirst=0.f, fatigue=0.f;
  float body_temp_C=37.f, wetness=0.f, health=1.f;
};
enum class Action { Idle, Walk, Run, Crouch, Bend, Grasp, Carry, Build, Drink, Rest };
struct Human {
  std::string id;
  bool controlled_by_player=false;
  Action action=Action::Idle;
  float attempted_speed_mps=0.f, actual_speed_mps=0.f;
  float carried_mass_kg=0.f, foot_mud_kg=0.f;
  BodyNeeds needs;
};
struct Event { std::string cause, effect; };
struct World {
  Weather weather;
  Soil soil;
  Water water;
  Vegetation vegetation;
  Campfire campfire;
  std::array<Human,5> humans{{
    {"PLAYER",true,Action::Idle,0,0,0,0,{}},
    {"NPC_01",false,Action::Idle,0,0,0,0,{}},
    {"NPC_02",false,Action::Idle,0,0,0,0,{}},
    {"NPC_03",false,Action::Idle,0,0,0,0,{}},
    {"NPC_04",false,Action::Idle,0,0,0,0,{}}
  }};
  std::vector<Event> latest_events;
  void step(float dt_seconds) {
    if (dt_seconds <= 0.f) return;
    const float dt_h=dt_seconds/3600.f;
    const float rain=std::max(0.f,weather.rain_mm_per_hour);
    const float wind=std::max(0.f,weather.wind_mps);
    latest_events.clear();
    const float evap=std::max(0.f,weather.air_temp_C-9.f)*
      (.00027f+.00002f*wind)*(1.f-clamp01(weather.relative_humidity));
    soil.water_saturation=clamp01(soil.water_saturation +
      (rain*.0021f-evap-.003f)*dt_h);
    soil.mud_depth_m=std::max(0.f,soil.water_saturation-.62f)*.16f;
    soil.ground_friction=std::clamp(.90f-soil.mud_depth_m*5.9f,.19f,.90f);
    water.ripple_amplitude_m=std::min(.13f,.0017f*wind*wind+.00004f*rain);
    water.level_m=std::clamp(water.level_m+
      (rain*.0009f-evap*.4f)*dt_h,0.f,1.75f);
    vegetation.bend_angle_deg=std::min(31.f,wind*1.6f)*vegetation.root_integrity;
    if(wind>.5f) latest_events.push_back({"szel","vizfodrozodas es lombhajlas"});
    if(soil.mud_depth_m>.001f)
      latest_events.push_back({"telitett talaj","sar: kisebb tapadas es labnyom"});
    campfire.wetness=clamp01(campfire.wetness+
      (rain*.012f-.32f*wind*.01f)*dt_h);
    if(campfire.burning && campfire.fuel_kg>0.f) {
      campfire.fuel_kg=std::max(0.f,campfire.fuel_kg-
        (.17f*(1.f+std::min(wind,9.f)*.045f))*dt_h);
      if(campfire.wetness>.82f || campfire.fuel_kg<=0.f) {
        campfire.burning=false;
        latest_events.push_back({"vizes tuzhely vagy elfogyott fa","tuz kialvas"});
      }
    }
    for(auto &h:humans) {
      const bool active=h.action!=Action::Idle && h.action!=Action::Rest;
      const bool exert=h.action==Action::Run || h.action==Action::Carry ||
        h.action==Action::Build;
      const float weight=std::clamp(h.carried_mass_kg/35.f,0.f,.45f);
      const float fatigue=std::clamp(h.needs.fatigue*.45f,0.f,.45f);
      const float soil_factor=std::clamp(1.f-soil.mud_depth_m*8.f,.42f,1.f);
      h.actual_speed_mps=h.attempted_speed_mps*soil_factor*(1.f-weight)*(1.f-fatigue);
      h.needs.hunger=clamp01(h.needs.hunger+(active?.016f:.006f)*dt_h);
      h.needs.thirst=clamp01(h.needs.thirst+(exert?.055f:.019f)*dt_h);
      h.needs.fatigue=clamp01(h.needs.fatigue+(active?.11f:-.19f)*dt_h);
      h.needs.wetness=clamp01(h.needs.wetness+
        (rain>0.f?.012f*rain:-.07f)*dt_h);
      if(active && soil.mud_depth_m>0.f) {
        h.foot_mud_kg=std::min(.7f,h.foot_mud_kg+
          soil.mud_depth_m*.15f*dt_seconds);
        soil.footprint_depth_m=std::max(soil.footprint_depth_m,
          std::min(.08f,soil.mud_depth_m*.6f));
      }
      if(h.action==Action::Drink && water.level_m>.03f)
        h.needs.thirst=clamp01(h.needs.thirst-.45f*dt_h);
    }
  }
};
} // namespace taks
