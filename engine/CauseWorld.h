#pragma once
#include <string>
#include "LivingWorld.h"
#include "Structure.h"
#include "Era.h"
#include <array>
#include <vector>
namespace taks {
// 3D lelőhely: egyediség, anyagtípus, világkoordináta és megmaradó készlet.
struct ResourceNode { int type; double x,z; int amount; };
// Négy önálló munkás; ugyanazt a véges lelőhelyrendszert használják, mint a játékos.
struct Npc {
    double x,z,energy=90.0,hunger=10.0,wetness=0.08;
    int role=0; // 0: fa, 1: kő, 2: levél, 3: gyújtós
    int carrying=0, task=0, delivered=0; // 0 indul,1 lelőhely,2 hazavisz,3 pihen,4 nincs anyag,5 menekül,6 vízhez megy
    double walkPhase=0.0;
    // NPCLEARN1: -1 unseen, -2 other person's unverified report,
    // >=0 personally observed amount (0 = empty). No omniscient map lookup.
    std::array<int,12> rememberedAmounts{{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};
    int discoveries=0,rumorsHeard=0,completedTrips=0,falseLeads=0,wetTrips=0,scoutSteps=0;
    // NPCSURV1: genuine finite-water survival and remembered local predator risk.
    // Memory expires; NPCs never know about unseen predators or invisible water.
    double thirst=18.0, fear=0.0;
    int dangerExperience=0, fleeEvents=0, waterTrips=0;
    double lastDangerX=0.0,lastDangerZ=0.0,dangerMemoryUntil=0.0;
};
struct BuildSite {
    double x=0.0,z=0.0;
    int stage=0;             // 0 kijelölt, 1 oszlop, 2 tetőváz, 3 fedés
    double condition=100.0; // eső okozta kopás, javítással helyreállítható
};
// Egyedi, nyomon követhető tárgy (F1): minden kő és faág saját azonosítót kap.
// where: 0 lelőhelyen, 1 játékosnál, 2 tábori készletben, 4 társ kezében. Elfogyott tárgy kikerül a főkönyvből.
struct Item { unsigned id=0; int type=0; /*0 faág, 1 kő*/ int where=0; int node=-1; int by=-1; };
struct World {
    World(){ rebuildLedgerFromCounts(); } // minden induló kő/faág azonnal egyedi azonosítót kap
    LivingWorld ecology; // canonical living ground, plants and animals
    int groundWood=14, groundStone=8, groundTinder=5;
    int carriedWood=0, carriedStone=0, carriedTinder=0;
    double hours=6.0, humidity=0.40, rainIntensity=0.0;
    double woodMoisture=0.13;
    // Persisted mirror for save/JNI compatibility; ecology.soilWater is canonical.
    double groundMoisture=0.13, stamina=100.0, fireFuel=0.0;
    bool hearthBuilt=false, fireLit=false;
    // Anyagok, feldolgozott alkatrészek és megőrzött munkafázisok.
    int groundLeaves=18, carriedLeaves=0;
    int chippedEdgeWear=0, stoneDebris=0, preparedPoles=0;
    int shelterStage=0; // kompatibilitási mező: a menedékek legmagasabb készültsége
    static constexpr int maxBuildSites=3;
    std::array<BuildSite,maxBuildSites> sites{};
    int siteCount=0;
    Structure structure; // F2: alkatrészes szerkezetek (cölöp, gerenda, kő, deszka)
    double playerWetness=0.10;
    double playerThirst=18.0;
    int waterCollectorStage=0; // 0 nincs, 1 tartóváz, 2 fedett esővízgyűjtő
    double storedRainwater=0.0; // finite liters; capacity 60
    static constexpr double waterCollectorX=1.0;
    static constexpr double waterCollectorZ=-3.4;
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
    // --- Egyedi tárgyak főkönyve (EntityID) ---
    std::vector<Item> items;
    unsigned nextItemId=1;
    unsigned consumedItems[2]={0,0};      // elhasznált faág, kő (története a naplóban)
    std::vector<std::string> causeLog;    // utolsó 64 ok-okozati bejegyzés (nem mentett)
    void ensureLedger();
    bool ledgerConsistent() const;
    unsigned countItems(int type,int where,int node=-1,int by=-1) const;
    // Korszak: a bizonyítékot a világ állapota adja. A mai világban nincs modellezett közösség, gyártási lánc vagy
    // űripar, ezért a bizonyíték nulla: őskor, és a logó rejtett. Az F6 lánc kapcsolja be ténylegesen.
    EraEvidence eraEvidence() const { return EraEvidence(); }
    int currentEra() const { return eraFor(eraEvidence()); }
    bool logoUnlocked() const { return logoUnlockedFor(currentEra()); }
    std::string act(const std::string& action);
    std::string move(double dx,double dz,bool airborne=false);
    std::string nearby() const;
    std::string nextObjective() const;
    std::string npcStatus() const;
    std::string causalReport() const; // actual environment + bounded in-session causal history
    std::string campStatus() const;
    void updateNpcs(double hoursElapsed);
    void syncGroundCounts();
    bool isNear(double x,double z,double radius) const;
    void step(double hoursElapsed);
    std::string summary() const;
    // Derived from saved time, humidity and rain so physics/UI/rendering agree.
    double windStrength() const;
    std::string shelterStatus() const;
    int nearbySiteIndex(double range=2.8) const;
    int nearbySiteStage() const;
    bool protectedAtPlayer() const;
    std::string save() const;
    bool load(const std::string& data);
    void forceRainForTest(double intensity);
    void grantCarriedForTest(int type,int count); // csak tesztelésre: azonosítót is létrehoz
private:
    bool testWeather=false;
    bool ledgerReady=false;
    void rebuildLedgerFromCounts();
    void moveItem(int type,int fromWhere,int fromNode,int fromBy,int toWhere,int toNode,int toBy,const char* cause);
    void consumeItems(int type,int count,const char* cause);
    void logCause(const std::string& line);
};
}
