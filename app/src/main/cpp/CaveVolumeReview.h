#pragma once
// TAKS World - interactive, physically three-dimensional cave geometry REVIEW.
//
// The Wonderwerk archaeological surface survey is NOT bundled here. This
// mathematically constructed cave mouth/tunnel is only a review-stage
// replacement for the flat front-wall placeholder; do not call it a
// geologically verified reconstruction or an approved visual asset.
//
// The emitted triangles all live in WorldTerrain coordinates. Their vertices
// carry actual XYZ depth and are not aligned to, or rotated with, the camera.
// Ground collision, objects, actors and cave share the same heightfield.
// No PNG "billboard" or flat front facade is emitted for the entry void.
#include <algorithm>
#include <cmath>

namespace taks::CaveVolume {
struct P { float x,y,z; };
constexpr float pi=3.14159265358979323846f;
constexpr float entryZ=3.65f;
constexpr float backZ=11.75f;
constexpr float mouthHalfWidth=1.80f;
inline float rockTop(float x) {
    const float taper=std::sqrt(std::max(0.f,1.f-x*x/(9.4f*9.4f)));
    return 5.15f*taper+.24f*taper*std::sin(x*.83f);
}
inline float roughFrontZ(float x,float y) {
    const float major=3.63f+.13f*std::sin(x*.74f+y*.28f)
        +.12f*std::sin(x*2.30f+y*2.10f);
    const float strata=.075f*std::sin(6.4f*y+1.3f*x)
        +.055f*std::sin(10.2f*y-3.7f*x);
    return major+strata;
}
inline float archCeiling(float x) {
    const float t=std::clamp(x/mouthHalfWidth,-1.f,1.f);
    const float curved=std::sqrt(std::max(0.f,1.f-t*t));
    return 1.21f+1.69f*curved +.045f*std::sin(x*3.1f);
}
inline float tubeWidth(float z) {
    return 1.76f-.10f*(z-entryZ)/(backZ-entryZ)
        +.10f*std::sin((z-entryZ)*.62f);
}
inline float tubeSideY(float z) {
    return 1.23f+.08f*std::sin(z*1.18f);
}
inline float tubeCrownY(float z) {
    return 2.96f+.20f*std::sin(z*.81f);
}
inline P ceilingAt(float theta,float z) {
    const float radial=tubeWidth(z);
    const float s=std::sin(theta), c=std::cos(theta);
    return {
        radial*c + .046f*s*std::sin(z*1.83f+theta*4.1f),
        tubeSideY(z)+(tubeCrownY(z)-tubeSideY(z))*s
            +.045f*s*std::sin(z*2.83f+theta*7.1f),
        z+.045f*s*std::sin(theta*5.3f+z*1.69f)
    };
}
// emit(P a,P b,P c,float material), ground(float x,float z)->float.
// Material 1 is the photographic dolomite outer cliff, material 16 is
// the same dolomite viewed in the darker tunnel; 17 is the far termination.
template<class Emit,class Ground>
void build(Emit emit,Ground ground) {
    // Separate masonry-free side banks and arched upper strip: unlike a
    // rectangular hole punched in a polygon grid, the mouth has a continuous
    // curved boundary and actual thickness for oblique camera views.
    auto frontSpan=[&](float from,float to,int cols,bool opening){
        constexpr int rows=30;
        for(int i=0;i<cols;++i){
            const float x0=from+(to-from)*float(i)/cols;
            const float x1=from+(to-from)*float(i+1)/cols;
            const float bottom0=opening?archCeiling(x0):ground(x0,entryZ);
            const float bottom1=opening?archCeiling(x1):ground(x1,entryZ);
            const float top0=std::max(bottom0+.02f,rockTop(x0));
            const float top1=std::max(bottom1+.02f,rockTop(x1));
            for(int j=0;j<rows;++j){
                const float t=float(j)/rows, t1=float(j+1)/rows;
                auto point=[&](float x,float bottom,float top,float t)->P{
                    const float y=bottom+(top-bottom)*t;
                    return {x,y,roughFrontZ(x,y)};
                };
                const auto a=point(x0,bottom0,top0,t);
                const auto b=point(x1,bottom1,top1,t);
                const auto c=point(x1,bottom1,top1,t1);
                const auto d=point(x0,bottom0,top0,t1);
                emit(a,b,c,1.f);emit(a,c,d,1.f);
            }
        }
    };
    frontSpan(-9.4f,-mouthHalfWidth,46,false);
    frontSpan(-mouthHalfWidth,mouthHalfWidth,36,true);
    frontSpan(mouthHalfWidth,9.4f,46,false);

    // Walkable cave tunnel, rough stone with real inner concave surface.
    // Radius narrows gently but does not turn into a tiny fake black box.
    constexpr int depth=64, angular=48;
    for(int i=0;i<depth;++i){
        const float z0=entryZ+(backZ-entryZ)*float(i)/depth;
        const float z1=entryZ+(backZ-entryZ)*float(i+1)/depth;
        for(int j=0;j<angular;++j){
            const float a=pi*float(j)/angular,b=pi*float(j+1)/angular;
            const P p=ceilingAt(a,z0),q=ceilingAt(b,z0);
            const P r=ceilingAt(b,z1),s=ceilingAt(a,z1);
            emit(p,q,r,16.f);emit(p,r,s,16.f);
        }
        for(int side=-1;side<=1;side+=2) {
            const float a=side<0?pi:0.f;
            const P q0=ceilingAt(a,z0),q1=ceilingAt(a,z1);
            const P floor0{q0.x,ground(q0.x,z0)-.03f,z0};
            const P floor1{q1.x,ground(q1.x,z1)-.03f,z1};
            constexpr int layers=6;
            for(int j=0;j<layers;++j){
                const float t=float(j)/layers,t1=float(j+1)/layers;
                auto lerp=[](P p,P q,float t)->P{
                    return {p.x+(q.x-p.x)*t,p.y+(q.y-p.y)*t,
                            p.z+(q.z-p.z)*t};
                };
                const P v0=lerp(floor0,q0,t),v1=lerp(floor1,q1,t);
                const P w0=lerp(floor0,q0,t1),w1=lerp(floor1,q1,t1);
                emit(v0,v1,w1,16.f);emit(v0,w1,w0,16.f);
            }
        }
    }
    // Far end is a genuine irregular back rock, not a black UI rectangle.
    // The original walkable heightfield remains the floor throughout.
    constexpr int endX=24,endY=20;
    for(int i=0;i<endX;++i){
        float x0=-1.70f+3.40f*i/endX,x1=-1.70f+3.40f*(i+1)/endX;
        for(int j=0;j<endY;++j){
            float t=float(j)/endY,t1=float(j+1)/endY;
            auto endPoint=[&](float x,float v)->P{
                const float bottom=ground(x,backZ);
                const float ceil=tubeCrownY(backZ);
                const float y=bottom+(ceil-bottom)*v;
                return {x,y,backZ+.12f*std::sin(x*3.1f+y*2.1f)};
            };
            P a=endPoint(x0,t),b=endPoint(x1,t);
            P c=endPoint(x1,t1),d=endPoint(x0,t1);
            emit(a,b,c,17.f);emit(a,c,d,17.f);
        }
    }
}
} // namespace taks::CaveVolume
