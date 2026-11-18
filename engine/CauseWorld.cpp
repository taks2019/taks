#include "CauseWorld.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace taks {
namespace { double clamp(double v, double lo, double hi){return std::max(lo, std::min(hi,v));} }

// ---------------- Egyedi tárgyak főkönyve ----------------
void World::logCause(const std::string& line){
    std::ostringstream o;o<<'#'<<eventCounter<<" t="<<std::fixed<<std::setprecision(2)<<hours<<"h "<<line;
    causeLog.push_back(o.str());
    if(causeLog.size()>64)causeLog.erase(causeLog.begin(),causeLog.begin()+(causeLog.size()-64));
}
unsigned World::countItems(int type,int where,int node,int by) const{
    unsigned n=0;
    for(const Item& it:items)
        if(it.type==type&&it.where==where&&(node<0||it.node==node)&&(by<0||it.by==by))++n;
    return n;
}
void World::rebuildLedgerFromCounts(){
    items.clear();
    for(int i=0;i<resourceCount;++i){
        const int type=nodes[i].type;if(type>1)continue;
        for(int k=0;k<nodes[i].amount;++k)items.push_back({nextItemId++,type,0,i,-1});
    }
    for(int k=0;k<carriedWood;++k)items.push_back({nextItemId++,0,1,-1,-1});
    for(int k=0;k<carriedStone;++k)items.push_back({nextItemId++,1,1,-1,-1});
    for(int k=0;k<campStock[0];++k)items.push_back({nextItemId++,0,2,-1,-1});
    for(int k=0;k<campStock[1];++k)items.push_back({nextItemId++,1,2,-1,-1});
    for(int i=0;i<npcCount;++i)
        if(npcs[i].carrying>0&&npcs[i].role<2)
            for(int k=0;k<npcs[i].carrying;++k)items.push_back({nextItemId++,npcs[i].role,4,-1,i});
    ledgerReady=true;
}
void World::ensureLedger(){ if(!ledgerReady)rebuildLedgerFromCounts(); }
bool World::ledgerConsistent() const{
    if(!ledgerReady)return true;
    for(int i=0;i<resourceCount;++i){
        const int type=nodes[i].type;if(type>1)continue;
        if(countItems(type,0,i)!=static_cast<unsigned>(nodes[i].amount))return false;
    }
    if(countItems(0,1)!=static_cast<unsigned>(carriedWood)||countItems(1,1)!=static_cast<unsigned>(carriedStone))return false;
    if(countItems(0,2)!=static_cast<unsigned>(campStock[0])||countItems(1,2)!=static_cast<unsigned>(campStock[1]))return false;
    for(int i=0;i<npcCount;++i){
        unsigned held=0;
        for(const Item& it:items)if(it.where==4&&it.by==i)++held;
        const unsigned expected=(npcs[i].role<2)?static_cast<unsigned>(npcs[i].carrying):0u;
        if(held!=expected)return false;
    }
    for(size_t a=0;a<items.size();++a){
        if(items[a].id==0||items[a].id>=nextItemId)return false;
        for(size_t b=a+1;b<items.size();++b)if(items[a].id==items[b].id)return false;
    }
    return true;
}
void World::moveItem(int type,int fromWhere,int fromNode,int fromBy,int toWhere,int toNode,int toBy,const char* cause){
    ensureLedger();
    int idx=-1;
    for(size_t i=0;i<items.size();++i){
        const Item& it=items[i];
        if(it.type==type&&it.where==fromWhere&&(fromNode<0||it.node==fromNode)&&(fromBy<0||it.by==fromBy)){idx=static_cast<int>(i);break;}
    }
    if(idx<0)return;
    Item& it=items[idx];
    std::ostringstream o;o<<(type==0?"faág":"kő")<<" #"<<it.id<<" ["<<cause<<"]: hely "<<fromWhere<<"->"<<toWhere;
    it.where=toWhere;it.node=toNode;it.by=toBy;
    logCause(o.str());
}
void World::grantCarriedForTest(int type,int count){
    ensureLedger();
    if(type<0||type>1)return;
    for(int k=0;k<count;++k){items.push_back({nextItemId++,type,1,-1,-1});(type==0?carriedWood:carriedStone)++;}
    logCause("TESZT: tárgyak adva a játékosnak");
}
void World::consumeItems(int type,int count,const char* cause){
    ensureLedger();
    for(int k=0;k<count;++k){
        int idx=-1;
        for(size_t i=0;i<items.size();++i)if(items[i].type==type&&items[i].where==1){idx=static_cast<int>(i);break;}
        if(idx<0)return;
        std::ostringstream o;o<<(type==0?"faág":"kő")<<" #"<<items[idx].id<<" ["<<cause<<"]: elhasználva";
        items.erase(items.begin()+idx);++consumedItems[type];
        logCause(o.str());
    }
}
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
        if(siteCount==0)return "CÉL: Keress alkalmas helyet, majd egy kővel jelöld ki az építési területet.";
        return "CÉL: Menj a kijelölt építési helyhez, és állítsd fel az oszlopokat.";
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
    ensureLedger();
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
                    if(npc.role<2)moveItem(npc.role,4,-1,i,2,-1,-1,"társ leadja a táborban");
                    campStock[npc.role]+=npc.carrying;
                    npc.delivered+=npc.carrying;
                    npc.carrying=0;
                    npc.task=0;
                    ++eventCounter;
                }else{
                    const double step=std::min(length,.25*(npc.wetness>.6?.65:1.)*ecology.walkSpeedFactor());
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
                if(node.type<2)moveItem(node.type,0,chosen,-1,4,-1,i,"társ felveszi");
                --node.amount;
                npc.carrying=1;
                npc.task=2;
                npc.energy-=1.8;
                syncGroundCounts();
                ++eventCounter;
            }else{
                const double step=std::min(length,.22*(npc.wetness>.6?.65:1.)*ecology.walkSpeedFactor());
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
    const double requested=std::hypot(dx,dz);
    if(requested<0.00001)return "A karakter helyben maradt.";
    if(stamina<0.5)return "A karakter kimerült: pihennie kell.";

    // Native step is a distance, not an arbitrary frames-per-second velocity.
    // Cap any unexpectedly large JNI request and incorporate real soil grip.
    const double factor=std::min(1.0,0.75/requested)*ecology.walkSpeedFactor();
    dx*=factor;dz*=factor;
    const double distance=std::hypot(dx,dz);
    const int slices=std::max(1,static_cast<int>(std::ceil(distance/.10)));
    const double sx=dx/slices,sz=dz/slices;

    auto walkable=[this](double x,double z)->bool{
        // Body radius avoids walking through the edge of the cave wall.
        if(x< -7.5||x>7.5||z< -7.5||z>13.0)return false;
        if(z>2.67&&z<4.23&&std::abs(x)>1.22&&std::abs(x)<7.8)return false;
        // Living tree trunks are solid until the tree dies, not invisible.
        for(const auto& tree:ecology.trees){
            if(!tree.alive||tree.heightM<=.5)continue;
            const double ax=x-tree.x,az=z-tree.z;
            if(ax*ax+az*az<.38*.38)return false;
        }
        return true;
    };
    double mx=playerX,mz=playerZ,traveled=0;
    // Swept substeps: cannot jump over narrow stone/trunk obstacles.
    // If a diagonal hits a wall, preserve the unobstructed direction:
    // physically slide along the wall instead of freezing the whole stick.
    for(int i=0;i<slices;++i){
        const double originalX=mx,originalZ=mz;
        if(walkable(mx+sx,mz+sz)){
            mx+=sx;mz+=sz;
        }else{
            const bool tryXFirst=std::abs(sx)>=std::abs(sz);
            if(tryXFirst){
                if(walkable(mx+sx,mz))mx+=sx;
                if(walkable(mx,mz+sz))mz+=sz;
            }else{
                if(walkable(mx,mz+sz))mz+=sz;
                if(walkable(mx+sx,mz))mx+=sx;
            }
        }
        traveled+=std::hypot(mx-originalX,mz-originalZ);
    }
    if(traveled<.00001){
        if(std::abs(playerX)>=7.49||playerZ<=-7.49||playerZ>=12.99)
            return "A világ jelenlegi bejárható határához értél.";
        return "A sziklafal vagy egy fatörzs elállja az utat.";
    }
    const double oldX=playerX,oldZ=playerZ;
    playerX=mx;playerZ=mz;
    ecology.trampleTrail(oldX,oldZ,playerX,playerZ,clamp(ecology.mud+groundMoisture*.45,0,1));
    walkPhase+=traveled*5.5;
    stamina=clamp(stamina-traveled*(0.50+groundMoisture*.65+ecology.mud*.8+
                   ecology.snowDepth*.35+ecology.slipRisk()*.4),0.0,100.0);
    step(traveled*.013);
    if(underShelter&&!protectedAtPlayer())underShelter=false;
    ++eventCounter;
    return nearby();
}
std::string World::act(const std::string& action) {
    ensureLedger();
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
        if(role<2)moveItem(role,2,-1,-1,1,-1,-1,"játékos átveszi a tábori készletből");
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
        if(type<2)moveItem(type,0,nodeId,-1,1,-1,-1,"játékos felveszi a lelőhelyről");
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
        consumeItems(1,2,"kőmegmunkálás: ütőkő és nyersanyag");
        carriedStone-=2;stoneDebris+=1;chippedEdgeWear=6;stamina-=6;step(0.40);++eventCounter;
        return "Éles kőszilánk készült: 2 kő -> 1 használható él + kőhulladék. A szerszám kopik.";
    }
    if(action=="pole") {
        if(chippedEdgeWear<=0) return "Fa megmunkálásához előbb éles kőeszköz szükséges.";
        if(carriedWood<1) return "Nincs nyers faág, amiből cölöp készülhetne.";
        if(stamina<5) return "Cölöp készítése: nincs elég állóképességed.";
        if(woodMoisture>0.85) return "A fa nagyon átázott: várj a megmunkálással.";
        consumeItems(0,1,"faág cölöppé munkálva");
        --carriedWood;++preparedPoles;--chippedEdgeWear;
        if(chippedEdgeWear==0)++stoneDebris;
        stamina-=5;step(0.30);++eventCounter;
        return "Egy faágból megmunkált cölöp lett; a kőeszköz éle kopott. (+0,3 óra)";
    }
    if(action=="site") {
        if(siteCount>=maxBuildSites)return "A próbaszektorban egyszerre három építési hely kezelhető.";
        if(playerX<-5.7||playerX>5.7||playerZ<-6.0||playerZ>1.3)
            return "Itt nincs elegendő szabad építési terület: menj síkabb, nyitott részre.";
        if(isNear(-.30,-3.40,1.9)||isNear(-1.15,-1.8,1.8))
            return "A tábori készletet és a tűzhelyet nem lehet beépíteni.";
        for(int i=0;i<siteCount;++i){
            const double dx=playerX-sites[i].x,dz=playerZ-sites[i].z;
            if(dx*dx+dz*dz<3.7*3.7)return "Az új menedék túl közel lenne egy másikhoz.";
        }
        if(carriedStone<1)return "Az építési hely kijelöléséhez egy követ kell elhelyezned a terepen.";
        if(stamina<2)return "A kijelöléshez előbb pihenned kell.";
        consumeItems(1,1,"építési hely jelölőköve");
        --carriedStone;stamina-=2;
        auto& site=sites[siteCount++];
        site.x=playerX;site.z=playerZ;site.stage=0;site.condition=100.0;
        step(.15);++eventCounter;
        return "Új építési hely kijelölve. A jelölőkő felhasználva; ide kerülhet az oszlopszerkezet.";
    }
    if(action=="base" || action=="frame" || action=="roof" || action=="repair") {
        const int index=nearbySiteIndex();
        if(index<0)return "A szerkezethez menj a kijelölt építési helyre, vagy jelölj ki újat.";
        auto& site=sites[index];
        if(action=="base"){
            if(site.stage!=0)return "Ezen a helyen az oszlopok már elkészültek.";
            if(preparedPoles<2)return "Oszlopok: két megmunkált cölöp hiányzik.";
            if(carriedStone<2)return "Oszlopok: két támasztókő hiányzik.";
            if(stamina<12)return "Oszlopok: a munkához kevés az állóképesség.";
            consumeItems(1,2,"oszlop támasztókövei");
            preparedPoles-=2;carriedStone-=2;stamina-=12;site.stage=1;
            shelterStage=std::max(shelterStage,site.stage);
            step(1.0);++eventCounter;
            return "A két oszlop a kijelölt helyen áll. Az anyagok ténylegesen elfogytak.";
        }
        if(action=="frame"){
            if(site.stage<1)return "Előbb két tartóoszlopot kell felállítani ezen a helyen.";
            if(site.stage>1)return "Itt már elkészült a tetőváz.";
            if(preparedPoles<3)return "A tetővázhoz három megmunkált cölöp kell.";
            if(stamina<14)return "A tetővázhoz előbb pihenned kell.";
            preparedPoles-=3;stamina-=14;site.stage=2;
            shelterStage=std::max(shelterStage,site.stage);
            step(1.1);++eventCounter;
            return "A tetőváz elkészült ezen a helyen, de még nincs fedése.";
        }
        if(action=="roof"){
            if(site.stage<2)return "Előbb oszlopokat és tetővázat kell megépíteni.";
            if(site.stage==3)return "Ez a menedék már fedett.";
            if(carriedLeaves<6)return "Fedéshez hat leveles ág szükséges.";
            if(stamina<9)return "A fedéshez előbb pihenned kell.";
            carriedLeaves-=6;stamina-=9;site.stage=3;
            shelterStage=std::max(shelterStage,site.stage);
            step(.8);++eventCounter;
            return "Felkerült a fedés. A szerkezet csak akkor véd, ha behúzódsz alá.";
        }
        if(site.stage<3)return "Javítani csak felépített menedéket lehet.";
        if(site.condition>=99.5)return "A szerkezet még ép, nem szorul javításra.";
        if(preparedPoles<1 || carriedLeaves<2)return "Javításhoz egy cölöp és két leveles ág kell.";
        if(stamina<8)return "Javításhoz előbb pihenned kell.";
        --preparedPoles;carriedLeaves-=2;stamina-=8;
        site.condition=std::min(100.0,site.condition+35.0);
        step(.7);++eventCounter;
        return "A sérült tartó és a fedés javítva; alkatrész és idő fogyott.";
    }
    if(action=="shelter_enter") {
        if(!protectedAtPlayer())
            return "Csak ténylegesen fedett, ép menedék alá húzódhatsz. Menj közelebb vagy javítsd meg.";
        if(underShelter)return "Már a fedés alatt vagy.";
        underShelter=true;step(.1);++eventCounter;
        return "Behúzódtál a fedés alá: csökkent a közvetlen esőterhelés.";
    }
    if(action=="shelter_exit") {
        if(!underShelter) return "Jelenleg nem vagy a menedékben.";
        underShelter=false;step(0.10);++eventCounter;
        return "Elhagytad a menedéket: újra közvetlenül ér a környezet.";
    }
    if(action=="part_pole"||action=="part_beam"||action=="part_stone"||action=="part_plank"){
        const int kind=action=="part_pole"?PART_POLE:action=="part_beam"?PART_BEAM:action=="part_stone"?PART_STONE:PART_PLANK;
        const int poleCost=kind==PART_BEAM?2:(kind==PART_POLE||kind==PART_PLANK)?1:0;
        if(poleCost>0&&preparedPoles<poleCost)return kind==PART_BEAM?"Gerendához két megmunkált cölöp kell.":"Ehhez egy megmunkált cölöp kell.";
        if(kind==PART_STONE&&carriedStone<1)return "Nincs nálad kő, amit le lehetne rakni.";
        if(stamina<3)return "Az építéshez előbb pihenned kell.";
        // Új alkatrész a játékos előtt áll; ha alatta már van alkatrész, rá kerül (és rögzül hozzá).
        const double px=playerX+0.6,pz=playerZ;
        double y=0.0;unsigned underId=0;
        for(const auto& q:structure.parts){
            const double dx=q.x-px,dz=q.z-pz;
            if(dx*dx+dz*dz>0.45*0.45)continue;
            double top=q.y+(q.kind==PART_POLE||q.kind==PART_STONE?q.length:(q.kind==PART_BEAM?0.25:0.1));
            if(top>y){y=top;underId=q.id;}
        }
        std::string why;
        const unsigned before=structure.nextId;
        const unsigned id=structure.addPart(kind,px,y,pz,0.0,&why);
        if(id==0)return why.empty()?"Ide nem rakható alkatrész.":why;
        if(underId!=0&&!structure.addJoint(id,underId,0,&why)){
            // Rögzítés nélkül nem marad ott: visszavonjuk a lerakást, anyag nem fogyott.
            structure.undo();structure.nextId=std::max(structure.nextId,before+1);
            return "Nem sikerült rögzíteni az alkatrészt: "+why;
        }
        if(poleCost>0)preparedPoles-=poleCost;
        if(kind==PART_STONE)consumeItems(1,1,"szerkezet alkatrésze: kő");
        if(kind==PART_STONE)--carriedStone;
        stamina-=3;step(.2);++eventCounter;
        std::ostringstream out;
        out<<"Alkatrész lerakva (#"<<id<<"). ";
        const StructureReport rep=structure.analyze();
        out<<(rep.stable?"A szerkezet most stabil.":"FIGYELEM: a szerkezet nem stabil (túlterhelt vagy lebegő rész).");
        return out.str();
    }
    if(action=="part_cut"||action=="part_turn"){
        // A játékoshoz legközelebbi alkatrész (1,5 m-en belül).
        unsigned target=0;double best=1.5*1.5;
        for(const auto& q:structure.parts){
            const double dx=q.x-playerX,dz=q.z-playerZ;const double d=dx*dx+dz*dz;
            if(d<best){best=d;target=q.id;}
        }
        if(target==0)return "Nincs alkatrész a közelben: menj közelebb.";
        std::string why;
        if(action=="part_turn"){
            if(stamina<1)return "A forgatáshoz előbb pihenned kell.";
            const Part* p=structure.find(target);
            if(!structure.rotatePart(target,p->yaw+0.7853981633974483,&why))return why;
            stamina-=1;step(0.05);++eventCounter;
            return "Az alkatrész (#"+std::to_string(target)+") elfordítva 45°-kal.";
        }
        if(chippedEdgeWear<=0)return "Vágáshoz éles kőeszköz kell.";
        if(stamina<4)return "A vágáshoz előbb pihenned kell.";
        const unsigned created=structure.cutPart(target,0.5,&why);
        if(created==0)return why.empty()?"Nem vágható el.":why; // hiba: sem kopás, sem idő nem fogy
        --chippedEdgeWear;if(chippedEdgeWear==0)++stoneDebris;
        stamina-=4;step(0.3);++eventCounter;
        std::ostringstream out;
        out<<"Az alkatrész (#"<<target<<") kettévágva; új darab: #"<<created<<". Kőél kopott, az anyag megmaradt. ";
        out<<(structure.analyze().stable?"A szerkezet stabil.":"FIGYELEM: a szerkezet nem stabil.");
        return out.str();
    }
    if(action=="hearth") {
        if(!isNear(-1.15,-1.8,2.7))return "Tűzhelyet csak a kijelölt munkaterületen építhetsz: menj oda.";
        if(hearthBuilt) return "A tűzhely már felépült.";
        if(carriedWood<3) return "Tűzhely: 3 faág szükséges.";
        if(carriedStone<2) return "Tűzhely: 2 kő szükséges.";
        if(stamina<12) return "Tűzhely: a munkához túl kevés az állóképességed.";
        consumeItems(0,3,"tűzhely építése: faág");consumeItems(1,2,"tűzhely építése: kő");
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
        consumeItems(0,2,"tűzgyújtás: tüzelő faág");
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
    structure.wear(hoursElapsed*(0.002+rainIntensity*0.03)); // az eső koptatja az alkatrészeket
    updateNpcs(hoursElapsed);
    for(int i=0;i<siteCount;++i){
        auto& site=sites[i];
        if(site.stage>=1){
            site.condition=clamp(site.condition-hoursElapsed*(0.10+rainIntensity*1.15),0.,100.);
        }
    }
    if(underShelter && !protectedAtPlayer())underShelter=false;
    // Közelítő determinisztikus időjárás, nem hiteles paleoklimatológiai rekonstrukció.
    humidity=clamp(0.38+0.32*std::sin(hours*0.30),0.04,0.98);
    if(!testWeather) rainIntensity=clamp((humidity-0.62)*2.8,0.0,1.0);
    const double wind=windStrength();
    groundMoisture=clamp(groundMoisture+rainIntensity*0.25*hoursElapsed-
                         (1-rainIntensity)*(.035+.022*wind)*hoursElapsed,0,1);
    woodMoisture=clamp(woodMoisture+rainIntensity*0.23*hoursElapsed-
                       (1-rainIntensity)*(.07+.035*wind)*hoursElapsed,0,1);
    // This first Wonderwerk climate profile is explicitly provisional: seasonal
    // gameplay temperature, not a reconstruction of local Pleistocene weather.
    constexpr double twoPi=6.2831853071795864769;
    const double temperature=22.+4.*std::sin((hours/(24.*365.))*twoPi)
                            +2.*std::sin((std::fmod(hours,24.)/24.)*twoPi);
    const double sunlight=clamp(1.-std::abs(std::fmod(hours,24.)-13.)/12.,0.,1.);
    ecology.step(hoursElapsed,rainIntensity,groundMoisture,sunlight,temperature,wind);
    // A tető NEM teleportálja szárazra a játékost. Csak fedett menedékbe húzódva véd.
    const double exposure=(underShelter && protectedAtPlayer())?0.07:1.0;
    playerWetness=clamp(playerWetness+rainIntensity*0.34*exposure*hoursElapsed
                        -(underShelter?0.09:0.035)*hoursElapsed,0.0,1.0);
    if(fireLit) {
        fireFuel-=hoursElapsed*(0.35+0.15*rainIntensity+0.10*wind);
        if(rainIntensity>0.85 || fireFuel<=0.0){fireFuel=0.0;fireLit=false;++eventCounter;}
    }
    const double gain=(underShelter?0.85:0.35)*hoursElapsed;
    const double dampPenalty=playerWetness>0.70?1.6*hoursElapsed:0.0;
    stamina=clamp(stamina+gain-dampPenalty,0.0,100.0);
}
std::string World::shelterStatus() const {
    std::ostringstream os;os<<std::fixed<<std::setprecision(1);
    os<<"ÉPÍTÉSI HELYEK: "<<siteCount<<"/"<<maxBuildSites<<"\n";
    const int i=nearbySiteIndex();
    if(i<0){
        os<<"Nincs közel kijelölt építési hely. Ha van nálad kő, válaszd a HELY KIJELÖLÉSE műveletet.";
    }else{
        const auto& site=sites[i];
        os<<"KÖZELI HELY #"<<i+1<<" ("<<site.x<<", "<<site.z<<"): ";
        switch(site.stage){
            case 0:os<<"Kijelölt: a tartóoszlopok még hiányoznak. 2 cölöp + 2 kő kell.";break;
            case 1:os<<"Álló oszlopok. Tetővázhoz 3 cölöp kell.";break;
            case 2:os<<"Tetőváz áll, fedés nélkül még beázik. 6 leveles ág kell.";break;
            case 3:os<<"Fedett menedék. Épség: "<<site.condition<<"%.";break;
        }
    }
    os<<"\nKŐÉL: "<<chippedEdgeWear<<"/6 · CÖLÖP: "<<preparedPoles
      <<" · LEVEL: "<<carriedLeaves;
    if(underShelter)os<<"\nA karakter védett helyen van.";
    return os.str();
}
std::string World::summary() const {
    std::ostringstream os;os<<std::fixed<<std::setprecision(2);
    const double wind=windStrength();
    const char* windLabel=wind<.30?"csendes":wind<.52?"mérsékelt":"élénk";
    os<<"IDŐ: "<<hours<<" óra\n"
      <<"CSAPADÉK: "<<rainIntensity*100<<"% | SZÉL: "<<wind*100<<"% ("<<windLabel<<")"
      <<" | TALAJNEDVESSÉG: "<<groundMoisture*100<<"%\n"
      <<"FA NEDVESSÉGE: "<<woodMoisture*100<<"% | ÁTÁZÁS: "<<playerWetness*100<<"%\n"
      <<"KÖRNYEZET: "<<groundWood<<" faág, "<<groundStone<<" kő, "<<groundLeaves<<" leveles ág, "<<groundTinder<<" gyújtós\n"
      <<"KÉSZLET: "<<carriedWood<<" faág, "<<carriedStone<<" kő, "<<carriedLeaves<<" leveles ág, "<<carriedTinder<<" gyújtós\n"      <<"JÁTÉKOS: "<<playerX<<" m, "<<playerZ<<" m | "<<nearby()<<"\n"

      <<"CÖLÖP: "<<preparedPoles<<" | KŐÉL: "<<chippedEdgeWear<<"/6 | KŐHULLADÉK: "<<stoneDebris<<"\n"
      <<"ÁLLÓKÉPESSÉG: "<<stamina<<"%\n"
      <<"MENEDÉKEK: "<<siteCount<<" db | LEGFEJLETTEBB: "<<shelterStage<<"/3 | VÉDETT: "<<(underShelter?"igen":"nem")<<"\n"
      <<"TŰZHELY: "<<(hearthBuilt?"felépült":"hiányzik")<<" | TŰZ: "<<(fireLit?"ég":"nem ég")<<"\n"
      <<"TÁBOR: "<<campStatus()<<"\n"
      <<"KÖZÖSSÉG: 4 társ · önálló munkavégzés\n"
      <<"OKOZATI ESEMÉNYEK: "<<eventCounter<<"\n"
      <<ecology.status();
    return os.str();
}
double World::windStrength() const {
    if(!std::isfinite(hours)||!std::isfinite(humidity)||!std::isfinite(rainIntensity))return .25;
    constexpr double twoPi=6.2831853071795864769;
    const double hour=std::fmod(std::fmod(hours,24.0)+24.0,24.0);
    const double daily=.5+.5*std::sin((hour/24.0)*twoPi-.7);
    const double gust=.5+.5*std::sin(hours*.17+std::sin(hours*.023)*.8);
    const double dryAir=clamp((.55-humidity)/.55,0.0,1.0);
    return clamp(.12+.18*daily+.13*gust+.16*rainIntensity+.06*dryAir,.08,.78);
}
std::string World::save() const {
    std::ostringstream o;o<<std::setprecision(17);
    o<<"TAKS_NATIVE_9 "<<groundWood<<' '<<groundStone<<' '<<groundTinder<<' '
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
    o<<' '<<siteCount;
    for(const auto& site:sites)
        o<<' '<<site.x<<' '<<site.z<<' '<<site.stage<<' '<<site.condition;
    ecology.write(o);
    // Egyedi tárgyak főkönyve (LEDG1): csak a meglévő tárgyak; az elhasználtakat számláló őrzi.
    o<<" LEDG1 "<<nextItemId<<' '<<consumedItems[0]<<' '<<consumedItems[1]<<' '<<items.size();
    for(const Item& it:items)o<<' '<<it.id<<' '<<it.type<<' '<<it.where<<' '<<it.node<<' '<<it.by;
    o<<' '<<structure.save(); // F2: szerkezetek (STRUCT2; a STRUCT1 is betölthető)
    return o.str();
}
bool World::load(const std::string& data) {
    std::istringstream is(data);std::string header;World tmp;int hearth=0,fire=0,covered=0;
    if(!(is>>header) ||
       (header!="TAKS_NATIVE_1" && header!="TAKS_NATIVE_2" && header!="TAKS_NATIVE_3" && header!="TAKS_NATIVE_4" && header!="TAKS_NATIVE_5" && header!="TAKS_NATIVE_6" && header!="TAKS_NATIVE_7" && header!="TAKS_NATIVE_8" && header!="TAKS_NATIVE_9"))return false;
    const bool hasLedger=(header=="TAKS_NATIVE_7"||header=="TAKS_NATIVE_8"||header=="TAKS_NATIVE_9");
    const bool hasStruct=(header=="TAKS_NATIVE_8"||header=="TAKS_NATIVE_9");
    if(hasLedger)header="TAKS_NATIVE_6";
    if(!(is>>tmp.groundWood>>tmp.groundStone>>tmp.groundTinder
      >>tmp.carriedWood>>tmp.carriedStone>>tmp.carriedTinder
      >>tmp.hours>>tmp.humidity>>tmp.rainIntensity>>tmp.woodMoisture
      >>tmp.groundMoisture>>tmp.stamina>>tmp.fireFuel>>hearth>>fire>>tmp.eventCounter))return false;
    if(header!="TAKS_NATIVE_1"){
        if(!(is>>tmp.groundLeaves>>tmp.carriedLeaves>>tmp.chippedEdgeWear>>tmp.stoneDebris
              >>tmp.preparedPoles>>tmp.shelterStage>>tmp.playerWetness>>covered))return false;
    }
    if(header=="TAKS_NATIVE_3"||header=="TAKS_NATIVE_4"||header=="TAKS_NATIVE_5"||header=="TAKS_NATIVE_6"){
        if(!(is>>tmp.playerX>>tmp.playerZ>>tmp.walkPhase))return false;
        for(auto& node:tmp.nodes){if(!(is>>node.amount))return false;}
    }
    if(header=="TAKS_NATIVE_4"||header=="TAKS_NATIVE_5"||header=="TAKS_NATIVE_6"){
        for(int& amount:tmp.campStock)if(!(is>>amount))return false;
        if(!(is>>tmp.npcSimulatedHours))return false;
        for(auto& n:tmp.npcs) {
            if(!(is>>n.x>>n.z>>n.energy>>n.hunger>>n.wetness
                   >>n.role>>n.carrying>>n.task>>n.delivered>>n.walkPhase))return false;
        }
    }
    if(header=="TAKS_NATIVE_5"||header=="TAKS_NATIVE_6"){
        if(!(is>>tmp.siteCount))return false;
        for(auto& site:tmp.sites)
            if(!(is>>site.x>>site.z>>site.stage>>site.condition))return false;
    }
    if(header=="TAKS_NATIVE_6" && !tmp.ecology.read(is))return false;
    std::vector<Item> loadedItems;unsigned loadedNext=1,loadedConsumed[2]={0,0};
    if(hasLedger){
        std::string tag;size_t n=0;
        if(!(is>>tag)||tag!="LEDG1"||!(is>>loadedNext>>loadedConsumed[0]>>loadedConsumed[1]>>n)||n>100000)return false;
        for(size_t i=0;i<n;++i){
            Item it;
            if(!(is>>it.id>>it.type>>it.where>>it.node>>it.by))return false;
            if(it.type<0||it.type>1||(it.where!=0&&it.where!=1&&it.where!=2&&it.where!=4))return false;
            if(it.where==0&&(it.node<0||it.node>=World::resourceCount))return false;
            if(it.where==4&&(it.by<0||it.by>=World::npcCount))return false;
            loadedItems.push_back(it);
        }
    }
    Structure loadedStructure;
    if(hasStruct){
        std::string rest;std::getline(is,rest);
        if(!loadedStructure.load(rest))return false;
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

    if(header=="TAKS_NATIVE_4"||header=="TAKS_NATIVE_5"||header=="TAKS_NATIVE_6"){
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
    if(header=="TAKS_NATIVE_3"||header=="TAKS_NATIVE_4"||header=="TAKS_NATIVE_5"||header=="TAKS_NATIVE_6"){
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
    if(header!="TAKS_NATIVE_5" && header!="TAKS_NATIVE_6"){
        // Régi APK-kban a menedék egy rögzített helyen állt.
        // Csak a már megépített szerkezetet migráljuk, ingyen építési helyet nem hozunk létre.
        if(tmp.shelterStage>0){
            tmp.siteCount=1;
            tmp.sites[0]={3.1,-1.55,tmp.shelterStage,100.0};
        }
    }else{
        if(tmp.siteCount<0||tmp.siteCount>World::maxBuildSites)return false;
        int maxStage=0;
        for(int i=0;i<World::maxBuildSites;++i){
            const auto& site=tmp.sites[i];
            if(!std::isfinite(site.x)||!std::isfinite(site.z)||!std::isfinite(site.condition)
                ||site.condition<0||site.condition>100||site.stage<0||site.stage>3)return false;
            if(i<tmp.siteCount){
                if(site.x<-5.7||site.x>5.7||site.z<-6.0||site.z>1.3)return false;
                maxStage=std::max(maxStage,site.stage);
                for(int j=0;j<i;++j){
                    const double dx=site.x-tmp.sites[j].x,dz=site.z-tmp.sites[j].z;
                    if(dx*dx+dz*dz<3.6*3.6)return false;
                }
            }else if(site.stage!=0||site.x!=0||site.z!=0||site.condition!=100)return false;
        }
        if(maxStage!=tmp.shelterStage)return false;
    }
    if(hasLedger){
        tmp.items=loadedItems;tmp.nextItemId=loadedNext;
        tmp.consumedItems[0]=loadedConsumed[0];tmp.consumedItems[1]=loadedConsumed[1];
        tmp.ledgerReady=true;
        tmp.structure=loadedStructure;
        if(!tmp.ledgerConsistent())return false; // a főkönyv és a készlet nem térhet el
    }else{
        tmp.rebuildLedgerFromCounts(); // régi mentés: azonosítók a készletből, előzmény nélkül
    }
    tmp.hearthBuilt=hearth!=0;tmp.fireLit=fire!=0;tmp.underShelter=covered!=0;
    // Sérült mentésben a fedés nem védhet a távolból.
    if(tmp.underShelter&&!tmp.protectedAtPlayer())tmp.underShelter=false;
    *this=tmp;return true;
}
}
