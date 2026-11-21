#pragma once
// TAKS World – genuine deterministic three-dimensional terrain for the
// Wonderwerk approach. CPU height is shared between the 3D mesh and every
// character/rock/plant foot. Uses no fake screen-space floor or new PNGs.
//
// The cave entrance used by FastScene is around (0, 3.63) with a 3 m wide
// opening. Keep this area walkable and blend the surrounding dolomite scree
// naturally into the existing cave walls. The detailed geology and
// paleoenvironment must still be validated against field references.
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace taks::WorldTerrain {
inline float clamp01(float x) { return std::clamp(x,0.f,1.f); }
inline float smooth(float a,float b,float x) {
    const float t=clamp01((x-a)/(b-a));
    return t*t*(3.f-2.f*t);
}
inline float hash(int x,int z) {
    std::uint32_t v=std::uint32_t(x)*0x9E3779B9u;
    v^=std::uint32_t(z)*0x85EBCA77u+0xC2B2AE3Du;
    v^=v>>16;v*=0x7FEB352Du;v^=v>>15;v*=0x846CA68Bu;v^=v>>16;
    return float(v&0x00FFFFFFu)/float(0x01000000u);
}
inline float noise(float x,float z) {
    const int ix=int(std::floor(x)), iz=int(std::floor(z));
    const float tx=smooth(0.f,1.f,x-float(ix));
    const float tz=smooth(0.f,1.f,z-float(iz));
    const float a=hash(ix,iz),b=hash(ix+1,iz);
    const float c=hash(ix,iz+1),d=hash(ix+1,iz+1);
    return (a+(b-a)*tx)*(1.f-tz)+(c+(d-c)*tx)*tz;
}
inline float octave(float x,float z) {
    return (.57f*noise(x*.055f,z*.055f)
            +.27f*noise(x*.19f+12.31f,z*.19f-8.42f)
            +.12f*noise(x*.69f-19.17f,z*.69f+6.8f)
            +.04f*noise(x*2.1f+7.f,z*2.1f-13.f))-.50f;
}
inline float gaussian(float x,float z,float cx,float cz,float rx,float rz) {
    const float dx=(x-cx)/rx,dz=(z-cz)/rz;
    return std::exp(-.5f*(dx*dx+dz*dz));
}
inline float height(float x,float z) {
    // Three-dimensional region: weathered Kalahari ground with variable
    // relief, not the old 160 m flat billboard or sinusoidal brown grid.
    const float regional=-.43f+.93f*octave(x,z);
    const float edgeRidge=.15f*(noise(x*.022f+31.f,z*.022f)-.5f);

    // Both outer rocky shoulders stand on the same slope as the cave entrance.
    // Rounded plateaus transition gradually to fallen dolomite scree.
    const float shoulderLeft=.97f*gaussian(x,z,-6.8f,6.2f,3.4f,7.1f);
    const float shoulderRight=1.04f*gaussian(x,z,6.5f,6.8f,3.7f,7.5f);
    const float limestoneFoot=.33f*gaussian(x,z,0.f,8.7f,8.4f,9.5f);
    const float scree=.19f*(noise(x*.72f+31.f,z*.72f+9.f)-.5f)*
        (gaussian(x,z,-7.f,5.f,5.f,7.f)+gaussian(x,z,7.f,5.f,5.f,7.f));

    // Continuous geological hill flank, not square terrain blocks.
    // A PROCEDURAL approximation of the slopes around Wonderwerk, not
    // surveyed or archaeologically verified digital elevation data.
    // Start the rise behind the cave to keep the original entrance passable.
    const float farSlope=smooth(9.f,43.f,z);
    const float kopje=15.8f*gaussian(x,z,-9.f,56.f,29.f,38.f)*farSlope;
    const float westernRidge=9.2f*gaussian(x,z,-56.f,42.f,31.f,56.f)*
                             smooth(11.f,48.f,z);
    const float distantShoulder=5.8f*gaussian(x,z,69.f,75.f,43.f,51.f)*
                                smooth(21.f,70.f,z);

    // Natural-scale relief and eroded gravel; smooth continuous noise,
    // without arbitrary hard edges or a flat material-patch polygon.
    const float moundNoise=noise(x*.14f+51.7f,z*.14f-31.1f)-.5f;
    const float microHummocks=.44f*moundNoise+
        .14f*(noise(x*.36f-21.3f,z*.36f+11.7f)-.5f);
    const float gravelSlope=.21f*(noise(x*.065f+11.f,z*.065f-8.f)-.5f);

    // The player can walk from the starting ground along the cave approach.
    // Its approximately 3 m wide opening is not buried by an arbitrary hill.
    const float approach=gaussian(x,z,0.f,.2f,2.0f,7.1f);
    const float corridor=gaussian(x,z,0.f,5.1f,1.9f,4.6f);
    const float approachTarget=-.10f+.04f*noise(x*.66f+2.f,z*.66f);
    float y=regional+edgeRidge+shoulderLeft+shoulderRight+limestoneFoot+scree
           +kopje+westernRidge+distantShoulder+microHummocks+gravelSlope;
    y=y*(1.f-.88f*approach)+approachTarget*(.88f*approach);
    y=y*(1.f-.98f*corridor)+(-.045f+.012f*noise(x*.75f,z*.75f))*(.98f*corridor);

    // Meandering runoff depression, distinct from the cave walkable door.
    const float washX=-9.2f+.087f*(z+10.f)
                      +.7f*std::sin(z*.12f+2.f);
    const float wash=gaussian(x,z,washX,-9.5f,1.15f,22.f);
    y-=.21f*wash*(1.f-.90f*corridor);
    return y;
}
inline void normal(float x,float z,float& nx,float& ny,float& nz) {
    constexpr float delta=.09f;
    const float dhdx=(height(x+delta,z)-height(x-delta,z))/(2.f*delta);
    const float dhdz=(height(x,z+delta)-height(x,z-delta))/(2.f*delta);
    const float inv=1.f/std::sqrt(dhdx*dhdx+1.f+dhdz*dhdz);
    nx=-dhdx*inv;ny=inv;nz=-dhdz*inv;
}
inline float limestoneMix(float x,float z,float ny) {
    const float exposed=gaussian(x,z,-6.7f,6.2f,5.f,8.f)
                       +gaussian(x,z,6.5f,6.8f,5.5f,8.5f)
                       +.43f*gaussian(x,z,0.f,9.1f,9.f,12.f);
    const float gravel=clamp01(exposed*.72f + (1.f-ny)*1.35f);
    const float entrance=gaussian(x,z,0.f,3.5f,2.6f,6.8f);
    return clamp01(gravel*(1.f-.82f*entrance));
}
} // namespace taks::WorldTerrain
