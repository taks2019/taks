# TAKS WORLD · natív Android / C++ TAKS Engine

> **KIINDULÓ ÁLLAPOT — ELUTASÍTOTT PROTOTÍPUS.**
> A 2026-11-17-i képernyőkép a korábbi, statikus fehér emberfigurákat, egyszerű talajt és hiányzó eredeti HUD-logót mutatja. A jelenlegi módosítás ezt az állapotot célozza, de nem állítja, hogy már kész a teljes, hiteles Wonderwerk-játék. A korábbi APK-k továbbra is tesztverziók.
>
> **Az eredeti TAKS WORLD logó:** a teljes átadási ZIP `original_assets/TAKS_WORLD_ORIGINAL.png` útvonalán található változatlanul. Az Android APK a menühöz méretezett, arányhelyes WebP változatot használja; az eredeti PNG-t nem írjuk felül.

## Újratervezéshez kötelező elfogadási kapuk

1. A legfrissebb, tulajdonos által jóváhagyott 3D Obsidian Biblia és az **eredeti** logó pontos forrásának rögzítése.
2. Valódi, forrásként tárolt 3D hálómodellek (GLB/GLTF vagy belső modellformátum), normális UV-k,
   anyagtérképek és csontvázas animáció. Nem sphere/cylinder/synthetic sticks a végső grafika.
3. Valós sziklafelszín, bejárható Wonderwerk-barlang, természetes lépték; modell- és anyageredet dokumentálva.
4. Képfelvételes Android GPU mérés Samsung Galaxy A56-on, memória- és FPS-adatokkal.
5. A Biblia szerinti ok-okozati környezet: fizikai anyag-, idő-, szerszám- és munka-előfeltételek.
6. Eredeti logó és Biblia-követő, kompakt, helyzetfüggő GUI; nincs képként szimulált játék.
7. Ezek hiányában tilos kész/élethű TAKS World APK-ként átadni.


**Jelenlegi mérföldkő: 0.6.0-isometric-fast — natív gyors 3D hálókirajzolás, valódi izometrikus kamera és kétirányú forgatás.**

## Következő oksági bővítés — szél és növénymozgás

A mentett időjárásból egyetlen szélerősség származik. Ugyanaz az érték hat a talaj- és faanyag-száradásra, a tűz fogyására, a státuszjelzésre és a GLES3 fotótextúrás növényhálók folyamatos lengésére. Ez egy tesztelhető játékrendszer-bővítés; önmagában nem kész grafikai mérföldkő vagy kész játék.

Ez a felhasználó saját játékmotorjának **valódi natív Android** fejlesztése, nem HTML/Canvas, Unity vagy Godot.

## 0.6.0 – Izometrikus kamera és mobilon is gyorsabb valódi 3D

A 0.5-ben a teljes képet képpontonként drága sugármeneteléssel számoltuk, ezért a játék szaggatott. A gyártásban használt 0.6-os natív 3D motor az aktuális játékvilágot **OpenGL ES 3 háromszögmesh-geometriával, valódi mélységkezeléssel és GPU-rasterizációval** rajzolja. Ezen a lépcsőn ez sebességoptimalizálás, **nem** a végső fotórealisztikus megoldás.

- **Valódi ortografikus izometrikus alapnézet:** 45° alapforgatás, 35,264°-os emelkedési szög. Nincs perspektivikus deformáció.
- **Szabad 3D kameraforgatás:** a jobb oldali érintés vízszintesen 360°-ban forgatja, függőlegesen pedig 20–80° között döntheti a kamerát.
- **Nagyítás:** kétujjas mozdulattal, ortografikus vetítés mellett. A kamera követi a játékost.
- **A mozgás és a kamera iránya összefügg:** a bal oldali irányítás forgatott kameránál is a képernyőhöz igazodik.
- **Lényegesen kevesebb GPU-terhelés:** nincs 92-szeres képpontonkénti világvizsgálat; helyette a terep, a sziklafal, a nyersanyagok, az NPC-jelölők és az építmények valódi 3D háromszögekből állnak.
- **30 Hz-es mozgásfrissítés** a korábbi 85 ms helyett 33 ms-onként, arányosan kisebb mozgási lépésekkel.
- **Három választható grafikai mód:** 640 / 840 / 1200 képpontos belső szélesség. A kijelző képaránya minden módban megmarad.
- **Menüből választható kameraállások:** izometrikus alaphelyzet, felülnézet, alacsonyabb 3D nézet.
- A három építési hely, az anyagfogyás, a négy NPC és a mentések változatlanul a korábbi natív C++ világállapotból dolgoznak.
- A GitHub Actions most már a **valóban futtatott háromszög-rasterizáló** GLES3 megjelenítőt teszteli négy külön ortografikus kameranézettel. A korábbi, kikapcsolt sugármenetelő shader tesztjének sikeressége nem volna elég.

**Mérések:** a GitHub Mesa szoftveres OpenGL ES tesztjében a bemelegítés utáni 640×360-as képkockák kb. 14–17 ms alatt készültek el. Ez nem Samsung Galaxy A56 vagy Honor Pad X8 mérés; a tényleges telefonos FPS-t a készüléken kell ellenőrizni.

**Továbbra is hiányzik:** eredeti, részletes embermodellek, valós fényképalapú PBR anyagok, hiteles Wonderwerk-felmérés és teljes 3D világszerkesztő. A 0.6-os lépés a sebességet és a kamerát javítja, nem állítja, hogy a látvány már megfelel a Bibliának.

## 0.5.0 – Szabad helyválasztású, különálló építmények
- **Új HELY KIJELÖLÉSE** művelet: a játékos gyalog elmegy a kívánt, szabad területre, és 1 követ elhasználva kijelöli a menedék helyét.
- **Három különálló építési hely** lehet a próbaszektorban. Nem fix világpozíción épülnek; minden kijelölt hely megőrzi saját koordinátáját, munkafázisát és állapotát.
- **Valódi előfeltételek:** 2 cölöp + 2 kő = oszlopok; +3 cölöp = tetőváz; +6 leveles ág = fedés. A munkafázis az adott építési helyet módosítja, nem az összeset.
- **Környezeti kopás és javítás:** a fedés állapota az időjárás és az eltelt idő miatt romlik; javításhoz 1 cölöp és 2 leveles ág, munka és állóképesség kell. A végletekig sérült fedés nem ad megbízható esővédelmet.
- A játékos csak akkor kap fedést, ha **a ténylegesen felépített és ép menedék alatt áll**; a fedés nem hat távolról.
- Az Androidos ÉPÍTÉS panel az adott helyen végrehajtható munkafázist mutatja, és kezeli az új hely kijelölését és a javítást.
- **TAKS_NATIVE_5** mentés: mindhárom hely külön állapota megmarad; a v1–v4 régi mentések olvashatók, és a korábbi rögzített helyszínen tényleg meglévő menedék v5 térbeli építményként folytatható.
- GitHub Actions: C++ regressziók + valós GLES3 képtesztek, 4 renderkockával (hely, oszlop, fedés, két hely) + natív Android APK.

**Korlátok:** a maximálisan 3 építési hely és az előre meghatározott menedékrecept még nem teljes alkatrészszintű szabad szerkesztő. Az OpenGL ES-ben programozott geometria nem élethű Wonderwerk-felmérés, és továbbra sincs részletes animált embermodell. A telefonos futás és vezérlés kizárólag valódi készüléken igazolható.

## 0.4.0 – Élő közösség és ok-okozati munkavégzés
- Négy autonóm NPC jelenik meg külön világpozícióval és négy eltérő feladattal: fa, kő, levél, gyújtós.
- A társak **ugyanazt a véges 12 lelőhelyet** használják, mint a játékos; nincsen ingyenes anyagteremtés.
- Elmennek a nyersanyaghoz, felveszik, visszaviszik a közös táborba, és **tábori készletet** építenek.
- Minden NPC saját energiával, éhséggel, átázással, szállítmánnyal és feladattal rendelkezik; kimerüléskor pihen, súlyos éhségnél megáll.
- **TÁRSAK** panel: a munkások helye és tevékenysége látható, a játékos a tábor közelében átveheti a már összegyűjtött anyagokat.
- A világ ideje a játék közben automatikusan halad, nem kizárólag gombnyomáskor.
- **4-es mentési formátum:** NPC-k és tábori készlet állapota is megőrződik; korábbi v1-v3 natív mentések betölthetők.
- C++ anyagmérleg- és mentéstesztek, valamint natív GLES3 shaderes képtesztek kötelezőek az APK-fordítás előtt.

**Korlát:** a négy társ világhelyzetét egyelőre fejlesztői 3D jelölők mutatják, nem kész élethű embermodellek. Az NPC-k tevékenysége egy első, erőforrásalapú munkaciklus; nincs még teljes szükséglet- és tanulásrendszer. Az élelemhiányt nem oldja fel a semmiből létrejövő étel.

## 0.3.0 – játszható mechanikai fejlesztés
- **Bal oldali MOZGÁS érintőzóna:** a játékos világkoordinátában mozog, a kamera követi.
- **Jobb oldali húzás:** kamera forgatás; **kétujjas gesztus:** nagyítás/kicsinyítés.
- **FELVESZ:** csak közelben lévő, azonosított, véges készletű nyersanyagot szed fel.
- A terepen 12 rögzített, külön készletű lelőhely látszik (fa, kő, gyújtós, levél); ezek a gyűjtéssel kiürülnek.
- A barlang homlokzati falán a karakter nem sétálhat át; az első világ határai és a mozgás fáradtsága számítanak.
- **Térbeli építés:** a tűzhelyhez és a menedékhez valóban oda kell menni.
- **V3 mentés:** játékoskoordináta, haladás és lelőhelyenkénti megmaradt mennyiség; v1/v2 betöltés továbbra is támogatott.
- **MENÜ:** Folytatás, Következő cél és vezérlés, Mentés, Új világ, Kamera visszaállítás, Készítők, Kilépés.
- GitHub Actions: az Android-fordítás és a C++ tesztek mellett külön **headless GLES3 shaderes képteszt** fut.

### Játék indulása
1. A kezdőhely közelében kő található. Menj oda a bal oldali MOZGÁS területen, majd **FELVESZ**.
2. Két kőből készíts kőélt az ÉPÍTÉS menüben.
3. Keress faágakat, készíts belőlük cölöpöket.
4. Menj a menedék helyére, építs oszlopokat, tetővázat és leveles fedést.
5. Használd a menedéket; az időjárás, a munka és a mentés ugyanazt a világállapotot módosítja.

**Korlát:** a jelenlegi karakter még egyszerű, a 3D terep generált teszthelyszín, a menedék egy rögzített pozícióban építhető. Ez még nem a hiteles Wonderwerk, és nem a teljes, NPC-kkel benépesített játék.

## Mit lehet jelenleg kipróbálni?
- GPU-s sziklás tesztjelenet, forgatható kamera, kétujjas nagyítás.
- Véges fa/kő/levél/gyújtós készletek; sikertelen cselekvés semmit sem teremt.
- **Kőeszköz:** két kőből elkészíthető, kopik, hulladékot hagy.
- **Alkatrész:** a nyers faágat kőéllel cölöppé kell megmunkálni.
- **Menedék:** 2 cölöp + 2 kő → tartóoszlopok; további 3 cölöp → tetőváz; 6 leveles ág → fedés. A három munkafázis külön megjelenik a 3D képen.
- A menedék csak akkor csökkenti az átázást, ha a játékos ténylegesen behúzódik.
- Idő, állóképesség, csapadék, fa nedvesség, tűz, helyi mentés és v1 mentések visszaolvasása.

**Korlát:** a jelenet shaderes 3D teszthelyszín, NEM a hiteles Wonderwerk és NEM fotórealisztikus. A munkafázisok ma egy rögzített építési helyet érintenek; egyedi, szabadon mozgatható alkatrész-entitások és szerkesztő a következő feladat. NPC-k, teljes fizikai modell és korszakrendszer még nincsenek készen.

## GitHub Actions
Az [Android APK munkafolyamat](.github/workflows/android-apk.yml) minden `master` commitnál:
1. CMake segítségével lefordítja a C++ szimulációs magot.
2. Regressziós tesztekkel ellenőrzi a tűz/hearth és menedék okozati láncait és a mentés migrációját.
3. Android SDK + NDK segítségével összeállítja a `app-debug.apk` fájlt.
4. A futás **Artifacts / TAKS-World-native-debug-APK** részében közzéteszi a ZIP-et.

A GitHubon futó APK-fordítás **nem igazolja**, hogy a 3D futás minden Android-készüléken hibátlan. A grafikus shader külön szoftveres OpenGL ES próbán ellenőrizhető, de Samsung/Honor készüléktesztre továbbra is szükség van.

### Frissítés és aláírás
A GitHub Actions debug APK-k alkalmi debug kulccsal készülnek. Ennek aláírása külön futások közt eltérhet, ezért a korábbi teszt-APK közvetlen felültelepítése **aláírásütközés miatt meghiúsulhat**. Ne töröld a meglévő játékot, ha abban fontos mentésed van, amíg nincs ellenőrzött export és tartós aláírás. Release aláírási terv és stabil mentésmigráció szükséges.

## Teljes építési sorrend
Olvasd el: **[FEJLESZTESI_TERV.md](FEJLESZTESI_TERV.md)**.

**Elsődleges szabály:** nyersanyag -> munka -> alkatrész -> szerkezet -> használat -> kopás -> javítás; minden állapotváltozásnak azonosítható oka van.

## 2026-11-18 — natív humanoid- és logófrissítés

- A static TKM1 embertest helyére 121 422 csúcsos, GPU-s bőrsúlyozást és 53 forráscsontot tartalmazó TKS1 asset került. A projekt a forrás `idle`, `idle_breathe`, `idle_fidget`, `walk`, `run`, `strafe_left` és `strafe_right` animációit használja; a mozgásállapot-váltás 280 ms alatt keveredik.
- A játékos és a négy NPC ugyanezt a csontozott modellt használja. A 3D rúdember-tartalékrajzolás törölve; a natív szereplőmodell hibája esetén a program hibát jelez, nem mutat fehér bábut.
- Az NPC-k szimulált koordinátáit a natív renderer a 1,8 másodperces szimulációs lépés között interpolálja. A játék GPU-képe 40 ms-onként frissül, a szimuláció ettől független, ritkább időlépésekkel fut.
- Az eredeti logó arányhelyes, 512×341-es WebP másolata szerepel a főmenüben és a játék HUD-ján; a teljes felbontású PNG változatlanul a kiadási ZIP-be kerül. A környezeti keret a mentett világ esőjére, tüzére és napszakára reagál.
- A renderer a meglévő, valódi fotogrammetriai sziklákat több terepponton is elhelyezi. A terep és a barlang azonban továbbra sem a Wonderwerk helyszíni felmérése, a növények és az éghajlat sem régészetileg hitelesített.
- A CharacterCreator-eredetű ruhahálók CC0-k; a bőr-szerű textúra saját generált anyag. A szabás kortárs, ezért a ruházat és a humanoid fajazonossága **nincs történetileg hitelesítve**.
- A build 13 GLES3 tesztképet készít és Android emulátoron ellenőrzi az eredeti logót, a 53 csontos asset betöltését, a beállítások/almenük X gombját, az érintőpadról történő mozgást és a mentésfájl létrejöttét.

Az ellenőrzött build állapotát, teszteredményeit és további hiányosságait a [2026-11-18-i változásnapló](CHANGELOG_2026-11-18.md) rögzíti.
