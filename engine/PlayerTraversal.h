#pragma once
#include <algorithm>
#include <cmath>
namespace taks {
// A continuous, transient ballistic arc. No invented hop loop or save-format
// changes. Only explicit player input launches a new jump.
class PlayerJump {
public:
    static constexpr double gravity=10.8;
    bool airborne(double seconds) const {
        return active_ && std::isfinite(seconds) && seconds>=start_ &&
               seconds-start_<2.0*velocity_/gravity;
    }
    double height(double seconds) const {
        if(!airborne(seconds))return 0.0;
        const double t=seconds-start_;
        return std::max(0.0,velocity_*t-.5*gravity*t*t);
    }
    bool launch(double seconds,double initialVelocity){
        if(!std::isfinite(seconds)||!std::isfinite(initialVelocity)||
           initialVelocity<1.0||initialVelocity>6.0||airborne(seconds))return false;
        start_=seconds;velocity_=initialVelocity;active_=true;
        return true;
    }
    void reset(){active_=false;start_=0.0;velocity_=0.0;}
private:
    double start_=0.0,velocity_=0.0;
    bool active_=false;
};
} // namespace taks
