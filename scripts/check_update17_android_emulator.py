#!/usr/bin/env python3
"""Android screenshot + tap regression for BOTH video menu and real gameplay.

No claim of visual quality is derived from a successful build alone.
"""
import argparse
import subprocess
from pathlib import Path
import numpy as np
from PIL import Image

def screen(path):
    return np.asarray(Image.open(path).convert("RGB"),dtype=np.int16)

def difference(a,b):
    old=screen(a);new=screen(b)
    if old.shape!=new.shape:raise AssertionError("Android changed orientation unexpectedly")
    return float(np.abs(old-new).mean())

def tap(x,y):
    subprocess.run(["adb","shell","input","tap",str(x),str(y)],check=True)

def main():
    p=argparse.ArgumentParser()
    p.add_argument("mode",choices=("menu","tap-game","game","tap-cave","inside"))
    p.add_argument("screen",type=Path)
    p.add_argument("--other",type=Path)
    args=p.parse_args()
    pic=screen(args.screen)
    h,w=pic.shape[:2]
    dev=float(np.std(np.mean(pic,axis=2)))
    print("Android",args.mode,w,h,"contrast",round(dev,2),flush=True)
    if args.mode=="menu":
        if dev<25:raise AssertionError("The approved menu remains black/brown!")
        if args.other:
            diff=difference(args.screen,args.other)
            print("Animated frames difference",round(diff,2))
            if diff<.6:raise AssertionError("MP4 menu is static or not playing")
    elif args.mode=="tap-game":
        scale=min(w/1664,h/768)
        x=round((w-1664*scale)/2+928*scale)
        y=round((h-768*scale)/2+387*scale)
        print("Tap original ÚJ JÁTÉK",x,y,flush=True)
        tap(x,y)
    elif args.mode=="game":
        if not args.other:raise AssertionError("Need menu comparison")
        diff=difference(args.screen,args.other)
        print("Menu -> actual game pixels difference:",round(diff,2))
        if diff<12:raise AssertionError("Új játék button did not open playable world")
        if dev<15:raise AssertionError("Native gameplay is black/blank")
    elif args.mode=="tap-cave":
        p=subprocess.run(["adb","shell","wm","density"],capture_output=True,
                         text=True,check=True)
        import re
        m=re.search(r"(?:Override|Physical) density:\\s*(\\d+)",p.stdout)
        density=(int(m.group(1)) if m else 420)/160
        x=round((12+125/2)*density);y=round((12+39/2)*density)
        print("Tap BARLANG BELSEJE",x,y,flush=True)
        tap(x,y)
    elif args.mode=="inside":
        if not args.other:raise AssertionError("Need outside comparison")
        diff=difference(args.screen,args.other)
        print("Exterior -> real native cave interior camera difference:",round(diff,2))
        if diff<3:raise AssertionError("Cave button did not change native view")
        if dev<11:raise AssertionError("Cave interior image is a blank rectangle")

if __name__=="__main__":
    main()
