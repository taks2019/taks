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

## Kibővített műhely
A Studio külön képernyőjén GLB anyagfaktorok tényleges módosítása (új GLB, megőrzött BIN/textúrák), modell metaadatok, receptek, numerikus szabálypróba, XYZ világterv, projekt ZIP export és HTTPS ComfyUI API kliens található. A PC kapcsolat saját futó HTTPS szervert igényel; API-formátumú workflow JSON kell. A kiválasztott képet a LoadImage node-hoz tudja feltölteni, a promptot sorba állítja, a history kimenetei közül a GLB-ket letölti. A projekt JSON saját, dokumentálatlanul még nem motorintegrált előkészítő formátum. A világterv nem 3D színtér, a szabálypróba nem fizikai motor; nincs automatikus rigging, animációgenerálás, többes modellformátum-konverzió vagy felhős könyvtárszinkron. A forrás és a helyi adatok nem kerülnek automatikusan GitHubra.

## Kezelőfelület
A kezdőlap natív kártyás dashboard, közvetlen modulnyitással és a legutóbb importált GLB élő, rögzített emelkedési szögű előnézetével. Üres könyvtár esetén importálási felhívás látható. Nem tartalmaz előre gyártott látványképet vagy jóvá nem hagyott terepmodellt. A néző perspektivikus, nem a TAKS Engine ortografikus renderelője. A készülékes megjelenés tesztelendő.

## 3D jelenetszerkesztő
A GLB-k valódi ortografikus 3D jelenetben rakhatók össze (rögzített 35,264° emelkedés, 360° forgatás, zoom), a pozíció, méret, forgatás és projektmentés működésre tervezve. 64 objektumos korlát, eszközmemória függvényében kevesebb is lehet. Three.js alapú Studio előnézet, nem a játék TAKS Engine renderelője. A meglévő GLB animációk kiválaszthatók és lejátszhatók. Automatikus rigging és új animációgenerálás nincs implementálva. A teljes Biblia fájl nem volt hozzáférhető; a jelenet nem állítja elő automatikusan a Wonderwerk-barlangot.
