#pragma once
#include <string>
namespace taks {
struct FastFrame {
    float yaw=0.785398f, pitch=0.615480f, zoom=14.0f;
    float playerX=0,playerZ=-5.2f,walkPhase=0;
    float rain=0, moisture=0, clock=8;
    float hearth=0, fire=0;
    float nodes[48]={};
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
