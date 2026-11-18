#pragma once
#include <array>
#include <iosfwd>
#include <string>

namespace taks {

// First systemic ecology layer. One set of persistent organisms and ground
// states drives rendering, gameplay traction, foraging, predation and death.
// All numbers are adjustable GAMEPLAY estimates, not a Wonderwerk fossil survey.
struct LivingWorld {
    static constexpr int grassCount=4;
    static constexpr int treeCount=3;
    static constexpr int animalCount=8;
    struct Grass {
        double x,z;
        double biomass;      // 0..1, food actually available to grazing animals
        double seedBank;     // 0..1, enables regrowth after grazing
        double health;       // 0..1
        double trampling=0;  // persistent flattened/compacted patch state, 0..1
    };
    struct Tree {
        double x,z,heightM,ageDays,health,seeds;
        bool alive;
    };
    struct Animal {
        int kind;             // 0 grazer, 1 predator; species not yet identified
        int sex;              // deterministic breeding pair constraint
        double x,z,ageDays,energy,hunger,thirst,health,breedingDays;
        bool alive;
    };
    std::array<Grass,grassCount> grass{{
        {-2.5,-4.6,.62,.7,.95},{2.25,-4.2,.57,.66,.9},
        {-3.8,.9,.45,.8,.94},{4.4,.5,.53,.62,.91}
    }};
    std::array<Tree,treeCount> trees{{
        {-6.4,-1.3,2.0,690,.90,.68,true},
        {5.7,3.1,1.6,480,.86,.57,true},
        {-5.8,5.7,.7,170,.81,.34,true}
    }};
    std::array<Animal,animalCount> animals{{
        {0,0,-3.8,-2.3,580,75,15,16,100,0,true},
        {0,1, 3.2,-2.1,510,72,18,15,100,0,true},
        {0,0, 1.9, 1.1,160,65,18,18,100,0,true},
        {1,1, 6.6, 0.0,670,78,25,14,100,0,true},
        {0,0, 0.0, 0.0,0,0,0,0,0,0,false},
        {0,0, 0.0, 0.0,0,0,0,0,0,0,false},
        {1,0, 0.0, 0.0,0,0,0,0,0,0,false},
        {1,1, 0.0, 0.0,0,0,0,0,0,0,false}
    }};
    double temperatureC=22.0, mud=0.0, snowDepth=0.0, ice=0.0;
    double soilWater=.13, simulatedDays=0.0, deadAnimals=0.0;
    // A zero value is an unloaded species/knowledge, NOT a visible fern/vine.
    bool verifiedFerns=false, verifiedVines=false;

    void step(double hours,double precipitation,double moisture,double sunlight,double temperature);
    void trampleTrail(double x0,double z0,double x1,double z1,double wetness);
    double walkSpeedFactor() const;
    double slipRisk() const;
    double grassCover() const;
    int aliveAnimals(int kind=-1) const;
    std::string status() const;
    void write(std::ostream& out) const;
    bool read(std::istream& in);
    bool valid() const;
};
}
