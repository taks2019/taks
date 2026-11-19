# TAKS WORLD – ELFOGADOTT WONDERWERK LÁTVÁNYZÁR (2026-11-19)

**Állapot:** az elfogadott képi alap megőrzése. **Nem** elkészült, fotórealisztikus 3D játék és **nem** telepített APK.

## Jóváhagyott eredeti

- **Kép:** `Széljárta ősi szavanna a dolomitbarlangnál.png`
- **Méret:** 1536 × 1024 képpont, eredeti RGB PNG, 3 946 758 bájt.
- **SHA-256:** `980ea229cb87bda9f18891681a9227db111a01ec56ad2e5149c6426b21b63aa5`.
- **Forrás:** a felhasználó 2026. november 19-i ChatGPT-képi anyaga. A PNG változatlan forráspéldánya a külön átadott referencia ZIP-ben megvan.
- **Elsődleges szabály:** a teljes barlangszikla és a környező táj együtt maradjon; ne csonkoljuk, ne helyettesítsük általános kősziklával, ne rendezzük át az elfogadott kompozíciót.

## Mit őrzünk változatlanul?

A teljes barlangbejáratot és dolomitsziklát, az akáciás-szavannás környezetet, a száraz és füves talajmintázatot, a vízmedencét és lefolyást, a tűzrakóhelyet, az egyszerű menedéket, a kő- és faanyagokat és a lábnyomokat. Az eredeti képi részletek, színvilág, árnyékok és arányok **vizuális etalonként** maradnak meg.

A játék az elfogadott kép hangulatát és elrendezését **saját natív TAKS Engine 3D geometriával és anyagokkal** reprodukálja; nem vetítjük a képet működő 3D játékként, és nem nevezzük késznek a jelenlegi próbageometriát.

## Az élő világ kötelező ok-okozati kapcsolatai

- **Szél:** a fű, a fák ágrendszere és levelei, a füst és a vízfelület a ténylegesen szimulált szélből kapják az irányt és erősséget.
- **Eső:** nedves talaj → sár → lassulás, nyomhagyás, csúszás és erózió; a vízmedence szintje az utánpótlástól és a párolgástól függ.
- **Hőség:** melegedő levegő és napsütés → vízvesztés, szomjúság, növények stressze és tűzveszély.
- **Tűz:** tüzelőanyag és száraz gyújtós kell, az eső/szél módosítja az égést; nem korlátlan dekoráció.
- **Növények/NPC-k:** a látvány mögötti állapot és mozgás a játékmotor adatmodelljéből érkezik; a képi elemekből nem következik automatikusan valós 3D animáció.
- **Afrikai évszakosság:** száraz és csapadékos időszak, nem a modern mérsékelt övi évszakok automatikus átemelése.

## A Word/Biblia külön kezelendő anyaga

A `TW62.docx` szerint a `TER_GROUND_001` járható füves/földes terepelem képi referenciája elfogadott, 8 alapiránnyal (`000°`, `045°`, `090°`, `135°`, `180°`, `225°`, `270°`, `315°`). A dokumentált gyártási cél **8192 × 8192 RGBA PNG / nézet**, rétegek: fű → termőtalaj → altalaj → kavics → alapkőzet. A Wordbe ágyazott előnézeti képek önmagukban **nem** 8 db végleges, valódi alfaátlátszóságú master. Nem cserélik le a november 19-i jóváhagyott teljes barlangképet.

## Fejlesztési szabály

1. A `codex/taks-world-0-14-github-build` natív fejlesztőágból származó, 0.15-ös kód képezi az induló alapot. A `master` régebbi 0.8-as állapot.
2. **Most nem módosítjuk** a motor szikla-, terep- és karakterrajzoló forráskódját pusztán látványhelyettesítés céljából.
3. A jelenlegi teszt-3D és a jóváhagyott 2D látvány külön státuszban marad. A ténylegesen beépített jelenet csak összevetés és a felhasználó elfogadása után léphet elő.
4. Az eredeti `PNG` és a `TW62.docx` soha nem írható felül; a feldolgozott rétegek külön kimeneti assetek.
5. A barlang kivágása mindig a **teljes eredeti sziklatömböt** kezeli, a növények és talajrétegek külön animálhatók; a 8 nézet nem helyettesíthető egyetlen 2D kép forgatásával.
6. A forráskép ellenőrzése: `python3 scripts/verify_accepted_scene.py <eredeti.png>`. Az azonos SHA és képméret igazolja a bájthű megőrzést, **nem** a 3D játék készültségét.
