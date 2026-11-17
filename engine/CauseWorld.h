#pragma once
#include <string>
#include <array>
namespace taks {
// 3D lelőhely: egyediség, anyagtípus, világkoordináta és megmaradó készlet.
struct ResourceNode { int type; double x,z; int amount; };
// Négy önálló munkás; ugyanazt a véges lelőhelyrendszert használják, mint a játékos.
struct Npc {
    double x,z,energy=90.0,hunger=10.0,wetness=0.08;
    int role=0; // 0: fa, 1: kő, 2: levél, 3: gyújtós
    int carrying=0, task=0, delivered=0; // task: 0 indul,1 odamegy,2 hazavisz,3 pihen,4 nincs anyag
    double walkPhase=0.0;
};
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
    double playerX=0.0,playerZ=-5.2,walkPhase=0.0;
    static constexpr int resourceCount=12;
    std::array<ResourceNode,resourceCount> nodes{{
        {0,-2.0,-5.5,3},{0,3.0,-4.0,3},{0,-4.0,-1.2,4},{0,5.0,0.5,4},
        {1,1.0,-5.4,3},{1,3.0,-0.8,3},{1,-2.0,1.5,2},
        {2,-1.0,-3.9,2},{2,4.0,1.1,3},
        {3,-2.8,-4.0,6},{3,4.1,-3.3,6},{3,-4.0,0.0,6}
    }};
    static constexpr int npcCount=4;
    std::array<Npc,npcCount> npcs{{
        {-3.6,-2.1,90,10,.08,0,0,0,0,0.0},
        {2.7,-1.1,89,15,.08,1,0,0,0,0.0},
        {4.4,-2.0,90,12,.08,2,0,0,0,0.0},
        {-1.8,-2.4,88,13,.08,3,0,0,0,0.0}
    }};
    std::array<int,4> campStock{{0,0,0,0}};
    double npcSimulatedHours=0.0;
    unsigned long eventCounter=0;
    std::string act(const std::string& action);
    std::string move(double dx,double dz);
    std::string nearby() const;
    std::string nextObjective() const;
    std::string npcStatus() const;
    std::string campStatus() const;
    void updateNpcs(double hoursElapsed);
    void syncGroundCounts();
    bool isNear(double x,double z,double radius) const;
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
