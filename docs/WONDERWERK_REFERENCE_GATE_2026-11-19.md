# TAKS World – Wonderwerk, elfogadott referenciából történő folytatás (2026-11-19)

## Megőrzendő forrás
- Eredeti elfogadott, vágatlan kép: `Széljárta ősi szavanna a dolomitbarlangnál.png`
- 1536 × 1024 RGB; SHA-256: `980ea229cb87bda9f18891681a9227db111a01ec56ad2e5149c6426b21b63aa5`
- A teljes barlang-sziklatömb egy elem, nem darabolható félbe. A meglévő grafikai és Android kód nem írható felül automatikusan.
- Kanonikus Word-forrás: `TW62.docx`; különösen a 12–14. fejezetet kell követni.

## A 2026-11-19-i műhelycsomag tényleges eredménye
- 11 veszteségmentes, forráspixelekből vágott téglalap-kivágás; 5 durva, jóváhagyásra váró alfa-maszk.
- 16 képkockás korai mozgáspróba: szél által fodrozott víz, lomb lengése, füst sodródása.
- Egy natív C++ `WorldCauseEffect.hpp` tesztmodul a szél, eső, sár, víz, tűz és 1 játékos+4 NPC szükségleteire; helyi C++17 regressziós teszt sikeres.
- `humanoid_rig_spec.json`: humanoid csontváz, IK-alapú kézfogás és földkontaktus, 8 irány.

**Ezek NEM a kész motorba kötött, elfogadott elemek.** Az összes preview/vágás a beszélgetésben átadott teljes ZIP-ben szerepel, a 334 MB-os TW62.docx nélkül.

## Kötelező 8 izometrikus irány
```
000  045  090  135
180  225  270  315
```
Az eredeti referencia egyetlen nézet. A takart 7 oldalt nem szabad tükrözéssel késznek hazudni. Ugyanazon tárgy 3D/geometriai termelőforrásából 8 konzisztens 2D ortografikus sprite master kell, azonos talpponttal. A Biblia előírja az 8192 × 8192 RGBA / nézet CÉLMÉRETET, nem ez a jelenlegi export.

## Okozati követelmények
- szél → vízhullám, lomb és füst; vihar → esetleges törés,
- eső → talajnedvesség → sár → tapadásvesztés / járáslassulás / lábnyom / saras láb,
- nedvesség és szél → tűzállapot / füst / tüzelőanyag-fogyás,
- meleg, hideg, pihenés, éhség és szomjúság → külön döntések az 5 embernél,
- valódi térbeli fogás / emelés / hordás / építés: kéz IK + talppont + anyagmérleg.

## Megjelenítés: friss Biblia elsőbbsége
A TW62.docx 12. fejezete végső 2D izometrikus, 8 nézetes sprite-élményt ír elő. A korábbi natív GLES3 3D technikai ág egyelőre változatlan marad; a 3D geometria a sprite-gyártás forrása lehet. Az eltérő megjelenítőmódokat nem keverjük rejtett módon.

## Elfogadási kapu
Még hátravan: a maszkolás pontosítása; nyolc tényleges irány; barlang belseje; anatómiai korhű embermodellek, egyedi karakterek; animációk és a natív motor integrációja; Android APK és valós telefonos mérés. A játékos karakter test/arc kialakítása külön jóváhagyandó.

A meglévő kódhoz csak jóváhagyott, tesztelt és additív megoldás kerülhet.