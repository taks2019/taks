#include "CauseWorld.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace taks {
namespace { double clamp(double v, double lo, double hi){return std::max(lo, std::min(hi,v));} }
int World::nearbySiteIndex(double range) const {
    int chosen=-1;
    double minSq=range*range;
    for(int i=0;i<siteCount;++i){
        const double dx=playerX-sites[i].x,dz=playerZ-sites[i].z;
        const double ds=dx*dx+dz*dz;
        if(ds<=minSq){chosen=i;minSq=ds;}
    }
    return chosen;
}
int World::nearbySiteStage() const {
    const int index=nearbySiteIndex();
    return index<0?-1:sites[index].stage;
}
bool World::protectedAtPlayer() const {
    for(int i=0;i<siteCount;++i){
        const auto& site=sites[i];
        if(site.stage<3 || site.condition<25.0)continue;
        const double dx=playerX-site.x,dz=playerZ-site.z;
        if(dx*dx+dz*dz<=1.65*1.65)return true;
    }
    return false;
}
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
std::string World::nextObjective() const {
    if(shelterStage==0){
        if(chippedEdgeWear<=0 && preparedPoles<2 && carriedStone<2 && campStock[1]>0)
            return "CÉL: A társak követ tettek le a táborba. Vedd át, és készíts kőeszközt.";
        if(chippedEdgeWear<=0 && preparedPoles<2)
            return carriedStone<2?"CÉL: Gyűjts két követ, és készíts belőlük kőélt.":"CÉL: Készíts kőélt a begyűjtött kövekből.";
        if(preparedPoles<2 && carriedWood==0 && campStock[0]>0)
            return "CÉL: A társak faágat hoztak a táborba. Menj érte, majd munkáld meg.";
        if(preparedPoles<2)return "CÉL: Munkálj meg két faágat, hogy tartóoszlopok készülhessenek.";
        if(carriedStone<2)return "CÉL: Gyűjts két támasztókövet az oszlopokhoz.";
        return "CÉL: Menj az építés helyére, és állítsd fel az oszlopokat.";
    }
    if(shelterStage==1){
        if(preparedPoles<3)return "CÉL: Készíts még három cölöpöt a tetővázhoz.";
        return "CÉL: Menj a menedékhez és építsd meg a tetővázat.";
    }
    if(shelterStage==2){
        if(carriedLeaves<6 && campStock[2]>0)
            return "CÉL: A társak levélanyagot szállítottak a táborba. Vedd át a fedéshez.";
        if(carriedLeaves<6)return "CÉL: Gyűjts hat leveles ágat a fedéshez.";
        return "CÉL: Készíts tetőfedést a menedéknél.";
    }
    if(!underShelter)return "CÉL: Lépj a kész menedék közelébe, és húzódj alá.";
    if(!hearthBuilt)return "CÉL: Gyűjts 3 faágat és 2 követ a tűzhelyhez.";
    if(!fireLit)return "CÉL: Szerezz száraz tüzelőt és gyújtóst, majd gyújts tüzet.";
    return "Az első menedék és tűz elkészült. Kísérletezz az időjárással, a tűzzel és a mentéssel.";
}
std::string World::campStatus() const {
    std::ostringstream out;
    out<<"TÁBORI KÉSZLET: "<<campStock[0]<<" faág, "<<campStock[1]
       <<" kő, "<<campStock[2]<<" leveles ág, "<<campStock[3]<<" gyújtós.";
    out<<"\nÁtvétel: menj a közös táborhelyhez (0, -3.4), majd válassz anyagot.";
    return out.str();
}
std::string World::npcStatus() const {
    static const char* roleNames[]={"Fa gyűjtése","Kő keresése","Levél gyűjtése","Gyújtós keresése"};
    static const char* taskNames[]={"Készülődik","Lelőhelyhez tart","Táborba szállít","Pihen","Nincs szabad anyag"};
    std::ostringstream out;out<<std::fixed<<std::setprecision(1);
    for(int i=0;i<npcCount;++i){
        const auto& npc=npcs[i];
        out<<"Társ "<<i+1<<" · "<<roleNames[npc.role]
           <<" · "<<taskNames[npc.task]
           <<" · pozíció "<<npc.x<<", "<<npc.z
           <<" · erő "<<npc.energy<<"%"
           <<" · éhség "<<npc.hunger<<"%"
           <<" · szállít "<<npc.carrying
           <<" · leadott "<<npc.delivered;
        if(i+1<npcCount)out<<"\n";
    }
    return out.str();
}
void World::updateNpcs(double dt){
    if(!std::isfinite(dt)||dt<=0)return;
    npcSimulatedHours+=dt;
    // Rögzített szimulációs részlés: 1 óra 25 lépés, mentés után is determinisztikus.
    const int roles[4]={0,1,3,2};
    constexpr double tick=0.04;
    int loopGuard=0;
    while(npcSimulatedHours+1.e-9>=tick && loopGuard++<600){
        npcSimulatedHours-=tick;
        for(int i=0;i<npcCount;++i){
            Npc& npc=npcs[i];const int type=roles[npc.role];
            npc.hunger=clamp(npc.hunger+0.11,0,100);
            npc.wetness=clamp(npc.wetness+rainIntensity*.027-.005,0,1);
            if(npc.hunger>=96){npc.task=3;continue;} // élelem nélkül nem végtelen munkagépek
            if(npc.energy<=17){
                npc.task=3;
                npc.energy=clamp(npc.energy+2.4,0,100);
                continue;
            }
            if(npc.task==3 && npc.energy<72){
                npc.energy=clamp(npc.energy+2.4,0,100);
                continue;
            }
            if(npc.carrying>0) {
                npc.task=2;
                const double dx=-.30-npc.x,dz=-3.40-npc.z;
                const double length=std::sqrt(dx*dx+dz*dz);
                if(length<.58){
                    campStock[npc.role]+=npc.carrying;
                    npc.delivered+=npc.carrying;
                    npc.carrying=0;
                    npc.task=0;
                    ++eventCounter;
                }else{
                    const double step=std::min(length,.25*(npc.wetness>.6?.65:1.));
                    npc.x+=dx/length*step;npc.z+=dz/length*step;
                    npc.walkPhase+=step*5;
                    npc.energy=clamp(npc.energy-step*.42,0,100);
                }
                continue;
            }
            int chosen=-1;double best=1.e10;
            for(int j=0;j<resourceCount;++j){
                const ResourceNode& node=nodes[j];
                if(node.amount<=0||node.type!=type)continue;
                const double dx=node.x-npc.x,dz=node.z-npc.z;
                const double distSq=dx*dx+dz*dz;
                if(distSq<best){chosen=j;best=distSq;}
            }
            if(chosen<0){npc.task=4;continue;}
            npc.task=1;
            ResourceNode& node=nodes[chosen];
            const double dx=node.x-npc.x,dz=node.z-npc.z;
            const double length=std::sqrt(dx*dx+dz*dz);
            if(length<.62){
                if(npc.energy<3){npc.task=3;continue;}
                --node.amount;
                npc.carrying=1;
                npc.task=2;
                npc.energy-=1.8;
                syncGroundCounts();
                ++eventCounter;
            }else{
                const double step=std::min(length,.22*(npc.wetness>.6?.65:1.));
                npc.x+=dx/length*step;npc.z+=dz/length*step;
                npc.walkPhase+=step*5;
                npc.energy=clamp(npc.energy-step*.44,0,100);
            }
        }
    }
    if(npcSimulatedHours<0.0)npcSimulatedHours=0.0;
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
    if(underShelter && !protectedAtPlayer())underShelter=false;
    ++eventCounter;
    return nearby();
}
std::string World::act(const std::string& action) {
    if(action=="npc_talk"){
        for(int i=0;i<npcCount;++i){
            const auto& n=npcs[i];
            const double dx=n.x-playerX,dz=n.z-playerZ;
            if(dx*dx+dz*dz<=2.3*2.3){
                std::ostringstream out;
                out<<"Társ "<<i+1<<": "<<(n.task==1?"Éppen anyagért indulok.":n.task==2?"Anyagot szállítok a táborba.":n.task==3?"Pihenésre van szükségem.":"Itt dolgozom.")
                   <<" Erő: "<<static_cast<int>(n.energy)<<"%, éhség: "<<static_cast<int>(n.hunger)<<"%.";
                return out.str();
            }
        }
        return "Nincs beszélgetési távolságban társ. Menj közelebb.";
    }
    if(action=="camp_wood"||action=="camp_stone"||action=="camp_leaves"||action=="camp_tinder"){
        if(!isNear(-.30,-3.40,1.65))return "A tábor készleteihez oda kell menned.";
        const int role=action=="camp_wood"?0:action=="camp_stone"?1:action=="camp_leaves"?2:3;
        if(campStock[role]<=0)return "Ebből az anyagból a társak még nem hoztak a táborba.";
        if(stamina<.5)return "Kimerült vagy, előbb pihenj.";
        --campStock[role];
        int* bag=role==0?&carriedWood:role==1?&carriedStone:role==2?&carriedLeaves:&carriedTinder;
        ++(*bag);stamina-=.5;step(.025);++eventCounter;
        return "Egy egységet átvettél a tábor közös készletéből.";
    }
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
    updateNpcs(hoursElapsed);
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
      <<"TÁBOR: "<<campStatus()<<"\n"
      <<"KÖZÖSSÉG: 4 társ · önálló munkavégzés\n"
      <<"OKOZATI ESEMÉNYEK: "<<eventCounter;
    return os.str();
}
std::string World::save() const {
    std::ostringstream o;o<<std::setprecision(17);
    o<<"TAKS_NATIVE_4 "<<groundWood<<' '<<groundStone<<' '<<groundTinder<<' '
     <<carriedWood<<' '<<carriedStone<<' '<<carriedTinder<<' '
     <<hours<<' '<<humidity<<' '<<rainIntensity<<' '<<woodMoisture<<' '
     <<groundMoisture<<' '<<stamina<<' '<<fireFuel<<' '
     <<(hearthBuilt?1:0)<<' '<<(fireLit?1:0)<<' '<<eventCounter<<' '
     <<groundLeaves<<' '<<carriedLeaves<<' '<<chippedEdgeWear<<' '<<stoneDebris<<' '
     <<preparedPoles<<' '<<shelterStage<<' '<<playerWetness<<' '<<(underShelter?1:0)
     <<' '<<playerX<<' '<<playerZ<<' '<<walkPhase;
    for(const auto& node:nodes)o<<' '<<node.amount;
    for(int amount:campStock)o<<' '<<amount;
    o<<' '<<npcSimulatedHours;
    for(const auto& n:npcs)
        o<<' '<<n.x<<' '<<n.z<<' '<<n.energy<<' '<<n.hunger<<' '<<n.wetness
         <<' '<<n.role<<' '<<n.carrying<<' '<<n.task<<' '<<n.delivered<<' '<<n.walkPhase;
    return o.str();
}
bool World::load(const std::string& data) {
    std::istringstream is(data);std::string header;World tmp;int hearth=0,fire=0,covered=0;
    if(!(is>>header) ||
       (header!="TAKS_NATIVE_1" && header!="TAKS_NATIVE_2" && header!="TAKS_NATIVE_3" && header!="TAKS_NATIVE_4"))return false;
    if(!(is>>tmp.groundWood>>tmp.groundStone>>tmp.groundTinder
      >>tmp.carriedWood>>tmp.carriedStone>>tmp.carriedTinder
      >>tmp.hours>>tmp.humidity>>tmp.rainIntensity>>tmp.woodMoisture
      >>tmp.groundMoisture>>tmp.stamina>>tmp.fireFuel>>hearth>>fire>>tmp.eventCounter))return false;
    if(header!="TAKS_NATIVE_1"){
        if(!(is>>tmp.groundLeaves>>tmp.carriedLeaves>>tmp.chippedEdgeWear>>tmp.stoneDebris
              >>tmp.preparedPoles>>tmp.shelterStage>>tmp.playerWetness>>covered))return false;
    }
    if(header=="TAKS_NATIVE_3"||header=="TAKS_NATIVE_4"){
        if(!(is>>tmp.playerX>>tmp.playerZ>>tmp.walkPhase))return false;
        for(auto& node:tmp.nodes){if(!(is>>node.amount))return false;}
    }
    if(header=="TAKS_NATIVE_4"){
        for(int& amount:tmp.campStock)if(!(is>>amount))return false;
        if(!(is>>tmp.npcSimulatedHours))return false;
        for(auto& n:tmp.npcs) {
            if(!(is>>n.x>>n.z>>n.energy>>n.hunger>>n.wetness
                   >>n.role>>n.carrying>>n.task>>n.delivered>>n.walkPhase))return false;
        }
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
       !std::isfinite(tmp.playerX)||tmp.playerX<-7.5||tmp.playerX>7.5||
       !std::isfinite(tmp.playerZ)||tmp.playerZ<-7.5||tmp.playerZ>13.0||
       !std::isfinite(tmp.walkPhase)||tmp.walkPhase<0||tmp.walkPhase>1e9||
       (hearth!=0&&hearth!=1)||(fire!=0&&fire!=1)||(covered!=0&&covered!=1))return false;
    if(covered && tmp.shelterStage!=3)return false;
    if(fire && !hearth)return false;

    if(header=="TAKS_NATIVE_4"){
        if(!std::isfinite(tmp.npcSimulatedHours)||tmp.npcSimulatedHours<0||tmp.npcSimulatedHours>=0.0400001)return false;
        for(int amount:tmp.campStock)if(amount<0||amount>100000)return false;
        for(int i=0;i<npcCount;++i){
            const auto& n=tmp.npcs[i];
            if(n.role!=i||n.carrying<0||n.carrying>1||n.delivered<0||n.delivered>100000||
               n.task<0||n.task>4||
               !std::isfinite(n.x)||n.x<-7.5||n.x>7.5||
               !std::isfinite(n.z)||n.z<-7.5||n.z>13.0||
               !std::isfinite(n.energy)||n.energy<0||n.energy>100||
               !std::isfinite(n.hunger)||n.hunger<0||n.hunger>100||
               !std::isfinite(n.wetness)||n.wetness<0||n.wetness>1||
               !std::isfinite(n.walkPhase)||n.walkPhase<0||n.walkPhase>1e9)return false;
        }
    }
    if(header=="TAKS_NATIVE_3"||header=="TAKS_NATIVE_4"){
        // Sérült/túlírt mentésből nem szabad nyersanyagot teremteni.
        for(const auto& node:tmp.nodes)if(node.amount<0 || node.amount>100000)return false;
        const int expected[4]={tmp.groundWood,tmp.groundStone,tmp.groundTinder,tmp.groundLeaves};
        tmp.syncGroundCounts();
        if(tmp.groundWood!=expected[0]||tmp.groundStone!=expected[1]||
           tmp.groundTinder!=expected[2]||tmp.groundLeaves!=expected[3])return false;
    }else{
        // A v1/v2 még csak globális készleteket mentett. Visszamenőleges
        // kompatibilitás: a készletet térbeli lelőhelyek között osztjuk szét.
        const int counts[4]={tmp.groundWood,tmp.groundStone,tmp.groundTinder,tmp.groundLeaves};
        for(int type=0;type<4;++type){
            int remaining=counts[type],first=-1;
            for(int i=0;i<World::resourceCount;++i){
                auto& node=tmp.nodes[i];
                if(node.type!=type)continue;
                if(first<0)first=i;
                const int amount=std::min(remaining,node.amount);
                node.amount=amount;remaining-=amount;
            }
            if(remaining>0 && first>=0)tmp.nodes[first].amount+=remaining;
        }
        tmp.syncGroundCounts();
    }
    tmp.hearthBuilt=hearth!=0;tmp.fireLit=fire!=0;tmp.underShelter=covered!=0;
    *this=tmp;return true;
}
}
