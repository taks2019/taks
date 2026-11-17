#include "../CauseWorld.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <sstream>

static void walkTo(taks::World& w,double x,double z){
    for(int step=0;step<128;++step){
        const double dx=x-w.playerX,dz=z-w.playerZ;
        const double dist=std::sqrt(dx*dx+dz*dz);
        if(dist<0.08)return;
        const double scale=std::min(0.60/dist,1.0);
        const std::string result=w.move(dx*scale,dz*scale);
        assert(result.find("elállja")==std::string::npos);
        assert(result.find("határához")==std::string::npos);
    }
    assert(false && "The player could not reach the test destination");
}
static void gather(taks::World& w,const char* type,int count){
    for(int i=0;i<count;++i){
        const std::string message=w.act(type);
        assert(message.find("felvéve")!=std::string::npos);
    }
}
int main(){
    // A játékos a terepen mozog; a távoli gyűjtés semmilyen készletet nem változtathat.
    taks::World w;
    // A játékos útvonaltesztben a társak éhesek, így nem fogyasztanak el semmit a teszt elől.
    for(auto& npc:w.npcs)npc.hunger=100;
    const std::string before=w.save();
    assert(w.act("wood").find("menj közelebb")!=std::string::npos);
    assert(w.save()==before);
    assert(w.nearby().find("Kő")!=std::string::npos);
    const auto nearRock=w.nearby();
    walkTo(w,1,-5.4);
    gather(w,"stone",2);
    assert(w.carriedStone==2 && w.groundStone==6);
    walkTo(w,-2,-5.5);
    gather(w,"wood",3);
    walkTo(w,3,-4);
    gather(w,"wood",2);
    walkTo(w,-1,-3.9);
    gather(w,"tinder",1);
    walkTo(w,-1.15,-1.8);
    assert(w.act("hearth").find("elkészült")!=std::string::npos);
    assert(w.act("ignite").find("meggyulladt")!=std::string::npos);
    assert(w.fireLit && w.hearthBuilt);
    // Minden világállapotot bitpontosan megőrzünk, beleértve a pozíciókat és lelőhelyeket.
    const std::string saved=w.save();
    taks::World loaded;
    assert(loaded.load(saved) && loaded.save()==saved);
    assert(std::abs(loaded.playerX-w.playerX)<1e-9);
    assert(!loaded.load("TAKS_NATIVE_3 missing"));
    assert(loaded.save()==saved);
    loaded.step(20);
    assert(!loaded.fireLit);

    // Kőél -> 5 darab cölöp -> oszlop -> tetőváz -> fedés, valós helyszínen.
    taks::World b;
    for(auto& npc:b.npcs)npc.hunger=100;
    const auto initial=b.save();
    assert(b.act("roof").find("menedéknél")!=std::string::npos);
    assert(b.act("base").find("helyszíne")!=std::string::npos);
    assert(b.act("pole").find("kőeszköz")!=std::string::npos);
    assert(b.save()==initial);
    walkTo(b,1,-5.4);
    gather(b,"stone",2);
    assert(b.act("knap").find("kőszilánk")!=std::string::npos);
    assert(b.chippedEdgeWear==6 && b.stoneDebris==1);
    walkTo(b,-2,-5.5);
    gather(b,"wood",3);
    walkTo(b,3,-4);
    gather(b,"wood",2);
    for(int i=0;i<5;++i)assert(b.act("pole").find("cölöp")!=std::string::npos);
    assert(b.preparedPoles==5 && b.chippedEdgeWear==1);
    // A korábbi két kőből a szerszám készült, az oszlopokhoz új kövek kellenek.
    walkTo(b,1,-5.4);gather(b,"stone",1);
    walkTo(b,3,-0.8);gather(b,"stone",1);
    walkTo(b,3.1,-1.55);
    assert(b.act("base").find("tartóoszlopok")!=std::string::npos);
    assert(b.shelterStage==1 && b.preparedPoles==3);
    assert(b.act("frame").find("Tetőváz")!=std::string::npos);
    assert(b.shelterStage==2 && b.preparedPoles==0);
    assert(b.act("shelter_enter").find("nem fedett")!=std::string::npos);
    walkTo(b,4.1,-3.3);
    gather(b,"leaves",6);
    walkTo(b,3.1,-1.55);
    if(b.stamina<12)b.act("rest");
    assert(b.act("roof").find("levélfedés")!=std::string::npos);
    assert(b.shelterStage==3 && b.carriedLeaves==0);
    assert(b.act("shelter_enter").find("Behúzódtál")!=std::string::npos);
    assert(b.underShelter);
    const std::string roofSave=b.save();
    taks::World roofReload;
    assert(roofReload.load(roofSave) && roofReload.save()==roofSave);

    taks::World rainOutside=b,rainUnder=b;
    rainOutside.underShelter=false;
    rainOutside.playerWetness=0.1;
    rainUnder.playerWetness=0.1;
    rainOutside.forceRainForTest(.8);
    rainUnder.forceRainForTest(.8);
    rainOutside.step(1);
    rainUnder.step(1);
    assert(rainUnder.playerWetness<rainOutside.playerWetness);
    walkTo(b,5.0,-4.0);
    assert(!b.underShelter);

    // V1 és V2 mentés betöltése új térbeli kezdő lelőhelyekkel, anyagmérleg nélkül nem.
    taks::World old1,old2;
    assert(old1.load("TAKS_NATIVE_1 14 8 5 0 0 0 6 0.4 0 0.13 0.13 100 0 0 0 0"));
    assert(old2.load("TAKS_NATIVE_2 12 7 4 1 1 1 8 0.4 0 0.13 0.13 88 0 0 0 3 15 2 6 1 0 1 0.12 0"));
    assert(old1.save().find("TAKS_NATIVE_4")==0);
    assert(old2.groundWood==12 && old2.groundStone==7 && old2.groundLeaves==15);
    assert(old2.nodes[0].amount<=3);
    const auto stable=old2.save();
    assert(!old2.load("TAKS_NATIVE_3 1 2 bad"));
    assert(old2.save()==stable);
    // Kimerülés, rossz érték és sziklafal ne hozzon létre anyagot.
    taks::World exhausted;
    exhausted.stamina=0;
    const auto noEnergy=exhausted.save();
    assert(exhausted.move(1,0).find("kimerült")!=std::string::npos);
    assert(exhausted.save()==noEnergy);
    taks::World wet;wet.forceRainForTest(1);
    double oldWet=wet.groundMoisture;wet.step(1);
    assert(wet.groundMoisture>oldWet);

    // Négy tényleges autonóm munkás, véges lelőhely, összesített anyagmegmaradás.
    taks::World community;
    assert(community.campStock[0]==0);
    assert(community.npcs.size()==4);
    const int originalMaterials[4]={community.groundWood,community.groundStone,
                                    community.groundLeaves,community.groundTinder};
    community.step(4.0);
    int rolesDelivered=0;
    for(const auto& worker:community.npcs){
        assert(worker.energy>=0 && worker.energy<=100);
        assert(worker.hunger>10);
        if(worker.delivered>0)rolesDelivered++;
    }
    assert(rolesDelivered>=2);
    const int roleToType[4]={0,1,3,2};
    for(int type=0;type<4;++type){
        int inNodes=0,onNpc=0,inCamp=0;
        for(const auto& node:community.nodes)if(node.type==type)inNodes+=node.amount;
        for(const auto& worker:community.npcs)
            if(roleToType[worker.role]==type)onNpc+=worker.carrying;
        for(int role=0;role<4;++role)if(roleToType[role]==type)inCamp+=community.campStock[role];
        assert(inNodes+onNpc+inCamp==originalMaterials[type]);
    }
    assert(community.campStock[0]>0);
    assert(community.act("camp_wood").find("oda kell menned")!=std::string::npos);
    community.move(0.0,0.6);
    const int stored=community.campStock[0],bag=community.carriedWood;
    assert(community.act("camp_wood").find("átvettél")!=std::string::npos);
    assert(community.campStock[0]==stored-1 && community.carriedWood==bag+1);
    const auto npcSave=community.save();
    taks::World resumedNpc;
    assert(resumedNpc.load(npcSave) && resumedNpc.save()==npcSave);
    assert(resumedNpc.npcs[0].delivered==community.npcs[0].delivered);
    assert(resumedNpc.campStock[0]==community.campStock[0]);
    assert(resumedNpc.npcStatus().find("Társ 4")!=std::string::npos);
    assert(resumedNpc.campStatus().find("TÁBORI KÉSZLET")!=std::string::npos);
    taks::World legacy3;
    const auto v4Save=taks::World().save();
    // V3-ból induló mentés új NPC mezők nélkül is migrál.
    const auto suffix=v4Save.substr(v4Save.find("TAKS_NATIVE_4")+13);
    std::istringstream fields(suffix);
    std::string rawToken;
    // A V3-migráció kódútját a korábbi 27+12 mezővel is ellenőrizzük.
    std::string v3="TAKS_NATIVE_3";
    for(int i=0;i<39;++i){
        assert(fields>>rawToken);
        v3+=" "+rawToken;
    }
    assert(legacy3.load(v3));
    assert(legacy3.npcs[0].role==0);
    assert(legacy3.save().find("TAKS_NATIVE_4")==0);
    // A társakkal helyben lehet beszélni, távolról nincs mágikus kommunikáció.
    taks::World talk;
    assert(talk.act("npc_talk").find("Nincs beszélgetési")!=std::string::npos);
    walkTo(talk,-1.8,-2.4);
    assert(talk.act("npc_talk").find("Társ ")!=std::string::npos);
    std::cout<<"TAKS spatial game, construction and save migration tests PASSED\n";
}
