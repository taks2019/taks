#include "CauseWorld.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace taks {
namespace { double clamp(double v, double lo, double hi){return std::max(lo, std::min(hi,v));} }
std::string World::act(const std::string& action) {
    // Az összes cselekvés ugyanazt az ellenőrzés -> költség -> következmény útvonalat használja.
    if(action=="wood" || action=="stone" || action=="tinder") {
        int* source = action=="wood"? &groundWood : action=="stone"? &groundStone : &groundTinder;
        int* bag = action=="wood"? &carriedWood : action=="stone"? &carriedStone : &carriedTinder;
        if(*source<=0) return "Nincs több elérhető nyersanyag a közelben.";
        if(stamina<2) return "Túl fáradt vagy a gyűjtéshez. Pihenned kell.";
        --(*source); ++(*bag); stamina-=2; ++eventCounter;
        return action=="wood" ? "Összegyűjtöttél egy faágat. A helyi készlet csökkent." :
               action=="stone"? "Felvettél egy követ. A helyi készlet csökkent." :
                                 "Száraz gyújtóst gyűjtöttél. A helyi készlet csökkent.";
    }
    if(action=="hearth") {
        if(hearthBuilt) return "A tűzhely már felépült.";
        if(carriedWood<3) return "Tűzhely: legalább 3 faág hiányzik vagy nincs nálad.";
        if(carriedStone<2) return "Tűzhely: 2 kő szükséges.";
        if(stamina<12) return "Nem tudod felépíteni: túl kevés az állóképességed.";
        carriedWood-=3; carriedStone-=2; stamina-=12; hearthBuilt=true; step(0.75); ++eventCounter;
        return "A tűzhely elkészült. 3 faág és 2 kő felhasználva; nőtt a fáradtság.";
    }
    if(action=="ignite") {
        if(!hearthBuilt) return "Nem gyújthatsz tüzet: előbb tűzhely szükséges.";
        if(fireLit) return "A tűz már ég.";
        if(carriedTinder<1) return "Nem gyújthatsz tüzet: hiányzik a gyújtós.";
        if(carriedWood<2) return "Nem gyújthatsz tüzet: legalább 2 faágnyi tüzelő kell.";
        if(woodMoisture>0.40) return "A fa túl nedves: előbb száradnia kell.";
        if(rainIntensity>0.60) return "A csapadék túl intenzív; az előkészítetlen tűzhely beázik.";
        if(stamina<4) return "Túl fáradt vagy a tűzgyújtáshoz.";
        --carriedTinder; carriedWood-=2; stamina-=4; fireFuel=2.0; fireLit=true; ++eventCounter;
        return "A tűz meggyulladt. Gyújtós és tüzelő fogyott; most hőt és fényt termel.";
    }
    if(action=="rest") {
        step(1.0); stamina=clamp(stamina+25.0,0.0,100.0); ++eventCounter;
        return "Pihentél: az állóképességed helyreállt, az idő azonban tovább halad.";
    }
    return "Ismeretlen cselekvés: a világ állapota nem módosult.";
}
void World::forceRainForTest(double intensity){ testWeather=true; rainIntensity=clamp(intensity,0,1); }
void World::step(double hoursElapsed) {
    if(!std::isfinite(hoursElapsed) || hoursElapsed<=0.0 || hoursElapsed>24.0) return;
    hours+=hoursElapsed;
    // Egyszerű, determinisztikus éghajlati modell; még nem éghajlati rekonstrukció.
    humidity=clamp(0.38+0.32*std::sin(hours*0.30),0.04,0.98);
    if(!testWeather) rainIntensity=clamp((humidity-0.62)*2.8,0.0,1.0);
    groundMoisture=clamp(groundMoisture+rainIntensity*0.25*hoursElapsed-(1-rainIntensity)*0.035*hoursElapsed,0,1);
    woodMoisture=clamp(woodMoisture+rainIntensity*0.23*hoursElapsed-(1-rainIntensity)*0.07*hoursElapsed,0,1);
    if(fireLit) {
        fireFuel-=hoursElapsed*(0.35+0.15*rainIntensity);
        if(rainIntensity>0.85 || fireFuel<=0) {fireFuel=0; fireLit=false; ++eventCounter;}
    }
    stamina=clamp(stamina+0.5*hoursElapsed,0.0,100.0);
}
std::string World::summary() const {
    std::ostringstream os; os<<std::fixed<<std::setprecision(2);
    os<<"IDŐ: "<<hours<<" óra\n"
      <<"PÁRA: "<<humidity*100<<"% | CSAPADÉK: "<<rainIntensity*100<<"%\n"
      <<"TALAJNEDVESSÉG: "<<groundMoisture*100<<"% | FA NEDVESSÉGE: "<<woodMoisture*100<<"%\n\n"
      <<"KÖZELI KÉSZLET: "<<groundWood<<" faág, "<<groundStone<<" kő, "<<groundTinder<<" gyújtós\n"
      <<"NÁLAD: "<<carriedWood<<" faág, "<<carriedStone<<" kő, "<<carriedTinder<<" gyújtós\n"
      <<"ÁLLÓKÉPESSÉG: "<<stamina<<"%\n"
      <<"TŰZHELY: "<<(hearthBuilt?"elkészült":"hiányzik")<<" | TŰZ: "<<(fireLit?"ég":"nem ég")<<"\n"
      <<"ESEMÉNYEK: "<<eventCounter;
    return os.str();
}
std::string World::save() const {
    std::ostringstream o; o<<std::setprecision(17);
    o<<"TAKS_NATIVE_1 "<<groundWood<<' '<<groundStone<<' '<<groundTinder<<' '
     <<carriedWood<<' '<<carriedStone<<' '<<carriedTinder<<' '
     <<hours<<' '<<humidity<<' '<<rainIntensity<<' '<<woodMoisture<<' '
     <<groundMoisture<<' '<<stamina<<' '<<fireFuel<<' '
     <<(hearthBuilt?1:0)<<' '<<(fireLit?1:0)<<' '<<eventCounter;
    return o.str();
}
bool World::load(const std::string& data) {
    std::istringstream is(data); std::string header; World tmp; int hearth=0, fire=0;
    if(!(is>>header) || header!="TAKS_NATIVE_1") return false;
    if(!(is>>tmp.groundWood>>tmp.groundStone>>tmp.groundTinder
      >>tmp.carriedWood>>tmp.carriedStone>>tmp.carriedTinder
      >>tmp.hours>>tmp.humidity>>tmp.rainIntensity>>tmp.woodMoisture
      >>tmp.groundMoisture>>tmp.stamina>>tmp.fireFuel>>hearth>>fire>>tmp.eventCounter)) return false;
    if(tmp.groundWood<0 || tmp.groundStone<0 || tmp.groundTinder<0 || tmp.carriedWood<0 || tmp.carriedStone<0 || tmp.carriedTinder<0 ||
       !std::isfinite(tmp.hours) || tmp.hours<0 || tmp.hours>1e9 || !std::isfinite(tmp.humidity) || tmp.humidity<0 || tmp.humidity>1 ||
       !std::isfinite(tmp.rainIntensity) || tmp.rainIntensity<0 || tmp.rainIntensity>1 || !std::isfinite(tmp.woodMoisture) || tmp.woodMoisture<0 || tmp.woodMoisture>1 ||
       !std::isfinite(tmp.groundMoisture) || tmp.groundMoisture<0 || tmp.groundMoisture>1 || !std::isfinite(tmp.stamina) || tmp.stamina<0 || tmp.stamina>100 ||
       !std::isfinite(tmp.fireFuel) || tmp.fireFuel<0 || (hearth!=0&&hearth!=1) || (fire!=0&&fire!=1)) return false;
    tmp.hearthBuilt=hearth!=0;tmp.fireLit=fire!=0;
    *this=tmp;return true;
}
}