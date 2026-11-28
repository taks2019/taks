# TAKS World Android — vizuális jóváhagyás

A tulajdonos 2026. november 28-i, kifejezett döntése. A Biblia az elsődleges követelményforrás. Ez a kiegészítés nem írja felül a korábban elfogadott vizuális elemeket.

1. Minden új kép, 3D látvány vagy vizuális séma először külön jelölt; a játékba végleges elemként csak a tulajdonos egyértelmű jóváhagyása után kerülhet.
2. Az elfogadás mindig a bemutatott konkrét változatra vonatkozik. Az eredeti fájlt változatlanul kell megőrizni, azonosítóval, verzióval és SHA-256 ellenőrzőösszeggel. Ha az eredeti nem áll rendelkezésre, meg kell állni az adott elem beépítésével; nem helyettesíthető újragenerált vagy hasonló képpel.
3. Elutasított vagy még nem jóváhagyott változat nem kerülhet be végleges játékelemként. A hallgatás nem jóváhagyás.
4. Textúrakonverzió, GLB-rekonstrukció, animáció, mobilos optimalizálás vagy bármely látványt érintő változtatás külön származtatott fájl legyen. Az eredeti megmarad. A módosult látvány és a játékbeli eredmény külön jóváhagyást igényel. A képből származtatott modell nem azonos az eredeti képpel.
5. A nyilvántartásban külön státusz: JELÖLT, ELUTASÍTOTT, KÉP_ELFOGADVA, MODELL_ELFOGADVA, JÁTÉKBAN_ELFOGADVA. A jóváhagyás szakaszai nem helyettesítik egymást. Minden döntéshez az érintett változat azonosítóját és a tulajdonosi döntést kell rögzíteni.
6. Meglévő, jóváhagyott elem felülírása vagy lecserélése kizárólag új, kifejezett tulajdonosi jóváhagyással történhet. Technikai akadály esetén azt jelezni kell, nem önállóan eltérni a jóváhagyott látványtól.

Ez jelenleg dokumentált fejlesztési szabály, nem automatikus build-ellenőrzés. Az automatikus ellenőrzés és a meglévő assetek tételes jóváhagyási nyilvántartása külön implementációt és ellenőrzést igényel. Ez a dokumentum önmagában egyetlen assetet sem minősít jóváhagyottnak.
