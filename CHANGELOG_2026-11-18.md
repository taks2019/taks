# TAKS World Android — 2026-11-18

## Módosítások

- A régi TKM1 statikus emberimportot TKS1 bőrsúlyozott formátum váltja fel. A csomag 121 422 hálócsúcsot, 53 csontot és hét forrásból származó animációt tartalmaz.
- A GLES3 vertex shader a négy csontbefolyást súlyozva keveri. Az idle, walk és run állapotok közti átmenet 280 ms; a négy NPC szimulált pozícióját a renderelés a szimulációs lépések között simítja.
- Javítva az animációs asset bináris elrendezése: a teljes csontforgatás-blokkot külön követi a gyökérmozgás-blokk; az importáló most az eredeti gyökérmintákkal is ellenőrzi az eltolt adatokat.
- A hiányzó modell esetére rajzolt fehér rúd- és bábualakok eltávolítva. Hibás humanoid asset esetén a renderer naplóz, nem rajzol hamis figurát.
- A pályaszélekre valós CC0 fotogrammetriai sziklák kerültek; a már importált nagyobb kőzetmodellt is használja a terep.
- A GPU-s jelenetben több méretben és eltérő forgatással jelennek meg a valódi sziklamodellek; a fűfoltok sűrűsége és kiterjedése a mentett biomasszához igazodik.
- A MakeHuman diffúz bőrtextúráján melegebb színkorrekció jelenik meg, így az alakok nem hófehér bábuként tűnnek fel.
- A felhasználó eredeti logója a teljes átadási ZIP-ben változatlan PNG-ként megmarad. Az APK-ba a képarányt megtartó, 512×341-es WebP másolat kerül, és a HUD is ezt mutatja.
- A játék 25 FPS-es renderfrissítést kap, hogy a csontozott idle és járás ne csak ritka szimulációs frissítéseknél lépjen tovább.
- Az Android UI ellenőrzése kiegészült a 53 csontos modell betöltésének logjával, az érintőpadról indított mozgás koordináta-változásával és a SharedPreferences mentés meglétével. Az XML hierarchia olvasása `adb pull` helyett shellen keresztül történik.
- A GPU regressziókép-csomag 13 képkockát ellenőriz: kameranézetek, környezeti változások, idle, séta és futás.

## Ellenőrzés

- Helyben sikeres: világ- és ökológiai C++ tesztek; Python importer szintaxis; `git diff --check`; GLES3 C++ források szintaktikai ellenőrzése minimális fejléc-stubokkal.
- A fejlesztői környezetben nincs Android SDK/NDK és GLES fejléckészlet, ezért helyi APK- vagy valós GPU-build nem készült. A natív Android APK, GLES3 képek és Android UI tesztek eredménye a GitHub Actions futásából kerül be a végső csomagba.

## Még nincs kész / nincs hitelesítve

- A MakeHuman mesh általános humanoid, nem régészetileg ellenőrzött Homo erectus.
- A hozzáadott ruhamodell-hálók kortárs szabásúak; az állatbőr-szerű textúra nem bizonyít történeti ruházatot.
- A Wonderwerk-barlang, a paleokörnyezet, a növényfajok, az éghajlat és a teljes, fizikai ok-okozati ökoszisztéma további hitelesítésre és fejlesztésre vár.
- Az Android APK debug tesztbuild; kiadási aláírás és végleges játékminőség nem része ennek a változatnak.
