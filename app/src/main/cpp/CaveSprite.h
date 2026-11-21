#pragma once
#include <cstdint>
#include <string>
namespace taks::CaveSprite {
    // An unapproved visual-review renderer. World simulation and collisions
    // remain native and independent. No sprites are ever committed as final art.
    bool init();
    void shutdown();
    bool upload(int direction, const std::uint32_t* argb, int width, int height);
    bool ready();
    void render(const float* viewProjection, float yaw, float pitch, float zoom,
                float caveGroundY, float clock, float rain, float alpha);
    const std::string& lastError();
}
