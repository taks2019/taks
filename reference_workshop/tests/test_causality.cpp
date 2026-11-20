#include "../src/WorldCauseEffect.hpp"
#include <cassert>
#include <iostream>
int main(){
  taks::World world;
  assert(world.humans.size()==5);
  assert(world.humans[0].controlled_by_player);
  for(int i=1;i<5;++i) assert(!world.humans[i].controlled_by_player);
  world.weather.wind_mps=8.f;
  world.step(60.f);
  assert(world.water.ripple_amplitude_m>0.f);
  assert(world.vegetation.bend_angle_deg>0.f);
  world.humans[0].action=taks::Action::Walk;
  world.humans[0].attempted_speed_mps=1.2f;
  world.step(1.f);
  const float dry=world.humans[0].actual_speed_mps;
  world.soil.water_saturation=.95f;
  world.step(1.f);
  assert(world.humans[0].actual_speed_mps<dry);
  assert(world.soil.footprint_depth_m>0.f);
  assert(world.humans[0].foot_mud_kg>0.f);
  world.weather.rain_mm_per_hour=250.f;
  world.step(3600.f);
  assert(!world.campfire.burning);
  assert(world.humans[0].needs.hunger>0.f);
  assert(world.humans[0].needs.thirst>0.f);
  std::cout<<"PASS five humans; wind->water, foliage; mud->tracks; rain->fire; needs\n";
}
