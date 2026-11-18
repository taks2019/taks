#pragma once
#include <string>
#include <vector>
namespace taks {
// F2 – alkatrészszintű szerkezet. Minden alkatrésznek egyedi azonosítója, anyaga, tömege és állapota van.
// Nyersanyagot ez a modul nem teremt: a hívó (World) vonja le az anyagot, mielőtt addPart-ot hív.
enum PartKind { PART_POLE=0, PART_BEAM=1, PART_STONE=2, PART_PLANK=3 };
struct Part {
    unsigned id=0;
    int kind=PART_POLE;
    double x=0,y=0,z=0;     // középpont, méter; y=0 a talaj
    double yaw=0;           // vízszintes forgás, radián
    double length=1.0;      // méter
    double mass=1.0;        // kg
    double strength=6000.0; // N: ennyi teherig bírja a nyomást
    double condition=100.0; // 0..100, kopással csökken
};
// type 0: kötés (kötéllel rögzített, húzást is bír, kb. 900 N), 1: támasz (csak nyomás: az alsó tartja a felsőt).
struct Joint { unsigned a=0,b=0; int type=1; };
struct StructureReport {
    bool stable=true;
    std::vector<unsigned> failedParts;   // túlterhelt vagy lebegő alkatrészek
    std::vector<unsigned> failedJoints;  // index a joints vektorban
    double maxLoadRatio=0.0;             // legnagyobb terhelés / teherbírás
};
class Structure {
public:
    static constexpr int maxParts=400;
    static constexpr int maxUndo=50;
    std::vector<Part> parts;
    std::vector<Joint> joints;
    unsigned nextId=1;
    // Alapértelmezett fizikai adatok fajtánként (tervezői becslés, nem mért érték).
    static Part makeDefault(int kind);
    // Siker: új alkatrész azonosítója; hiba: 0 (és semmi sem változik).
    unsigned addPart(int kind,double x,double y,double z,double yaw,std::string* why=nullptr);
    // Csatlakozás csak elég közeli alkatrészek között; a távolság az alkatrészek méretéből jön.
    bool addJoint(unsigned a,unsigned b,int type,std::string* why=nullptr);
    bool removePart(unsigned id);           // a hozzá tartozó kötések is megszűnnek
    const Part* find(unsigned id) const;
    StructureReport analyze() const;        // statikus teherszámítás fentről lefelé
    void wear(double amount);               // időjárás/kopás: minden alkatrész állapota csökken
    // Visszavonás/újra: teljes állapot-pillanatképekkel, így az azonosítók stabilak.
    bool undo();
    bool redo();
    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }
    // F2/2: forgatás és vágás. Vágásnál az anyag megmarad: a két darab tömege = az eredeti tömege.
    bool rotatePart(unsigned id,double yaw,std::string* why=nullptr);
    // fraction: 0.2..0.8 a hossz mentén. Siker: az új (második) darab azonosítója; hiba: 0, semmi sem változik.
    unsigned cutPart(unsigned id,double fraction,std::string* why=nullptr);
    // Mód: 0 túlélő (a hívó anyagot von le), 1 sandbox (kísérlet; a hívó nem érinti a készletet).
    int mode=0;
    std::string save() const;               // "STRUCT2 ..." (a STRUCT1 továbbra is betölthető)
    bool load(const std::string& data);     // hiba esetén nem változtat semmit
private:
    struct Snapshot { std::vector<Part> parts; std::vector<Joint> joints; unsigned nextId; };
    std::vector<Snapshot> undoStack_, redoStack_;
    void checkpoint();
};
}
