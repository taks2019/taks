# TAKS WORLD — ZÁROLT ALAP / KIZÁRÓLAG UPDATE (2026-11-21)

A tulajdonos egyértelmű, kötelező kérése: **nem szabad a kialakított menüt, eredeti jóváhagyott látványvilágot és játék-architektúrát újratervezni vagy felülírni.** A következő munkák ugyanannak a programnak egymásra épülő, visszafelé kompatibilis frissítései.

## 1. A már jóváhagyott állapot változatlan

- Középen az eredeti, töredezett kő TAKS WORLD logó, alatta négy funkcionális kőgomb: **ÚJ JÁTÉK**, **BEÁLLÍTÁSOK**, **KÉSZÍTŐ**, **KILÉPÉS**. Hely, arány, tipográfia, eredeti pixelek, érintőcélok: nem módosíthatók.
- A már átadott eredeti menükép SHA-256: `9369d3668b69424d31132e70accbfdb76282d7d9b3f3adfaa51c7c3ca1ea7bc8` (1664×768).
- A kód `RestoredStoneMenu.java` teljes forrása az elfogadott referencia. A `MainActivity.showMainMenu()` teljes blokkját védi a CI. A `newWorldFromMenu`, `showSettings`, `showCredits`, `confirmExit` eredeti funkciók maradnak.
- Nem cserélhető le semmilyen elem kitalált logóra, statikus-videós trükkre, átmeneti placeholderre, újra kivágott fára vagy idegen barlangra.
- A barlang magas színvonalú PNG-képeinek szépítése, az állatok, emberek, növények és terep a meglévő izometrikus, 2,5D TAKS Engine struktúrában külön felülvizsgálatot és elfogadást igényelnek.
- A meglévő játékállásokat és fordításokat az UPDATE nem törölheti, nem nullázhatja le.

## 2. Az Android UPDATE technikai azonossága

A felhasználó legutóbb átadott APK-jának azonosítója: **`hu.taks.world.menufixedgame`**. A további teszt-UPDATE-ek ugyanennél az alkalmazásnál növelik a `versionCode`-ot: 15 → 16 → 17 ... . Új `applicationIdSuffix`, külön "demó", új projekt, mentési könyvtár vagy reset **tilos**.

**KRITIKUS:** Android csak azonos aláíró tanúsítvánnyal enged helyben frissítést. A tényleges, legutóbb átadott aláírás:

`ddf758c3c311da162ff49f9876591284526a42c7daa7e918994a3a57de67299c`

A privát aláírókulcs *nem tölthető fel nyilvános GitHub-fájlba*. A helyi, az előző APK-hoz használt kulcsot vagy biztonságosan beállított GitHub Actions titkot kell használni. A GitHub minden alkalommal újonnan generált debug-keystore-ja **nem alkalmas** UPDATE-re. A hibás kulccsal készült APK átadása **tilos**.

Azonos, vagy nagyobb versionCode nélkül, mint a jelenleg telepített csomagé, UPDATE nem kínálható. A telepített csomag hiteles adatait `adb shell dumpsys package hu.taks.world.menufixedgame` segítségével ellenőrizni kell az első valódi eszközfrissítésnél.

## 3. Csomagolás és ellenőrzés

A `scripts/enforce_taks_update_policy.py` build-kapu:

- Egy byte-ot sem enged eltérni az eredeti menüképtől.
- Nem enged változást a menü rajzolóján és a teljes menükezelő metóduson.
- Nem enged régi jóváhagyott logo/menu elem törlését, cseréjét.
- Nem engedi új app ID vagy alacsonyabb verzió megadását.

Az aláírás ellenőrzésére külön `scripts/verify_taks_update_apk.py` szolgál; a kiadható APK **az eredeti aláírókulccsal** készül, nem új debug-tanúsítvánnyal. Telefonos telepítés és képernyőteszt szükséges a teljes siker kimondásához.

## 4. A látvány animálása a menü cseréje nélkül

A szélben mozgó lomb, páfrány, fű, vízfelszín, porszemek és napsugár kizárólag **az eredeti látvány változatlan megtartásával**, külön grafikai/animációs rétegben készülhet. A menü középső logója és négy gombja minden képkockán geometriailag változatlan marad. Az új mozgó média és az animáció kezelője először külön tesztként készül, majd kizárólag felülvizsgálat után kapcsolható be a normál menüben.

A játék nem videólejátszó: minden játékbeli növény, ember, állat és víz mozgása továbbra is a natív TAKS Engine megfelelő világállapotához, szélhez, nedvességhez és ok-okozati rendszerekhez kötődik.

## 5. Átadási szabály

**Csak UPDATE**: azonos Android csomagnév, változatlan eredeti menü, eggyel magasabb verzió, azonos tanúsítvány, mentésmegőrzés. Nincs új "V01", "V02", nincs önkényes grafika-felülírás. Ha a build nem felel meg akár egy pontnak, az új telepítő **nem adható át frissítésként**.

Minden új grafika külön tétel marad a már meglévő `assets/visual_approvals.json` jóváhagyási rendszerben. A felhasználó kifejezett jóváhagyása nélkül a korábban elfogadott képet semmi nem váltja le.
