#!/usr/bin/env python3
"""Actual Android UI + GLES screenshot smoke test for approved single-layer
menu and continuously rendered isometric gameplay. On-device screenshots,
not mockups. This job does NOT approve newly imported photogrammetry assets.
"""
import json, re, subprocess, time
from pathlib import Path
from PIL import Image, ImageStat, ImageChops

OUT=Path("snapshots")
OUT.mkdir(parents=True,exist_ok=True)
PACKAGE="hu.taks.world.menufixedgame"

def adb(*args,check=True):
    run=subprocess.run(["adb",*args],capture_output=True,check=check)
    return run.stdout

def screenshot(name):
    raw=adb("exec-out","screencap","-p")
    if not raw.startswith(b"\x89PNG\r\n\x1a\n") or len(raw)<10000:
        raise AssertionError("Actual emulator screenshot missing: "+name)
    fn=OUT/(name+".png")
    fn.write_bytes(raw)
    from io import BytesIO
    with Image.open(BytesIO(raw)) as src:
        im=src.convert("RGB")
    return im

def difference(a,b):
    if a.size != b.size:
        raise AssertionError("Screen orientation changed unexpectedly")
    return float(sum(ImageStat.Stat(ImageChops.difference(a,b)).mean)/3)

def contrast(a):
    return float(sum(ImageStat.Stat(a).stddev)/3)

def log_line():
    return adb("logcat","-d","-s",
               "TAKS_MENU:I","TAKS_RENDER:E","TAKS_MODEL:E",
               "TAKS_SURFACE:E","TAKS_HUMAN:I","AndroidRuntime:E",
               check=False).decode(errors="replace")

def tap_designed(im,x,y):
    w,h=im.size
    scale=min(w/1664,h/768)
    px=round((w-1664*scale)*.5+x*scale)
    py=round((h-768*scale)*.5+y*scale)
    adb("shell","input","tap",str(px),str(py))
    return px,py

def dump():
    adb("shell","uiautomator","dump","--compressed","/sdcard/taks.xml",check=False)
    return adb("exec-out","cat","/sdcard/taks.xml",check=False).decode(errors="replace")

def fail_on_render_errors():
    logs=log_line()
    if "Fast GLES3 init:" in logs or "FATAL EXCEPTION" in logs:
        raise AssertionError("Native GL or Android runtime failure:\n"+logs[-7500:])

def run():
    adb("shell","settings","put","secure","immersive_mode_confirmations","confirmed",check=False)
    # The emulator is created fresh per GitHub job. An immediate 'pm clear'
    # can race the ActivityManager and kill the app *after* am start returns,
    # producing a false Android Home screenshot. Never clear after install.
    adb("shell","am","force-stop",PACKAGE,check=False)
    time.sleep(1)
    adb("logcat","-c",check=False)
    launched=adb("shell","am","start","-W","-n",PACKAGE+"/hu.taks.world.MainActivity",check=False).decode(errors="replace")
    print("ANDROID LAUNCH:",launched,flush=True)
    time.sleep(12)
    pid=adb("shell","pidof",PACKAGE,check=False).decode(errors="replace").strip()
    activity=adb("shell","dumpsys","activity","activities",check=False).decode(errors="replace")
    (OUT/"activity_state.txt").write_text("PID="+pid+"\n"+launched+"\n"+activity[-15000:],encoding="utf-8")
    print("ACTUAL APP PID:",pid,flush=True)
    if not pid:
        raise AssertionError("TAKS World did not remain running: ANDROID HOME IS NOT A GAME MENU. Read AndroidRuntime and crash logs.")
    initial=screenshot("01_original_approved_main_menu")
    if initial.height>initial.width:
        # The approved game is exclusively landscape; a home portrait frame
        # is not evidence that the menu or native renderer has loaded.
        raise AssertionError("Android remained in portrait: native TAKS World is not the foreground landscape activity.")
    
    print("MENU",initial.size,"contrast",contrast(initial),flush=True)
    if contrast(initial)<24:raise AssertionError("Menu blank/dark brown: original art not displayed")
    time.sleep(2)
    again=screenshot("02_same_menu_second_frame")
    # The approved background is currently deliberately STATIC until the
    # owner approves an independently checked animation.
    menuDiff=difference(initial,again)
    print("STATIC APPROVED MENU DIFFERENCE",menuDiff,flush=True)
    if menuDiff>3:
        raise AssertionError("Unexpected second/compositor image over original menu")
    fail_on_render_errors()
    tap_designed(initial,928,479) # BEÁLLÍTÁSOK
    time.sleep(2)
    settings=screenshot("03_settings_same_approved_background")
    change=difference(initial,settings)
    print("SETTINGS CHANGE",change,flush=True)
    if change<2.0:raise AssertionError("Settings did not open")
    if "3D felbontás" not in dump():
        raise AssertionError("Settings lacks existing 3D resolution menu")
    adb("shell","input","keyevent","4")
    time.sleep(2)
    home=screenshot("04_menu_after_settings")
    if difference(initial,home)>8:
        raise AssertionError("Returning from Settings does not restore the same original menu")
    tap_designed(initial,928,387) # ÚJ JÁTÉK
    time.sleep(9)
    game=screenshot("05_native_isometric_gameplay")
    print("PLAYABLE GAME",game.size,"contrast",contrast(game),
          "diff from menu",difference(initial,game),flush=True)
    if difference(initial,game)<12:
        raise AssertionError("ÚJ JÁTÉK failed: still displaying menu instead of native world")
    if contrast(game)<15:
        raise AssertionError("3D renderer blank or single-colour; do not mark graphics complete")
    ui=dump()
    if "BARLANG BELSEJE" not in ui or "ÉPÍTÉS" not in ui:
        raise AssertionError("Actual gameplay HUD missing; screen may be a modal")
    # Cave inspection changes the camera in place without rewriting world.
    w,h=game.size
    adb("shell","input","tap",str(round(w*.09)),str(round(h*.08)))
    time.sleep(4)
    cave=screenshot("06_real_cave_inspection")
    if difference(game,cave)<3:
        raise AssertionError("Cave interior button did not alter native 3D view")
    fail_on_render_errors()
    (OUT/"android_runtime_log.txt").write_text(log_line())
    (OUT/"evidence.json").write_text(json.dumps({
        "menu_size":initial.size,
        "menu_repeated_frame_mean_difference":round(menuDiff,3),
        "settings_mean_difference":round(change,3),
        "game_mean_difference_vs_menu":round(difference(initial,game),3),
        "game_contrast":round(contrast(game),3),
        "cave_mean_difference":round(difference(game,cave),3),
        "source_images":"approved original main menu, byte-identical SHA checked in CI",
        "superimage_atlas_in_apk":False,
        "superimage_import_action":"User chooses original SuperImage.zip via Settings; not proven by these screenshots",
        "approved_for_final_release":False
    },ensure_ascii=False,indent=2)+"\n")
    print("PASS: actual Android menu, Settings, gameplay and cave inspection",flush=True)

if __name__=="__main__":
    try:
        run()
    finally:
        # Always capture diagnostics, even when app crashes or opens Home.
        (OUT/"full_logcat.txt").write_text(
            adb("logcat","-d","-v","time",check=False).decode(errors="replace")[-500000:],
            encoding="utf-8"
        )
        (OUT/"targeted_crash_logcat.txt").write_text(
            adb("logcat","-d","-v","time","-s","AndroidRuntime:E","ActivityTaskManager:I",
                "ActivityManager:I","libc:F","DEBUG:E","TAKS_RENDER:E","TAKS_MODEL:E",
                "TAKS_HUMAN:I","TAKS_MENU:I",check=False).decode(errors="replace")[-300000:],
            encoding="utf-8"
        )
        (OUT/"window_focus.txt").write_text(
            adb("shell","dumpsys","window","windows",check=False).decode(errors="replace")[-20000:],
            encoding="utf-8"
        )

