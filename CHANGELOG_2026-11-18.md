# TAKS World Android — 2026-11-18

## Módosítások

- A régi TKM1 statikus emberimportot TKS1 bőrsúlyozott formátum váltja fel. A csomag 121 422 hálócsúcsot, 53 csontot és hét forrásból származó animációt tartalmaz.
- A GLES3 vertex shader a négy csontbefolyást súlyozva keveri. Az idle, walk és run állapotok közti átmenet 280 ms; a négy NPC szimulált pozícióját a renderelés a szimulációs lépések között simítja.
- Javítva az animációs asset bináris elrendezése: a teljes csontforgatás-blokkot külön követi a gyökérmozgás-blokk; az importáló most az eredeti gyökérmintákkal is ellenőrzi az eltolt adatokat.
- A hiányzó modell esetére rajzolt fehér rúd- és bábualakok eltávolítva. Hibás humanoid asset esetén a renderer naplóz, nem rajzol hamis figurát.
- A pályaszélekre valós CC0 fotogrammetriai sziklák kerültek; a már importált nagyobb kőzetmodellt is használja a terep.
- A GPU-s jelenetben több méretben és eltérő forgatással jelennek meg a valódi sziklamodellek; a fűfoltok sűrűsége és kiterjedése a mentett biomasszához igazodik.
- A MakeHuman diffúz bőrtextúráján melegebb színkorrekció jelenik meg, így az alakok nem hófehér bábuként tűnnek fel.
- A felhasználó eredeti logója a teljes átadási ZIP-ben változatlan PNG-ként megmarad. Az APK-ba a képarányt megtartó, 512×341-es WebP másolat kerül, és a HUD is ezt mutatja.
- A játék 25 FPS-es renderfrissítést kap, hogy a csontozott idle és járás ne csak ritka szimulációs frissítéseknél lépjen tovább.
- Az Android UI ellenőrzése kiegészült a 53 csontos modell betöltésének logjával, az érintőpadról indított mozgás koordináta-változásával és a SharedPreferences mentés meglétével. Az XML hierarchia olvasása `adb pull` helyett shellen keresztül történik.
- A GPU regressziókép-csomag 13 képkockát ellenőriz: kameranézetek, környezeti változások, idle, séta és futás.

## Ellenőrzés

- Helyben sikeres: világ- és ökológiai C++ tesztek; Python importer szintaxis; `git diff --check`; GLES3 C++ források szintaktikai ellenőrzése minimális fejléc-stubokkal.
- A fejlesztői környezetben nincs Android SDK/NDK és GLES fejléckészlet, ezért helyi APK- vagy valós GPU-build nem készült. A natív Android APK, GLES3 képek és Android UI tesztek eredménye a GitHub Actions futásából kerül be a végső csomagba.

## Még nincs kész / nincs hitelesítve

- A MakeHuman mesh általános humanoid, nem régészetileg ellenőrzött Homo erectus.
- A hozzáadott ruhamodell-hálók kortárs szabásúak; az állatbőr-szerű textúra nem bizonyít történeti ruházatot.
- A Wonderwerk-barlang, a paleokörnyezet, a növényfajok, az éghajlat és a teljes, fizikai ok-okozati ökoszisztéma további hitelesítésre és fejlesztésre vár.
- Az Android APK debug tesztbuild; kiadási aláírás és végleges játékminőség nem része ennek a változatnak.

## F1 – egyedi azonosítójú kő és faág (tesztbuild, nem elfogadott játék)
- Állapot: **kódolt és natívan tesztelt** (cmake/ctest, 2/2 sikeres). Buildelve APK-ként, telefonon tesztelve és tulajdonosilag elfogadva: **nem**.
- `engine/CauseWorld.*`: minden induló kő és faág egyedi `Item` azonosítót kap (lelőhely / játékos / tábor / társ keze). Felvétel, táborba szállítás, átvétel és elhasználás (kőél, cölöp, építési jelölőkő, oszlop, tűzhely, tűzgyújtás) a főkönyvben követhető; elhasznált tárgy kikerül, számláló őrzi; az utolsó 64 ok-okozati bejegyzés a `causeLog`-ban (nem mentett).
- Mentés: `TAKS_NATIVE_7` = a v6 tartalom + `LEDG1` blokk. v1–v6 betölthető, az azonosítók a készletből újraépülnek (előzmény nélkül). Hamisított vagy a készlettel nem egyező főkönyv betöltése elutasított.
- Tesztek: új F1 teszt; a meglévő tesztekben a várt mentésfejléc v7-re módosult; a közvetlen készletállítás helyett `grantCarriedForTest()` létrehozza az azonosítót is.
- Nincs lefedve még: levél, gyújtós, cölöp mint egyedi tárgy (jelenleg számláló).
- Logó: az eredeti 1536x1024 PNG változtatás nélkül, külön fájlként (`TAKS_WORLD_LOGO_ORIGINAL_1536x1024.png`). A repóbeli WEBP-ek ugyanennek kisebb méretű változatai (140x93, 512x341); nem lettek lecserélve.

## F2 első szelet – alkatrészes szerkezet (`engine/Structure.*`)
- Állapot: **kódolt és natívan tesztelt** (ctest 3/3). Még **nincs bekötve** a `World`-be és az Android UI-ba, ezért játékban nem látható; telefonon nem tesztelt, tulajdonosilag nem elfogadott.
- Alkatrész: cölöp, gerenda, kő, deszka; egyedi azonosító, tömeg, teherbírás, állapot (kopás). Csatlakozás: kötés (900 N) vagy támasz (nyomás). A csatlakozás csak elég közeli alkatrészek között megy.
- Statika: fentről lefelé számolt teher; lebegő alkatrész, túlterhelt alkatrész vagy kötés = instabil. A kopás csökkenti a teherbírást.
- Undo/redo (50 lépés, azonosító nem használódik újra), mentés `STRUCT1`, hibás adat betöltése nem változtat semmit.
- Értékek (tömeg, teherbírás) tervezői becslések, nem mért anyagadatok.
- Következő: bekötés a `World`-be (anyagfelhasználás az egyedi főkönyvből, mentés v8), Android-érintéses építés, forgatás/kötél/csavar.

## F2 bekötés (tesztbuild)
- `World::structure`: `part_pole`, `part_beam`, `part_stone`, `part_plank` akciók. Anyag csak sikeres lerakásnál fogy (cölöp / kő az egyedi főkönyvből); egymásra rakott alkatrész rögzül, rögzítés nélkül nem marad meg.
- Mentés: `TAKS_NATIVE_8` = v7 + `STRUCT1`. v7 betölthető üres szerkezettel. Az eső koptatja az alkatrészeket.
- Android: négy gomb a „2/B" részben (`MainActivity.java`). **Nincs buildelve APK-ként és nincs telefonon tesztelve**; a Java-módosítást CI fordítja le először.
- Hiányzik: 3D megjelenítés az alkatrészeknek (a renderer még nem rajzolja őket), forgatás, kötél/csavar, vágás/fúrás, sandbox mód.

## F2 második szelet – vágás, forgatás, mód (tesztbuild, nem elfogadott játék)
- Állapot: **kódolt és natívan tesztelt** (g++ -std=c++17, három tesztcsomag zöld: WorldTests, StructureTests, LivingWorldTests). CMake/ctest, APK-build, telefonos teszt és tulajdonosi elfogadás: **nem történt**.
- `Structure::cutPart`: cölöp/gerenda/deszka kettévágása a hossz 20–80%-án; a két darab tömege = az eredeti tömege; a kötések a közelebbi darabra kerülnek; kő nem vágható; hibás kérésnél semmi sem változik; undo visszaállítja.
- `Structure::rotatePart`: vízszintes forgatás, undo-val. (Terhelésszámítást a yaw nem befolyásolja.)
- `Structure::mode` (0 túlélő, 1 sandbox): **csak tárolt jelző**; a World még nem használja, ingyen anyagot nem ad.
- `World` akciók: `part_cut` (kőél kell, kopik; 4 állóképesség, 0,3 óra), `part_turn` (45°). Hiányzó előfeltételnél a mentés bitazonos marad.
- Mentés: `TAKS_NATIVE_9` = v8 + `STRUCT2` (mód). v8 (`STRUCT1`) betölthető túlélő móddal; v7 üres szerkezettel. Regressziós teszt: v8→v9 újramentés bitazonos.
- Értékek (tömeg, teherbírás) továbbra is tervezői becslések. A vágott darab szilárdsága nincs külön modellezve.
- Hiányzik: Java/UI gombok a két új akcióhoz, 3D megjelenítés, kötél/csavar anyagmodell, fúrás, sandbox mód tényleges bekötése.

## Korszakkapu – az eredeti logó csak az űrkorszakban jelenik meg (tesztbuild)
- Tulajdonosi döntés (2026-11-18): a logó azt a világot jelképezi, amelyben az ok-okozati lánc eljut az űrig; ezért nem látszik a korábbi korszakokban.
- `engine/Era.h`: 8 korszak (őskor → közösségek → eszközök → mezőgazdaság → történelem → ipar → modern kor → űr). A korszakot csak bizonyíték adja: tudás, nyersanyag, energia, infrastruktúra és munka mind elérje a küszöböt; az űrkorszakhoz ezen felül legalább egy tényleges pályára állás kell. Korszak nem hagyható ki, és nincs ugrógomb.
- `World::currentEra()` / `logoUnlocked()`. A mai világban nincs modellezett közösség, gyártási lánc vagy űripar, ezért a bizonyíték nulla: őskor, a logó rejtett. Az F6 lánc fogja a bizonyítékot ténylegesen szolgáltatni.
- Android: `nativeLogoUnlocked()` (JNI) és `MainActivity.originalLogoOrTitle()`: amíg nincs űrkorszak, földszínű „TAKS WORLD” felirat látszik. A logófájlok és pixeladatok **változatlanok** (az eredeti 1536×1024 PNG SHA-256 egyezik a feltöltöttel).
- Teszt: `engine/tests/EraTests.cpp` (kapu, sorrend, hiányzó tényező, űr pályára állás nélkül). Minden C++ tesztcsomag zöld.
- A küszöbértékek tervezői becslések, nem forrásolt adatok. A Java/JNI módosítás **nincs lefordítva** (nincs Android SDK/NDK itt), a CI fordítja először. Telefonon nem tesztelt, nincs tulajdonosi elfogadás.
- Nyitott: az űrkorszakban megjelenő logó végleges életciklusa (felfedezési animáció, PNG vagy WEBP az elfogadott fájl) a tulajdonossal egyeztetendő.

## F2 – UI-gombok a vágáshoz és forgatáshoz (tesztbuild)
- `MainActivity.java`, „2/B · SZABAD ALKATRÉSZEK”: két új gomb, `part_cut` (kettévágás, kőél kell) és `part_turn` (45° forgatás). Ugyanazt az `act()` utat használják, mint a többi építőgomb, tehát nincs külön UI-állapot.
- **Nincs lefordítva** (nincs Android SDK/javac itt), a CI fordítja először; telefonon nem tesztelt, nincs tulajdonosi elfogadás. Az alkatrészek 3D megjelenítése továbbra is hiányzik.

## Main-menu visual pass – 2026-11-18

- Bundled a 1500×844 Wonderwerk cave / savanna image plate in the native Android app and placed the existing live menu controls over it.
- Kept the user-provided TAKS WORLD logo as a separate, untouched asset; the existing four actions remain native, accessible and interactive.
- The scene overlay now crossfades when the native world reports night, rain, wet ground or a burning fire; it does not invent weather or discoveries.
- This is a main-menu image layer, not the freely rotatable in-game world and not the completed eight-view object-atlas renderer. The Android emulator screenshot and APK verification run in CI.
