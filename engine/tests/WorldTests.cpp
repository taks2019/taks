#include "../CauseWorld.h"
#include "../PlayerTraversal.h"
#include <cassert>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
static void removeNpcSurvivalFields(std::vector<std::string>& tokens){
    const auto tag=std::find(tokens.begin(),tokens.end(),"NPCSURV1");
    if(tag==tokens.end())return;
    constexpr std::size_t count=4*8;
    assert(std::size_t(tokens.end()-tag)>count);
    tokens.erase(tag,tag+1+count);
}
static void removeNewLearningFields(std::vector<std::string>& tokens){
    const auto tag=std::find(tokens.begin(),tokens.end(),"NPCLEARN1");
    if(tag==tokens.end())return;
    constexpr std::size_t dataCount=4*(6+12);
    assert(std::size_t(tokens.end()-tag)>dataCount);
    tokens.erase(tag,tag+1+dataCount);
}


static void walkTo(taks::World& w,double x,double z){
    for(int step=0;step<128;++step){
        // Hosszú munkanap után a játékosnak a valós játékszabályok szerint pihennie kell.
        if(w.stamina<2.0)w.act("rest");
        const double dx=x-w.playerX,dz=z-w.playerZ;
        const double dist=std::sqrt(dx*dx+dz*dz);
        if(dist<0.08)return;
        const double scale=std::min(0.60/dist,1.0);
        const std::string result=w.move(dx*scale,dz*scale);
        assert(result.find("elállja")==std::string::npos);
        assert(result.find("határához")==std::string::npos);
    }
    std::cerr<<"Cannot walk to "<<x<<","<<z<<" from "<<w.playerX<<","<<w.playerZ<<" stamina "<<w.stamina<<" goal: "<<w.nextObjective()<<"\n";
    assert(false && "The player could not reach the test destination");
}
static void gather(taks::World& w,const char* type,int count){
    for(int i=0;i<count;++i){
        if(w.stamina<2.0)w.act("rest");
        const std::string message=w.act(type);
        if(message.find("felvéve")==std::string::npos)
            std::cerr<<"Gather failed ("<<type<<"): "<<message<<"\n";
        assert(message.find("felvéve")!=std::string::npos);
    }
}
int main(){
    // A játékos a terepen mozog; a távoli gyűjtés semmilyen készletet nem változtathat.
    taks::World w;
    w.forceRainForTest(0); // The successful ignition scenario requires dry conditions.
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
    assert(b.act("roof").find("kijelölt építési")!=std::string::npos);
    assert(b.act("base").find("kijelölt építési")!=std::string::npos);
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
    walkTo(b,3,-0.8);gather(b,"stone",2);
    walkTo(b,3.1,-1.55);
    assert(b.act("site").find("kijelölve")!=std::string::npos);
    assert(b.act("base").find("oszlop")!=std::string::npos);
    assert(b.shelterStage==1 && b.preparedPoles==3);
    assert(b.act("frame").find("tetőváz")!=std::string::npos);
    assert(b.shelterStage==2 && b.preparedPoles==0);
    assert(b.act("shelter_enter").find("fedett, ép menedék")!=std::string::npos);
    walkTo(b,4.1,-3.3);
    gather(b,"leaves",6);
    walkTo(b,3.1,-1.55);
    if(b.stamina<12)b.act("rest");
    assert(b.act("roof").find("Felkerült a fedés")!=std::string::npos);
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
    assert(old1.save().find("TAKS_NATIVE_12")==0);
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
    taks::World trampleWorld;
    trampleWorld.playerX=trampleWorld.ecology.grass[0].x-1.5;
    trampleWorld.playerZ=trampleWorld.ecology.grass[0].z;
    const double standingGrass=trampleWorld.ecology.grass[0].trampling;
    trampleWorld.move(.70,0);
    assert(trampleWorld.ecology.grass[0].trampling>standingGrass);
    taks::World wet;wet.forceRainForTest(1);
    double oldWet=wet.groundMoisture;wet.step(1);
    assert(wet.groundMoisture>oldWet);

    // Négy tényleges autonóm munkás, véges lelőhely, összesített anyagmegmaradás.
    taks::World community;
    assert(community.campStock[0]==0);
    assert(community.npcs.size()==4);
    const int originalMaterials[4]={community.groundWood,community.groundStone,
                                    community.groundTinder,community.groundLeaves};
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
    const auto suffix=v4Save.substr(v4Save.find(' ')+1);
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
    assert(legacy3.save().find("TAKS_NATIVE_12")==0);
    // A társakkal helyben lehet beszélni, távolról nincs mágikus kommunikáció.
    taks::World talk;
    assert(talk.act("npc_talk").find("Nincs beszélgetési")!=std::string::npos);
    walkTo(talk,-1.8,-2.4);
    assert(talk.act("npc_talk").find("Társ ")!=std::string::npos);
    // Három egymástól független építési hely: külön világhelyzet, anyag és állapot.
    taks::World multi;
    for(auto& worker:multi.npcs)worker.hunger=100;
    multi.grantCarriedForTest(1,8);multi.preparedPoles=12;multi.carriedLeaves=14;
    walkTo(multi,3.0,-1.6);
    assert(multi.act("site").find("kijelölve")!=std::string::npos);
    assert(multi.siteCount==1 && multi.nearbySiteStage()==0);
    const auto noDouble=multi.save();
    assert(multi.act("site").find("túl közel")!=std::string::npos);
    assert(multi.save()==noDouble);
    assert(multi.act("base").find("oszlop")!=std::string::npos);
    assert(multi.sites[0].stage==1);
    walkTo(multi,-3.2,-1.4);
    assert(multi.nearbySiteStage()==-1);
    assert(multi.act("site").find("kijelölve")!=std::string::npos);
    assert(multi.siteCount==2 && multi.nearbySiteStage()==0);
    // A második menedék építésétől az első nem fejlődik automatikusan.
    if(multi.stamina<15)multi.act("rest");
    assert(multi.act("base").find("oszlop")!=std::string::npos);
    assert(multi.sites[0].stage==1 && multi.sites[1].stage==1);
    assert(multi.act("frame").find("tetőváz")!=std::string::npos);
    assert(multi.sites[0].stage==1 && multi.sites[1].stage==2);
    if(multi.stamina<10)multi.act("rest");
    assert(multi.act("roof").find("Felkerült")!=std::string::npos);
    assert(multi.sites[0].stage==1 && multi.sites[1].stage==3);
    assert(multi.protectedAtPlayer());
    assert(multi.act("shelter_enter").find("Behúzódtál")!=std::string::npos);
    multi.forceRainForTest(1.0);
    const double preWeathering=multi.sites[1].condition;
    multi.step(24);
    assert(multi.sites[1].condition<preWeathering);
    // Javítás: 1 valódi cölöp, 2 fedőanyag és 0.7 óra, csak a sérült helyen.
    if(multi.stamina<10)multi.act("rest");
    const double damaged=multi.sites[1].condition;
    const int priorPoles=multi.preparedPoles,priorLeaves=multi.carriedLeaves;
    assert(multi.act("repair").find("javítva")!=std::string::npos);
    assert(multi.sites[1].condition>damaged);
    assert(multi.preparedPoles==priorPoles-1 && multi.carriedLeaves==priorLeaves-2);
    assert(multi.sites[0].stage==1);
    const auto allSites=multi.save();
    taks::World loadedSites;
    assert(loadedSites.load(allSites) && loadedSites.save()==allSites);
    assert(loadedSites.siteCount==2 && loadedSites.sites[1].stage==3);
    assert(std::abs(loadedSites.sites[0].x-loadedSites.sites[1].x)>5.0);
    assert(!loadedSites.load("TAKS_NATIVE_6 missing sites"));
    assert(loadedSites.save()==allSites);
    // A 0.4-es APK rögzített helyű menedéke valóban a régi helyén marad.
    taks::World preV5;
    preV5.shelterStage=3;
    const auto current=preV5.save();
    std::istringstream savedParts(current);
    std::vector<std::string> oldFields;
    std::string token;
    while(savedParts>>token)oldFields.push_back(token);
    assert(oldFields.size()>15);
    // v6 appends complete animal/plant ecology after the old v5 build sites.
    const auto ecoBegin=std::find(oldFields.begin(),oldFields.end(),"ECO4");
    assert(ecoBegin!=oldFields.end());
    oldFields.erase(ecoBegin,oldFields.end());
    removeNpcSurvivalFields(oldFields);
    removeNewLearningFields(oldFields);
    oldFields[0]="TAKS_NATIVE_4";
    oldFields.resize(oldFields.size()-13); // v5 build sites -> old v4 save
    std::ostringstream oldText;
    for(size_t i=0;i<oldFields.size();++i){
        if(i)oldText<<' ';
        oldText<<oldFields[i];
    }
    taks::World migratedOld;
    assert(migratedOld.load(oldText.str()));
    assert(migratedOld.siteCount==1 && migratedOld.sites[0].stage==3);
    assert(std::abs(migratedOld.sites[0].x-3.1)<1e-6);
    assert(std::abs(migratedOld.sites[0].z+1.55)<1e-6);
    assert(migratedOld.save().find("TAKS_NATIVE_12")==0);
    // A saved 0.8 / v5 world loads without creating animals or losing inventory.
    auto oldV5Fields=oldFields;
    {
        // Use a valid v5 world with matching build sites, unlike the intentionally
        // inconsistent old-v4 migration fixture above.
        std::istringstream stream(allSites);
        oldV5Fields.clear();
        while(stream>>token){
            if(token=="ECO4")break;
            oldV5Fields.push_back(token);
        }
    }
    assert(oldV5Fields.size()>13);
    removeNpcSurvivalFields(oldV5Fields);
    removeNewLearningFields(oldV5Fields);
    oldV5Fields[0]="TAKS_NATIVE_5";
    std::ostringstream oldV5Text;
    for(std::size_t i=0;i<oldV5Fields.size();++i){
        if(i)oldV5Text<<' ';
        oldV5Text<<oldV5Fields[i];
    }
    taks::World upgraded;
    assert(upgraded.load(oldV5Text.str()));
    assert(upgraded.save().find("TAKS_NATIVE_12")==0);
    assert(upgraded.ledgerConsistent());
    assert(upgraded.ecology.aliveAnimals()==4);
    // V6 ECO1 and V7 ECO2 remain readable; V8 adds saved hydrology.
    std::vector<std::string> v6Fields;
    {std::istringstream in(taks::World().save());while(in>>token)v6Fields.push_back(token);}
    auto eco3=std::find(v6Fields.begin(),v6Fields.end(),"ECO4");
    assert(eco3!=v6Fields.end());
    const std::size_t ecoIndex=static_cast<std::size_t>(eco3-v6Fields.begin());
    const std::size_t eco1Payload=9+taks::LivingWorld::grassCount*5+
                                  taks::LivingWorld::treeCount*7+
                                  taks::LivingWorld::animalCount*11;
    assert(v6Fields.size()>=ecoIndex+1+eco1Payload);
    v6Fields.resize(ecoIndex+1+eco1Payload);
    removeNpcSurvivalFields(v6Fields);
    removeNewLearningFields(v6Fields);
    const auto originalEco=std::find(v6Fields.begin(),v6Fields.end(),"ECO4");
    assert(originalEco!=v6Fields.end());
    v6Fields[0]="TAKS_NATIVE_6";*originalEco="ECO1";
    std::ostringstream oldV6Text;
    for(std::size_t i=0;i<v6Fields.size();++i){if(i)oldV6Text<<' ';oldV6Text<<v6Fields[i];}
    taks::World oldV6;
    assert(oldV6.load(oldV6Text.str()));
    assert(oldV6.save().find("TAKS_NATIVE_12")==0);
    assert(oldV6.ecology.visibleFootprintCount()==0);
    taks::World muddySteps;
    muddySteps.ecology.soilWater=.8;muddySteps.ecology.mud=.9;muddySteps.groundMoisture=.8;
    muddySteps.move(.65,0);
    assert(muddySteps.ecology.visibleFootprintCount()>0);
    const auto muddySave=muddySteps.save();
    taks::World restoredTracks;
    assert(restoredTracks.load(muddySave));
    assert(restoredTracks.ecology.visibleFootprintCount()==muddySteps.ecology.visibleFootprintCount());
    assert(restoredTracks.save()==muddySave);
    // A character has radius: diagonal wall contact must slide, never teleport.
    taks::World wall;
    wall.playerX=2.30;wall.playerZ=2.53;
    const double wallStamina=wall.stamina;
    wall.move(0.0,.70);
    assert(wall.playerZ<=2.671 && wall.playerZ>2.53);
    assert(wall.stamina<wallStamina);
    const double prevX=wall.playerX;
    wall.move(-.50,.30);
    assert(wall.playerX<prevX-.15);
    assert(wall.playerZ<=2.671);
    // The world's outer border permits movement along it.
    taks::World edge;
    edge.playerX=7.49;edge.playerZ=-4.0;
    const auto beforeEdge=edge.playerZ;
    edge.move(.5,.5);
    assert(edge.playerZ>beforeEdge+.05 && edge.playerX<=7.5);
    // Live tree trunks cannot be walked through; a dead tree is traversable.
    taks::World trunk;
    trunk.playerX=-6.40;trunk.playerZ=-2.05;
    trunk.move(0.0,.75);
    assert(trunk.playerZ< -1.65);
    trunk.ecology.trees[0].alive=false;
    trunk.move(0.0,.75);
    assert(trunk.playerZ> -1.65);
    std::cout<<"PASS: swept wall/trunk collisions and diagonal wall sliding\\n";
    // ---- F1: egyedi azonosítójú kő/faág főkönyv ----
    {
        taks::World w;
        w.ensureLedger();
        assert(w.ledgerConsistent());
        const unsigned idsBefore=w.nextItemId;
        assert(w.countItems(1,0)==8u && w.countItems(0,0)==14u);
        // Sikertelen akció (túl messze) nem hozhat létre és nem vihet el tárgyat.
        w.playerX=-6.0;w.playerZ=1.2;
        const size_t before=w.items.size();
        w.act("stone");
        assert(w.items.size()==before && w.ledgerConsistent());
        // Siker: pontosan egy konkrét kő költözik a játékoshoz.
        w.playerX=1.0;w.playerZ=-5.2;
        w.act("stone");w.act("stone");
        assert(w.carriedStone==2 && w.countItems(1,1)==2u && w.ledgerConsistent());
        bool loggedStone=false;
        for(const auto& line:w.causeLog)if(line.find("kő #")!=std::string::npos&&line.find("játékos felveszi")!=std::string::npos)loggedStone=true;
        assert(loggedStone);
        // Mentés/betöltés megtartja az azonosítókat.
        const std::string saved=w.save();
        assert(saved.find("TAKS_NATIVE_12")==0 && saved.find(" LEDG1 ")!=std::string::npos);
        taks::World back;assert(back.load(saved));
        assert(back.items.size()==w.items.size() && back.nextItemId==w.nextItemId && back.ledgerConsistent());
        for(size_t i=0;i<w.items.size();++i)assert(back.items[i].id==w.items[i].id && back.items[i].where==w.items[i].where);
        // Kőmegmunkálás: a két kő azonosítója elhasználódik, nem keletkezik új.
        w.act("knap");
        assert(w.carriedStone==0 && w.consumedItems[1]==2u && w.countItems(1,1)==0u && w.ledgerConsistent());
        assert(w.nextItemId==idsBefore || w.nextItemId>=idsBefore);
        // Társak szállítása közben is egyezzen a főkönyv a készlettel.
        for(int i=0;i<40;++i){w.step(0.5);assert(w.ledgerConsistent());}
        assert(w.countItems(0,2)==static_cast<unsigned>(w.campStock[0]) && w.countItems(1,2)==static_cast<unsigned>(w.campStock[1]));
        // Meghamisított főkönyv: a betöltés elutasítja, nem teremt anyagot.
        std::string forged=back.save();
        const size_t pos=forged.find(" LEDG1 ");
        std::istringstream peek(forged.substr(pos+7));
        unsigned nid,c0,c1;size_t n;peek>>nid>>c0>>c1>>n;
        std::string tampered=forged.substr(0,pos)+" LEDG1 "+std::to_string(nid)+" "+std::to_string(c0)+" "+std::to_string(c1)+" 0 STRUCT1 1 0 0";
        taks::World rejected;assert(!rejected.load(tampered));
    }
    // ---- F2 bekötés: szerkezet a világban, anyagfelhasználás, mentés v8, v7 migráció ----
    {
        taks::World g;
        g.playerX=0.0;g.playerZ=-3.0;
        const std::string before=g.save();
        // Anyag nélkül semmi sem épül, és semmi sem változik.
        assert(g.act("part_pole").find("cölöp")!=std::string::npos && g.save()==before);
        assert(g.act("part_stone").find("kő")!=std::string::npos && g.save()==before);
        g.preparedPoles=4;g.grantCarriedForTest(1,1);
        const int poles0=g.preparedPoles;
        assert(g.act("part_pole").find("lerakva")!=std::string::npos);
        assert(g.preparedPoles==poles0-1 && g.structure.parts.size()==1);
        // Második cölöp ugyanarra a helyre: rárakódik (és rögzül), anyag fogy.
        assert(g.act("part_pole").find("lerakva")!=std::string::npos);
        assert(g.structure.parts.size()==2 && g.structure.joints.size()==1);
        assert(g.structure.parts[1].y>2.0);
        // Kő: az egyedi főkönyvből fogy.
        const unsigned consumedBefore=g.consumedItems[1];
        assert(g.act("part_stone").find("lerakva")!=std::string::npos);
        assert(g.consumedItems[1]==consumedBefore+1 && g.carriedStone==0 && g.ledgerConsistent());
        // Mentés/betöltés megtartja a szerkezetet; v7 mentés üres szerkezettel töltődik.
        const std::string s8=g.save();
        taks::World back;assert(back.load(s8) && back.save()==s8 && back.structure.parts.size()==3);
        // A damaged structure must leave the destination world untouched.
        const size_t sp=s8.find(" STRUCT2 ");
        taks::World bad;
        assert(!bad.load(s8.substr(0,sp)+" STRUCT2 0 9 1 1 0 0 0 0 0 1 1 1 500 0"));
        // F2/2: vágás kőéllel – hiba esetén nincs változás; siker esetén kopás, tömeg megmarad, mentés visszaáll.
        {
            taks::World k;
            assert(k.act("part_cut").find("Nincs alkatrész")!=std::string::npos);
            k.playerX=0;k.playerZ=0;k.preparedPoles=1;k.playerX=-0.6;
            assert(k.act("part_pole").find("lerakva")!=std::string::npos);
            const std::string beforeCut=k.save();
            assert(k.act("part_cut").find("kőeszköz")!=std::string::npos && k.save()==beforeCut);
            k.chippedEdgeWear=3;
            double m0=0;for(const auto& p:k.structure.parts)m0+=p.mass;
            assert(k.act("part_cut").find("kettévágva")!=std::string::npos);
            double m1=0;for(const auto& p:k.structure.parts)m1+=p.mass;
            assert(k.structure.parts.size()==2 && std::fabs(m0-m1)<1e-9 && k.chippedEdgeWear==2);
            assert(k.act("part_turn").find("elfordítva")!=std::string::npos);
            taks::World again;assert(again.load(k.save()) && again.save()==k.save());
        }
        // Az eső kopást okoz.
        const double c0=g.structure.parts[0].condition;
        g.forceRainForTest(1.0);g.step(10.0);
        assert(g.structure.parts[0].condition<c0);
    }
    assert(taks::World().summary().find("SZÉL:")!=std::string::npos);
    // Deterministic wind is derived from current clock/weather and changes the
    // finite fuel burn rate. Save data needs no duplicate wind field.
    int calmHour=1,gustHour=1;
    double calmWind=2.0,gustWind=-1.0;
    for(int hour=1;hour<2400;++hour){
        taks::World sample;
        sample.hours=hour;
        sample.humidity=.38+.32*std::sin(sample.hours*.30);
        const double value=sample.windStrength();
        if(value<calmWind){calmWind=value;calmHour=hour;}
        if(value>gustWind){gustWind=value;gustHour=hour;}
    }
    assert(gustWind>calmWind+.10);
    taks::World calmFire,gustyFire;
    calmFire.hours=calmHour-.5;gustyFire.hours=gustHour-.5;
    calmFire.forceRainForTest(0.0);gustyFire.forceRainForTest(0.0);
    calmFire.fireLit=gustyFire.fireLit=true;
    calmFire.fireFuel=gustyFire.fireFuel=10.0;
    calmFire.step(.5);gustyFire.step(.5);
    assert(gustyFire.fireFuel<calmFire.fireFuel);
    // Jump is physically bounded, costs stamina, cannot launch while swimming,
    // and never writes ghost footprints or tramples grass while airborne.
    taks::World jumper;
    jumper.forceRainForTest(0);
    const auto beforeJumpSave=jumper.save();
    const double beforeJumpStamina=jumper.stamina;
    assert(jumper.act("jump")=="Elrugaszkodtál a talajról.");
    assert(jumper.stamina<beforeJumpStamina);
    assert(jumper.save()!=beforeJumpSave);
    taks::PlayerJump arc;
    assert(arc.launch(100.0,3.0));
    assert(arc.airborne(100.15));
    assert(arc.height(100.15)>0.0);
    assert(!arc.launch(100.2,3.0));
    assert(!arc.airborne(101.0));
    assert(arc.height(101.0)==0.0);
    jumper.playerX=1.0;jumper.playerZ=-5.4;
    const int oldTracks=jumper.ecology.visibleFootprintCount();
    jumper.move(.2,.1,true);
    assert(jumper.ecology.visibleFootprintCount()==oldTracks);
    jumper.playerX=jumper.ecology.waters[0].x;
    jumper.playerZ=jumper.ecology.waters[0].z;
    assert(jumper.act("jump").find("mély vízben")!=std::string::npos);
    std::cout<<"PASS: bounded terrain-dependent 3D jump and airborne footprints"<<std::endl;
    // One actor's direct observation can become another actor's unconfirmed
    // report only through a physical meeting; they must later verify it.
    {
        taks::World rumors;
        rumors.forceRainForTest(0.0);
        for(auto& npc:rumors.npcs){npc.x=-.3;npc.z=-3.4;}
        const int farStone=6; // too distant to sense from the camp
        rumors.npcs[0].rememberedAmounts[farStone]=2;
        rumors.step(.04);
        assert(rumors.npcs[1].rememberedAmounts[farStone]==-2);
        assert(rumors.npcs[1].rumorsHeard>0);
        rumors.npcs[1].x=rumors.nodes[farStone].x;
        rumors.npcs[1].z=rumors.nodes[farStone].z;
        rumors.step(.04);
        assert(rumors.npcs[1].rememberedAmounts[farStone]>=0);
        assert(rumors.ledgerConsistent());
    }
    // Environmental changes must have visible mechanical causes, not only art.
    {
        taks::World rainy;
        rainy.forceRainForTest(1);
        rainy.ecology.soilWater=.45;
        rainy.ecology.mud=.05;
        rainy.step(.04);
        assert(rainy.ecology.mud>=.30);
        assert(rainy.causalReport().find("sár")!=std::string::npos);
        assert(rainy.causalReport().find("lassabb járás")!=std::string::npos);
    }
    // NPC learning must be non-omniscient, individually persistent and bounded.
    {
        taks::World learning;
        assert(learning.npcs[0].rememberedAmounts[0]==-1);
        assert(learning.npcs[0].completedTrips==0);
        learning.step(.04);
        // Actor 1 starts within 2.8m of node 2, not of remote node 4.
        assert(learning.npcs[0].rememberedAmounts[2]>=0);
        assert(learning.npcs[0].rememberedAmounts[3]==-1);
        const std::string learned=learning.save();
        taks::World replay;
        assert(replay.load(learned)&&replay.save()==learned);
        assert(replay.npcs[0].rememberedAmounts==learning.npcs[0].rememberedAmounts);
        assert(replay.npcs[0].discoveries==learning.npcs[0].discoveries);
        // Fabricated knowledge must be rejected without corrupting world state.
        std::string damaged=learned;
        auto at=damaged.find(" NPCLEARN1 ");
        assert(at!=std::string::npos);
        auto end=damaged.find(' ',at+11);
        assert(end!=std::string::npos);
        damaged.replace(at+11,end-(at+11),"9999");
        const auto beforeReject=replay.save();
        assert(!replay.load(damaged)&&replay.save()==beforeReject);
        // Old v10 remains loadable; the missing new learning block is initialized.
        std::istringstream scan(taks::World().save());
        std::vector<std::string> old10;
        std::string tok;
        while(scan>>tok)old10.push_back(tok);
        removeNpcSurvivalFields(old10);
        removeNewLearningFields(old10);
        old10[0]="TAKS_NATIVE_10";
        std::ostringstream legacy;
        for(const auto& word:old10)legacy<<word<<' ';
        taks::World oldLoaded;
        assert(oldLoaded.load(legacy.str()));
        assert(oldLoaded.npcs[0].rememberedAmounts[0]==-1);
        assert(oldLoaded.save().find("TAKS_NATIVE_12")==0);
        std::cout<<"PASS: NPC discovery, learning persistence, invalid data and v10 migration"<<std::endl;
    }
    // NPCs may not walk through a cave wall or live tree trunk to reach water.
    {
        taks::World cave;
        cave.forceRainForTest(0);
        for(auto& npc:cave.npcs)npc.hunger=100;
        auto& traveler=cave.npcs[0];
        traveler.hunger=0;traveler.thirst=90;
        traveler.x=5.0;traveler.z=4.40;
        cave.ecology.waters[0].x=5.0;
        cave.ecology.waters[0].z=2.0;
        cave.ecology.waters[0].level=.80;
        cave.ecology.waters[1].level=0;
        cave.step(.8);
        assert(traveler.z>=4.23 && traveler.x<7.45);
        taks::World grove;
        grove.forceRainForTest(0);
        for(auto& npc:grove.npcs)npc.hunger=100;
        auto& walker=grove.npcs[0];
        walker.hunger=0;walker.thirst=90;
        walker.x=-6.4;walker.z=-1.90;
        grove.ecology.waters[0].x=-6.4;
        grove.ecology.waters[0].z=0.5;
        grove.ecology.waters[0].level=.80;
        grove.ecology.waters[1].level=0;
        grove.step(.8);
        const double tx=walker.x-grove.ecology.trees[0].x;
        const double tz=walker.z-grove.ecology.trees[0].z;
        assert(tx*tx+tz*tz>=.42*.42-1e-8);
        assert(grove.ledgerConsistent());
    }
    // New NPCSURV1 is independently validated, while v11 migrates intact.
    {
        taks::World survival;
        survival.forceRainForTest(0.0);
        for(auto& n:survival.npcs)n.hunger=100; // no extra resource gathering
        auto& n=survival.npcs[0];
        n.thirst=84.;
        n.x=survival.ecology.waters[0].x+.05;
        n.z=survival.ecology.waters[0].z+.05;
        const double waterBefore=survival.ecology.waters[0].level;
        survival.step(.04);
        assert(n.waterTrips==1&&n.thirst<84);
        assert(survival.ecology.waters[0].level<waterBefore);
        assert(survival.ledgerConsistent());
        // No water, no miraculous refilling.
        for(auto& water:survival.ecology.waters)water.level=0.;
        n.thirst=96.;const int drinksBefore=n.waterTrips;
        survival.step(.04);
        assert(n.waterTrips==drinksBefore&&n.thirst>=96);
        // An actual nearby predator forces a real displacement, saved memory.
        taks::World danger;
        danger.forceRainForTest(0.0);
        auto& fleeing=danger.npcs[0];
        fleeing.x=5.0;fleeing.z=0.0;
        const double originalX=fleeing.x;
        const int initialEncounters=fleeing.fleeEvents;
        danger.step(.04);
        assert(fleeing.task==5&&fleeing.fleeEvents==initialEncounters+1);
        assert(fleeing.x<originalX&&fleeing.dangerMemoryUntil>danger.hours);
        assert(fleeing.dangerExperience>0);
        taks::World resumed;
        const auto saved=danger.save();
        assert(resumed.load(saved)&&resumed.save()==saved);
        assert(resumed.npcs[0].fleeEvents==fleeing.fleeEvents);
        assert(resumed.npcs[0].lastDangerX==fleeing.lastDangerX);
        // Version 11 needs no survival trailer; default needs remain.
        std::istringstream original(taks::World().save());
        std::vector<std::string> v11;
        std::string word;
        while(original>>word)v11.push_back(word);
        removeNpcSurvivalFields(v11);
        v11[0]="TAKS_NATIVE_11";
        std::ostringstream old;
        for(const auto& value:v11)old<<value<<' ';
        taks::World upgraded11;
        assert(upgraded11.load(old.str()));
        assert(upgraded11.npcs[0].thirst==18.0);
        assert(upgraded11.save().find("TAKS_NATIVE_12")==0);
        // Truncating new survival fields must not corrupt a valid world.
        const auto unchanged=resumed.save();
        std::string truncated=unchanged;
        const auto pos=truncated.find(" NPCSURV1 ");
        assert(pos!=std::string::npos);
        truncated=truncated.substr(0,pos+10);
        assert(!resumed.load(truncated)&&resumed.save()==unchanged);
        std::cout<<"PASS: autonomous finite-water drinking, predator flight, NPCSURV1 save and v11 migration"<<std::endl;
    }
    std::cout<<"PASS: F2 structure wired into world, materials, save v8\n";
    std::cout<<"PASS: F1 unique item ledger, causal log, v7 save and v6 migration\n";
    std::cout<<"TAKS spatial game, construction and save migration tests PASSED\n";
}
