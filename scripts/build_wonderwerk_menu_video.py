#!/usr/bin/env python3
"""Single seamless menu video from EXACT approved original artwork.

No replacement logos, cave geometry, buttons, animals or vegetation.
Leaf/sky/water motion is localized; original rock/logo/button pixels remain
locked. The original PNG is read-only. This is not a photorealistic 3D render.
"""
import argparse
import math
import subprocess
from pathlib import Path
import numpy as np
from PIL import Image, ImageFilter

def generate(source: Path, output: Path, width=1152, height=532, fps=20, seconds=6):
    with Image.open(source) as img:
        base=np.asarray(img.convert("RGB").resize((width,height),Image.Resampling.LANCZOS),dtype=np.float32)
    yy,xx=np.indices((height,width),dtype=np.float32)
    red,green,blue=base[:,:,0],base[:,:,1],base[:,:,2]
    leaf_color=np.clip((green-.71*red-7.)/62.,0,1)*np.clip((green-.70*blue)/54.,0,1)
    leaves_region=((xx<width*.37)&(yy<height*.43))|((xx>width*.78)&(yy>height*.43))
    leaves=leaf_color*leaves_region
    water=((xx<width*.41)&(yy>height*.69))*np.clip((blue*.75+green*.25-red*.52-10)/65,0,1)
    cloud=((xx<width*.38)&(yy>height*.12)&(yy<height*.47))*np.clip((blue-red*.8-12)/70,0,1)
    def blur(mask,sigma):
        pic=Image.fromarray(np.uint8(np.clip(mask,0,1)*255),"L")
        return np.asarray(pic.filter(ImageFilter.GaussianBlur(sigma)),dtype=np.float32)/255
    leaves=blur(leaves,5)
    water=blur(water,4)
    cloud=blur(cloud,7)
    protected=(xx>width*.40)&(xx<width*.79)
    leaves[protected]=0
    water[protected]=0
    cloud[protected]=0
    mask=np.maximum.reduce([leaves,water*.85,cloud*.55])
    output.parent.mkdir(parents=True,exist_ok=True)
    process=subprocess.Popen([
        "ffmpeg","-hide_banner","-loglevel","error","-y","-f","rawvideo",
        "-pixel_format","rgb24","-video_size",f"{width}x{height}",
        "-framerate",str(fps),"-i","pipe:0","-an","-c:v","libx264",
        "-preset","veryfast","-crf","21","-pix_fmt","yuv420p",
        "-movflags","+faststart",str(output)
    ],stdin=subprocess.PIPE)
    rows=np.arange(height,dtype=np.int32)[:,None]
    cols=np.arange(width,dtype=np.int32)[None,:]
    count=fps*seconds
    try:
        for frame in range(count):
            phase=2*math.pi*frame/count
            shift_x=np.rint(1.85*np.sin(phase+yy*.023+xx*.009)).astype(np.int32)
            shift_y=np.rint(.85*np.cos(phase+xx*.012)).astype(np.int32)
            ix=np.clip(cols+shift_x,0,width-1)
            iy=np.clip(rows+shift_y,0,height-1)
            moved=base[iy,ix]
            scene=base*(1-mask[:,:,None])+moved*mask[:,:,None]
            ripple=np.sin(phase+xx*.062+yy*.09)
            daylight=np.sin(phase+xx*.011)*.013
            scene*=1+(water*.035*ripple+cloud*.021*daylight)[:,:,None]
            process.stdin.write(
                np.ascontiguousarray(np.uint8(np.clip(scene,0,255))).tobytes())
    except BaseException:
        process.kill()
        raise
    finally:
        if process.stdin and not process.stdin.closed:
            process.stdin.close()
    if process.wait()!=0 or not output.is_file() or output.stat().st_size<120000:
        raise RuntimeError("Menu MP4 generation or FFmpeg encode failed")
    print(f"Original-approved menu motion ready: {output} ({seconds}s, {fps}fps)")

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--input",type=Path,default=Path("app/src/main/assets/menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png"))
    p.add_argument("--output",type=Path,default=Path("app/src/main/assets/menu/wonderwerk_loop.mp4"))
    p.add_argument("--width",type=int,default=1152)
    p.add_argument("--height",type=int,default=532)
    p.add_argument("--fps",type=int,default=20)
    p.add_argument("--seconds",type=int,default=6)
    a=p.parse_args()
    if(a.width<320 or a.height<180 or a.width*a.height>1600*900 or
       a.fps<12 or a.fps>30 or a.seconds<3 or a.seconds>20):
        p.error("Invalid mobile video dimensions, fps or duration")
    generate(a.input,a.output,a.width,a.height,a.fps,a.seconds)

if __name__=="__main__":
    main()
