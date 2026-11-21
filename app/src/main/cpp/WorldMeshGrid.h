#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace taks::WorldMeshGrid {
// A single monotone axis is shared by ALL terrain cells. Do not subdivide
// separate inner/outer meshes: floating point T-junctions cause visible seams
// and jagged, clipped rocks during 360-degree camera rotation.
struct Axes { std::vector<float> xs,zs; };

inline std::vector<float> gradedAxis(
        const std::array<float,4>& stops,
        const std::array<float,3>& targetSteps) {
    std::vector<float> result;
    for(int band=0;band<3;++band){
        const float begin=stops[band],end=stops[band+1];
        const int count=std::max(1,int(std::ceil((end-begin)/targetSteps[band])));
        for(int i=0;i<count;++i)
            result.push_back(begin+(end-begin)*(float(i)/float(count)));
    }
    result.push_back(stops.back());
    return result;
}
inline Axes defaultAxes(){
    return {gradedAxis({-120.f,-28.f,28.f,120.f},{4.f,.5f,4.f}),
            gradedAxis({-120.f,-28.f,40.f,120.f},{4.f,.5f,4.f})};
}
}
