# TAKS 3D Studio Android

Önálló Android-eszköz a TAKS World modellgyártásához. Ez az ág nem a játék, és nem módosítja a master ágat.

## Használat
- A Felhős 3D generálás a Microsoft TRELLIS.2 hivatalos Hugging Face demóját nyitja meg az alkalmazáson belül. Képfeltöltés → Generate → Extract GLB → letöltés. Ha a beágyazott oldal nem működik, nyisd meg a böngészőben.
- A letöltött GLB a Modell importálása gombbal adható a helyi könyvtárhoz. A modell forgatható és nagyítható; a beágyazott animációk lejátszhatók. A meglévő GLB menthető másik helyre.
- A felhős szolgáltatás internetet, esetenként bejelentkezést és rendelkezésre álló GPU-kvótát igényel. A generálás nem helyi Android-inferencia. Az UltraTex, automatikus riggelés és a TAKS Engine még nincs integrálva. A textúraméretet a szolgáltatás felületén kell beállítani; a 4K nem garantált.
- A felhőszolgáltatásra feltöltött képet a szolgáltató dolgozza fel; csak olyan képet tölts fel, amelyet megoszthatsz vele.
- A helyi GLB-néző a GitHub build során csomagolt model-viewer könyvtárat használja, nincs szüksége internetre normál, beágyazott textúrájú GLB esetén. Nagy vagy speciális tömörítésű modellek eszközfüggően nem nyithatók meg.

## APK
GitHub Actions → TAKS Studio Android APK → Artifacts → TAKS_3D_Studio. Debug-aláírású APK, nem Play Store kiadás. Android 8+ (minSdk 26). Készülékteszt szükséges.

TRELLIS.2: https://huggingface.co/spaces/microsoft/TRELLIS.2
Model viewer: https://github.com/google/model-viewer (Apache-2.0).
