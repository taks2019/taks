#pragma once
#include <string>
#include <cstdint>
namespace taks {
struct FastFrame {
    float yaw=-0.785398f, pitch=0.615480f, zoom=19.0f;
    float playerX=0,playerZ=-5.2f,walkPhase=0,playerHeading=0,playerJumpHeight=0;
    bool playerMoving=false,playerRunning=false,playerCrouching=false;
    float rain=0, moisture=0, mud=0, clock=8,animationTime=0;
    float wetSeason=0,windStrength=0,windX=1,windZ=0,temperatureC=20;
    float hearth=0, fire=0;
    float nodes[48]={};
    float grass[20]={}; // [x,z,biomass,health,trampling] x4
    float trees[12]={}; // [x,z,real height,alive] x3
    float footprints[128]={}; // [x,z,heading,impression strength] x32
    float waters[8]={}; // [x,z,radius,level] x2; follows LivingWorld's saved water state
    float animals[48]={}; // [x,z,kind,alive,thirst,hunger] x8; species remain unverified
    bool verifiedFerns=false;
    float npcs[16]={};
    float sites[12]={};
    float collectorStage=0,collectorWater=0;
};
namespace FastScene {
    bool init();
    bool uploadTerrainTexture(int slot,const std::uint32_t* argb,int width,int height);
    // True RGBA-PNG upload, 8 views of the same actor at 45 degree increments.
    // Upload from the active GLES3 thread, AFTER init(), BEFORE render().
    bool uploadActorSprite(int view,const std::uint32_t* argb,int width,int height);
    bool actorSpritesComplete();
    void shutdown();
    void render(int width,int height,const FastFrame& scene);
    const std::string& lastError();
}
}
