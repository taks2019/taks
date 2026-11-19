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
PACKAGE="hu.taks.world.windseasons"
APK="android/TAKS_World_0_15_Android.apk"

def adb(*args, check=True):
    result=subprocess.run(["adb",*args],stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,check=check)
    return result.stdout

def dump(name):
    remote="/sdcard/taks-ui.xml"
    for attempt in range(4):
        result=subprocess.run(["adb","shell","uiautomator","dump",
                               "--compressed",remote],capture_output=True)
        # `adb pull` is unavailable on some Android 35 images even though the
        # dump command succeeds. Read the finished file over the adb shell pipe.
        raw=adb("exec-out","cat",remote,check=False)
        if not raw:
            raw=adb("shell","cat",remote,check=False)
        if b"<hierarchy" in raw:
            raw=raw[raw.index(b"<hierarchy"):]
            try:
                root=ET.fromstring(raw)
                return [n.attrib for n in root.iter("node")]
            except ET.ParseError:
                pass
        print("UI dump retry",name,attempt,"dump",result.returncode,
              "size",len(raw),"diagnostic",result.stderr.decode(errors="replace")[-240:],flush=True)
        time.sleep(2.0)
    picture("DIAGNOSTIC-"+name)
    raise AssertionError("Android accessibility hierarchy unavailable: "+name)

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

def position(nodes):
    for node in nodes:
        match=re.search(r"JÁTÉKOS:\s*(-?[0-9]+(?:\.[0-9]+)?) m,\s*(-?[0-9]+(?:\.[0-9]+)?) m",
                        node.get("text",""))
        if match:
            return tuple(map(float,match.groups()))
    raise AssertionError("The native status panel did not expose the live player position")

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
launch=adb("shell","am","start","-W","-n",PACKAGE+"/hu.taks.world.MainActivity",check=False)
print("Android activity launch:\n"+launch.decode(errors="replace"),flush=True)
pid=b""
for _ in range(15):
    pid=adb("shell","pidof",PACKAGE,check=False).strip()
    if pid:break
    time.sleep(1)
if not pid:
    diagnostics=[]
    for title,args in (
        ("installed package",("shell","pm","list","packages",PACKAGE)),
        ("activity state",("shell","dumpsys","activity","activities")),
        ("Android crash/runtime log",("logcat","-d","-s","AndroidRuntime:E","ActivityTaskManager:E","System.err:W","TAKS_RENDER:E","TAKS_MODEL:E")),
    ):
        result=adb(*args,check=False).decode(errors="replace")
        diagnostics.append("--- "+title+" ---\n"+result[-8000:])
    raise AssertionError("TAKS activity did not stay running after launch.\n"+"\n".join(diagnostics))
time.sleep(3)
runtime_log=b""
human_loaded=b"Loaded skinned humanoid: 53-bone rig, 7 animation clips"
for _ in range(24):
    runtime_log=adb("logcat","-d","-s","TAKS_HUMAN:I","TAKS_RENDER:E",
                    "TAKS_MODEL:E","TAKS_SURFACE:E")
    if human_loaded in runtime_log or b"Rigged skinned humanoid asset unavailable" in runtime_log:
        break
    time.sleep(2)
if human_loaded not in runtime_log:
    print("Android render-thread diagnostics:\n"+
          runtime_log.decode(errors="replace")[-8000:],flush=True)
assert human_loaded in runtime_log, \
    "The Android GPU thread did not load the rigged character asset within 48 seconds"
assert b"Rigged skinned humanoid asset unavailable" not in runtime_log, \
    "Android could not decode the skinned humanoid model"

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

click(nodes,"KÉSZÍTŐK")
time.sleep(1)
nodes=dump("credits")
assert find(nodes,"KÉSZÍTŐK")
assert find(nodes,"X"),"Credits modal lacks its working close button"
picture("02-android-keszitok")
click(nodes,"X")
time.sleep(1)
nodes=dump("main-after-credits")
click(nodes,"KILÉPÉS")
time.sleep(1)
nodes=dump("exit-confirm")
assert find(nodes,"Mentés és kilépés")
picture("03-android-kilepes-megerosites")
click(nodes,"Vissza")
time.sleep(1)
nodes=dump("main-after-exit-confirm")

click(nodes,"BEÁLLÍTÁSOK")
time.sleep(2)
nodes=dump("settings")
assert find(nodes,"X"),"Settings lacks functional X button"
assert find(nodes,"3D felbontás",True),"Graphics setting not visible"
picture("04-android-beallitasok-elo-skin")
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
picture("05-android-jatek-elomenuk")

# Drive the actual left touch pad and compare live native coordinates.
click(nodes,"TÁSKA")
time.sleep(1)
nodes=dump("movement-before")
before=position(nodes)
click(nodes,"X")
controls=dump("movement-pad")
pad=next((node for node in controls if node.get("content-desc")=="Mozgásvezérlő"), None)
assert pad, "The Android movement pad was not accessible"
left,top,right,bottom=map(int,re.findall(r"-?\d+",pad["bounds"]))
# Hold the stick at its right edge for a full stride, using its visible bounds.
x=round(left+(right-left)*.86)
y=(top+bottom)//2
adb("shell","input","swipe",str(x),str(y),str(x),str(y),"1800")
time.sleep(1)
click(dump("movement-after-touch"),"TÁSKA")
time.sleep(1)
nodes=dump("movement-verified")
after=position(nodes)
assert abs(after[0]-before[0])+abs(after[1]-before[1])>.12, \
    f"Android joystick did not move the native player: {before} -> {after}"
picture("06-android-erintesvezerles-mozgas")
click(nodes,"X")
time.sleep(1)

click(nodes,"ÉPÍTÉS")
time.sleep(1)
nodes=dump("building")
assert find(nodes,"ÉPÍTÉS ÉS ANYAGOK"),"Building submenu missing"
assert find(nodes,"⚙"),"Building submenu lacks Settings gear"
assert find(nodes,"X"),"Building submenu lacks X close"
picture("07-android-epites-almenu")

click(nodes,"⚙")
time.sleep(1)
nodes=dump("settings-from-building")
assert find(nodes,"3D felbontás",True)
picture("08-android-beallitasok-epitesbol")
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
picture("09-android-tarsak-almenu")
click(nodes,"X")
time.sleep(1)

nodes=dump("game-after-npcs")
click(nodes,"TÁSKA")
time.sleep(1)
nodes=dump("inventory")
assert find(nodes,"KÖRNYEZET ÉS ÁLLAPOT")
assert find(nodes,"⚙")
picture("10-android-taska-allapot")
click(nodes,"X")
time.sleep(1)

nodes=dump("game-after-inventory")
click(nodes,"MENÜ")
time.sleep(1)
nodes=dump("game-menu")
assert find(nodes,"JÁTÉKMENÜ")
assert find(nodes,"Beállítások")
picture("11-android-jatekmenu")
click(nodes,"Mentés")
time.sleep(1)
saved=adb("shell","run-as",PACKAGE,"cat","shared_prefs/taks_native_save_v1.xml")
assert b'name="state"' in saved and b'TAKS_NATIVE_10' in saved, \
    "The save button did not persist a native world-state snapshot"
picture("12-android-mentes-ellenorizve")
print("PASS: Android launch, 53-bone rig load, menu/navigation, touch movement, save persistence, settings and exit.",flush=True)
