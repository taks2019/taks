#include "LivingWorld.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <istream>
#include <ostream>
#include <sstream>

namespace taks {
namespace {
double bound(double x,double lo,double hi){return std::max(lo,std::min(hi,x));}
bool number(double x,double lo,double hi){return std::isfinite(x)&&x>=lo&&x<=hi;}
double sq(double x,double z){return x*x+z*z;}
}

double LivingWorld::walkSpeedFactor() const {
    // Wet mud clings and slows everybody; snow and ice reduce traction.
    return bound(1.0-.42*mud-.28*bound(snowDepth/.30,0,1)-.27*ice,.25,1.);
}
double LivingWorld::slipRisk() const {
    return bound(.08*mud+.44*ice+.14*bound(snowDepth/.30,0,1),0,1);
}
double LivingWorld::grassCover() const {
    double sum=0;
    for(const auto& g:grass)sum+=g.biomass;
    return sum/grass.size();
}
int LivingWorld::visibleFootprintCount() const {
    int count=0;
    for(const auto& mark:footprints)if(mark.intensity>.025)++count;
    return count;
}
int LivingWorld::nearestWaterBody(double x,double z,double maxDistance) const {
    if(!number(x,-1000,1000)||!number(z,-1000,1000)||
       !number(maxDistance,0,2000))return -1;
    const double limit=maxDistance*maxDistance;
    double best=limit;
    int chosen=-1;
    for(int i=0;i<waterBodyCount;++i){
        const auto& water=waters[i];
        if(water.level<=.015)continue;
        const double dx=x-water.x,dz=z-water.z,distance=dx*dx+dz*dz;
        if(distance<=best){best=distance;chosen=i;}
    }
    return chosen;
}
void LivingWorld::stampFootstep(double x,double z,double heading) {
    if(!number(x,-1000,1000)||!number(z,-1000,1000)||
       !number(heading,-1000000,1000000)||mud<.24)return;
    const int previous=(nextFootprint+footprintCount-1)%footprintCount;
    const auto& last=footprints[previous];
    const double dx=x-last.x,dz=z-last.z;
    // Movement updates arrive at frame rate. Leave a natural stride gap rather
    // than stacking dozens of impressions at one point.
    if(last.intensity>.025&&dx*dx+dz*dz<.34*.34)return;
    const double side=(nextFootprint%2==0)?-.5:.5;
    const double sideX=std::cos(heading),sideZ=-std::sin(heading);
    Footprint& mark=footprints[nextFootprint];
    mark.x=x+sideX*.12*side;
    mark.z=z+sideZ*.12*side;
    mark.heading=heading;
    mark.intensity=bound(mud,0,1);
    nextFootprint=(nextFootprint+1)%footprintCount;
}
int LivingWorld::aliveAnimals(int kind) const {
    int result=0;
    for(const auto& a:animals)if(a.alive&&(kind<0||a.kind==kind))++result;
    return result;
}
void LivingWorld::step(double hours,double precipitation,double moistureTarget,double sunlight,
                       double temperature,double windStrength){
    if(!number(hours,0,24)||hours==0||!number(precipitation,0,1)||
       !number(moistureTarget,0,1)||!number(sunlight,0,1)||!number(temperature,-60,60)||
       !number(windStrength,0,1))return;
    const double days=hours/24.0;
    simulatedDays+=days;
    temperatureC=temperature;
    soilWater=bound(soilWater+.30*precipitation*days
                   +.25*(moistureTarget-soilWater)*days
                   -.065*(.4+sunlight)*(1.0+.34*windStrength)*days,0,1);
    if(temperature<=-.5){
        snowDepth=bound(snowDepth+precipitation*.18*days,0,1.5);
        ice=bound(ice+(snowDepth>.03?.28:.04)*days,0,1);
    }else{
        const double thaw=temperature/9.0*days;
        snowDepth=bound(snowDepth-thaw,0,1.5);
        ice=bound(ice-thaw*.75,0,1);
    }
    mud=bound((soilWater-.27)*1.7+precipitation*.16-ice*.15,0,1);
    // Rain, excess surface runoff, groundwater and evaporation share one finite
    // water state. The second basin is seasonal; only the first has a weak seep.
    const double runoff=std::max(0.0,soilWater-.32)*.28*days;
    for(auto& water:waters){
        const double rainIn=precipitation*(water.source==0?.24:.34)*days;
        const double seep=water.source==0?.009*bound((soilWater-.12)/.50,0,1)*days:0.0;
        const double runoffIn=runoff*(water.source==0?.22:.55);
        const double evaporation=(.004+sunlight*.020+
                                  std::max(0.0,temperature)*.00012)*
                                  (1.0+.30*windStrength)*days;
        water.level=bound(water.level+rainIn+seep+runoffIn-evaporation,0,1);
        const double sediment=runoffIn*.16+precipitation*days*.006;
        water.quality=bound(water.quality+precipitation*days*.004-
                            sediment-days*.0005,.05,1);
    }
    // Footprints are a reversible ground effect, not permanent terrain paint.
    // Dry sunlight removes them gradually; strong later rain softens them faster.
    for(auto& mark:footprints){
        if(mark.intensity<=0)continue;
        const double fadePerHour=.0015+sunlight*.002+precipitation*.015+(mud<.18?.012:0.);
        mark.intensity=bound(mark.intensity-hours*fadePerHour,0,1);
    }
    // Grass remains a finite living food store: grazing removes biomass.
    for(auto& g:grass){
        const double warmth=bound((temperature+3)/21,0,1);
        const double growth=days*.085*sunlight*warmth
                           *(soilWater>.15?1.:.28)*g.health*g.seedBank
                           *(1.0-.55*g.trampling);
        const double stress=days*((soilWater<.10?.025:0.)+
                                  (temperature<0?.013:0.)+
                                  (temperature>40?.020:0.)+
                                  (soilWater<.20&&windStrength>.65?.012*(windStrength-.65):0.));
        g.biomass=bound(g.biomass+growth-stress,0,1);
        g.health=bound(g.health+days*(soilWater>.22?.002:-.004),0,1);
        g.seedBank=bound(g.seedBank+days*(g.biomass>.4?.001:-.002),0,1);
        // A trampled patch heals faster when soil water can support new shoots.
        g.trampling=bound(g.trampling-days*(soilWater>.20?.035:.009),0,1);
    }
    // Trees gain actual height/age only while living and with soil resources.
    for(auto& tree:trees){
        if(!tree.alive)continue;
        tree.ageDays+=days;
        const double suitable=soilWater>.17&&temperature>3&&temperature<39?1.:0.;
        tree.heightM=bound(tree.heightM+days*.004*suitable*sunlight*tree.health,0,30);
        tree.seeds=bound(tree.seeds+days*.0008*suitable,0,1);
        const double dryWindStress=(soilWater<.16&&windStrength>.72)?
                                   .0018*(windStrength-.72):0.;
        tree.health=bound(tree.health+days*(suitable?.0006:-.006)-days*dryWindStress,0,1);
        if(tree.health<=0){tree.alive=false;}
    }
    // Animals consume only available biomass. Predators cannot conjure prey.
    for(auto& a:animals){
        if(!a.alive)continue;
        a.ageDays+=days;
        a.breedingDays+=days;
        a.hunger=bound(a.hunger+days*6.0,0,100);
        a.thirst=bound(a.thirst+days*8.0*(1.0+.22*windStrength),0,100);
        a.energy=bound(a.energy-days*1.5,0,100);
        if(a.thirst>35){
            const int waterIndex=nearestWaterBody(a.x,a.z);
            if(waterIndex>=0){
                auto& water=waters[waterIndex];
                const double dx=water.x-a.x,dz=water.z-a.z;
                const double distance=std::sqrt(dx*dx+dz*dz);
                const double stoppingDistance=std::max(.48,water.radius*.58);
                if(distance>stoppingDistance){
                    const double move=std::min(distance-stoppingDistance,
                                               hours*2.2*walkSpeedFactor());
                    if(distance>0){a.x+=dx/distance*move;a.z+=dz/distance*move;}
                    a.energy=bound(a.energy-move*.45,0,100);
                }else{
                    const double drink=std::min(water.level,
                                                std::min(.18,days*.10+a.thirst*.001));
                    water.level=bound(water.level-drink,0,1);
                    a.thirst=bound(a.thirst-days*42.0,0,100);
                    a.energy=bound(a.energy+days*2.0,0,100);
                }
                continue;
            }
        }
        if(a.kind==0){
            int target=-1;double best=1e12;
            for(int i=0;i<grassCount;++i){
                if(grass[i].biomass<=.001)continue;
                double d=sq(grass[i].x-a.x,grass[i].z-a.z);
                if(d<best){best=d;target=i;}
            }
            if(target>=0){
                Grass& g=grass[target];
                double d=std::sqrt(best);
                if(d>.75){
                    const double move=std::min(d-.65,hours*2.2*walkSpeedFactor());
                    if(d>0){a.x+=(g.x-a.x)/d*move;a.z+=(g.z-a.z)/d*move;}
                    a.energy=bound(a.energy-move*.5,0,100);
                }else{
                    const double eaten=std::min(g.biomass,days*.20);
                    g.biomass-=eaten;
                    a.hunger=bound(a.hunger-eaten*85,0,100);
                    a.energy=bound(a.energy+eaten*35,0,100);
                }
            }
        }else{
            int prey=-1;double best=1e12;
            for(int i=0;i<animalCount;++i){
                const auto& b=animals[i];
                if(!b.alive||b.kind!=0)continue;
                const double d=sq(b.x-a.x,b.z-a.z);
                if(d<best){best=d;prey=i;}
            }
            if(prey>=0){
                auto& target=animals[prey];
                double d=std::sqrt(best);
                if(d>.72){
                    const double move=std::min(d-.6,hours*2.9*walkSpeedFactor());
                    if(d>0){a.x+=(target.x-a.x)/d*move;a.z+=(target.z-a.z)/d*move;}
                    a.energy=bound(a.energy-move*.7,0,100);
                }else if(a.hunger>32){
                    target.health=bound(target.health-days*48,0,100);
                    a.hunger=bound(a.hunger-days*50,0,100);
                    a.energy=bound(a.energy+days*16,0,100);
                    if(target.health<=0){target.alive=false;deadAnimals+=1;}
                }
            }
        }
        // Surface water is a finite *condition*, not a magical drink source:
        // high soil moisture is not automatically potable drinking water.
        if(a.hunger>80)a.health=bound(a.health-(a.hunger-80)*days*.20,0,100);
        if(a.thirst>85)a.health=bound(a.health-(a.thirst-85)*days*.30,0,100);
        if(a.energy<8)a.health=bound(a.health-days*1.0,0,100);
        if(a.health<=0&&a.alive){a.alive=false;deadAnimals+=1;}
    }
    // No spontaneous generation: reproduction requires living adults, two sexes,
    // food availability, time and a free ecological capacity slot.
    if(grassCover()>.28){
        for(int kind=0;kind<=1;++kind){
            int male=-1,female=-1,slot=-1;
            for(int i=0;i<animalCount;++i){
                auto& a=animals[i];
                if(!a.alive){if(slot<0)slot=i;continue;}
                if(a.kind!=kind||a.ageDays<240||a.breedingDays<70||a.hunger>55||
                   a.health<60)continue;
                if(a.sex==0&&male<0)male=i;
                if(a.sex==1&&female<0)female=i;
            }
            if(male>=0&&female>=0&&slot>=0){
                const Animal mother=animals[female];
                // Young animal starts weak and hungry, not an adult clone.
                animals[slot]={kind,slot%2,mother.x+.3,mother.z-.2,0,28,35,32,72,0,true};
                animals[male].breedingDays=0;
                animals[female].breedingDays=0;
                animals[female].energy=bound(animals[female].energy-15,0,100);
            }
        }
    }
}
void LivingWorld::trampleTrail(double x0,double z0,double x1,double z1,double wetness){
    if(!number(x0,-1000,1000)||!number(z0,-1000,1000)||
       !number(x1,-1000,1000)||!number(z1,-1000,1000)||!number(wetness,0,1))return;
    const double dx=x1-x0,dz=z1-z0,length=std::hypot(dx,dz);
    if(length<.00001)return;
    constexpr double patchRadius=2.05;
    for(auto& g:grass){
        const double denominator=dx*dx+dz*dz;
        const double t=denominator>0?bound(((g.x-x0)*dx+(g.z-z0)*dz)/denominator,0,1):0;
        const double px=x0+t*dx,pz=z0+t*dz;
        const double distance=std::hypot(g.x-px,g.z-pz);
        if(distance>=patchRadius||g.biomass<=0)continue;
        const double contact=1.0-distance/patchRadius;
        const double impact=length*.115*contact*(1.0+.85*wetness);
        g.trampling=bound(g.trampling+impact,0,1);
        // Flattening crushes some living blades; it does not create harvested food.
        g.biomass=bound(g.biomass-impact*.10,0,1);
    }
}
std::string LivingWorld::status() const {
    std::ostringstream os;os<<std::fixed<<std::setprecision(2);
    os<<"ÉLŐVILÁG: fűállomány "<<grassCover()*100.<<"% · élő fák ";
    int livingTrees=0;for(const auto& t:trees)if(t.alive)++livingTrees;
    int flattened=0;for(const auto& g:grass)if(g.trampling>.05)++flattened;
    os<<livingTrees<<"/"<<treeCount<<" · letaposott fűfoltok "<<flattened<<"/"<<grassCount
      <<" · növényevők "<<aliveAnimals(0)
      <<" · látható lábnyomok "<<visibleFootprintCount()<<" · ragadozók "<<aliveAnimals(1)<<" · elhullott "<<deadAnimals
      <<"\nTALAJ: nedvesség "<<soilWater*100.<<"% · sár "<<mud*100.
      <<"% · hó "<<snowDepth<<" m · jég "<<ice*100.
      <<"% · mozgássebesség "<<walkSpeedFactor()*100.
      <<"%\nFELSZÍNI VÍZ: forrásmedence "<<waters[0].level*100.
      <<"% (tisztaság "<<waters[0].quality*100.
      <<"%) · időszakos medence "<<waters[1].level*100.
      <<"% (tisztaság "<<waters[1].quality*100.<<"%)";
    return os.str();
}
void LivingWorld::write(std::ostream& out) const {
    out<<" ECO4 "<<temperatureC<<' '<<mud<<' '<<snowDepth<<' '<<ice
       <<' '<<soilWater<<' '<<simulatedDays<<' '<<deadAnimals
       <<' '<<(verifiedFerns?1:0)<<' '<<(verifiedVines?1:0);
    for(const auto& g:grass)out<<' '<<g.x<<' '<<g.z<<' '<<g.biomass<<' '<<g.seedBank<<' '<<g.health;
    for(const auto& t:trees)out<<' '<<t.x<<' '<<t.z<<' '<<t.heightM<<' '<<t.ageDays
                                <<' '<<t.health<<' '<<t.seeds<<' '<<(t.alive?1:0);
    for(const auto& a:animals)out<<' '<<a.kind<<' '<<a.sex<<' '<<a.x<<' '<<a.z
        <<' '<<a.ageDays<<' '<<a.energy<<' '<<a.hunger<<' '<<a.thirst
        <<' '<<a.health<<' '<<a.breedingDays<<' '<<(a.alive?1:0);
    out<<' '<<nextFootprint;
    for(const auto& mark:footprints)
        out<<' '<<mark.x<<' '<<mark.z<<' '<<mark.heading<<' '<<mark.intensity;
    for(const auto& water:waters)
        out<<' '<<water.x<<' '<<water.z<<' '<<water.radius<<' '<<water.level
           <<' '<<water.quality<<' '<<water.source;
    for(const auto& g:grass)out<<' '<<g.trampling;
}
bool LivingWorld::read(std::istream& in,SaveFormat* loadedFormat){
    LivingWorld copy;
    std::string version;
    int fern=0,vine=0;
    if(!(in>>version)||(version!="ECO1"&&version!="ECO2"&&version!="ECO3"&&version!="ECO4"))return false;
    if(!(in>>copy.temperatureC>>copy.mud>>copy.snowDepth>>copy.ice
         >>copy.soilWater>>copy.simulatedDays>>copy.deadAnimals>>fern>>vine))return false;
    if((fern!=0&&fern!=1)||(vine!=0&&vine!=1))return false;
    copy.verifiedFerns=fern;copy.verifiedVines=vine;
    for(auto& g:copy.grass)if(!(in>>g.x>>g.z>>g.biomass>>g.seedBank>>g.health))return false;
    for(auto& t:copy.trees){
        int alive=0;
        if(!(in>>t.x>>t.z>>t.heightM>>t.ageDays>>t.health>>t.seeds>>alive))return false;
        if(alive!=0&&alive!=1)return false;
        t.alive=alive!=0;
    }
    for(auto& a:copy.animals){
        int alive=0;
        if(!(in>>a.kind>>a.sex>>a.x>>a.z>>a.ageDays>>a.energy>>a.hunger
              >>a.thirst>>a.health>>a.breedingDays>>alive))return false;
        if(alive!=0&&alive!=1)return false;
        a.alive=alive!=0;
    }
    // Both development lines used ECO2. Its short form ends after the four
    // trampling values (at EOF or the ledger tag). The long form contains
    // the footprint cursor and 32 records. Later formats are unambiguous.
    SaveFormat format=SaveFormat::Initial;
    int firstMark=0;
    if(version=="ECO2"){
        std::array<double,4> first{};
        for(auto& value:first)if(!(in>>value))return false;
        in>>std::ws;
        if(in.eof() || in.peek()=='L'){
            format=SaveFormat::TrampledGrass;
            for(std::size_t i=0;i<first.size();++i)copy.grass[i].trampling=first[i];
        }else{
            format=SaveFormat::Footprints;
            if(!number(first[0],0,footprintCount-1)||std::floor(first[0])!=first[0])return false;
            copy.nextFootprint=static_cast<int>(first[0]);
            copy.footprints[0]={first[1],first[2],first[3],0.0};
            if(!(in>>copy.footprints[0].intensity))return false;
            firstMark=1;
        }
    }else if(version=="ECO3"||version=="ECO4"){
        format=version=="ECO4"?SaveFormat::Combined:SaveFormat::Water;
        if(!(in>>copy.nextFootprint))return false;
    }
    if(format==SaveFormat::Footprints||format==SaveFormat::Water||format==SaveFormat::Combined){
        for(int i=firstMark;i<footprintCount;++i){
            auto& mark=copy.footprints[i];
            if(!(in>>mark.x>>mark.z>>mark.heading>>mark.intensity))return false;
        }
    }
    if(format==SaveFormat::Water||format==SaveFormat::Combined){
        for(auto& water:copy.waters)
            if(!(in>>water.x>>water.z>>water.radius>>water.level>>water.quality>>water.source))return false;
    }
    if(format==SaveFormat::Combined)
        for(auto& g:copy.grass)if(!(in>>g.trampling))return false;
    if(!copy.valid())return false;
    if(loadedFormat)*loadedFormat=format;
    *this=copy;
    return true;
}
bool LivingWorld::valid() const {
    if(!number(temperatureC,-60,60)||!number(mud,0,1)||
       !number(snowDepth,0,1.5)||!number(ice,0,1)||!number(soilWater,0,1)||
       !number(simulatedDays,0,1e9)||!number(deadAnimals,0,1e8)||
       nextFootprint<0||nextFootprint>=footprintCount)return false;
    for(const auto& g:grass){
        if(!number(g.x,-1000,1000)||!number(g.z,-1000,1000)||
           !number(g.biomass,0,1)||!number(g.seedBank,0,1)||!number(g.health,0,1)||
           !number(g.trampling,0,1))return false;
    }
    for(const auto& t:trees){
        if(!number(t.x,-1000,1000)||!number(t.z,-1000,1000)||
           !number(t.heightM,0,30)||!number(t.ageDays,0,1e9)||
           !number(t.health,0,1)||!number(t.seeds,0,1))return false;
    }
    for(const auto& a:animals){
        if((a.kind!=0&&a.kind!=1)||(a.sex!=0&&a.sex!=1)||
           !number(a.x,-1000,1000)||!number(a.z,-1000,1000)||
           !number(a.ageDays,0,1e9)||!number(a.energy,0,100)||
           !number(a.hunger,0,100)||!number(a.thirst,0,100)||
           !number(a.health,0,100)||!number(a.breedingDays,0,1e9))return false;
    }
    for(const auto& mark:footprints){
        if(!number(mark.x,-1000,1000)||!number(mark.z,-1000,1000)||
           !number(mark.heading,-1000000,1000000)||!number(mark.intensity,0,1))return false;
    }
    for(const auto& water:waters){
        if(!number(water.x,-1000,1000)||!number(water.z,-1000,1000)||
           !number(water.radius,.05,100)||!number(water.level,0,1)||
           !number(water.quality,.05,1)||(water.source!=0&&water.source!=1))return false;
    }
    return true;
}
} // namespace taks
