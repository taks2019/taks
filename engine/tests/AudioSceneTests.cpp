#include "AudioScene.h"
#include <cassert>
#include <iostream>
int main(){
    taks::World w;
    w.forceRainForTest(0);auto dry=taks::audioScene(w);
    w.forceRainForTest(1);auto rain=taks::audioScene(w);
    assert(rain[0]>dry[0]);
    w.playerZ=6;w.ecology.soilWater=.8;
    auto cave=taks::audioScene(w);
    assert(cave[0]<rain[0]&&cave[8]==1&&cave[3]>0&&cave[9]==2);
    w.playerX=-1.15;w.playerZ=-1.8;w.fireLit=true;w.fireFuel=2;
    auto near=taks::audioScene(w);w.playerX=7;auto far=taks::audioScene(w);
    assert(near[4]>far[4]);w.fireLit=false;assert(taks::audioScene(w)[4]==0);
    w.playerX=0;w.playerZ=-5;w.ecology.mud=.8;
    assert(taks::audioScene(w)[9]==1);
    w.playerX=w.ecology.waters[0].x;w.playerZ=w.ecology.waters[0].z;
    auto wet=taks::audioScene(w);assert(wet[9]==4);
    for(auto& b:w.ecology.waters)b.level=0;
    assert(taks::audioScene(w)[2]==0);
    for(auto& a:w.ecology.animals)a.alive=false;
    assert(taks::audioScene(w)[6]==0);
    assert(taks::audioScene(w,true)[10]==-1);
    w.walkPhase=22;assert(taks::audioScene(w)[10]==4);
    for(float v:taks::audioScene(w))assert(std::isfinite(v));
    std::cout<<"PASS rain, cave, distance, fire extinction, mud, water, death, airborne, actual travel\n";
}
