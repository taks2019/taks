#include "../../app/src/main/cpp/RoofCutaway.h"
#include <cmath>
#include <iostream>
#include <limits>
int main() {
    using taks::RoofCutaway::visibility;
    auto require=[](bool check,const char* what){if(!check){std::cerr<<what<<"\n";return false;}return true;};
    if(!require(visibility(6.f)==0.f,"Close zoom: roof must be invisible")) return 1;
    if(!require(visibility(8.f)==0.f,"Hidden threshold")) return 1;
    if(!require(visibility(11.f)==.5f,"Mid-zoom: progressive roof reveal")) return 1;
    if(!require(visibility(14.f)==1.f,"Far zoom: roof intact")) return 1;
    if(!require(visibility(26.f)==1.f,"Maximum distant zoom")) return 1;
    if(!require(visibility(std::numeric_limits<float>::quiet_NaN())==1.f,"NaN camera keeps roof")) return 1;
    if(!require(visibility(std::numeric_limits<float>::infinity())==1.f,"Invalid camera keeps roof")) return 1;
    float previous=0.f;
    for(int i=0;i<=200;++i){
        float z=6.f+20.f*float(i)/200.f;
        float alpha=visibility(z);
        if(!require(alpha>=0.f&&alpha<=1.f&&alpha>=previous,"Visibility must be monotonic and bounded"))return 1;
        previous=alpha;
    }
    std::cout<<"Roof cutaway zoom regression passed\n";
    return 0;
}
