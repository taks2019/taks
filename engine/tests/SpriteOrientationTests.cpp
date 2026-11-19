#include "../SpriteOrientation.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
    using taks::SpriteOrientation::cameraRelativeDirection;
    using taks::SpriteOrientation::pi;
    // The camera encircles a stationary actor by 45 degrees per sprite.
    for(int i=0;i<8;i++){
        const float yaw=pi-i*pi/4.f;
        const int actual=cameraRelativeDirection(yaw,0.f);
        assert(actual==((i+4)%8));
    }
    // Actor turns while camera remains fixed; the image rotates oppositely.
    for(int i=0;i<8;i++){
        const int actual=cameraRelativeDirection(pi,i*pi/4.f);
        assert(actual==(8-i)%8);
    }
    assert(cameraRelativeDirection(0,pi)==0); // actor facing camera
    assert(cameraRelativeDirection(0,0)==4); // actor back to camera
    assert(cameraRelativeDirection(0,-pi/2.f)==2);
    assert(cameraRelativeDirection(0,pi/2.f)==6);
    assert(cameraRelativeDirection(100000*pi,0)>=0);
    assert(cameraRelativeDirection(NAN,0)==0);
    assert(cameraRelativeDirection(0,NAN)==0);
    assert(std::string(taks::SpriteOrientation::directionName(5))=="SW");
    std::cout<<"OK: eight actual sprite views; camera and character rotation; wraparound; NaN.\n";
}
