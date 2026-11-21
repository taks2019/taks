# TAKS WORLD – 3D világ összeépítése (felülvizsgálati ág)

**Állapot:** egységes natív C++ / OpenGL ES 3 játéktér, kizárólag tesztelésre. Nem végleges, nem régészetileg hitelesített Wonderwerk, nem jóváhagyott látvány és nem telepített frissítés.

## Egyetlen világ / egyetlen GPU-jelenet

| Modul | Játékban használt forrás | Állapot |
|---|---|---|
| Domborzat, talajmagasság | `WorldTerrain.h`, `FastScene.cpp` | natív 3D háló |
| Barlangkapu és belső járat | `CaveVolumeReview.h` | új, mélységben bejárható 3D geometriai teszt |
| Barlangtető takarása | `RoofCutaway.h` + `FastScene.cpp` | zoommal vezérelt, ugyanazon világ felett |
| Kőzetek, talajtextúra | `scripts/import_cc0_rocks.py`, `RealModels.cpp` | forrásalapú PBR-hez közelítő albedó render, nem teljes PBR |
| Fű, fa, páfrány | `scripts/import_cc0_foliage.py`, `RealModels.cpp` | széllengés és ökológiai állapothoz kötött láthatóság, hitelesség felülvizsgálandó |
| Játékos és négy NPC | `scripts/import_cc0_rigged_human.py`, `RealModels.cpp` | 53 csontos tesztrig, nem hitelesített őskori ember |
| Vadállatok | `FastScene.cpp::addWildlife` | **elutasított geometriai helykitöltők**; a végleges megoldáshoz külön ellenőrzött modellek szükségesek |
| Eső, víz, sár, lábnyom, tűz | `FastScene.cpp` + `engine/` | részleges ok-okozati működés, natív mentés |
| Főmenü és eredeti logó | `RestoredStoneMenu.java` és a korábbi elfogadott menükép | **érintetlen**, CI ellenőrzi a bájtpontos képet |
| 8 irányú barlang-PNG | `CaveSprite.cpp` | kizárólag manuálisan bekapcsolható 2,5D előnézet, az alapjátékban KI |

## Mi lett ténylegesen új?

1. A korábbi rácson négyzetesen kilyukasztott függőleges sziklafal helyett a `CaveVolumeReview.h` valódi **3D sziklakaput, természetes ívű mennyezetet, két oldalfalat, mélyülő járatot és hátsó zárófalat** épít háromszögekből.
2. A barlangbelső ugyanazt az XYZ koordinátateret és `WorldTerrain::height` talajfüggvényt használja, mint a natív terep és a szereplők grafikai lábpozíciója.
3. A belső kőzettriplanár fény és sötétedés a járat fizikai mélységével változik, nem egy 2D fekete kép.
4. Külön **C++ geometriateszt** vizsgálja, hogy nyitott marad-e a bejárat, helyesek-e a háromszögkoordináták, és létezik-e teljes mélységű járat.
5. A CI külön **Android teszt-APK-t és natív GLES3 jelenetképeket** készít ellenőrzésre.

## Zárolt jóváhagyási és technikai korlátok

- Ez a barlang **procedurális 3D próbamodell**, nem helyszíni felmérésből visszaállított Wonderwerk. A régészeti hitelességhez georeferált, megfelelő licenccel használható felmérési háló kell.
- A jelenlegi állatmodelleket, a primitív emberelőnézetet, illetve az ökológiailag még nem igazolt növényeket **nem tekintjük jóváhagyottnak**. A felhasználónak minden cseremodellt külön meg kell mutatni.
- A jelenlegi világfal-ütközés még durva geometriai határokat használ (`engine/CauseWorld.cpp`). A valós barlangmesh-alapú teljes ütközés későbbi feladat.
- A `TAKS WORLD` menü eredeti képe, logója, menükezelése és meglévő mentések nem változtak.
- A teszt-APK **nem in-place UPDATE**: az Android csak eredeti aláírókulccsal, azonos csomagnévvel és megfelelő verziókóddal fogadja el a helyben frissítést. A régi APK törlése tilos.
- Az egyenkénti grafikai jóváhagyás (`assets/visual_approvals.json`) érvényben marad. Ez az ág nincs végleges játékba olvasztva.
