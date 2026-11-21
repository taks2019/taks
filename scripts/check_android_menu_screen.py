#!/usr/bin/env python3
"""Minimal ADB screenshot quality gate for the original menu and native game.

Never treats a successfully built APK as proof the rendered game looks right.
The GitHub emulator runner executes each script line as its own shell process,
so these commands are one process per invocation, not a heredoc.
"""
import argparse
import statistics
import subprocess
from pathlib import Path
from PIL import Image


def diagnostics(path):
    image=Image.open(path).convert("RGB")
    values=[sum(rgb)/3.0 for rgb in image.resize((96,54)).getdata()]
    return image, statistics.pstdev(values), sum(v>170 for v in values)/len(values)


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("command",choices=("check-menu","tap-new-game","check-game"))
    parser.add_argument("image",type=Path)
    parser.add_argument("--compare",type=Path,default=None)
    args=parser.parse_args()
    image,contrast,bright=diagnostics(args.image)
    print(args.command,"size",image.size,"contrast",round(contrast,2),
          "sunlit_fraction",round(bright,3),flush=True)
    if args.command=="check-menu":
        if contrast<=24 or bright<=.04:
            raise SystemExit("FAIL: the original menu is not visibly rendered!")
    elif args.command=="tap-new-game":
        width,height=image.size
        scale=min(width/1664,height/768)
        x=round((width-1664*scale)/2+928*scale)
        y=round((height-768*scale)/2+387*scale)
        print("Tap original ÚJ JÁTÉK at",x,y,flush=True)
        subprocess.run(["adb","shell","input","tap",str(x),str(y)],check=True)
    elif args.command=="check-game":
        if not args.compare:
            raise SystemExit("--compare menu.png is required")
        original=Image.open(args.compare).convert("RGB").resize((96,54))
        now=image.resize((96,54))
        diff=sum(abs(v-u) for a,b in zip(now.getdata(),original.getdata())
                 for v,u in zip(a,b))/(96*54*3)
        print("menu_to_game_changed_mean",round(diff,2),flush=True)
        if diff<8:
            raise SystemExit("FAIL: tapping Új játék did not open native gameplay")
        if contrast<=13:
            raise SystemExit("FAIL: gameplay is visually blank or a flat dark field")


if __name__=="__main__":
    main()
