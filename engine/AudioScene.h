#pragma once
#include "CauseWorld.h"
#include <algorithm>
#include <array>
#include <cmath>
namespace taks {
// Version 1 JNI contract. Gains 0..7, enclosure 8, material 9,
// accumulated actual grounded travel 10, water/fire pan 11..12.
// Surfaces: dry soil, mud, stone, grass, water, snow.
inline std::array<float,13> audioScene(const World& w, bool airborne=false) {
    std::array<float,13> s{};
    auto gain=[](double x){return static_cast<float>(std::clamp(x,0.0,1.0));};
    auto attenuation=[&](double x,double z){
        double d=std::hypot(x-w.playerX,z-w.playerZ);
        return 1.0/(1.0+d*d/9.0);
    };
    // Same opening plane and walkable cave extent as World::move / FastScene.
    double cave=std::clamp((w.playerZ-3.5)/1.5,0.0,1.0);
    double enclosure=std::max(cave,w.protectedAtPlayer()?0.72:0.0);
    double outside=1.0-enclosure*.88;
    s[0]=gain(w.rainIntensity*outside);
    s[1]=gain(w.windStrength()*outside);
    double water=0,waterPan=0;
    int material=cave>.5?2:0;
    for(const auto& b:w.ecology.waters){
        double g=b.level*attenuation(b.x,b.z);
        water+=g;waterPan+=g*std::clamp((b.x-w.playerX)/4.0,-1.0,1.0);
        if(b.level>.05&&std::hypot(b.x-w.playerX,b.z-w.playerZ)<b.radius)material=4;
    }
    s[2]=gain(water*outside);
    s[3]=gain(cave*w.ecology.soilWater*(.3+.7*w.rainIntensity));
    s[4]=gain(w.fireLit?attenuation(-1.15,-1.8)*std::min(1.0,w.fireFuel):0.0);
    double vegetation=0;
    for(const auto& g:w.ecology.grass){
        vegetation+=g.biomass*g.health*attenuation(g.x,g.z);
        if(material==0&&g.biomass>.2&&std::hypot(g.x-w.playerX,g.z-w.playerZ)<1.0)material=3;
    }
    for(const auto& t:w.ecology.trees)if(t.alive)vegetation+=t.health*attenuation(t.x,t.z);
    s[5]=gain(vegetation*w.windStrength()*outside*.5);
    double animals=0;
    for(const auto& a:w.ecology.animals)if(a.alive)animals+=attenuation(a.x,a.z)*.3;
    s[6]=gain(animals*outside);
    double h=std::fmod(w.hours,24.0);
    // Birds are an ambient soundscape estimate, not invented simulated entities.
    s[7]=gain((h>=5&&h<=19?1.0:0.0)*(1-w.rainIntensity)*outside*.28);
    s[8]=gain(enclosure);
    if(material!=4&&material!=2&&w.ecology.mud>.25)material=1;
    if(material!=4&&material!=2&&w.ecology.snowDepth>.05)material=5;
    s[9]=static_cast<float>(material);
    s[10]=airborne?-1.f:static_cast<float>(w.walkPhase/5.5);
    s[11]=water>0?static_cast<float>(waterPan/water):0.f;
    s[12]=static_cast<float>(std::clamp((-1.15-w.playerX)/4.0,-1.0,1.0));
    return s;
}
}
