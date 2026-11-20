# TAKS WORLD — Wonderwerk elfogadott főmenü és natív bekötés

**Státusz (2026-11-19):** elkészült a lokálisan tesztelt, 30 FPS-es, néma MP4-animáció, a működő érintésérzékeny négygombos HTML-menü és az Android Java integrációja. **APK build és eszközteszt NEM igazolt.** A GitHub ágon a Java kód van fent; a nagyobb bináris MP4 és JPG a külön átadott ZIP-ben található.

## Jóváhagyott látvány és logó

- A felhasználó által elfogadott, eredeti GIF master: `1795091681961.gif` (553×466, 6 képkocka); csak másolatban archiváljuk. Nem módosítható vagy helyettesíthető új generált képpel.
- Az első korszak kőlogójának **teteje leomlott, egyenetlen, töredezett**, **nem szabályos kör**, nem glória és nem csillagászati bolygó. A külön jóváhagyott őskori logó master `TAKS_WORLD_LOGO_ERA_01_KO_ELFOGADOTT.png` pixelei változatlanok.
- A későbbi, elutasított kör alakú vagy önkényesen áttervezett logókat tilos ehhez az ághoz keverni.
- A mozgó animáció kizárólag vizuális, a háttér víz, növényzet, fény és por enyhe változtatásával; a természeti ok–okozat teljes natív összekötése még fejlesztendő.
- A menü `Új Játék`, `Beállítások`, `Készítő`, `Kilépés` gombokat kínálja. Készítő egyes számban.

## A szükséges Android média-assetek

A felhasználónak átadott ZIP fájlból másold a megfelelő fájlokat a tárolóba:

```
TAKS_WORLD_WONDERWERK_ELFOGADOTT_MENU/assets/wonderwerk_menu_30fps.mp4
    → app/src/main/assets/menu/wonderwerk_menu_30fps.mp4

TAKS_WORLD_WONDERWERK_ELFOGADOTT_MENU/assets/wonderwerk_menu_poster.jpg
    → app/src/main/assets/menu/wonderwerk_menu_poster.jpg
```

Enélkül az új Android-menü **nem** aktiválódik: a `AcceptedWonderwerkMenu.tryCreate()` a régi menüre vált vissza. A két bináris hiányát nem szabad sikeres játék-integrációként feltüntetni.

## Natív Android implementáció

- `AcceptedWonderwerkMenu.java`: fekvő (16:9), natív `FrameLayout` + `VideoView`, offline, némított H.264, hurokban 30 FPS, érinthető négy kőgomb; hiba esetén állóképes fallback, ha a média már csomagolt.
- `MainActivity.java`: a meglévő callbackek közvetlen bekötése: `newWorldFromMenu()`, `showSettings()`, `showCredits()`, `confirmExit()`. A Készítő minden új menüfelületen egyes szám. A mentések változatlanul működnek.
- A `AndroidManifest.xml` jelenlegi főképernyője fekvő tájolású.
- Külön letölthető `index.html` webes menüelőnézet: gombok és dialógusok funkcionálnak, de az Új Játék **nem** indítja el a natív motort weben.
- Jövőbeli munka: eső, szél, talaj és napszak a TAKS Engine *valós mentett világállapotából*, a GIF-ből utólag képzett videó helyett külön képrétegeken; valódi készüléken 30–60 FPS mérések.

## Regressziós és elfogadási feltételek

1. A jóváhagyott eredeti master képfájl hash-e nem változik.
2. A kőlogó felső pereme szabálytalan törött kő, nem kör. Nem jelenik meg körpálya/glória/bolygó.
3. A négy gomb a megfelelő funkciót nyitja, és fekvő kijelzőn pontos az érintési helye.
4. A GIF 4 FPS helyett a kimeneti MP4 30 képkocka/s, lassú mozgás; **nem** állítjuk, hogy a mozgás már fizikailag tökéletes.
5. Nincs alapból tűzhely, feltalált fogaskerék, fejlett eszköz vagy előre feloldott tudás.
6. APK-build és valódi Android-készülék ellenőrzése addig nyitott, amíg a két médiabináris bekerül a repo megfelelő útvonalára.
