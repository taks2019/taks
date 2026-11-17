# TAKS WORLD – 1. lépés: őskori élő menü

**Fejlesztői ág:** `taks-world-biblia-menu-step-1`  
**Státusz:** tényleges forráskód-változás és unit tesztek; **NEM végleges grafika, NEM új APK.**

## Kanonikus forrás és elfogadás

- `TAKS_WORLD_ADAPTIVE_UI/01_FOMENU.md` és `02_BETOLTOKEPERNYO.md`, valamint a MEGALODON UI-jegyzék.
- A négy központi menüpont: **Új játék, Beállítások, Készítők, Kilépés**.
- Az eredeti logó pixeladatait tilos önkényesen felülírni vagy helyettesíteni.
- A 2026-11-17-én elutasított primitív FastScene figurák és korábbi kétes logó **nem** elfogadott végleges vizuális elemek.

## Ebben a commitban ténylegesen megváltozott

1. A régi rögzített, hűvös kékes `glClearColor` megszűnt. A natív GLES3 háttér most a világórából és esőből természetes, okker–föld–szén színátmenetet állít elő. Nem stúdió/neon fény.
2. Az Android főmenü sötétkékes szűrője helyett a valódi 3D jelenet feletti, átlátszó, meleg földszínű réteg jelenik meg.
3. A gombok meleg kő/fa/okker tónusokat használnak; a kiemelés csak a tényleges világállapotra reagál.
4. A natív World szolgáltatja a témához az órát, csapadékot, talajnedvességet, égő tűz állapotát, növényzetet és bizonyított mesterségbeli előrehaladást.
5. A menü megmutatja a ténylegesen bizonyítható felismeréseket. Kötél, páfrány és inda **nem** oldódhat fel véletlenszerűen. A még nem modellezett fajok/technológiák jelzői alapból nullák.
6. Java tesztek ellenőrzik a természetszín-palettát, napszakot, nedvességet és felfedezés-függést; a meglévő ok-okozati C++ tesztek megmaradnak.

## Amit NEM állítunk késznek

- A valódi **páfrány- és indanövények 3D mesh + textúra + fizikai/biom entitása** még hiányzik; ezért ezek megjelenítését nem hamisítjuk oda a menübe.
- A főmenü végleges, szerkeszthető, több rétegű, eredeti képi kellékei és animációi még nem készültek el. A régi tesztgombok nem kanonikus masterek.
- A felhasználó által elfogadott eredeti játéklogó exportját, azonosítását és beépítését külön kell igazolni.
- A korábbi gombóc NPC-k és helyettesítő terepmodellek még a motor fejlesztői jelenetében szerepelnek; **nem** elfogadott látvány.
- Nincs új APK, és nincs telefonos grafikai teszt. Kifejezett jóváhagyás előtt nem érkezik új „kész” játék.

## Következő fejlesztési kapu

Valós, forrásellenőrzött Wonderwerk növényi és kőzetassetek (3D + PBR + licenc),
majd ezek objektumainak összekötése a környezeti rendszerrel és a menü motívumrétegeivel.
A menü képvilágát az eredeti logóval, natív képrétegekkel és telefonos képernyőteszttel kell elfogadtatni.
