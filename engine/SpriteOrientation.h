#pragma once
// Isometric PNG sprite view selection: no camera-angle-dependent image distortion.
// Actor heading uses atan2(dx,dz), camera's horizontal direction points FROM actor TO eye.
#include <cmath>
namespace taks::SpriteOrientation {
constexpr float pi=3.14159265358979323846f;
inline int cameraRelativeDirection(float cameraYaw,float actorHeading) {
    if(!std::isfinite(cameraYaw)||!std::isfinite(actorHeading))return 0;
    // Camera position: (+sin(yaw),-cos(yaw)) relative to the character.
    // View N = character viewed from the FRONT, S = viewed from behind.
    const float viewerAzimuth=std::atan2(std::sin(cameraYaw),-std::cos(cameraYaw));
    const float delta=std::remainder(viewerAzimuth-actorHeading,2.f*pi);
    const int octant=static_cast<int>(std::floor(delta/(pi/4.f)+.5f));
    return (octant%8+8)%8;
}
inline const char* directionName(int i){
    static constexpr const char* names[8]={"N","NE","E","SE","S","SW","W","NW"};
    return names[(i%8+8)%8];
}
}
