# TAKS WORLD · natív Android / C++ TAKS Engine

**Jelenlegi mérföldkő: 0.2.0-dev — alkatrészekből történő próbaépítés.**

Ez a felhasználó saját játékmotorjának **valódi natív Android** fejlesztése, nem HTML/Canvas, Unity vagy Godot.

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
