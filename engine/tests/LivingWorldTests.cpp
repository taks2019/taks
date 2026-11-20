#include "../LivingWorld.h"
#include <cassert>
#include <cmath>
#include <sstream>
#include <iostream>
#include <vector>

int main(){
    taks::LivingWorld e;
    assert(e.valid());
    const double initialGrass=e.grassCover();
    const int initialLiving=e.aliveAnimals();
    assert(initialLiving==4 && initialGrass>0.4);
    assert(e.walkSpeedFactor()==1.);
    e.step(24,1,1,.8,22);
    assert(e.soilWater>.13);
    assert(e.mud>0);
    assert(e.walkSpeedFactor()<1.);
    e.stampFootstep(0.0,0.0,0.0);
    e.stampFootstep(.5,0.0,0.0);
    assert(e.visibleFootprintCount()==2);
    assert(e.snowDepth==0 && e.ice==0);
    const double speedWet=e.walkSpeedFactor();
    e.step(24,1,1,.4,-8);
    assert(e.snowDepth>0 && e.ice>0);
    assert(e.walkSpeedFactor()<speedWet);
    assert(e.slipRisk()>0);
    taks::LivingWorld trampled;
    trampled.trampleTrail(trampled.grass[0].x,trampled.grass[0].z,
                          trampled.grass[0].x+.7,trampled.grass[0].z,0.0);
    const double dryTrample=trampled.grass[0].trampling;
    assert(dryTrample>0 && trampled.grass[0].biomass<.62);
    taks::LivingWorld muddyTrample;
    muddyTrample.trampleTrail(muddyTrample.grass[0].x,muddyTrample.grass[0].z,
                              muddyTrample.grass[0].x+.7,muddyTrample.grass[0].z,1.0);
    assert(muddyTrample.grass[0].trampling>dryTrample);
    const double beforeRecovery=trampled.grass[0].trampling;
    trampled.step(24,0,.8,.8,22);
    assert(trampled.grass[0].trampling<beforeRecovery);
    e.step(24,0,.0,1.,30);
    assert(e.snowDepth>=0 && e.snowDepth<.18);
    // Species labels are NEVER invented when ferns or vines have not been verified.
    assert(!e.verifiedFerns && !e.verifiedVines);
    // Living vegetation is not an infinite static decoration.
    taks::LivingWorld grassOnly;
    for(auto& animal:grassOnly.animals)animal.alive=false;
    double first=grassOnly.grass[0].biomass;
    for(int i=0;i<15;++i)grassOnly.step(24,.8,.8,.85,22);
    assert(grassOnly.grass[0].biomass>first);
    const double height=grassOnly.trees[0].heightM;
    assert(height>2.0);
    // Hunger, thirst and finite biomass can cause mortality.
    taks::LivingWorld deaths;
    for(auto& g:deaths.grass)g.biomass=0;
    for(int i=0;i<80;++i)deaths.step(24,0,0,1,38);
    assert(deaths.aliveAnimals()<initialLiving);
    assert(deaths.deadAnimals>0);
    // Movement is real, leg animation must be keyed to positional changes.
    taks::LivingWorld active;
    for(auto& a:active.animals)a.alive=false;
    active.animals[0].alive=true;
    active.animals[0].x=-3.8;active.animals[0].z=-2.3;
    active.animals[0].thirst=60.; // nearest natural pool is still not here
    const double firstX=active.animals[0].x,firstZ=active.animals[0].z;
    const double sourceBefore=active.waters[0].level;
    active.step(.04,0,.13,.7,25);
    const double moved=std::hypot(active.animals[0].x-firstX,active.animals[0].z-firstZ);
    assert(moved>.04&&moved<.20);
    assert(active.waters[0].level<=sourceBefore);
    assert(active.valid());
    // Saving/restoring the world must preserve every living animal and soil state.
    std::ostringstream out;out.precision(17);e.write(out);
    std::istringstream input(out.str());
    taks::LivingWorld loaded;
    assert(loaded.read(input));
    std::ostringstream again;again.precision(17);loaded.write(again);
    assert(again.str()==out.str());
    // ECO1 saves remain readable and receive an untrampled default state.
    std::istringstream current(out.str());
    std::vector<std::string> legacyTokens;std::string token;
    while(current>>token)legacyTokens.push_back(token);
    assert(legacyTokens.front()=="ECO4");
    legacyTokens.front()="ECO1";
    legacyTokens.resize(1+9+taks::LivingWorld::grassCount*5+taks::LivingWorld::treeCount*7+
                        taks::LivingWorld::animalCount*11);
    std::ostringstream legacyText;
    for(std::size_t i=0;i<legacyTokens.size();++i){if(i)legacyText<<' ';legacyText<<legacyTokens[i];}
    std::istringstream oldSave(legacyText.str());
    taks::LivingWorld legacy;
    assert(legacy.read(oldSave));
    for(const auto& patch:legacy.grass)assert(patch.trampling==0);
    auto original=loaded.grassCover();
    std::istringstream bad("ECO2 3 x");
    assert(!loaded.read(bad) && loaded.grassCover()==original);
    // Equal hot, dry conditions lose more soil moisture in stronger wind.
    taks::LivingWorld calm,windy;
    calm.soilWater=windy.soilWater=.70;
    calm.step(24,0,.70,1,30,.10);
    windy.step(24,0,.70,1,30,.80);
    assert(windy.soilWater<calm.soilWater);
    taks::LivingWorld drying;
    drying.soilWater=.8;drying.mud=.9;
    drying.stampFootstep(0,0,0);
    assert(drying.visibleFootprintCount()==1);
    for(int day=0;day<5;++day)drying.step(24,0,0,1,30);
    assert(drying.visibleFootprintCount()==0);
    std::cout<<"PASS: grass, trees, animals, mortality, mud, footprints, snow and save/load\n";
}
