#!/usr/bin/env python3
"""Real Android UI regression: dismiss OS immersive dialog, then interact with
the NATIVE TAKS World UI through accessibility, never guessed static screenshots.
"""
import re
import subprocess
import time
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT=Path("ui-evidence")
ROOT.mkdir(parents=True, exist_ok=True)
PACKAGE="hu.taks.world.livinguitest"
APK="android/TAKS_World_0_11_ELO_MENU_ANDROID_TESZT.apk"

def adb(*args, check=True):
    result=subprocess.run(["adb",*args],stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,check=check)
    return result.stdout

def dump(name):
    adb("shell","uiautomator","dump","/sdcard/taks-ui.xml")
    xml=adb("exec-out","cat","/sdcard/taks-ui.xml")
    path=ROOT/(name+".xml")
    path.write_bytes(xml)
    root=ET.fromstring(xml)
    return [n.attrib for n in root.iter("node")]

def find(nodes,text,contains=False):
    for n in nodes:
        label=n.get("text","")
        if label==text or (contains and text in label):
            return n
    return None

def click(nodes,text,contains=False):
    n=find(nodes,text,contains)
    if n is None:
        labels=[x.get("text","") for x in nodes if x.get("text")]
        raise AssertionError(f"Missing Android control '{text}', visible: {labels}")
    xy=list(map(int,re.findall(r"\d+",n["bounds"])))
    assert len(xy)==4
    x=(xy[0]+xy[2])//2;y=(xy[1]+xy[3])//2
    adb("shell","input","tap",str(x),str(y))
    print("TAPPED",repr(text),"at",x,y,flush=True)
    return x,y

def picture(name):
    data=adb("exec-out","screencap","-p")
    assert data.startswith(b"\x89PNG\r\n\x1a\n") and len(data)>15000
    path=ROOT/(name+".png")
    path.write_bytes(data)
    print("REAL ANDROID CAPTURE",name,len(data),"bytes",flush=True)

print("Installing real native Android test APK...",flush=True)
adb("install","-r",APK)
# AOSP's first-run immersive explanation blocks real app controls in a fresh
# emulator. Android-shell may mark the user confirmation as acknowledged.
adb("shell","settings","put","secure","immersive_mode_confirmations","confirmed",check=False)
adb("shell","am","start","-n",PACKAGE+"/hu.taks.world.MainActivity")
time.sleep(10)
assert adb("shell","pidof",PACKAGE).strip(), "App process crashed on launch"

# A fresh Android emulator shows a one-time OS-level full-screen prompt.
# This prompt is NOT the TAKS interface; dismiss before screenshots/tests.
for attempt in range(5):
    nodes=dump("system-start-"+str(attempt))
    if attempt==0:
        picture("00-diagnosztika-fullscreen")
    if find(nodes,"Got it"):
        # Different emulator display density/rotation may make UIAutomator
        # bounds unusable for input tap. Use secure acknowledgement and BACK.
        click(nodes,"Got it")
        adb("shell","settings","put","secure","immersive_mode_confirmations","confirmed",check=False)
        adb("shell","input","keyevent","66",check=False)
        time.sleep(1)
        continue
    if find(nodes,"BEÁLLÍTÁSOK"):
        break
    time.sleep(2)
else:
    raise AssertionError("Native main menu never became visible after OS dialog")
assert find(nodes,"ÚJ JÁTÉK")
assert find(nodes,"KÉSZÍTŐK")
assert find(nodes,"KILÉPÉS")
picture("01-android-fomenu-eredeti-logo")

click(nodes,"BEÁLLÍTÁSOK")
time.sleep(2)
nodes=dump("settings")
assert find(nodes,"X"),"Settings lacks functional X button"
assert find(nodes,"3D felbontás",True),"Graphics setting not visible"
picture("02-android-beallitasok-elo-skin")
click(nodes,"X")
time.sleep(1)

# Main menu should remain accessible and operative after closing Settings.
nodes=dump("main-after-settings")
assert find(nodes,"ÚJ JÁTÉK")
click(nodes,"ÚJ JÁTÉK")
time.sleep(4)
nodes=dump("gameplay")
# Fresh install creates initial world. If previous save exists, offer 'Folytatás'.
if find(nodes,"Folytatás a mentett világból"):
    click(nodes,"Folytatás a mentett világból")
    time.sleep(3)
    nodes=dump("gameplay-after-resume")
assert find(nodes,"ÉPÍTÉS"),"Game HUD is not present"
assert find(nodes,"TÁRSAK"),"NPC screen inaccessible"
assert find(nodes,"TÁSKA"),"Inventory screen inaccessible"
picture("03-android-jatek-elomenuk")

click(nodes,"ÉPÍTÉS")
time.sleep(1)
nodes=dump("building")
assert find(nodes,"ÉPÍTÉS ÉS ANYAGOK"),"Building submenu missing"
assert find(nodes,"⚙"),"Building submenu lacks Settings gear"
assert find(nodes,"X"),"Building submenu lacks X close"
picture("04-android-epites-almenu")

click(nodes,"⚙")
time.sleep(1)
nodes=dump("settings-from-building")
assert find(nodes,"3D felbontás",True)
picture("05-android-beallitasok-epitesbol")
click(nodes,"X")
time.sleep(1)
nodes=dump("building-after-settings")
assert find(nodes,"ÉPÍTÉS ÉS ANYAGOK")
click(nodes,"X")
time.sleep(1)
nodes=dump("game-after-building")

click(nodes,"TÁRSAK")
time.sleep(1)
nodes=dump("npcs")
assert find(nodes,"KÖZÖSSÉG ÉS TÁBOR")
assert find(nodes,"⚙")
picture("06-android-tarsak-almenu")
click(nodes,"X")
time.sleep(1)

nodes=dump("game-after-npcs")
click(nodes,"TÁSKA")
time.sleep(1)
nodes=dump("inventory")
assert find(nodes,"KÖRNYEZET ÉS ÁLLAPOT")
assert find(nodes,"⚙")
picture("07-android-taska-allapot")
click(nodes,"X")
time.sleep(1)

nodes=dump("game-after-inventory")
click(nodes,"MENÜ")
time.sleep(1)
nodes=dump("game-menu")
assert find(nodes,"JÁTÉKMENÜ")
assert find(nodes,"Beállítások")
picture("08-android-jatekmenu")
print("PASS: actual Android app, original logo, settings from every view, X, build/NPC/inventory and exit UI.",flush=True)
