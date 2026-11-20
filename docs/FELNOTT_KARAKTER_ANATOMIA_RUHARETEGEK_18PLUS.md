# TAKS World – felnőtt anatómia, külön ruhák, PNG-izometrikus karakter (2026-11-19)

## Döntés: elfogadott látványhoz hozzáadva, felülírás nélkül
A `TW62.docx` és a 2026-11-19-i elfogadott Wonderwerk-szavanna/barlang referencia kötelező. Futás közben PNG/RGBA izometrikus sprite-rétegek használata a cél, 8 irányban (000°–315°, 45° lépésben), közös pivot/talppont, azonos fényirány, modell- és képregisztráció. Az alaptest nem azonos a ruhával. Fényképi textúrák és az animációkat vezérlő 2D skeleton együttműködése szükséges.

## Test, 18+, korhatár
- Felnőtt szereplőknél a test anatómiai variánsokat, arányokat, bőrt, hajat és az intim testrészek adatkészletét külön tartja nyilván.
- Az intim/anatómiai részletek vizuális kezelése független a ruhacserétől, a képanyag **még nincs legyártva**.
- Korhatár a karakterre: ismeretlen életkor esetén **nincs felnőtt anatómiai megjelenítés**. 18 év alatt soha. Csak 18+ szereplőnél, külön engedélyezett felnőtt megjelenítési módban válhat elérhetővé.
- A '18+' jelzés önmagában nem korhatár-besorolás és nem helyettesít platform- és területspecifikus megfelelőséget vagy kor-ellenőrzést; a játék besorolását a végleges tartalom alapján kell rendezni.
- Kapcsolati és családi rendszer, egészség, magánélet, várandósság és születés későbbi, külön, etikus és életkori korlátokat érvényesítő modulok lehetnek. Semmilyen fiatalkorú karakter nem része felnőtt intim ábrázolásnak.

## Cserélhető PNG-eszközhelyek
`torso` felsőruha; `legs` nadrág/alsóruha; `feet` cipő/saru; `head` fejfedő; `belt` öv; `back` hátizsák; `hands` kézben tartott eszköz; `underwear_top` alsó felső; `underwear_bottom` alsónemű; `bra` melltartó/melltámasz; `socks` zokni/kapca; `outerwear` kabát/köpeny; `jewelry` ékszer; `intimate_cover` történeti ágyékfedő/kötés. A már bevezetett első hét felszereléshely számozása változatlan.

- A ruhadarabnak külön tárgyazonosítója, mérete, testprofil-kompatibilitása, típusa, anyaga, tömege, melegtartása, nedvessége, szennyezettsége, kopása és tulajdonosa lesz.
- Képi sorrend: test → aláöltözet/melltartó → felső/alsó ruházat → cipő/öv → köpeny/kiegészítők. A konkrét takarási sorrend irány- és pózfüggő: pl. hátulnézetben hátizsák, elölnézetben kéz vagy köpeny külön front/back maszkolást igényel.
- Minden cserélhető elemnek 8 nézetben és minden szükséges akcióban követnie kell a skeletonnal azonos talppontot: járás, futás, guggolás, fekvés, mászás, ugrás, úszás, fogás, emelés, építés.
- Az `engine/SpriteActor2D.h` egyelőre állapot- és sprite-réteg leíró; sem a teljes fedési maszk, sem a felnőtt anatómiai megjelenítő, sem az Androidon aktivált 2D-skinning nem kész.

## Ok → folyamat → okozat
- Eső + szél → anyag nedvesedése, hűlés, nehezebb ruha, száradási idő.
- Sár → ruhaszennyeződés, csúszásveszély, járáslassulás, tisztítási munka és vízigény.
- Hőség/hideg + ruha szigetelése → komfort és állóképesség változása.
- Ruha/cipő készítése → alapanyag, feldolgozás, idő, tudás és szerszám → konkrét egyedi tárgy, minőség és kopás.
- Későbbi bolt → gyártás, készlet, szállítás, vétel és eladás; a felszerelés nem keletkezhet ingyen, előzmény nélkül.
- Az őskori korszakban csak a rendelkezésre álló anyagokból, technológiával előállítható ruhák hozzáférhetők; mai melltartó, cipő, üzlet nem kerül visszamenőleg az őskorba.

## Fejlesztési kapu
1. 8 jóváhagyott testmaster minden felnőtt testprofilra, ruháktól külön.
2. Pózonként 2D csontvázpontok, teljes láb-/kéz-IK, kúszás, guggolás, fekvés, mászás stb.
3. Ruhák per-testforma, per-póz, per-irány animált PNG-je, maszkjai.
4. Rendezettségi/átfedési teszt mind a 8 irányra; felnőtt megjelenítés megnyitása csak valódi korhatárszabályozás után.
5. Natív játék-renderer, mentés/visszatöltés, teljesítménymérés, Android APK és valós eszközteszt.

**Jelenleg kész:** ruhaszlotok kód- és metaadatai, aláöltözet felszerelés/prioritás, alap anyagi ok-okozat, C++ tesztesetek forrása. **Nem kész:** további PNG-k, fedőmaszkok, 2D IK, 18+ felhasználói kapu, hivatalos besorolás, teljes élő játék.
