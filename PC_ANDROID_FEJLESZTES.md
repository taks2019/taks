# TAKS WORLD — PC fejlesztés Androidra

**Cél:** a meglévő natív TAKS Engine (C++ / OpenGL ES 3) szerkesztése Windows 10/11 alatt, egy menüből fordított és USB-n telepített Android TESZT APK-val. Nem Unity, nem Godot, nem HTML Canvas. A korábban elfogadott képeket és a telefonon lévő mentéseket nem írjuk felül.

## Egyszeri telepítés Windowsra

1. Telepítsd az ingyenes [Android Studio](https://developer.android.com/studio) programot. Az **SDK Manager** részen jelöld be az **Android 15 (API 35)** platformot, **Android SDK Build-Tools 35**, **Android SDK Platform-Tools**, valamint a **Show Package Details** után az **NDK 27.2.12479018** és **CMake 3.22.1** verziókat. Fogadd el az SDK licencfeltételeket.
2. Telepítsd a [Python 3.11](https://www.python.org/downloads/) vagy újabb változatát; a telepítőben a **pip** és **venv** is legyen kiválasztva. A program helyi `.venv` mappát használ, nem módosítja a többi Python-projektedet.
3. Nyisd meg (vagy klónozd) a GitHubról ezt a fejlesztési ágat. Csomagold ki úgy, hogy a fájlok és az `app`, `engine`, `scripts`, `tools` mappák együtt maradjanak. Ha Git is kell PC-n: telepítsd a [Git for Windows](https://git-scm.com/downloads/win) programot.
4. Tedd a már elfogadott **`SuperImage.zip`** fájlt az új `approved_source` mappába, pontos útvonal: `approved_source/SuperImage.zip`. Vagy fordításkor add meg a fájl tényleges útvonalát a `-SourceZip` paraméterrel.
5. Kattints duplán a gyökérben lévő **`PC_TAKS_WORLD.cmd`** fájlra. Először válaszd az **1 — Ellenőrzés**, utána a **2 — Android TESZT APK építése** menüpontot. Az építő automatikusan elkészíti a hat jóváhagyott talajtextúra másolatát, és az APK tartalmát is visszaellenőrzi.

Az első fordítás internetet igényel. A segédprogram ellenőrzött forrásból tölti le az eredeti elfogadott menüképet, az ingyenes CC0 modell-importhoz szükséges fájlokat, a Python-csomagokat és a **Gradle 8.9** verzióját. A Gradle ZIP SHA256-ellenőrzést kap. A későbbi fordítások a már előkészített állományokat újrahasznosítják, de az Android Gradle-függőségek az első építéskor külön is letöltődhetnek.

**A kész teszt-APK itt jelenik meg:** `output/TAKS_WORLD_PC_TESZT.apk`. Ez nem jóváhagyott grafikai kiadás. Ha egy újabb APK aláírása eltér a telefonon telepített verzióétól, az installálás hibával leállhat. **A korábbi játékot és a mentéseket ne töröld!** Az azonos aláírás vagy egy külön csomagnév szükséges.

## Telefonos kipróbálás

A Samsung Galaxy A56-on az **Fejlesztői beállítások → USB-hibakeresés** bekapcsolása után csatlakoztasd USB-adatkábellel a telefont a PC-hez, majd engedélyezd a számítógépet a telefonon. Válaszd a segédprogram **3 — APK telepítése** menüpontját. Ez `adb install -r` parancsot futtat, nem távolít el és nem töröl alkalmazásadatot. Több csatlakoztatott eszköznél kézzel kell kiválasztani az azonosítót.

A **PC-n jóváhagyott `SuperImage.zip`-ből fordított APK már automatikusan tartalmazhatja a hat tömörített, 512×512-es talajmásolatot** (összesen körülbelül 3,1 MB); nem kell a teljes, nagy ZIP-et csomagolni vagy minden telepítés után újra importálni. A `scripts/bake_approved_ground.py` SHA-256 alapján ellenőrzi az eredeti atlaszt, a `scripts/check_apk_ground_pack.py` pedig *a kész APK belsejében* igazolja mind a hat PNG-t és a változatlan elfogadott főmenüt. Ha hiányzik a ZIP és még nem készült ellenőrzött készlet, az építő **hibát jelez**: nem ad ki újabb barna teszt-APK-t.

A régi kézi módszer is megmaradt: a **4 — Másolás telefonra** funkcióval átküldheted a teljes ZIP-et a telefon Letöltések mappájába, majd a játék **Beállítások → SuperImage.zip talajok betöltése** opciójával importálhatod. A kézzel választott helyi készlet elsőbbséget élvez az APK-ba csomagolttal szemben.

A Windowsos indító új **8-as menüpontja** a talajok előkészítése; a **9-es** egy valódi Android-képernyőképet ment a PC `output/` mappájába, hogy könnyebb legyen összehasonlítani a jóváhagyott menüt a játékbeli látvánnyal. A készüléken is megőrzi a képet.

## PC-s fejlesztési ciklus

| Hol? | Mit szerkesztünk? | Mire való? |
|---|---|---|
| `engine/` | A C++ világ és ok-okozati szabályok | NPC-k, emlékezet, erőforrás, építés, időjárás |
| `app/src/main/cpp/FastScene.cpp` | Valódi GLES3 3D megjelenítés | Terep, fény, anyag, forgatható kamera |
| `app/src/main/java/hu/taks/world/` | Androidos felület és érintés | Főmenü, beállítások, gombok, mozgatás |
| `app/src/main/assets/` | Játékanyagok | Eredeti menü, textúrák, modellek, hangok |
| `scripts/` | Automatizált 3D import | GLTF/GLB → natív modellfájl és PNG-textúra |
| `tests/` | Automatikus ellenőrzések | Öröklött mentések, felület, anyagkezelés |

A **5 — Android Studio** menüpont az egész projektet megnyitja. Az Android Studio kódszerkesztője és a Logcat jól használható hibakereséshez. A projekt C++ motorját a Gradle/CMake együtt fordítja; nem kell külön kézzel újraépítened minden `.so` fájlt.

**A javasolt sorrend:** a menü és az Androidos alapjelenet ellenőrzése már sikeres emulátoron → az elfogadott talajmásolatokkal fordított APK külön ellenőrzése → jóváhagyott talajok valós 3D alkalmazása → hiteles barlangmodell és ütközés → NPC-k részletes modellje és animációi → éghajlat, szél és a további Biblia-szabályok. A teljes, hiteles Wonderwerk tájat nem lehet egyetlen PNG-ből tetszőleges irányból körbejárhatóvá tenni; a PNG-k a valódi 3D felületek anyagai lehetnek.

## Hibakezelés

- **SDK / NDK / CMake hiányzik:** Android Studio → SDK Manager → SDK Tools → Show Package Details.
- **Python nem található:** telepíts Python 3.11+; konzolban `py -3.11 --version`.
- **Nem látja a telefont:** a PC-n `adb devices` helyett a segédprogram 3-as menüje is ellenőrzi; telefonon fogadd el az USB-hibakeresést.
- **Telepítés aláírás miatt sikertelen:** ne távolítsd el a régit; külön debug-azonosítóval vagy a régi aláírókulccsal kell építeni.
- **Barna paca / kettős barlang / szaggatás:** a forráskód sikeres fordítása még nem jelenti a grafika jóváhagyását. Valós A56 képernyőkép, FPS, GPU-/memóriamérés szükséges. A **7-es menü** lementi a natív Android hibanaplót az `output/` mappába.

**Konzolos, menü nélküli használat:**
```powershell
.\PC_TAKS_WORLD.cmd -Mode check
.\PC_TAKS_WORLD.cmd -Mode ground -SourceZip "D:\Kepek\SuperImage.zip"
.\PC_TAKS_WORLD.cmd -Mode build
.\PC_TAKS_WORLD.cmd -Mode screenshot
.\PC_TAKS_WORLD.cmd -Mode install
.\PC_TAKS_WORLD.cmd -Mode tests
.\PC_TAKS_WORLD.cmd -Mode copy-images -SourceZip "D:\Kepek\SuperImage.zip"
```

**Biztonság:** a script sem a telefonos alkalmazást, sem a mentéseket, sem a jóváhagyott eredeti képet nem törli vagy generálja újra. A `local.properties` létező fájlt megőrzi; a saját segédprogramjait csak a `.tools/` és `.venv/` mappába készíti. A vizuális jóváhagyást kizárólag a felhasználó adhatja meg.


## Valódi Android-tesztek és látványjóváhagyás — 2026. november 21.

A GitHub emulátoros körének a tényleges futtatott alkalmazásról készített képein a főmenü, a Beállítások, a játék és a barlangellenőrző nézet megnyílt. Ez **működési próba**, nem grafikai elfogadás. Az akkori APK **nem tartalmazta a hat SuperImage-talajmásolatot**, ezért a képeken a korábban kifogásolt homogén barna játéktér látható. Az új PC-s építő ennek a konkrét hiánynak a kiszűrésére készült.

A későbbi lépés a teljesen hiteles Wonderwerk-környezet, részletesen modellezett barlangfal, domborzat és a korábban jóváhagyott, nyolc irányból ábrázolt növényzet élethű 3D megjelenítése. Ez **még nem készült el**; a hat jóváhagyott textúra nem helyettesíti a valódi részletgazdag geometriát, és a PNG-k nem automatikusan varratmentes, fizikailag helyes PBR-anyagok.
