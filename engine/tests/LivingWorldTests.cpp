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
    assert(legacyTokens.front()=="ECO2");
    legacyTokens.front()="ECO1";
    legacyTokens.resize(legacyTokens.size()-taks::LivingWorld::grassCount);
    std::ostringstream legacyText;
    for(std::size_t i=0;i<legacyTokens.size();++i){if(i)legacyText<<' ';legacyText<<legacyTokens[i];}
    std::istringstream oldSave(legacyText.str());
    taks::LivingWorld legacy;
    assert(legacy.read(oldSave));
    for(const auto& patch:legacy.grass)assert(patch.trampling==0);
    auto original=loaded.grassCover();
    std::istringstream bad("ECO1 3 x");
    assert(!loaded.read(bad) && loaded.grassCover()==original);
    std::cout<<"PASS: grass, trampling recovery, trees, animals, mortality, mud, snow and save/load\n";
}
