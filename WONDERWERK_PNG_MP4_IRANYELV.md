# TAKS World – Wonderwerk vizuális irányelv (2026-11-21)

**Elfogadott irány:** a korábban megmutatott, részletgazdag Wonderwerk-barlang látványminősége az egész játéktér etalonja. Ez stílus- és minőségi jóváhagyás, **nem** automatikus jóváhagyás minden új állományra és nem a régészeti pontosság igazolása.

## Változatlan
- Kezdőmenü felépítése, gombjai, logója, arányai és a magyar szöveg.
- A játékon belüli képernyőlogó ne térjen vissza.
- TAKS Engine: saját C++/GLES3 rendszer, izometrikus vetítés, natív mentés és ok-okozat.
- Wonderwerk helyszínét nem szabad kitalált barlangként, hiteles mérésnek beállítva megjeleníteni.

## Aktuális technikai munkairány
1. **Főmenü:** saját, valóban mozgó MP4 animáció a korábbi Wonderwerk-háttérből; növénylengés, por, fény. A meglévő teljes kép a fallback, a gombok ugyanabban a natív rétegben maradnak. MP4 hangtalan és a menüből kilépve leáll.
2. **Játéktér:** PNG RGBA alakzatok, külön vizuális és talajkontakt-rétegek, világkoordinátára rögzített quads; nem a kijelző fölé helyezett UI-billboardok. Az eredeti nyolc barlangnézet **teszt**. A takarás a mélységi puffer szerint történik, a fotórealitás nem a fizika helyettesítője.
3. **Belső/ külső:** barlangfal, bejárat, tető, padló és belső tér külön. A tető levétele zoomnál azonos, bejárható világállapotot fedjen fel.
4. **Az első bővítendő grafikai család:** talaj és sár, dolomit és kőtörmelék, szélben hajló fű, helyszínre illő növények; utána emberek és állatok (egyedi átlátszó PNG-ciklusok). Minden képet a világ anyagprofiljához és ütközési kontúrjához kell kötni.
5. **Animációk:** legalább nyugalom és mozgás (állás, járás, futás); állati és növényi állapotok a tényleges szél, eső, nedvesség és idő függvényei. A 8 forgási nézet nem önmagában szabadon forgatható 3D.
6. **Teljesítmény:** környezeti textúrák atlaszokban, megfelelő mipmapping és szigorú aktív textúralimit; Samsung Galaxy A56 és Honor Pad X8 céleszközön mérendő FPS, memória és akkumulátor.
7. **Jóváhagyás:** egy új PNG, atlasz vagy videó első körben csak felülvizsgálati asset; még nem véglegesítés. Az `assets/visual_approvals.json` külön azonosítóval, ellenőrizhető forrással követi az egyedi képeket.

## Kifejezett nem-cél
- Több másolt, egymás mellett lebegő barlang.
- A játékmenet helyére tett videó és a képbe égetett menügombok.
- Sztenderd low-poly vagy stílusban elütő figurák.
- A meglévő menü átszabása, illetve a korábbi mentések felülírása teszthez.

**Állapot:** a mozgó főmenü első műszaki implementációja kész forrásként; a teljes egységes, fotórealisztikus PNG világ még NEM készült el. A meglévő barlang a fejlesztői ágon 2,5D látványpróba.
