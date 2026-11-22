# TAKS World – valódi 3D Android APK, kizárólag elfogadott grafikával

**Fejlesztési ág:** `codex/taks-world-approved-only-3d-apk-2026-11-22`

A meglévő TAKS World natív C++ / OpenGL ES 3 projektet használjuk: ortografikus izometrikus kamera, folytonos körbeforgatás és valódi mélységi takarás. Ez a fejlesztési lépés az APK kiadási szabályát teszi biztonságossá, nem egy újabb állóképes demó.

## Kötelező jóváhagyási kapu

1. A forrásfájlokat a `scripts/check_visual_approvals.py` ellenőrzi az `assets/visual_approvals.json` alapján.
2. Minden új képet, textúrát, 3D modellt és animációt külön, kifejezetten jóvá kell hagynia a tulajdonosnak.
3. Egy jóváhagyás a **konkrét fájl SHA-256 lenyomatára** vonatkozik; a később módosított fájl új jóváhagyást igényel.
4. A `scripts/verify_approved_apk.py` **a már lefordított APK-t is** vizsgálja: minden becsomagolt vizuális fájl pontosan a jóváhagyott változat legyen.
5. A csomagban kötelező a natív `libtaks_engine.so`. Az önmagában álló PNG vagy ál-3D jelenet nem kielégítő.
6. Android `res/` alatt lévő, a jóváhagyási jegyzékben nem követhető képek is blokkolva vannak.
7. A meglévő képek és a fő játékprogram ezen az ágon változatlanok. A grafika nem cserélhető le automatikusan.

## Mit lehet letölteni GitHubon?

A `TAKS World · 3D APK · csak jóváhagyott grafika` GitHub Actions futása:
- Mindig készít egy `TAKS-World-Visual-Approval-Report` jelentést.
- A `TAKS-World-3D-Owner-Approved-APK` **csak** a jóváhagyás, a C++ tesztek, a Gradle-build és az APK tartalomellenőrzés sikeres lefutása után jelenhet meg.

**Jelenlegi akadály:** a meglévő manifestben több grafikai elem még `pending`, így az APK-kimenet szándékosan nem keletkezik. Nem jelentünk be sikeres APK-t, amíg ez nem változik.

## Teszt és korlát

A jóváhagyási audit 7 automatizált teszttel ellenőrzi a kihagyási kísérleteket, de önmagában nem bizonyítja a Wonderwerk-barlang régészeti hitelességét, a modell fotórealisztikus minőségét, a Samsung Galaxy A56-on mért FPS-t vagy a teljes játék befejezettségét.
