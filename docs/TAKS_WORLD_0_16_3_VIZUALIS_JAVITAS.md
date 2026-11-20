# TAKS World – 0.16.3 vizuális javítások (forráskód)

Ez a változtatás a **0.16.2-es elfogadott Wonderwerk menüágat** folytatja. Az eredeti fotó, kőlogó és menügomb-grafika bájtjait nem írja felül.

**Megvalósított forráskód-javítások**

- A négy magyar gombfelirat kisebb, külön körülvágott, 176 képpontos maximális szövegszélességű területre kerül; a funkciók natív Android gombokra kötve maradnak.
- A második, programból rárajzolt körkörös fényfolt megszűnik: csak a forrásfotón lévő nap marad, a mozgó felhőárnyék a táj fényességét módosítja.
- A vízmaszk korlátozza a csillanásokat és hullámokat: a sziklát, partot vagy kőgombot nem mozgatjuk.
- A páfrányok és füvek erősebb, eltérő frekvenciájú mozgást kapnak; a fa lombja a törzshöz rögzítve hajlik.
- A barlangpor és a madarak már létező animációja megmarad; az animált képkockákról Android-naplóbejegyzés készül az ellenőrzéshez.

**Még NEM igazolt vagy nincs kész**

- A forrásfotón a bal szélen levágott fát képszerkesztéssel/outpaintinggel kell befejezni; a kód nem tudja visszaállítani az eredetileg hiányzó részeket.
- Ha a forrásfotó eredetileg két napot tartalmaz, a másodikat új képpel kell eltávolítani. Ez a commit csak a programból rárajzolt második fényt szünteti meg.
- A valódi Android-képernyőfelvétel, a feliratok pixelpontos elhelyezése és az állatok 3D mozgása csak futtatott teszt után igazolható.
- Kész, természetfilm-minőségű játéknak vagy teljesen javított APK-nak a forrásmódosítás önmagában nem nevezhető.

**Elfogadási teszt:** egyetlen nap; minden fa és kő teljes és természetes; a négy magyar felirat minden képernyőn belül marad; 10 másodperces Android-rögzítésen látható folyamatos lomb, páfrány, víz, felhő, madár és barlangpor; videóugrás nincs.
