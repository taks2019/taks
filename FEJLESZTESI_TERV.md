# TAKS WORLD – natív játékmotor és teljes fejlesztési irány
Verzió: 2026-11-16 · Android ág: natív C++17 + OpenGL ES 3 · **munkaterv, nem kész játék**

## Kötelező termékdefiníció
A TAKS World nem kész 3D házak és járművek katalógusa. Az épületek, gépek, járművek és a civilizáció minden látható eredménye az anyagokból, alkatrészekből, munkából, tudásból, helyből, időből és energiából következik. A világ folyamatosan és visszakereshetően fejlődik az őskortól az űrkorszakig. **Nincs korszakváltó varázsgomb.**

A forrás a felhasználó 3D Obsidian Bibliájának legújabb hozzáférhető változata és a későbbi kifejezett döntések; az október 23-i konszolidáció és a 190 helyreállított jegyzetszekció önmagában nem a teljes eredeti vault. A fotórealisztikus Wonderwerk-rekonstrukció hitelessége és licencfeltételei külön jóváhagyásra várnak.

## Ellenőrizhető aktuális állapot
- [x] Saját natív Android-projekt (C++17/JNI/Gradle, nem HTML/Canvas).
- [x] Android GitHub Actionsben C++ tesztek és APK build.
- [x] Érintéssel forgatható és nagyítható natív 3D próbaterület OpenGL ES 3-ban.
- [x] Véges fa/kő/levél készlet; szerszámkopás; idő és állóképesség.
- [x] Kövek -> kőél; faág -> megmunkált cölöp; cölöp/kő -> oszlop -> tetőváz -> levélfedés.
- [x] Menedék csak behúzódás után csökkenti a játékos esőterhelését.
- [x] Mentési séma v2 és v1 mentés betöltése, forráskódos regressziós teszttel.
- [ ] Szabadon lehelyezhető épületek és egyedi fizikai alkatrész-azonosítók.
- [ ] Valósághű, felmért Wonderwerk, animált hiteles embermodellek, ökológia.
- [ ] Egy valóban bejárható világ 1 játékossal + 4 NPC-vel és hiteles emberi animációval.
- [ ] Minden tárgyra kiterjedő fizika, sérülés, karbantartás, gyártás és közösségi tudás.
- [ ] Biblia szerinti korszakfüggő, 3D anyagú főmenü és eredeti logó életciklusa.
- [ ] Samsung Galaxy A56 és Honor Pad X8 készülékes grafikai/fizikai tesztek.

**Őszinte korlát:** a mai jelenet shaderben leírt 3D geometriából áll. Ez NEM fotórealisztikus Wonderwerk és nem a tényleges sok-alkatrészes 3D szerkesztő.

## Közös oksági szerződés
Minden entitásnak:
- Egyedi EntityID, DefinitionID, RegionID, anyagprofil (tömeg, sűrűség, keménység, nedvesség, hő, kopás, rugalmas/teherbírási tulajdonság), hely és állapot.
- Értelmezhető alkatrész- és csatlakozási gráf (pl. csavar/tégla/gerenda/tengely/vezeték/fogaskerék), kapcsolódási és erőátviteli feltételekkel.
- Tudás-/képesség-feltétel, szükséges eszköz, energia és munkafolyamat.
- Okozati tranzakció: előfeltételek vizsgálata -> bemenetek zárolása -> anyag/energia/kopás elszámolása -> időbeli munka -> eseménynapló -> következmény -> mentés. Sikertelenségkor nincs ingyenes eredmény.
- Ugyanezt a modellvilágot olvassa a GUI, a 3D render, az NPC és a szerkesztő. A menügomb nem találhat ki saját, ellentmondó állapotot.

## Modulok építési sorrendben
### F0 – Alapok és ellenőrzés
Natív Android build, ABI, logolás, mentés-visszatöltés, hibákhoz reprodukálható teszt, forrás- és licencleltár, minőségi kapuk.

### F1 – Anyagok és egyedi világobjektumok
A ma használt globális készletszámlálók lecserélése **egyedi, térbeli entitásokra**. Minden faág és kő a világban létezik, a játékos tényleg felveszi, szállítja, lerakja, feldolgozza. Tömeg- és anyagmérleg, időjárás miatti minőségváltozás, használt szerszám kopása.

### F2 – Alkatrészszintű szerkesztő
Önálló magyar szerkesztő APK-n belül. Test-, felület-, anyag-, kötés- és mozgásparaméter; kivágás/fúrás/rögzítés; elem import/export, visszavonás, verziómentés. A játékmenetben azonos alkatrész adatok alapján valósul meg a kivitelezés; a szerkesztő rendelkezhet teszt/sandbox móddal, de ezt egyértelműen meg kell különböztetni a túlélő játéktól.

### F3 – Natív minőségi 3D
C++ motor valódi GPU mesh-feldolgozással, glTF/GLB-importtal, PBR anyaggal, saját részletes környezeti és embermodellekkel, csontvázas animációval, LOD, fény/árnyék, ütközés. Nem egy újabb gömb/cilinder vagy álló kép. Tudományosan forrásolt Wonderwerk, valódi méretarányok; mobil GPU mérés mindkét célkészüléken. Minden növény, kő és építőelem hiteles, ellenőrizhető anyagprofilt kap.

### F4 – Első teljes őskori nap
1 játékos + 4 NPC: mozgás, szükségletek, saját emlékezet, tudás, kísérlet, kudarc, beszéd/reakció, munka, gyűjtés, menedék. Eső -> talaj -> csúszás/átázás -> eszköz/építés -> védettség. Minden lépés eseménynaplóval és mentéssel ellenőrizhető.

### F5 – Élő korszakfüggő menü
Biblia szerinti főmenü: Új játék, Beállítások, Készítők, Kilépés. A logo és gombok réteges anyagai **csak felfedezett világállapothoz** igazodhatnak, a képernyőre kerülő 3D növényzet/fény/füst a valódi ökoszisztéma és technológia állapotát tükrözi. Mindig olvasható és érinthető marad. Nem díszmenü vagy technológiai csalógomb.

### F6 – Gyártási és civilizációs lánc
Kőzet/érc/bányászat -> feldolgozás -> energia -> eszköz/alkatrész -> gép -> gyár -> téglagyártás -> ház; és hasonló részletezettséggel traktor, daru, jármű, villamoshálózat, számítástechnika, űripar. Az infrastruktúra és közösségi tudás a fejlődés kapuja; javítás, hulladék, ellátási lánc és kopás kötelező.

### F7 – Bolygó és távoli szimuláció
Régióazonosítók, földrajz, élőlényciklus, időjárás és ökológia, erőforrás-újratermelődés, L0-L3 részletességi szintek. Közel és távol ugyanazt a következetes világállapotot adja vissza.

### F8 – Mobilkiadás és minőség
Offline APK, stabil aláírás és frissítés, Android-mentés migráció, stresszteszt, képkockaidő, memória, akkumulátor, kezelés és teljesítmény a Samsung Galaxy A56-on és Honor Pad X8-on. Az APK build nem azonos a valódi eszközön végzett sikeres grafikai teszttel.

## Első elfogadási vizsgálat
1. 2 követ valóban fel kell venni; ebből kopó kőél és hulladék lesz.
2. 5 faágat egyenként meg kell munkálni, mindegyiknél idő, energia és élkopás keletkezik.
3. 2 cölöp + 2 kő csak két oszlopot eredményez; tető még nincs.
4. 3 további cölöp után 3D tetőváz, 6 leveles ág után 3D fedés jelenik meg.
5. Erős esőben a szabadban nő az átázás; a kész menedék alatt, tényleges behúzódás után csökken.
6. Hiányzó anyag/eszköz/tudás esetén nincs állapotváltozás. Mentés után az állapot egyezik.

**Nem elfogadási állítás:** a shaderes próbaterület nem jelenti a valósághű 3D világ teljesülését. A következő nagy műszaki feladat a rögzített számlálók és rajzolt elemállapotok lecserélése valódi, térben elhelyezett alkatrész-entitásokra.
