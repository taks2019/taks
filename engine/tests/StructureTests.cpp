#include "../Structure.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace taks;
int main(){
    Structure s;std::string why;
    // Alap: két cölöp áll a talajon, a gerenda rajtuk nyugszik (kötéssel).
    const unsigned p1=s.addPart(PART_POLE,0,0,0,0);
    const unsigned p2=s.addPart(PART_POLE,2.0,0,0,0);
    assert(p1&&p2&&p1!=p2);
    // Ugyanarra a helyre nem rakható második alkatrész, és semmi sem változik.
    const size_t cnt=s.parts.size();
    assert(s.addPart(PART_POLE,0,0,0,0,&why)==0 && s.parts.size()==cnt && !why.empty());
    const unsigned beam=s.addPart(PART_BEAM,1.0,2.2,0,0);
    assert(beam);
    // Távoli alkatrészhez nem lehet csatlakozni.
    const unsigned far=s.addPart(PART_STONE,50,0,0,0);
    assert(!s.addJoint(far,p1,1,&why));
    // Lebegő gerenda: nincs csatlakozás -> instabil.
    assert(!s.analyze().stable);
    assert(s.addJoint(beam,p1,0));
    assert(s.addJoint(beam,p2,0));
    assert(!s.addJoint(beam,p1,0,&why)); // már össze vannak kötve
    // A messze lévő kő a talajon van, nem terheli a szerkezetet.
    StructureReport r=s.analyze();
    assert(r.stable && r.failedParts.empty());
    // Túlterhelés: nehéz kövek a gerendán: a kötés (900 N) előbb adja fel, mint a cölöp.
    bool collapsed=false;
    for(int i=0;i<20&&!collapsed;++i){
        const unsigned st=s.addPart(PART_STONE,1.0,2.4,0.1*(i+1),0);
        assert(st && s.addJoint(st,beam,1));
        const StructureReport rr=s.analyze();
        if(!rr.stable){collapsed=true;assert(!rr.failedJoints.empty()&&i>=3);} // négy kő még elbírható
    }
    assert(collapsed);
    // Kopás: lejjebb csökkenti a teherbírást, az állapot nem megy 0 alá.
    Structure w;
    const unsigned a=w.addPart(PART_POLE,0,0,0,0);
    const unsigned top=w.addPart(PART_STONE,0,2.2,0,0);
    assert(w.addJoint(top,a,0));
    assert(w.analyze().stable);          // 18 kg * 9.81 = ~176 N < 900 N
    w.wear(90.0);                        // a kötés 10%-os állapotú: 90 N kapacitás
    assert(!w.analyze().stable);
    assert(!w.analyze().failedJoints.empty());
    w.wear(500.0);assert(w.parts[0].condition==0.0);
    // Undo/redo: azonosító nem használódik újra.
    Structure u;
    const unsigned x1=u.addPart(PART_POLE,0,0,0,0);
    const unsigned x2=u.addPart(PART_POLE,3,0,0,0);
    assert(u.undo() && u.parts.size()==1 && u.find(x1) && !u.find(x2));
    assert(u.redo() && u.parts.size()==2 && u.find(x2));
    assert(u.undo());
    const unsigned x3=u.addPart(PART_POLE,3,0,0,0);
    assert(x3>x2 && !u.canRedo());       // új akció törli a redo ágat
    // Mentés/betöltés bitpontos, hibás adat nem változtat.
    Structure m=s;const std::string saved=m.save();
    Structure back;assert(back.load(saved) && back.save()==saved);
    assert(back.analyze().stable==m.analyze().stable);
    Structure keep;keep.addPart(PART_POLE,0,0,0,0);const std::string before=keep.save();
    assert(!keep.load("STRUCT1 5 1 1 0 0 0 0 0 1 1 1 1 1") && keep.save()==before); // hiányos
    assert(!keep.load("STRUCT1 5 1 9 0 0 0 0 0 1 1 1 100 0") && keep.save()==before); // id >= next
    assert(!keep.load(saved+" trailing") && keep.save()==before);
    // Eltávolítás: a kötések is eltűnnek, a gerenda lebegni kezd.
    Structure rem;
    const unsigned c1=rem.addPart(PART_POLE,0,0,0,0);
    const unsigned c2=rem.addPart(PART_BEAM,0,2.2,0,0);
    assert(rem.addJoint(c2,c1,0)&&rem.analyze().stable);
    assert(rem.removePart(c1)&&rem.joints.empty()&&!rem.analyze().stable);
    assert(!rem.removePart(c1));
    // F2/2: vágás – tömegmegmaradás, kötések átvétele, hibás kérés nem változtat.
    {
        Structure c;std::string why;
        const unsigned base=c.addPart(PART_POLE,0,0,0,0);
        const unsigned beam=c.addPart(PART_BEAM,0,2.2,0,0);
        assert(c.addJoint(beam,base,0));
        const double m0=c.find(beam)->mass,l0=c.find(beam)->length;
        const std::string before=c.save();
        assert(c.cutPart(beam,0.1,&why)==0 && c.save()==before && !why.empty());   // túl közeli vágás
        assert(c.cutPart(beam,0.95,&why)==0 && c.save()==before);
        const unsigned st=c.addPart(PART_STONE,5,0,0,0);
        const std::string before2=c.save();
        assert(c.cutPart(st,0.5,&why)==0 && c.save()==before2);                    // kő nem vágható
        assert(c.cutPart(9999,0.5,&why)==0 && c.save()==before2);
        const unsigned half=c.cutPart(beam,0.5);
        assert(half!=0 && half>st && c.find(beam) && c.find(half));
        assert(std::fabs(c.find(beam)->mass+c.find(half)->mass-m0)<1e-9);          // tömeg megmarad
        assert(std::fabs(c.find(beam)->length+c.find(half)->length-l0)<1e-9);
        // A kötés megmaradt, de csak az egyik darabhoz: a másik lebeg -> instabil, a hibás darab jelentett.
        assert(c.joints.size()==1);
        assert(!c.analyze().stable);
        // Visszavonás visszaállítja az eredetit (azonosító nem használódik újra).
        assert(c.undo() && !c.find(half) && std::fabs(c.find(beam)->length-l0)<1e-9);
        // Álló cölöp vágása: a felső darab a vágás magasságán kezdődik.
        Structure p;const unsigned pp=p.addPart(PART_POLE,0,0,0,0);const double pl=p.find(pp)->length;
        const unsigned top=p.cutPart(pp,0.5);
        assert(top && std::fabs(p.find(top)->y-pl*0.5)<1e-9 && !p.analyze().stable); // a felső rész lebeg
    }
    // F2/2: forgatás és mód; mentés STRUCT2, régi STRUCT1 betöltődik túlélő módban.
    {
        Structure r;const unsigned b=r.addPart(PART_BEAM,0,0,0,0);std::string why;
        assert(r.rotatePart(b,1.5)&&std::fabs(r.find(b)->yaw-1.5)<1e-12);
        assert(!r.rotatePart(b,std::nan(""),&why)&&!r.rotatePart(777,1.0,&why));
        assert(r.undo()&&r.find(b)->yaw==0.0);
        r.mode=1;const std::string s=r.save();
        assert(s.find("STRUCT2 1 ")==0);
        Structure back;assert(back.load(s)&&back.mode==1&&back.save()==s);
        Structure legacy;legacy.mode=1;
        assert(legacy.load("STRUCT1 2 1 1 0 0 0 0 0 2.2 6 6000 100 0")&&legacy.mode==0&&legacy.parts.size()==1);
        Structure badMode;assert(!badMode.load("STRUCT2 5 2 0 0"));
    }
    std::cout<<"PASS: F2 structure parts, load, wear, undo/redo, save\n";
    std::cout<<"PASS: F2/2 cut, rotate, mode, STRUCT2 migration\n";
}
