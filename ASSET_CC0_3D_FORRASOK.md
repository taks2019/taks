# TAKS WORLD · Hiteles CC0 3D és PBR alapanyagkatalógus (0.8)

**Mérföldkő:** első valós, internetes fotogrammetriai 3D modellfájlok ténylegesen az Android natív GPU-ján. Nem a kész Wonderwerk, nem a jóváhagyott végleges grafika.

## Valóban beszerzett, forrásolt anyagok

| Eszköz | Hivatalos forrás | Licenc | Felhasználás |
|---|---|---|---|
| Rock 07 (valódi 3D scan) | https://polyhaven.com/a/rock_07 | CC0 | Felvehető világkő, a készletet a játékos/NPC fogyasztja |
| Namaqualand Boulders 01 (valódi 3D scan) | https://polyhaven.com/a/namaqualand_boulders_01 | CC0 | Természeti sziklák |
| Namaqualand Cliff 01 (valódi 3D scan) | https://polyhaven.com/a/namaqualand_cliff_01 | CC0 | Dél-afrikai jellegű sziklaképződmények |
| Sandy Gravel 02 (fényképalapú PBR albedo) | https://polyhaven.com/a/sandy_gravel_02 | CC0 | Teljes talaj 3D UV textúrája |
| Cliff Side (fényképalapú PBR albedo) | https://polyhaven.com/a/cliff_side | CC0 | Sziklafelszín 3D textúrája |

Hivatalos licenc: https://polyhaven.com/license . A fotóalapú kellékeket csak ideiglenes, földtani terepanyagként lehet használni: a **Wonderwerk-barlang geológiai és régészeti hitelességét ezek önmagukban nem igazolják**. A csontvázas embermodellek nem ezekből lesznek.

## Játékmotor és offline csomagolás

- `scripts/import_cc0_rocks.py` a Poly Haven hivatalos JSON API-jából lekéri a 3D hálót, UV-koordinátákat és az eredeti fotótextúrát. A forrásadatokat ellenőrzi.
- Build során a geometria egy ellenőrizhető `TKM1` felosztású bináris formátummá alakul (háromszögek, valódi vertexnormálok, UV).
- A motor Android `assets/models/` mappába csomagolja a modell- és PNG-fájlokat. Az alkalmazás futtatásához **nem kell internet**.
- `RealModels.cpp` natív OpenGL ES 3-on tárolja a valódi geometriát és a fényképalapú texeleket: vertex-buffer, mipmap, sRGB albedo és 3D transzformáció.
- `FastScene.cpp` a talajon és falon a valódi fotótextúrákat használja, de a barlang formáját jelenleg még nem hitelesítettük.
- A nyersanyagkő nem dekoráció: a játék világállapotához kötött, csak akkor látható, amíg az adott lelőhely készlete pozitív.
- A GitHub Actions a teljes online beszerzést, a 3D grafikai képi tesztet, a natív C++ teszteket és az APK-ba bekerült fájlokat is ellenőrzi.

## Minőségi figyelmeztetés

A fényképalapú kő nem elegendő a végleges, fotórealisztikus TAKS Worldhöz: szükség van hiteles Wonderwerk felmérésre, egyedi domborzati meshre, részletes növényzetre, hihető atmoszférára/fényre és anatómiailag részletes, csontvázas animációjú emberekre. A korábbi elutasított egyszerű karakter- és építményformákat **nem minősítjük véglegesnek**. A saját eredeti logó továbbra sincs megerősítve, nem kerül bele másik.

## Következő ellenőrzési kapuk

1. Hatékony mobilon is részletes 3D barlangbelső, külső geo, PBR normal és roughness bevezetése, még a látványmérföldkő előtt.
2. Hiteles karaktermodell + valódi rig/animáció, nem primitívekből összerakott test.
3. Android GPU/performance mérés Galaxy A56 és Honor Pad X8 készüléken, külön képminőségi és használhatósági elfogadással.
4. A legfrissebb kanonikus 3D Biblia szerint történő műszaki audit.
