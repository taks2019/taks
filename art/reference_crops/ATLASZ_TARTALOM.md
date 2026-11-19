# TAKS World – forrásképből kivágott PNG-k

Minden asset az általad feltöltött izometrikus világkép eredeti képpontjainak téglalap alakú kivágása. Nincs hozzáadott vagy generált csempe, nincs színre festés vagy perspektívajavítás. A barlang teljes külső képrészlete egyetlen képként marad meg.

Az eredeti, teljes forráskép is benne van `Wonderwerk_etalon.jpg` néven.

A fű, a fák és a víz jelenlegi képkivágása egy nézőpontot tartalmaz. A mozgás nem a PNG-ben van: a játék 3D megjelenítőjében a szél és az évszak állapotához lesz kötve. Egyetlen állóképből valódi nyolc irányú nézet nem vágható ki; ahhoz 3D modell vagy több valós nézet szükséges.

## Barlangbelső — természetes, érintetlen állapot

A barlangbelső tervén a természetes dolomitfal, mennyezet, járat és a feltárás előtti talaj szerepeljen. Ne jelenjen meg ásatási négyzetrács, kutatóárok, kiemelt lelet vagy mai jelölés. A járat formáját a teljes barlang Zamani-felmérési geometriájához kell igazítani; a feltárás nyomait a felszínről el kell hagyni. A projekt jelenlegi fájljai nem tartalmazzák a teljes belső felmérési hálót, ezért ebből a külső képből nem szabad belső alaprajzot kitalálni.

## Kivágások

- **01_Wonderwerk_Teljes_Kulso_Barlang.png** — A teljes, képen látható barlangbejárat és dolomitszikla egyben. Méret: 1006×690 px; forráskoordináta: `[530, 0, 1536, 690]`.
- **02_Fa_fo_izometrikus_kivagas.png** — A bal oldali nagy fa a környező talajjal; eredeti képpontok. Méret: 575×540 px; forráskoordináta: `[0, 125, 575, 665]`.
- **03_Fa_hattersav_izometrikus_kivagas.png** — Háttérben álló kisebb fa és a hozzá tartozó eredeti tereprészlet. Méret: 485×370 px; forráskoordináta: `[360, 0, 845, 370]`.
- **04_Fa_jobb_izometrikus_kivagas.png** — Jobb alsó fa és környező talaj, az eredeti képből. Méret: 456×502 px; forráskoordináta: `[1080, 650, 1536, 1152]`.
- **05_Fu_es_novenyzet_talajfolt.png** — Fűcsomók és alacsony növényzet valódi képkivágása. Méret: 460×430 px; forráskoordináta: `[390, 690, 850, 1120]`.
- **06_Vizmedence_es_lefolyas.png** — A medence, part és lefolyás egyetlen kivágásban. Méret: 555×522 px; forráskoordináta: `[0, 630, 555, 1152]`.
- **07_Menedek_es_tuzhely.png** — A menedék és tűzhely együtt, eredeti kompozíció szerint. Méret: 660×490 px; forráskoordináta: `[505, 300, 1165, 790]`.
- **08_Faag_keszlet.png** — A tábor melletti faágkészlet eredeti kivágása. Méret: 446×370 px; forráskoordináta: `[1090, 435, 1536, 805]`.
- **09_Kokupac_keszlet.png** — A kőkupac eredeti kivágása. Méret: 375×395 px; forráskoordináta: `[835, 635, 1210, 1030]`.
- **10_Taposott_nedves_talaj_labnyomokkal.png** — Taposott talaj, lábnyomok és nedvesedés az eredeti képből. Méret: 465×452 px; forráskoordináta: `[500, 700, 965, 1152]`.

A tíz kivágás előállítása a projekt gyökeréből: `python3 scripts/export_reference_crops.py`.
