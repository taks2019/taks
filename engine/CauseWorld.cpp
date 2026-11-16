#include "CauseWorld.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace taks {
namespace { double clamp(double v, double lo, double hi){return std::max(lo, std::min(hi,v));} }
bool World::isNear(double x,double z,double radius) const {
    const double dx=playerX-x,dz=playerZ-z;
    return dx*dx+dz*dz<=radius*radius;
}
void World::syncGroundCounts(){
    groundWood=groundStone=groundTinder=groundLeaves=0;
    for(const auto& node:nodes) {
        if(node.type==0)groundWood+=node.amount;
        if(node.type==1)groundStone+=node.amount;
        if(node.type==2)groundTinder+=node.amount;
        if(node.type==3)groundLeaves+=node.amount;
    }
}
std::string World::nearby() const{
    const ResourceNode* best=nullptr;
    double distSq=1e12;
    for(const auto& node:nodes) {
        if(node.amount<=0)continue;
        const double dx=node.x-playerX,dz=node.z-playerZ;
        const double d=dx*dx+dz*dz;
        if(d<distSq){best=&node;distSq=d;}
    }
    if(!best)return "A bejárható terület ismert készletei elfogytak.";
    static const char* labels[]={"Faág","Kő","Gyújtós","Leveles ág"};
    std::ostringstream out;out<<std::fixed<<std::setprecision(1);
    out<<"KÖZELI: "<<labels[best->type]<<" · "<<std::sqrt(distSq)<<" m"
       <<" · készlet "<<best->amount;
    if(distSq<=1.7*1.7)out<<" · FELVEHETŐ";
    else out<<" · menj közelebb";
    return out.str();
}
std::string World::move(double dx,double dz){
    if(!std::isfinite(dx)||!std::isfinite(dz))return "Érvénytelen mozgás.";
    const double requested=std::sqrt(dx*dx+dz*dz);
    if(requested<0.00001)return "A karakter helyben maradt.";
    if(stamina<0.5)return "A karakter kimerült: pihennie kell.";
    // Az érintéses mozgás egy-egy kicsi világkoordináta-lépés; nem teleport.
    const double factor=std::min(1.0,0.75/requested);
    dx*=factor;dz*=factor;
    const double nx=playerX+dx,nz=playerZ+dz;
    if(nx< -7.5||nx>7.5||nz< -7.5||nz>13.0)return "A világ jelenlegi bejárható határához értél.";
    // A barlang homlokzatának oldalsó sziklafalán nem lehet átsétálni.
    if(nz>2.75 && nz<4.15 && std::abs(nx)>1.45 && std::abs(nx)<7.8)
        return "A sziklafal elállja az utat. A barlangnyílás középen található.";
    const double length=std::sqrt(dx*dx+dz*dz);
    playerX=nx;playerZ=nz;walkPhase+=length*5.5;
    stamina=clamp(stamina-length*(0.50+groundMoisture*0.65),0.0,100.0);
    step(length*.013);
    // A menedék nem véd, ha elhagytuk a valódi fedett területet.
    if(underShelter && !isNear(3.1,-1.55,1.65))underShelter=false;
    ++eventCounter;
    return nearby();
}
std::string World::act(const std::string& action) {
    // Minden sikeres cselekvés: előfeltételek, véges anyag, energia, idő, állapotváltozás.
    if(action=="gather" || action=="wood" || action=="stone" || action=="tinder" || action=="leaves") {
        const int wanted=action=="wood"?0:action=="stone"?1:action=="tinder"?2:action=="leaves"?3:-1;
        int nodeId=-1;double distanceSquared=1e12;
        for(int i=0;i<resourceCount;++i){
            const auto& node=nodes[i];if(node.amount<=0 || (wanted>=0 && node.type!=wanted))continue;
            const double dx=node.x-playerX,dz=node.z-playerZ;
            const double d=dx*dx+dz*dz;
            if(d<distanceSquared){distanceSquared=d;nodeId=i;}
        }
        if(nodeId<0)return "Ebből a nyersanyagból nincs több ismert készlet a területen.";
        if(distanceSquared>1.7*1.7){
            std::ostringstream out;out<<std::fixed<<std::setprecision(1);
            out<<"A nyersanyag "<<std::sqrt(distanceSquared)<<" méterre van: menj közelebb, nem gyűjthetsz távolról.";
            return out.str();
        }
        if(stamina<2)return "A gyűjtéshez pihenés szükséges: nincs elég állóképesség.";
        ResourceNode& node=nodes[nodeId];
        const int type=node.type;
        int* bag=type==0?&carriedWood:type==1?&carriedStone:type==2?&carriedTinder:&carriedLeaves;
        --node.amount;++(*bag);syncGroundCounts();
        stamina-=2;step(.10);++eventCounter;
        return type==0?"Faág felvéve a lelőhelyről. A világ készlete 1-gyel csökkent.":
               type==1?"Kő felvéve a lelőhelyről. A világ készlete 1-gyel csökkent.":
               type==2?"Gyújtós felvéve a lelőhelyről. A világ készlete 1-gyel csökkent.":
                       "Leveles ág felvéve a lelőhelyről. A világ készlete 1-gyel csökkent.";
    }
    if(action=="knap") {
        if(chippedEdgeWear>0) return "Már van használható kőéled; a követ a későbbi munkára tartalékold.";
        if(carriedStone<2) return "Kőeszköz: két kő szükséges (ütőkő és nyersanyag).";
        if(stamina<6) return "Kőmegmunkálás: előbb pihenned kell.";
        carriedStone-=2;stoneDebris+=1;chippedEdgeWear=6;stamina-=6;step(0.40);++eventCounter;
        return "Éles kőszilánk készült: 2 kő -> 1 használható él + kőhulladék. A szerszám kopik.";
    }
    if(action=="pole") {
        if(chippedEdgeWear<=0) return "Fa megmunkálásához előbb éles kőeszköz szükséges.";
        if(carriedWood<1) return "Nincs nyers faág, amiből cölöp készülhetne.";
        if(stamina<5) return "Cölöp készítése: nincs elég állóképességed.";
        if(woodMoisture>0.85) return "A fa nagyon átázott: várj a megmunkálással.";
        --carriedWood;++preparedPoles;--chippedEdgeWear;
        if(chippedEdgeWear==0)++stoneDebris;
        stamina-=5;step(0.30);++eventCounter;
        return "Egy faágból megmunkált cölöp lett; a kőeszköz éle kopott. (+0,3 óra)";
    }
    if(action=="base") {
        if(!isNear(3.1,-1.55,3.0))return "Az oszlopok helyszíne túl messze van: menj az építés helyére.";
        if(shelterStage!=0) return "Az oszlopalapok már a helyükön vannak.";
        if(preparedPoles<2) return "Oszlopalap: előbb 2 megmunkált cölöp kell.";
        if(carriedStone<2) return "Oszlopalap: 2 támasztókő szükséges.";
        if(stamina<12) return "Alapozás: kevés az állóképesség, előbb pihenj.";
        preparedPoles-=2;carriedStone-=2;stamina-=12;step(1.00);shelterStage=1;++eventCounter;
        return "Két cölöp és két kő elhelyezve: a tartóoszlopok állnak. A tető még hiányzik.";
    }
    if(action=="frame") {
        if(!isNear(3.1,-1.55,3.0))return "A tetővázat a menedék helyszínén kell összeszerelni.";
        if(shelterStage<1) return "Tetővázat nem lehet rögzíteni oszlopok nélkül.";
        if(shelterStage>1) return "A tetőváz már a helyén van.";
        if(preparedPoles<3) return "Tetőváz: 3 megmunkált cölöp (gerenda + 2 ferde tartó) kell.";
        if(stamina<14) return "Tetőváz: kevés az állóképesség.";
        preparedPoles-=3;stamina-=14;step(1.10);shelterStage=2;++eventCounter;
        return "Tetőváz felállítva: 3 cölöp beépült, fedés nélkül még átázik.";
    }
    if(action=="roof") {
        if(!isNear(3.1,-1.55,3.0))return "A tetőfedést a menedéknél kell elvégezni.";
        if(shelterStage<2) return "Tetőfedés előtt a tartóoszlopoknak és a tetőváznak is állniuk kell.";
        if(shelterStage==3) return "A levélfedés már elkészült.";
        if(carriedLeaves<6) return "Tetőfedés: 6 leveles ág kell az átlapolt fedéshez.";
        if(stamina<9) return "Tetőfedés: a munkához előbb pihenj.";
        carriedLeaves-=6;stamina-=9;step(0.80);shelterStage=3;++eventCounter;
        return "Elkészült a levélfedés. A menedék alatt már csökken az esőterhelés, de be is kell húzódnod.";
    }
    if(action=="shelter_enter") {
        if(!isNear(3.1,-1.55,2.0))return "Előbb oda kell menned a menedékhez.";
        if(shelterStage<3) return "A menedék még nem fedett, nem véd megfelelően az esőtől.";
        if(underShelter) return "Már a menedék alatt vagy.";
        underShelter=true;step(0.10);++eventCounter;
        return "Behúzódtál a menedékbe: a fedés mérsékli a rád jutó csapadékot.";
    }
    if(action=="shelter_exit") {
        if(!underShelter) return "Jelenleg nem vagy a menedékben.";
        underShelter=false;step(0.10);++eventCounter;
        return "Elhagytad a menedéket: újra közvetlenül ér a környezet.";
    }
    if(action=="hearth") {
        if(!isNear(-1.15,-1.8,2.7))return "Tűzhelyet csak a kijelölt munkaterületen építhetsz: menj oda.";
        if(hearthBuilt) return "A tűzhely már felépült.";
        if(carriedWood<3) return "Tűzhely: 3 faág szükséges.";
        if(carriedStone<2) return "Tűzhely: 2 kő szükséges.";
        if(stamina<12) return "Tűzhely: a munkához túl kevés az állóképességed.";
        carriedWood-=3;carriedStone-=2;stamina-=12;hearthBuilt=true;step(0.75);++eventCounter;
        return "A tűzhely elkészült. 3 faág, 2 kő és 0,75 játékóra elfogyott.";
    }
    if(action=="ignite") {
        if(!isNear(-1.15,-1.8,2.7))return "A tűzgyújtáshoz oda kell menned a tűzhelyhez.";
        if(!hearthBuilt) return "Nem gyújthatsz tüzet: előbb tűzhely szükséges.";
        if(fireLit) return "A tűz már ég.";
        if(carriedTinder<1) return "Nem gyújthatsz tüzet: hiányzik a gyújtós.";
        if(carriedWood<2) return "Nem gyújthatsz tüzet: legalább 2 faágnyi tüzelő kell.";
        if(woodMoisture>0.40) return "A fa túl nedves: előbb száradnia kell.";
        if(rainIntensity>0.60) return "Az erős eső eláztatja az előkészítetlen tűzhelyet.";
        if(stamina<4) return "Tűzgyújtáshoz pihenned kell.";
        --carriedTinder;carriedWood-=2;stamina-=4;fireFuel=2.0;fireLit=true;++eventCounter;
        return "A tűz meggyulladt. Gyújtós és tüzelő fogyott; fény és hő keletkezett.";
    }
    if(action=="rest") {
        step(1.0);stamina=clamp(stamina+(underShelter?30.0:25.0),0.0,100.0);++eventCounter;
        return underShelter?"Pihenés a menedék alatt: egy óra telt el, a fedés csökkentette a kitettséget.":
                            "Pihentél egy játékórát. Az időjárás és a tüzelőanyag ezalatt is változott.";
    }
    return "Ismeretlen cselekvés: a világ állapota nem módosult.";
}
void World::forceRainForTest(double intensity){ testWeather=true; rainIntensity=clamp(intensity,0,1); }
void World::step(double hoursElapsed) {
    if(!std::isfinite(hoursElapsed) || hoursElapsed<=0.0 || hoursElapsed>24.0) return;
    hours+=hoursElapsed;
    // Közelítő determinisztikus időjárás, nem hiteles paleoklimatológiai rekonstrukció.
    humidity=clamp(0.38+0.32*std::sin(hours*0.30),0.04,0.98);
    if(!testWeather) rainIntensity=clamp((humidity-0.62)*2.8,0.0,1.0);
    groundMoisture=clamp(groundMoisture+rainIntensity*0.25*hoursElapsed-(1-rainIntensity)*0.035*hoursElapsed,0,1);
    woodMoisture=clamp(woodMoisture+rainIntensity*0.23*hoursElapsed-(1-rainIntensity)*0.07*hoursElapsed,0,1);
    // A tető NEM teleportálja szárazra a játékost. Csak fedett menedékbe húzódva véd.
    const double exposure=(underShelter && shelterStage==3)?0.07:1.0;
    playerWetness=clamp(playerWetness+rainIntensity*0.34*exposure*hoursElapsed
                        -(underShelter?0.09:0.035)*hoursElapsed,0.0,1.0);
    if(fireLit) {
        fireFuel-=hoursElapsed*(0.35+0.15*rainIntensity);
        if(rainIntensity>0.85 || fireFuel<=0.0){fireFuel=0.0;fireLit=false;++eventCounter;}
    }
    const double gain=(underShelter?0.85:0.35)*hoursElapsed;
    const double dampPenalty=playerWetness>0.70?1.6*hoursElapsed:0.0;
    stamina=clamp(stamina+gain-dampPenalty,0.0,100.0);
}
std::string World::shelterStatus() const {
    std::ostringstream os;
    os<<"MENEDÉK: ";
    switch(shelterStage){
       case 0:os<<"Nincs alap. Következő: 2 cölöp + 2 kő, oszlopok beállítása.";break;
       case 1:os<<"Oszlopok állnak. Következő: 3 megmunkált cölöp, tetőváz.";break;
       case 2:os<<"A tetőváznak nincs fedése. Következő: 6 leveles ág.";break;
       default:os<<"Fedett. "<<(underShelter?"A menedék alatt vagy.":"A szabadban vagy, a fedés még nem véd téged.");break;
    }
    os<<"\nESZKÖZ: kőél "<<chippedEdgeWear<<"/6 használat"
      <<"\nALKATRÉSZ: "<<preparedPoles<<" megmunkált cölöp"
      <<"\nFEDŐANYAG: "<<carriedLeaves<<" leveles ág"
      <<"\nKŐHULLADÉK: "<<stoneDebris;
    return os.str();
}
std::string World::summary() const {
    std::ostringstream os;os<<std::fixed<<std::setprecision(2);
    os<<"IDŐ: "<<hours<<" óra\n"
      <<"CSAPADÉK: "<<rainIntensity*100<<"% | TALAJNEDVESSÉG: "<<groundMoisture*100<<"%\n"
      <<"FA NEDVESSÉGE: "<<woodMoisture*100<<"% | ÁTÁZÁS: "<<playerWetness*100<<"%\n"
      <<"KÖRNYEZET: "<<groundWood<<" faág, "<<groundStone<<" kő, "<<groundLeaves<<" leveles ág, "<<groundTinder<<" gyújtós\n"
      <<"KÉSZLET: "<<carriedWood<<" faág, "<<carriedStone<<" kő, "<<carriedLeaves<<" leveles ág, "<<carriedTinder<<" gyújtós\n"      <<"JÁTÉKOS: "<<playerX<<" m, "<<playerZ<<" m | "<<nearby()<<"\n"

      <<"CÖLÖP: "<<preparedPoles<<" | KŐÉL: "<<chippedEdgeWear<<"/6 | KŐHULLADÉK: "<<stoneDebris<<"\n"
      <<"ÁLLÓKÉPESSÉG: "<<stamina<<"%\n"
      <<"MENEDÉK FÁZIS: "<<shelterStage<<"/3 | FEDETT: "<<(underShelter?"igen":"nem")<<"\n"
      <<"TŰZHELY: "<<(hearthBuilt?"felépült":"hiányzik")<<" | TŰZ: "<<(fireLit?"ég":"nem ég")<<"\n"
      <<"OKOZATI ESEMÉNYEK: "<<eventCounter;
    return os.str();
}
std::string World::save() const {
    std::ostringstream o;o<<std::setprecision(17);
    o<<"TAKS_NATIVE_2 "<<groundWood<<' '<<groundStone<<' '<<groundTinder<<' '
     <<carriedWood<<' '<<carriedStone<<' '<<carriedTinder<<' '
     <<hours<<' '<<humidity<<' '<<rainIntensity<<' '<<woodMoisture<<' '
     <<groundMoisture<<' '<<stamina<<' '<<fireFuel<<' '
     <<(hearthBuilt?1:0)<<' '<<(fireLit?1:0)<<' '<<eventCounter<<' '
     <<groundLeaves<<' '<<carriedLeaves<<' '<<chippedEdgeWear<<' '<<stoneDebris<<' '
     <<preparedPoles<<' '<<shelterStage<<' '<<playerWetness<<' '<<(underShelter?1:0);
    return o.str();
}
bool World::load(const std::string& data) {
    std::istringstream is(data);std::string header;World tmp;int hearth=0,fire=0,covered=0;
    if(!(is>>header) || (header!="TAKS_NATIVE_1" && header!="TAKS_NATIVE_2"))return false;
    if(!(is>>tmp.groundWood>>tmp.groundStone>>tmp.groundTinder
      >>tmp.carriedWood>>tmp.carriedStone>>tmp.carriedTinder
      >>tmp.hours>>tmp.humidity>>tmp.rainIntensity>>tmp.woodMoisture
      >>tmp.groundMoisture>>tmp.stamina>>tmp.fireFuel>>hearth>>fire>>tmp.eventCounter))return false;
    if(header=="TAKS_NATIVE_2"){
        if(!(is>>tmp.groundLeaves>>tmp.carriedLeaves>>tmp.chippedEdgeWear>>tmp.stoneDebris
              >>tmp.preparedPoles>>tmp.shelterStage>>tmp.playerWetness>>covered))return false;
    }
    std::string trailing;
    if(is>>trailing)return false;
    if(tmp.groundWood<0||tmp.groundStone<0||tmp.groundTinder<0||
       tmp.carriedWood<0||tmp.carriedStone<0||tmp.carriedTinder<0||
       tmp.groundLeaves<0||tmp.carriedLeaves<0||tmp.chippedEdgeWear<0||tmp.chippedEdgeWear>6||
       tmp.stoneDebris<0||tmp.preparedPoles<0||tmp.shelterStage<0||tmp.shelterStage>3||
       !std::isfinite(tmp.hours)||tmp.hours<0||tmp.hours>1e9||
       !std::isfinite(tmp.humidity)||tmp.humidity<0||tmp.humidity>1||
       !std::isfinite(tmp.rainIntensity)||tmp.rainIntensity<0||tmp.rainIntensity>1||
       !std::isfinite(tmp.woodMoisture)||tmp.woodMoisture<0||tmp.woodMoisture>1||
       !std::isfinite(tmp.groundMoisture)||tmp.groundMoisture<0||tmp.groundMoisture>1||
       !std::isfinite(tmp.stamina)||tmp.stamina<0||tmp.stamina>100||
       !std::isfinite(tmp.fireFuel)||tmp.fireFuel<0||
       !std::isfinite(tmp.playerWetness)||tmp.playerWetness<0||tmp.playerWetness>1||
       (hearth!=0&&hearth!=1)||(fire!=0&&fire!=1)||(covered!=0&&covered!=1))return false;
    if(covered && tmp.shelterStage!=3)return false;
    if(fire && !hearth)return false;
    tmp.hearthBuilt=hearth!=0;tmp.fireLit=fire!=0;tmp.underShelter=covered!=0;
    *this=tmp;return true;
}
}
