#include "../CauseWorld.h"
#include <cassert>
#include <iostream>
#include <string>

int main(){
    taks::World w;
    // A meglévő tűzhely és tűz továbbra is ok-okozati alapú.
    assert(w.act("ignite").find("tűzhely")!=std::string::npos);
    assert(w.act("hearth").find("faág")!=std::string::npos);
    const int wood0=w.groundWood;
    for(int i=0;i<5;++i)w.act("wood");
    for(int i=0;i<2;++i)w.act("stone");
    w.act("tinder");
    assert(w.groundWood==wood0-5);
    assert(w.act("hearth").find("elkészült")!=std::string::npos);
    assert(w.carriedWood==2 && w.carriedStone==0);
    assert(w.act("ignite").find("meggyulladt")!=std::string::npos);
    assert(w.fireLit && w.fireFuel==2.0 && w.carriedWood==0);
    auto saved=w.save();taks::World restored;
    assert(restored.load(saved));assert(restored.save()==saved);
    assert(!restored.load("sérült vagy csaló mentés"));
    restored.step(20);assert(!restored.fireLit);assert(restored.fireFuel==0);

    // A teljes menedék egyetlen lépésben, alkatrészek nélkül nem teremthető meg.
    taks::World b;
    const auto initial=b.save();
    assert(b.act("roof").find("tetőváznak")!=std::string::npos);
    assert(b.act("base").find("cölöp")!=std::string::npos);
    assert(b.act("pole").find("kőeszköz")!=std::string::npos);
    assert(b.save()==initial); // egyetlen kudarc sem változtathatja meg az állapotot
    assert(b.act("shelter_enter").find("nem fedett")!=std::string::npos);

    // 2 kő => használható, kopó kőél + hulladék.
    for(int i=0;i<2;++i)assert(b.act("stone").find("felvéve")!=std::string::npos);
    assert(b.act("knap").find("kőszilánk")!=std::string::npos);
    assert(b.carriedStone==0 && b.chippedEdgeWear==6 && b.stoneDebris==1);
    assert(b.act("knap").find("van használható")!=std::string::npos);
    // 5 nyers ág -> 5 cölöp, kopó él és eltelt munkaidő.
    for(int i=0;i<5;++i)b.act("wood");
    double beforeWork=b.hours;
    for(int i=0;i<5;++i)assert(b.act("pole").find("cölöp")!=std::string::npos);
    assert(b.carriedWood==0 && b.preparedPoles==5 && b.chippedEdgeWear==1);
    assert(b.hours>beforeWork);
    assert(b.act("pole").find("nyers faág")!=std::string::npos);
    // 2 támasztókő -> tartóoszlopok; 3 cölöp -> tetőváz
    for(int i=0;i<2;++i)b.act("stone");
    assert(b.act("base").find("tartóoszlopok")!=std::string::npos);
    assert(b.shelterStage==1 && b.preparedPoles==3 && b.carriedStone==0);
    assert(b.act("frame").find("Tetőváz")!=std::string::npos);
    assert(b.shelterStage==2 && b.preparedPoles==0);
    assert(b.act("shelter_enter").find("nem fedett")!=std::string::npos);
    // 6 leveles ág => fedés, nincs újra ingyenes tárgyteremtés
    for(int i=0;i<6;++i)b.act("leaves");
    assert(b.groundLeaves==12 && b.carriedLeaves==6);
    assert(b.act("roof").find("levélfedés")!=std::string::npos);
    assert(b.shelterStage==3 && b.carriedLeaves==0);
    assert(b.act("roof").find("már elkészült")!=std::string::npos);
    assert(b.act("shelter_enter").find("Behúzódtál")!=std::string::npos);
    assert(b.underShelter);

    // Az elkészült menedék csak akkor véd, ha a karakter tényleg behúzódott.
    taks::World exposed=b,sheltered=b;
    exposed.underShelter=false;
    exposed.playerWetness=0.1;
    sheltered.playerWetness=0.1;
    exposed.forceRainForTest(0.8);
    sheltered.forceRainForTest(0.8);
    exposed.step(1.0);
    sheltered.step(1.0);
    assert(sheltered.playerWetness < exposed.playerWetness);
    // Visszatöltéskor a munkafázis, készlet és kitettség is megmarad.
    const std::string fullSave=b.save();
    taks::World resumed;
    assert(resumed.load(fullSave) && resumed.save()==fullSave);
    assert(resumed.shelterStage==3 && resumed.underShelter);
    const auto stable=resumed.save();
    assert(!resumed.load("TAKS_NATIVE_2 1 2 3 broken data"));
    assert(resumed.save()==stable);

    // Korábbi kiadás mentéseinek migrációja, a régi világ megőrzése.
    taks::World old;
    assert(old.load("TAKS_NATIVE_1 14 8 5 0 0 0 6 0.4 0 0.13 0.13 100 0 0 0 0"));
    assert(old.shelterStage==0 && old.groundLeaves==18);
    assert(old.save().find("TAKS_NATIVE_2")==0);
    // Időjárás és ismeretlen cselekvés.
    taks::World wet;double m=wet.woodMoisture;
    wet.forceRainForTest(1);wet.step(1);
    assert(wet.woodMoisture>m && wet.groundMoisture>0.13);
    const auto beforeUnknown=wet.save();
    assert(wet.act("unknown").find("nem módosult")!=std::string::npos);
    assert(wet.save()==beforeUnknown);
    std::cout<<"TAKS native cause/effect and staged shelter tests PASSED\n";
}
