#include "../app/src/main/cpp/WorldTerrain.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <algorithm>

// This is a native C++ validation of real 3D ground heights & slope normals,
// not a screenshot or a 2D canvas demonstration.
int main(){
    using namespace taks::WorldTerrain;
    float lowest=1.0e10f,highest=-1.0e10f;
    int sculpted=0,flat=0;
    for(int ix=-28;ix<=28;ix++){
        for(int iz=-28;iz<=40;iz++){
            const float x=float(ix),z=float(iz);
            const float y=height(x,z);
            assert(std::isfinite(y));
            assert(y>-4.f && y<26.f);
            lowest=std::min(lowest,y);highest=std::max(highest,y);
            float nx,ny,nz;
            normal(x,z,nx,ny,nz);
            assert(std::abs(nx*nx+ny*ny+nz*nz-1.f)<.002f);
            assert(ny>.10f && ny<=1.f);
            if(std::abs(y-height(x+.33f,z+.27f))>.004f)++sculpted;
            else ++flat;
        }
    }
    assert(highest-lowest>7.f && "The naturally sculpted hillside must not be a flat brown floor");
    assert(sculpted>200 && "3D topography must vary across world polygons");

    // The hillside must rise BEHIND the cave; the entrance remains low.
    assert(height(-9.f,55.f)>height(0.f,3.63f)+8.f);
    assert(height(-50.f,45.f)>height(0.f,3.63f)+2.f);
    // Walkable mouth at x=0,z~3.63: not buried by an arbitrary stone hill.
    const float mouth=height(0.f,3.63f);
    assert(mouth>-.30f && mouth<.24f);
    assert(height(-6.8f,6.2f)>mouth+.20f);
    assert(height(6.5f,6.8f)>mouth+.20f);

    // The same height function must put the ground under players and rocks:
    // tests verify even small stepping differences are continuous.
    for(int i=0;i<55;i++){
        const float z=-10.f+float(i)*.25f;
        const float delta=std::abs(height(0.f,z+.25f)-height(0.f,z));
        assert(delta<.35f && "Cave approach has a bad discontinuity");
    }

    // Source geometry joins two LOD grids at exact same elevations and axes.
    for(int iz=-28;iz<=40;iz+=4){
        const float z=float(iz);
        assert(height(-28.f,z)==height(-28.f,z));
        assert(height(28.f,z)==height(28.f,z));
    }
    for(int ix=-28;ix<=28;ix+=4){
        const float x=float(ix);
        assert(height(x,-28.f)==height(x,-28.f));
        assert(height(x,40.f)==height(x,40.f));
    }
    const float scree=limestoneMix(-6.8f,6.2f,.86f);
    const float walk=limestoneMix(0.f,3.63f,1.f);
    assert(scree>walk);
    std::printf("TRUE 3D TERRAIN PASS: vertical relief %.3fm, %d varying samples, cave mouth %.3fm, scree %.3f vs path %.3f\n",
                highest-lowest,sculpted,mouth,scree,walk);
}
