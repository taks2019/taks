#include "../CauseWorld.h"
#include <cassert>
#include <iostream>
using namespace taks;
int main(){
    // A mai világ: őskor, a logó rejtett; mentés/betöltés után is.
    World w;assert(w.currentEra()==ERA_PREHISTORIC&&!w.logoUnlocked());
    World back;assert(back.load(w.save())&&!back.logoUnlocked());
    // Nincs ingyenes ugrás: egyetlen hiányzó tényező megállítja a korszakot.
    EraEvidence e;assert(eraFor(e)==ERA_PREHISTORIC);
    e.knowledge=e.materials=e.energy=e.infrastructure=1e6;e.work=0;
    assert(eraFor(e)==ERA_PREHISTORIC);
    e.work=1e6;e.orbitalFlights=0;
    // Mind az öt tényező megvan, de pályára állás nélkül nincs űrkorszak: modern kor marad.
    assert(eraFor(e)==ERA_MODERN&&!logoUnlockedFor(eraFor(e)));
    e.orbitalFlights=1;assert(eraFor(e)==ERA_SPACE&&logoUnlockedFor(eraFor(e)));
    // Sorrend: a korszakok nem hagyhatók ki; kevés bizonyíték csak az alacsonyabb korszakot adja.
    EraEvidence m;m.knowledge=m.materials=m.energy=m.infrastructure=m.work=eraGateThreshold(ERA_AGRICULTURE);
    assert(eraFor(m)==ERA_AGRICULTURE&&!logoUnlockedFor(eraFor(m)));
    m.energy=eraGateThreshold(ERA_AGRICULTURE)-1; // egy tényező hiányzik
    assert(eraFor(m)==ERA_TOOLS);
    std::cout<<"PASS: era gate, logo only in space era\n";
}
