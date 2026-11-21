#!/usr/bin/env python3
"""Real Android smoke test: four unchanged stone buttons + menu animation +
native playability. Run INSIDE the Android emulator CI after APK install.

Static source checks do not prove touch correctness. This script proves the
actual Android display and actual touch listeners, without changing the menu.
"""
import re
import subprocess
import time
from pathlib import Path
import numpy as np
from PIL import Image

OUTPUT=Path("snapshots")
OUTPUT.mkdir(exist_ok=True)

def adb(*args,check=True):
    r=subprocess.run(["adb",*map(str,args)],capture_output=True,check=check)
    return r.stdout

def snapshot(name):
    file=OUTPUT/(name+".png")
    raw=adb("exec-out","screencap","-p")
    if not raw.startswith(b"\x89PNG\r\n\x1a\n"):
        raise AssertionError("Device did not produce actual screenshot")
    file.write_bytes(raw)
    image=np.asarray(Image.open(file).convert("RGB"),dtype=np.int16)
    print("SCREENSHOT",file,image.shape,flush=True)
    return image

def delta(a,b):
    if a.shape!=b.shape:
        raise AssertionError("Android changed orientation while running")
    return float(np.abs(a-b).mean())

def menu_tap(label,image,y_design):
    h,w=image.shape[:2]
    # Drawn menu is centered FIT within the actual landscape viewport.
    x=round(w*.58)
    y=round(h*y_design/768.)
    print("Touch real Android stone",label,x,y,flush=True)
    adb("shell","input","tap",x,y)
    time.sleep(2.7)
    log=adb("logcat","-d","-s","TAKS_MENU_TOUCH:I").decode(errors="replace")
    (OUTPUT/("touch_"+str(y_design)+".txt")).write_text(log,encoding="utf-8")
    if label not in log:
        raise AssertionError("STONE BUTTON DID NOT FIRE actual native Android handler: "+label+
                             "\n"+log[-4000:])

def menu_back():
    adb("shell","input","keyevent",4)
    time.sleep(1.6)

def main():
    adb("shell","settings","put","secure","immersive_mode_confirmations","confirmed",check=False)
    adb("shell","am","force-stop","hu.taks.world.menufixedgame",check=False)
    adb("shell","am","start","-n",
        "hu.taks.world.menufixedgame/hu.taks.world.MainActivity")
    time.sleep(20)
    first=snapshot("00_menu_wind_1")
    time.sleep(1.75)
    next_frame=snapshot("01_menu_wind_2")
    menu_difference=delta(first,next_frame)
    print("MENU REAL ANIMATION DIFF",menu_difference,flush=True)
    if menu_difference<.35:
        raise AssertionError("Static MP4 or Android hardware decode unavailable")
    # The middle stone emblem/labels must NOT ghost while natural edges move.
    h,w=first.shape[:2]
    central=delta(first[int(h*.10):int(h*.88),int(w*.44):int(w*.58)],
                  next_frame[int(h*.10):int(h*.88),int(w*.44):int(w*.58)])
    print("CENTRAL FROZEN LOGO TIME DIFFERENCE",central,flush=True)
    if central>3.0:
        raise AssertionError("Stone logo moved or was double-rendered")

    for label,y in [("BEÁLLÍTÁSOK",479),("KÉSZÍTŐ",573),("KILÉPÉS",668)]:
        menu_tap(label,first,y)
        modal=snapshot("02_modal_"+str(y))
        if delta(first,modal)<2.5:
            raise AssertionError(label+" did not open its real Android modal")
        menu_back()
        again=snapshot("03_back_"+str(y))
        if delta(first,again)>35:
            raise AssertionError("Back from "+label+" did not restore original menu")

    menu_tap("ÚJ JÁTÉK",first,387)
    time.sleep(8)
    game=snapshot("04_actual_native_game")
    if delta(first,game)<14:
        raise AssertionError("ÚJ JÁTÉK did not navigate to actual game")
    if float(np.std(game.mean(axis=2)))<16:
        raise AssertionError("Game is still blank / flat colour")
    adb("shell","input","tap",round(w*.12),round(h*.085))
    time.sleep(4)
    inside=snapshot("05_native_cave_inspection")
    if delta(game,inside)<3:
        raise AssertionError("BARLANG BELSEJE navigation did not change view")
    logs=adb("logcat","-d","-v","time").decode(errors="replace")
    (OUTPUT/"android_full_log.txt").write_text(logs,encoding="utf-8")
    print("PASSED actual 4 stone buttons, moving video, game and cave inside",flush=True)

if __name__=="__main__":
    main()
