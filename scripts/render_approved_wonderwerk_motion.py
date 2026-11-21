#!/usr/bin/env python3
"""Render real per-frame nature animation FROM THE UNCHANGED APPROVED MASTER.

The existing Wonderwerk image supplies all scenery, logo and stone buttons.
Only original photographed vegetation and water regions are spatially warped;
stones and the central TAKS logo are never translated, resized or redrawn.

The resulting offline MP4 runs in the *actual native Android main menu*;
interactive controls are independent Android Views drawn on top.
"""
from __future__ import annotations
import argparse
import math
import subprocess
from pathlib import Path
import cv2
import numpy as np

MASTER = Path("app/src/main/assets/menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png")
DEST = Path("app/src/main/assets/menu/wonderwerk_loop.mp4")


def soft_rectangle(xx, yy, x0, y0, x1, y1, softness=0.027):
    def smooth(z):
        z = np.clip(z, 0., 1.)
        return z * z * (3. - 2. * z)
    return smooth((xx - x0) / softness) * smooth((x1 - xx) / softness) * (
        smooth((yy - y0) / softness) * smooth((y1 - yy) / softness))


def prepare(image):
    h, w, _ = image.shape
    y, x = np.mgrid[:h, :w].astype(np.float32)
    xx, yy = x / w, y / h
    hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)
    hue = hsv[:, :, 0].astype(np.float32)
    sat = hsv[:, :, 1].astype(np.float32)
    blue, green, red = cv2.split(image.astype(np.float32))
    # Look for leaf-coloured source pixels, not skies, rock or the oak trunk.
    greenish = np.clip((green - blue * .75 - 3.) / 48., 0., 1.)
    colourful = np.clip((sat - 48.) / 115., 0., 1.)
    botanical = np.clip(greenish * colourful, 0., 1.)
    tree = (botanical * soft_rectangle(xx, yy, -.025, -.025, .405, .375)
            * np.clip((.48 - yy) / .22, 0., 1.))
    fern = (botanical * soft_rectangle(xx, yy, .674, .488, 1.025, 1.025)
            * np.clip((yy - .435) / .13, 0., 1.))
    grass = botanical * soft_rectangle(xx, yy, .0, .48, .46, 1.02) * .52
    # Water area on the LOWER LEFT. Keep river rocks and dry land fixed.
    watery = (soft_rectangle(xx, yy, .03, .66, .438, 1.025, .028)
              * np.clip((blue * 1.3 + green - red * 1.1 - 55.) / 170., 0., 1.))
    # Central stone logo and four real buttons are absolutely still.
    protected = soft_rectangle(xx, yy, .384, .005, .667, 1.03, .014)
    protected = np.maximum(protected, soft_rectangle(xx, yy, .398, .0, .623, .45, .008))
    safe = 1.0 - protected
    tree *= safe; fern *= safe; grass *= safe; watery *= safe
    # Clean up tiny isolated pixels; keep leaf boundaries feathered.
    kernel = np.ones((3, 3), np.uint8)
    masks = []
    for mask in (tree, fern, grass, watery):
        mask = cv2.GaussianBlur(mask.astype(np.float32), (0, 0), 1.1)
        masks.append(np.clip(mask, 0., .98))
    return x, y, xx, yy, masks


def render_frame(source, geometry, k, total):
    x, y, xx, yy, masks = geometry
    tree, fern, grass, water = masks
    h, w = x.shape
    phase = np.float32(2.0 * math.pi * (k / total))
    # Different frequencies, but every movement completes full cycles by the
    # last frame. Tree trunks and fern roots stay anchored to their substrate.
    tree_bend = (np.sin(phase + yy * 7.0) * 6.1
                 + np.sin(2*phase + x * .021) * 2.2)
    fern_bend = (np.sin(phase + yy * 9.0 + .8) * 10.8
                 + np.sin(2*phase + x * .026) * 3.1)
    grass_bend = np.sin(phase + xx * 11.0 + yy * 3.8) * 4.3
    # Water flows visibly: moving highlights and ripples from the existing
    # original photograph, not a fixed still encoded into an MP4 container.
    ripple = (np.sin(2*phase + x * .035 + yy * 4.5) * 4.3
              + np.sin(3*phase + y * .10) * 1.7)
    offset_x = (tree_bend * tree + fern_bend * fern +
                grass_bend * grass + ripple * water).astype(np.float32)
    offset_y = ((np.sin(phase + yy*11)*1.45)*fern
                + np.cos(2*phase+x*.055)*1.5*water).astype(np.float32)
    moved = cv2.remap(source,
                      x - offset_x, y - offset_y,
                      cv2.INTER_LINEAR, borderMode=cv2.BORDER_REPLICATE)
    # Transparency is not a whole-screen moving plate: only the named plants
    # and water have moving source pixels; rocks, plaques and geology stay put.
    motion = np.clip(tree + fern + grass + water, 0., 1.)
    frame = (source.astype(np.float32) * (1 - motion[:, :, None])
             + moved.astype(np.float32) * motion[:, :, None])

    # Sunbeams shimmer around the APPROVED real sun; no synthetic second sun.
    beam = (np.exp(-((xx-.275)**2/.022+(yy-.245)**2/.037))
            * np.sin(phase + xx * 12.) * .075)
    protection = 1. - soft_rectangle(xx, yy, .378, .015, .68, .98, .008)
    beam *= protection
    frame += (beam[:, :, None] * np.array([19., 31., 48.], np.float32))
    # Water glints change position in every frame while stones stay fixed.
    surface = (np.sin(phase*3 + x*.041 + y*.077) * .50 +
               np.sin(phase*2 + x*.084 - y*.040)*.50)
    highlight = (np.maximum(0., surface)**3 * water * 31.)[:, :, None]
    frame += highlight * np.array([1.15, 1.08, .86], np.float32)

    # Visible dust moves IN the entrance's actual existing light shaft.
    # Loopable parametric paths, independent of frame-time accumulation.
    for i in range(58):
        a=i*2.39996323
        px=int(w*(.665 + (i % 11) * .014)
               + math.sin(float(phase)*2+a)*w*.007)
        py=int(h*(.43 + (i % 9)*.018)
               + math.cos(float(phase)+a)*h*.013)
        if 0<=px<w and 0<=py<h:
            radius=1 if i%4 else 2
            strength=round(42 + (i%5)*17)
            cv2.circle(frame,(px,py),radius,
                       (min(255,frame[py,px,0]+strength*.72),
                        min(255,frame[py,px,1]+strength*.90),
                        min(255,frame[py,px,2]+strength)),
                       -1,lineType=cv2.LINE_AA)
    return np.clip(frame,0,255).astype(np.uint8)


def generate(master: Path, dest: Path, width: int, fps: int, seconds: int):
    source = cv2.imread(str(master), cv2.IMREAD_COLOR)
    if source is None:
        raise FileNotFoundError("Approved image missing: " + str(master))
    if source.shape[:2] != (768,1664):
        raise ValueError(f"Approved scene changed dimensions: {source.shape}")
    height = 2*round(width*768/1664/2)
    source = cv2.resize(source,(width,height),interpolation=cv2.INTER_AREA)
    geometry = prepare(source)
    print("Source approved:",master,"output:",width,height,"duration:",seconds,"fps:",fps,flush=True)
    dest.parent.mkdir(parents=True,exist_ok=True)
    command=["ffmpeg","-hide_banner","-loglevel","error","-y",
        "-f","rawvideo","-pixel_format","bgr24","-video_size",f"{width}x{height}",
        "-framerate",str(fps),"-i","pipe:0",
        "-an","-c:v","libx264","-preset","veryfast","-crf","22",
        "-profile:v","baseline","-level","3.1","-pix_fmt","yuv420p",
        "-movflags","+faststart","-g",str(fps*2),
        str(dest)]
    p=subprocess.Popen(command,stdin=subprocess.PIPE)
    try:
        for frame in range(fps*seconds):
            p.stdin.write(render_frame(source,geometry,frame,fps*seconds).tobytes())
            if frame%fps==0:
                print("Rendered second",frame//fps,flush=True)
    except BaseException:
        p.kill()
        raise
    finally:
        if p.stdin and not p.stdin.closed:
            p.stdin.close()
    if p.wait()!=0 or not dest.is_file() or dest.stat().st_size<100_000:
        raise RuntimeError("Animated Wonderwerk MP4 encoding failed")
    print("Generated actual animated MP4:",dest, dest.stat().st_size, "bytes",flush=True)


if __name__=="__main__":
    cli=argparse.ArgumentParser()
    cli.add_argument("--input",type=Path,default=MASTER)
    cli.add_argument("--output",type=Path,default=DEST)
    cli.add_argument("--width",type=int,default=1152)
    cli.add_argument("--fps",type=int,default=24)
    cli.add_argument("--seconds",type=int,default=6)
    args=cli.parse_args()
    if args.width<320 or args.width>1920 or args.width%2 or args.fps not in range(12,61) or args.seconds not in range(3,16):
        cli.error("Invalid dimensions, fps or duration")
    generate(args.input,args.output,args.width,args.fps,args.seconds)
