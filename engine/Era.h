#pragma once
// Korszakkapu (Biblia 6. fejezet: "Technológiai ugráshoz tudás, nyersanyag, energia, infrastruktúra és munka kell").
// Nincs korszakváltó gomb: a korszakot kizárólag a világból származó bizonyíték határozza meg.
// Az eredeti TAKS World logó csak az ŰR korszakban jelenik meg (tulajdonosi döntés, 2026-11-18).
// A küszöbök TERVEZŐI BECSLÉSEK (helykitöltők), nem mért vagy forrásolt adatok; az F6 gyártási lánc kalibrálja őket.
namespace taks {
enum Era { ERA_PREHISTORIC=0, ERA_COMMUNITY=1, ERA_TOOLS=2, ERA_AGRICULTURE=3, ERA_HISTORY=4, ERA_INDUSTRY=5, ERA_MODERN=6, ERA_SPACE=7 };
struct EraEvidence {
    double knowledge=0,materials=0,energy=0,infrastructure=0,work=0; // a világból mért, összegzett értékek (0..)
    int orbitalFlights=0;                                            // ténylegesen végrehajtott pályára állás
};
inline double eraGateThreshold(int era){ // belépési küszöb korszakonként (1..7); minden mezőre egyformán
    static const double gate[8]={0,10,30,80,200,500,1200,3000};
    return (era<1||era>7)?1e300:gate[era];
}
inline int eraFor(const EraEvidence& e){
    int era=ERA_PREHISTORIC;
    for(int next=1;next<=7;++next){
        const double g=eraGateThreshold(next);
        const bool ok=e.knowledge>=g&&e.materials>=g&&e.energy>=g&&e.infrastructure>=g&&e.work>=g
                      &&(next<ERA_SPACE||e.orbitalFlights>=1);
        if(!ok)break;
        era=next;
    }
    return era;
}
inline bool logoUnlockedFor(int era){ return era>=ERA_SPACE; }
}
