#pragma once
#include <string>
namespace taks {
struct World {
    int groundWood=14, groundStone=8, groundTinder=5;
    int carriedWood=0, carriedStone=0, carriedTinder=0;
    double hours=6.0, humidity=0.40, rainIntensity=0.0;
    double woodMoisture=0.13, groundMoisture=0.13, stamina=100.0, fireFuel=0.0;
    bool hearthBuilt=false, fireLit=false;
    unsigned long eventCounter=0;
    std::string act(const std::string& action);
    void step(double hoursElapsed);
    std::string summary() const;
    std::string save() const;
    bool load(const std::string& data);
    void forceRainForTest(double intensity);
private:
    bool testWeather=false;
};
}
