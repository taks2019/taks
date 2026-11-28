# Android – univerzális ok-okozati gyártás, Cessna 182 teszt

Tulajdonosi döntés, 2026-11-28: az alfa hátterű törzs látványa elfogadva, teszttervként. Nem végleges játékbeli modell és nem hiteles repülőgép-mérnöki terv.

- Referenciaazonosító: TEST-C182-FUSELAGE-PNG-v002
- Eredeti fájl: a_highly_detailed_photorealistic_3d_render_of_the.png
- SHA-256: ae8dbedf2c943189dc0d074b3f6c8316a088c09022789977e587c8e1d89f4fa4
- Ellenőrzött formátum: PNG, RGBA, 1774 × 887, alfa 0–255.
- Státusz: KÉP_ELFOGADVA; GLB, illesztés, gyártás, játékbeli működés még nem igazolt. A dokumentum a fájl azonosítóját rögzíti, a bináris kép nem része ennek a commitnak.

## Közös gyártási elv (tulajdonosi követelmény)
A gyár, gép, szerszám, anyag, alkatrész, munkaerő, tudás, energia, idő és szállítás egyetlen oksági rendszerben kapcsolódik össze. A gyárat is tényleges alapanyagból és munkából kell létrehozni. Minden késztermék alkatrészekből, minden alkatrész az előző folyamatok termékeiből készül; nincs késztermék feltétel nélküli létrehozása. A párhuzamos munka a rendelkezésre álló gépek, munkások, munkahelyek és anyagok függvénye, nem egyszerű automatikus szorzó.

## Javasolt hét tesztfázis (játékmechanika, nem valós Cessna-gyártási előírás)
1. Tervrajz, technológia, követelmények, beszállítói lánc.
2. Műhely/gyár és géppark építése, telepítése, energiaellátás.
3. Anyagbeszerzés, feldolgozás, félkész termékek.
4. Alkatrészek előállítása, ellenőrzése, készletezése.
5. Részegységek szerelése, illesztési pontok és műveletenkénti munka.
6. Végszerelés, rendszerek és vizuális készültség.
7. Játékbeli működés- és minőségvizsgálat; csak ezután használható kész jármű.

A Studio javasolt közös adatmodellje: Blueprint, Material, Part, Operation, Machine, Facility, WorkerSkill, Inventory, Energy, WorkOrder, ConnectionPoint, Animation, QualityCheck. Művelet: előfeltételek, bemenet, kimenet, idő, munkás, gép, energia, melléktermék, kopás, hibafeltétel, vizuális/animációs állapot. A terv általános, házra, gyárra, rollerre, buszra, hajóra, repülőre és őskori műveletekre is alkalmazandó. A csontozott ember mozgása és a tárgy megfogása a szimulált művelet tényleges állapotához kötendő.

A hét fázis és adatmodell javaslat, nem lezárt műszaki specifikáció. A teljes Biblia ellenőrzése és a Studio/Engine implementációja, eszköztesztje hátravan.
