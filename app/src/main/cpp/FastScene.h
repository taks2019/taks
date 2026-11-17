#pragma once
#include <string>
#include <cstdint>
namespace taks {
struct FastFrame {
    float yaw=0.785398f, pitch=0.615480f, zoom=10.0f;
    float playerX=0,playerZ=-5.2f,walkPhase=0;
    float rain=0, moisture=0, mud=0, clock=8;
    float hearth=0, fire=0;
    float nodes[48]={};
    float grass[16]={}; // [x,z,real biomass,real health] x4
    float trees[12]={}; // [x,z,real height,alive] x3
    bool verifiedFerns=false;
    float npcs[16]={};
    float sites[12]={};
};
namespace FastScene {
    bool init();
    bool uploadTerrainTexture(int slot,const std::uint32_t* argb,int width,int height);
    void shutdown();
    void render(int width,int height,const FastFrame& scene);
    const std::string& lastError();
}
}
