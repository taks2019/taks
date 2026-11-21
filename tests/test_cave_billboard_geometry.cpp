#include "../app/src/main/cpp/CaveBillboardGeometry.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using taks::CaveBillboard::aligned;
using taks::CaveBillboard::Point;
static float dot(Point p,Point q){return p.x*q.x+p.y*q.y+p.z*q.z;}
static float distance(Point p,Point q){
    return std::sqrt(std::pow(p.x-q.x,2)+std::pow(p.y-q.y,2)+std::pow(p.z-q.z,2));
}
static Point diff(Point a,Point b){return {a.x-b.x,a.y-b.y,a.z-b.z,0,0};}
static Point cross(Point a,Point b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,
                                       a.x*b.y-a.y*b.x,0,0};}
int main(){
    constexpr float rad=.01745329251994329577f;
    constexpr float foot=444.f/512.f;
    constexpr float size=20.f;
    // Real camera elevation range: 20°..80°, all eight 45° viewing angles.
    // The sprite must NEVER turn into a wide pancake.
    for(int direction=0;direction<8;++direction){
      for(int elevation=20;elevation<=80;elevation+=5){
        const float yaw=direction*45.f*rad,pitch=elevation*rad;
        auto quad=aligned(yaw,pitch,0.f,-.48f,2.2f,size,foot);
        auto tl=quad.vertices[0],bl=quad.vertices[1],br=quad.vertices[2],tr=quad.vertices[5];
        const float width=distance(tl,tr),height=distance(tl,bl);
        assert(std::abs(width-size)<.002f);
        assert(std::abs(height-size)<.002f);
        assert(std::abs(width-height)<.002f);
        auto widthV=diff(tr,tl),heightV=diff(tl,bl);
        assert(std::abs(dot(widthV,heightV))<.02f);
        // The midpoint at the lowest actual opaque pixel stays pinned on the
        // same WORLD terrain position, independent of rotation and zoom.
        Point alphaFoot{
           (bl.x+br.x)*.5f+(tl.x-bl.x)*(1.f-foot),
           (bl.y+br.y)*.5f+(tl.y-bl.y)*(1.f-foot),
           (bl.z+br.z)*.5f+(tl.z-bl.z)*(1.f-foot),0,0};
        assert(std::abs(alphaFoot.x)<.002f);
        assert(std::abs(alphaFoot.y+.48f)<.002f);
        assert(std::abs(alphaFoot.z-2.2f)<.002f);
      }
    }
    std::puts("OK: 1:1 pixel proportions and anchored PNG feet on every camera angle");
}
