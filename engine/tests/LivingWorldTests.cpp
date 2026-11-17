#include "../LivingWorld.h"
#include <cassert>
#include <cmath>
#include <sstream>
#include <iostream>

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
    auto original=loaded.grassCover();
    std::istringstream bad("ECO1 3 x");
    assert(!loaded.read(bad) && loaded.grassCover()==original);
    std::cout<<"PASS: grass, trees, animals, mortality, mud, snow and save/load\n";
}
