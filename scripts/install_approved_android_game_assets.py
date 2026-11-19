#!/usr/bin/env python3
"""Install only the user-approved Wonderwerk art for a REAL APK build.

Input: repository-root TAKS_WORLD_GITHUB_ANDROID_JATEK_ASSETS.zip
Created from the approved, immutable ancient stone logo + newest wide
landscape video background and the actual 16 transparent PNG actor views.
No images are generated, distorted, repainted or replaced by placeholders.
"""
from pathlib import Path
from hashlib import sha256
from zipfile import ZipFile
from io import BytesIO
import json

ROOT=Path(__file__).resolve().parents[1]
PACKAGE=ROOT/"TAKS_WORLD_GITHUB_ANDROID_JATEK_ASSETS.zip"
DEST=ROOT/"app/src/main/assets"
EXPECTED_ARCHIVE="3ee394b05cc3c0125823198f72951175246de1cf25f3049717cb1a27a635535a"
EXPECTED_MASTER="575c8b67fe3cc5fe2fbb97e4a08bdeebca3c22a3efc1adc61624523f61bcae9f"
EXPECTED_WIDE="9369d3668b69424d31132e70accbfdb76282d7d9b3f3adfaa51c7c3ca1ea7bc8"

def fail(message):
    raise SystemExit("TAKS World approved assets: "+message)

if not PACKAGE.is_file():
    fail("A jóváhagyott grafikai ZIP még nincs feltöltve a GitHub-ág gyökerébe: "
         +PACKAGE.name+". Emiatt nincs APK, és nem használunk pótlékképet.")
if sha256(PACKAGE.read_bytes()).hexdigest()!=EXPECTED_ARCHIVE:
    fail("Az eredeti, jóváhagyott grafikai csomag SHA256-értéke nem egyezik!")
try:
    from PIL import Image
except ImportError:
    fail("Pillow szükséges az élesség, az alfa és a nyolc nézet ellenőrzéséhez.")

with ZipFile(PACKAGE) as archive:
    manifest=json.loads(archive.read("MANIFEST.json"))
    expected=manifest["files_sha256"]
    files=set(archive.namelist())
    for name in expected:
        if name not in files or sha256(archive.read(name)).hexdigest()!=expected[name]:
            fail("Hiányzó vagy módosított PNG: "+name)
    if expected.get("approved/menu/wonderwerk_menu_MASTER_UNCHANGED.png")!=EXPECTED_MASTER:
        fail("Az eredeti szent logóval ellátott mestermű megváltozott!")
    if expected.get("approved/menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png")!=EXPECTED_WIDE:
        fail("A szélesvásznú, elfogadott háttér megváltozott!")
    directions=("N","NE","E","SE","S","SW","W","NW")
    character_files=[]
    for pose in ("idle","crouch"):
        for direction in directions:
            suffix="iso" if pose=="idle" else "p00"
            character_files.append(f"approved/characters/actor_m01_{pose}_{direction}_{suffix}.png")
    for name in character_files:
        with Image.open(BytesIO(archive.read(name))) as img:
            if img.mode!="RGBA" or img.size!=(512,512):
                fail("A karakter nem 512x512-es, valódi RGBA-kép: "+name)
            low, high=img.getchannel("A").getextrema()
            if low!=0 or high<220:
                fail("Hamis vagy hiányzó alfa a karakternél: "+name)
    with Image.open(BytesIO(archive.read("approved/menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png"))) as main:
        if main.size!=(1664,768):fail("A jóváhagyott menü nem 1664x768-as!")
    for key in ("WATER_MASK","FERN_MASK","GRASS_MASK","CLOUD_MASK"):
        with Image.open(BytesIO(archive.read(f"approved/menu/{key}.png"))) as mask:
            if mask.size!=(1664,768):
                fail("Maszkfelbontási hiba: "+key)
    # Copy exactly the approved game inputs. Master/MP4 are verification copies,
    # not live UI artwork; titles remain editable text in menu_texts.json.
    required=character_files+[f"approved/menu/{key}.png" for key in
             ("WONDERWERK_APPROVED_WIDE_NO_TEXT","WATER_MASK",
              "FERN_MASK","GRASS_MASK","CLOUD_MASK")]
    for name in required:
        path=DEST/name.removeprefix("approved/")
        path.parent.mkdir(parents=True,exist_ok=True)
        path.write_bytes(archive.read(name))
        print("ORIGINAL BYTES INSTALLED",path.relative_to(ROOT))
print("PASS: 16 native actor PNGs + 5 full-screen causal scene layers installed.")
print("The sacred logo, the original poster and the human-editable text script are untouched.")
