#pragma once
// Pure math: camera-facing RGBA PNGs in the native TAKS 2.5D world.
// No GL, no Android. A full image's screen width AND height are equal,
// independent of camera angle. The alpha-foot pixel stays attached to
// the same physical ground point. No invented photo/geometry is added.
#include <array>
#include <cmath>
namespace taks::CaveBillboard {
struct Vec3 {float x,y,z;};
struct Point {float x,y,z,u,v;};
struct Quad {std::array<Point,6> vertices;};
inline Quad aligned(float yaw,float pitch,
                    float footX,float footY,float footZ,
                    float width,float sourceFootRow01){
    // The camera's normalized screen basis, orthogonal for all valid pitches.
    const float sy=std::sin(yaw),cy=std::cos(yaw);
    const float sp=std::sin(pitch),cp=std::cos(pitch);
    const Vec3 right{-cy,0.f,-sy};
    const Vec3 up{-sy*sp,cp,cy*sp};
    const float left=-width*.5f, r=width*.5f;
    // Source PNG rows increase DOWN; y=sourceFootRow01 is ground level.
    const float top=(sourceFootRow01)*width;
    const float bottom=(sourceFootRow01-1.f)*width;
    auto pos=[&](float side,float height,float u,float v)->Point{
        return {footX+right.x*side+up.x*height,
                footY+right.y*side+up.y*height,
                footZ+right.z*side+up.z*height,u,v};
    };
    const Point tl=pos(left,top,0.f,0.f);
    const Point bl=pos(left,bottom,0.f,1.f);
    const Point br=pos(r,bottom,1.f,1.f);
    const Point tr=pos(r,top,1.f,0.f);
    return {{{tl,bl,br,tl,br,tr}}};
}
}
