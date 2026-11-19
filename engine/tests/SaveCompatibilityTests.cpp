#include "../CauseWorld.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static std::vector<std::string> tokensFrom(const char* filename){
    std::ifstream source(filename);
    assert(source);
    std::vector<std::string> result;
    for(std::string token;source>>token;)result.push_back(token);
    return result;
}

static std::string encode(const std::vector<std::string>& tokens){
    std::ostringstream out;
    for(const auto& token:tokens)out<<token<<' ';
    return out.str();
}

static void checkRoundTrip(const taks::World& original){
    taks::World restored;
    assert(restored.load(original.save()));
    assert(restored.save()==original.save() && restored.ledgerConsistent());
}

int main(int argc,char** argv){
    assert(argc==3);
    auto ledger=tokensFrom(argv[1]);  // Serialized by the previous GitHub engine.
    auto water=tokensFrom(argv[2]);   // Serialized by the 0.14 source ZIP.
    taks::World withItems,withWater;
    assert(withItems.load(encode(ledger)));
    assert(withItems.carriedStone==2 && withItems.ledgerConsistent());
    assert(withItems.structure.parts.size()==1 && withItems.ecology.grass[1].trampling>.44);
    const auto item=withItems.items.back().id;
    checkRoundTrip(withItems);
    assert(withWater.load(encode(water)));
    assert(withWater.waterCollectorStage==2 && withWater.storedRainwater==9.75);
    assert(withWater.playerThirst==53.25 && withWater.ecology.visibleFootprintCount()==1);
    assert(std::abs(withWater.ecology.waters[1].level-.345)<1e-8);
    assert(withWater.carriedStone==2 && withWater.ledgerConsistent());
    checkRoundTrip(withWater);

    // The older structures used STRUCT1, without a mode token.
    ledger[0]="TAKS_NATIVE_8";
    auto structure=std::find(ledger.begin(),ledger.end(),"STRUCT2");
    assert(structure!=ledger.end());
    *structure="STRUCT1";
    ledger.erase(structure+1);
    taks::World structureV8;
    assert(structureV8.load(encode(ledger)));
    assert(structureV8.structure.parts.size()==1 && structureV8.items.back().id==item);
    assert(structureV8.save()==withItems.save());

    // Version 7 existed in two independent releases.
    structure=std::find(ledger.begin(),ledger.end(),"STRUCT1");
    ledger.erase(structure,ledger.end());
    ledger[0]="TAKS_NATIVE_7";
    taks::World itemsV7;
    assert(itemsV7.load(encode(ledger)) && itemsV7.structure.parts.empty());
    assert(itemsV7.ecology.grass[1].trampling>.44 && itemsV7.items.back().id==item);

    water[0]="TAKS_NATIVE_7";
    auto ecology=std::find(water.begin(),water.end(),"ECO3");
    assert(ecology!=water.end());
    *ecology="ECO2";
    water.resize(water.size()-taks::LivingWorld::waterBodyCount*6-3);
    taks::World tracksV7;
    assert(tracksV7.load(encode(water)) && tracksV7.ecology.visibleFootprintCount()==1);
    assert(tracksV7.waterCollectorStage==0 && tracksV7.ledgerConsistent());
    checkRoundTrip(tracksV7);

    auto tag=std::find(ledger.begin(),ledger.end(),"LEDG1");
    assert(tag!=ledger.end());
    ledger.erase(tag,ledger.end());
    const auto unchanged=withWater.save();
    assert(!withWater.load(encode(ledger)) && withWater.save()==unchanged);
    ledger[0]="TAKS_NATIVE_6";
    taks::World oldStock;
    assert(oldStock.load(encode(ledger)) && oldStock.carriedStone==2 && oldStock.ledgerConsistent());

    // The water collector must consume the same stones recorded in the ledger.
    taks::World builder;
    builder.playerX=builder.waterCollectorX;builder.playerZ=builder.waterCollectorZ;
    builder.preparedPoles=2;builder.grantCarriedForTest(1,2);
    builder.act("catchment_frame");
    assert(builder.waterCollectorStage==1 && builder.consumedItems[1]==2 && builder.ledgerConsistent());
    checkRoundTrip(builder);
    std::cout<<"PASS: both save lineages, mixed version 7/8 layouts and collector inventory\n";
}
