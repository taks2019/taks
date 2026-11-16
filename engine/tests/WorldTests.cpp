#include "../CauseWorld.h"
#include <cassert>
#include <iostream>
#include <string>
int main(){
    taks::World w;
    assert(w.act("ignite").find("tűzhely")!=std::string::npos);
    assert(w.act("hearth").find("faág")!=std::string::npos);
    const int wood0=w.groundWood;
    for(int i=0;i<5;++i) w.act("wood");
    for(int i=0;i<2;++i) w.act("stone");
    w.act("tinder");
    assert(w.groundWood==wood0-5);
    assert(w.act("hearth").find("elkészült")!=std::string::npos);
    assert(w.carriedWood==2 && w.carriedStone==0);
    assert(w.act("ignite").find("meggyulladt")!=std::string::npos);
    assert(w.fireLit && w.fireFuel==2.0 && w.carriedWood==0);
    auto saved=w.save(); taks::World restored;
    assert(restored.load(saved)); assert(restored.save()==saved);
    assert(!restored.load("sérült vagy csaló mentés"));
    restored.step(20); assert(!restored.fireLit); assert(restored.fireFuel==0);
    taks::World wet;double before=wet.woodMoisture;
    wet.forceRainForTest(1);wet.step(1);
    assert(wet.woodMoisture>before && wet.groundMoisture>0.13);
    assert(wet.act("unknown").find("nem módosult")!=std::string::npos);
    std::cout<<"TAKS native cause/effect tests PASSED\n";
}
