# TAKS World — natív Android motor

Ez a saját TAKS Engine C++17 / Android Java / OpenGL ES 3 alapjának fejlesztői tárolója. Nem a régi HTML/Canvas projekt, és még nem a kész fotórealisztikus TAKS World.

A GitHub Actions a `.github/workflows/android-apk.yml` fájllal először a közös ok-okozati motort teszteli, majd Android debug APK-t épít. Az eredmény az Actions futás Artifacts részében tölthető le.

A jelenlegi műszaki lépcső az építési lánc és állapotmegőrzés alapja: nyersanyagok, készlet, tűzhely, tűz, nedvesség, idő és mentés. Következő lépcsők: szerkesztő, alkatrészszintű építés, karakterek, környezet, 3D eszközök és világfizika.
