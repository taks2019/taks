#pragma once
#include <string>
namespace taks {
struct World {
    int groundWood=14, groundStone=8, groundTinder=5;
    int carriedWood=0, carriedStone=0, carriedTinder=0;
    double hours=6.0, humidity=0.40, rainIntensity=0.0;
    double woodMoisture=0.13, groundMoisture=0.13, stamina=100.0, fireFuel=0.0;
    bool hearthBuilt=false, fireLit=false;
    // Anyagok, feldolgozott alkatrészek és megőrzött munkafázisok.
    int groundLeaves=18, carriedLeaves=0;
    int chippedEdgeWear=0, stoneDebris=0, preparedPoles=0;
    int shelterStage=0; // 0: üres hely, 1: oszlopok, 2: tetőváz, 3: fedett menedék
    double playerWetness=0.10;
    bool underShelter=false;
    unsigned long eventCounter=0;
    std::string act(const std::string& action);
    void step(double hoursElapsed);
    std::string summary() const;
    std::string shelterStatus() const;
    std::string save() const;
    bool load(const std::string& data);
    void forceRainForTest(double intensity);
private:
    bool testWeather=false;
};
}
