#include "../app/src/main/cpp/CaveVolumeReview.h"
#include "../app/src/main/cpp/WorldTerrain.h"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>

int main() {
    unsigned long total=0,face=0,tunnel=0,end=0;
    float minZ=1000.f,maxZ=-1000.f;
    unsigned long blockingMouth=0;
    auto emit=[&](taks::CaveVolume::P a,taks::CaveVolume::P b,
                  taks::CaveVolume::P c,float material) {
        ++total;
        if(material==1.f)++face;
        if(material==16.f)++tunnel;
        if(material==17.f)++end;
        for(const auto& p : {a,b,c}){
            assert(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z));
            assert(p.x>=-9.5f&&p.x<=9.5f);
            assert(p.y>-2.0f&&p.y<6.2f);
            minZ=std::min(minZ,p.z);maxZ=std::max(maxZ,p.z);
        }
        // No solid front-wall triangles may cover the footpath through
        // the entrance: a player at the center must see the interior.
        if(material==1.f){
            const float cx=(a.x+b.x+c.x)/3.f;
            const float cy=(a.y+b.y+c.y)/3.f;
            if(std::abs(cx)<1.10f && cy<1.05f)++blockingMouth;
        }
    };
    auto height=[](float x,float z) {return taks::WorldTerrain::height(x,z);};
    taks::CaveVolume::build(emit,height);
    assert(total>15000 && total<70000);
    assert(face>3000 && tunnel>5000 && end>500);
    assert(blockingMouth==0);
    assert(minZ<3.6f&&maxZ>11.6f);
    assert(taks::CaveVolume::archCeiling(0.f)>2.7f);
    assert(taks::CaveVolume::archCeiling(1.75f)<2.0f);
    std::cout<<"Cave volume geometry OK: "<<total<<" triangles, "
             <<face<<" external, "<<tunnel<<" interior, "<<end
             <<" rear; open entrance verified\n";
}
