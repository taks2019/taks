# TAKS World – jóváhagyott látványból fotórealista PNG karakter

2026-11-19: a korábban elfogadott Wonderwerk barlang/szavanna kép változatlanul megmarad. A karakterhez hozzáadó jellegű, 8 irányú, **2.5D/PNG + 2D csontváz/IK** munkafolyamat készül, nem GLB-központú megjelenítés.

## Irányok és rétegek
- 000°, 045°, 090°, 135°, 180°, 225°, 270°, 315°.
- Közös talppont, perspektíva, méret és fényirány kell minden karakterhez, ruhadarabhoz és animációs képkockához.
- A test és a ruházat nem egyetlen összesütött PNG: külön torso/láb/fej/öv/hát/kéz/lábbeli képsorozatok szükségesek.
- Mind az öt karakter saját szükségletekkel, azonos tárgy- és anyagmérlegszabályok szerint működjön.
- Az `engine/SpriteActor2D.h` valós, külön nyilvántartott tárgyazonosítót kér a felszereléshez. Ruhadarab nem keletkezik a semmiből.

## Animáció, a későbbi renderer szerződése
Állás, járás, futás, guggolás, fekvés, kúszás, mászás, ugrás, gyűjtés, cipelés, építés, úszás és pihenés: minden állapothoz saját képkockák és irányok. Egy statikus teljes-test PNG-n nem elég a csontkoordinátákat mozgatni: a tagokat külön képrétegekre kell bontani, vagy további mozdulatképeket alkotni. 2D IK segíti a láb talajkontaktusát és a kéz tárgyfogását.

## Ok és okozat
Nedves sár: vontatási tényező, sárnyom és kifáradás. Eső/szél: ruhák és test nedvesedése/száradása. Felszerelés: súly és hőszigetelés; környezet: hőség miatti szomjúság. A kódban szereplő számok elsődleges játékmenet-paraméterek, nem tudományosan kalibrált egészségügyi értékek.

## Állapot, korlátok
Létrejött nyolc külön RGB+A PNG, egy 4×2 atlasz és a C++ állapot-/felszerelés-/rétegsorrend-modul. A PNG-k a külön letölthető beszélgetési ZIP-ben vannak; **nem** kerültek bináris formában a GitHubba. A C++ modul még nem rajzol sprite-ot az Android megjelenítőben, és nem testesít meg kész 2D IK-t. A guggolás, fekvés, mászás és ugrás teljes animációs képkockái hiányoznak; az atlaszok és anatómiai következetesség grafikai ellenőrzést igényelnek.

A jóváhagyott referencia változatlan; nem kész APK és nem kész játék.
