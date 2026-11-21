#!/usr/bin/env python3
"""TAKS World: one-shot photorealistic menu source -> genuinely moving H.264 MP4.

Do not mirror, pan, crop or redraw the accepted menu, tree trunk, logo, rocks,
clouds, cave or four stone buttons. The older implementation simply warped
source pixels over the source, resulting in TWO copies of leaves and waves.

Instead we extract vegetation as one semi-transparent layer per plant type,
INPAINT ITS OLD LOCATION ONCE, then draw precisely one moving vegetation
layer over that cleaned background. The persistent approved PNG is immutable.
This is an honest motion-study from a flattened image, not full skeletal 3D;
individually rigged source PNG assets will be needed for anatomical movement.
"""
from __future__ import annotations
import argparse
import math
import subprocess
from dataclasses import dataclass
from pathlib import Path
import cv2
import numpy as np

MASTER=Path("app/src/main/assets/menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png")
DEST=Path("app/src/main/assets/menu/wonderwerk_loop.mp4")

def clip01(x):
    return np.clip(x,0.,1.)

def transition(x,a,b):
    t=clip01((x-a)/(b-a))
    return t*t*(3-2*t)

def region(x,y,l,t,r,b,edge=.012):
    return (transition(x,l,l+edge)*transition(r-x,0,edge)*
            transition(y,t,t+edge)*transition(b-y,0,edge))

@dataclass
class Foreground:
    alpha: np.ndarray
    premultiplied: np.ndarray

class SingleSceneAnimator:
    def __init__(self,original):
        self.source=original.astype(np.float32)
        self.h,self.w=original.shape[:2]
        self.yy,self.xx=np.indices((self.h,self.w),dtype=np.float32)
        self.u=self.xx/self.w
        self.v=self.yy/self.h
        hsv=cv2.cvtColor(original,cv2.COLOR_BGR2HSV)
        hue=hsv[:,:,0].astype(np.float32)
        sat=hsv[:,:,1].astype(np.float32)
        b,g,r=cv2.split(self.source)
        botanical=(transition(hue,13,22)*transition(90-hue,0,18)*
                   transition(sat,28,86)*
                   transition(g-b*.78,0,40))
        # Tree leaves grow from anchored branches. Trunk, sun, distant clouds
        # and the hero cave stay stationary; only real leaf colour is extracted.
        tree=botanical*region(self.u,self.v,0.,0.,.435,.387,.013)
        tree*=transition(.407-self.v,0,.23)
        # The distant elephant and shrubs do not belong to the tree mask.
        ferns=botanical*region(self.u,self.v,.663,.473,1.,1.,.014)
        grasses=botanical*(
            region(self.u,self.v,.0,.535,.393,1.,.015)*.62 +
            region(self.u,self.v,.588,.750,.711,1.,.015)*.28)
        # Rock labels and tree-carved central original emblem are never
        # manipulated, including the invisible margins around all 4 buttons.
        locked=region(self.u,self.v,.375,.010,.667,1.,.017)
        tree*=1-locked;ferns*=1-locked;grasses*=1-locked
        # Alpha source comes from actual coloured foliage, not a duplicate
        # full-image warp. Threshold ambiguous pixels conservatively.
        self.foliage=[]
        removal=np.zeros((self.h,self.w),dtype=np.uint8)
        for name,confidence in (("tree",tree),("ferns",ferns),("grasses",grasses)):
            high=clip01((confidence-.19)/.40)
            high=transition(high,.18,.80)
            alpha=cv2.GaussianBlur(high.astype(np.float32),(0,0),.65)
            alpha[alpha<.025]=0
            if np.count_nonzero(alpha) < 100:
                raise ValueError("Cannot isolate the original approved "+name+" foliage")
            removal=np.maximum(removal,(alpha>.14).astype(np.uint8)*255)
            self.foliage.append((name,Foreground(alpha,self.source*alpha[:,:,None])))
            print(name,"isolated leaf pixels",np.count_nonzero(alpha>.15),flush=True)

        # The elephant head is too small for a full-body walk cycle from a
        # flattened photograph. Animate a SUBTLE grazing-head movement only.
        gray=cv2.cvtColor(original,cv2.COLOR_BGR2GRAY).astype(np.float32)
        head_region=region(self.u,self.v,.281,.498,.297,.549,.0025)
        head_alpha=clip01((156-gray)/44.)*head_region*.78
        head_alpha=cv2.GaussianBlur(head_alpha.astype(np.float32),(0,0),.55)
        if np.count_nonzero(head_alpha>.15)>18:
            removal=np.maximum(removal,(head_alpha>.13).astype(np.uint8)*255)
            self.foliage.append(("elephant_head",
                                 Foreground(head_alpha,self.source*head_alpha[:,:,None])))
        # Smoothly repair the actual occluded old leaf / head positions ONCE.
        # No static copy may remain under the displaced leaf pixels.
        # Keep original terrain and detailed rocks outside extracted masks.
        removal=cv2.dilate(removal,np.ones((3,3),np.uint8),iterations=1)
        self.background=cv2.inpaint(original,removal,3.,cv2.INPAINT_TELEA).astype(np.float32)
        self.water=(
            region(self.u,self.v,.043,.694,.413,1.,.012)*
            transition(b*.85+g*.5-r*.67,16,120))
        self.water=cv2.GaussianBlur(self.water.astype(np.float32),(0,0),1.2)
        self.water*=1.-locked
        # The water is a single in-place surface. We do NOT copy the river.
        self.background=self.background.astype(np.float32)
        self.locked=locked

    def draw_layer(self,canvas,layer,dx,dy):
        # Premultiplied alpha prevents dark fringes at real leaf silhouettes.
        mapx=self.xx-dx.astype(np.float32)
        mapy=self.yy-dy.astype(np.float32)
        remap=dict(interpolation=cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT)
        alpha=cv2.remap(layer.alpha,mapx,mapy,**remap)
        rgb=cv2.remap(layer.premultiplied,mapx,mapy,**remap)
        return canvas*(1.-alpha[:,:,None])+rgb

    def frame(self,k,total):
        t=k/total
        phase=np.float32(2*math.pi*t)
        x,y=self.xx,self.yy
        u,v=self.u,self.v
        # Layer motion is NOT a flowing undulation of the full image:
        # independent slow gusts + flutter at different branch positions,
        # plus gradual anchoring towards stem and leaf-root regions.
        gust=(.72*np.sin(phase+.4)+.24*np.sin(2*phase+2.1)
              +.13*np.sin(3*phase+.13))
        branch_groups=(.57*np.sin(phase+u*7.0+v*3.7)
                       +.32*np.sin(2*phase+u*14.0-v*4.4+1.2)
                       +.14*np.sin(3*phase+u*5.5+.7))
        leaf_root_weight=transition(.43-v,.02,.34)
        fern_root_weight=transition(1.03-v,.02,.39)
        graze=np.sin(phase+.24)*.78+.22*np.sin(2*phase+1.1)
        canvas=self.background.copy()
        for kind,layer in self.foliage:
            if kind=="tree":
                dx=(1.25*gust+1.3*branch_groups)*leaf_root_weight
                dy=(.42*branch_groups+.32*np.sin(2*phase+u*11.))*leaf_root_weight
            elif kind=="ferns":
                dx=(2.15*gust+2.1*np.sin(phase+u*19.+v*6.8)
                    +.48*np.sin(3*phase+u*12.3))*fern_root_weight
                dy=(.75*np.sin(phase+u*13.5+1.0))*fern_root_weight
            elif kind=="grasses":
                weight=transition(.96-v,.005,.26)
                dx=(1.4*gust+1.5*np.sin(phase+u*26.6))*weight
                dy=(.45*np.cos(2*phase+u*12.))*weight
            else: # small distant elephant: anchored subtle head grazing
                dx=np.zeros_like(x)
                dy=np.full_like(y,graze*.9)
            canvas=self.draw_layer(canvas,layer,dx,dy)

        # Moving river texture/reflections: highlights ADVECT downstream
        # (diagonally toward the foreground); original riverbed and stones
        # stay fixed. Both frequencies wrap seamlessly at loop boundaries.
        drift=(np.sin(x*.049+y*.073-2*phase)+
               .52*np.sin(x*.096-y*.028-3*phase+.8))
        shimmer=(clip01(drift*.37+.43)**4)*self.water
        canvas+=((shimmer*.37)[:,:,None]*
                 np.array([28.,36.,52.],dtype=np.float32))
        # Tiny refraction within water, no warped rock banks or dual image.
        dx=(np.sin(x*.068+y*.042-3*phase)*1.1*self.water).astype(np.float32)
        dy=(np.cos(y*.079-x*.019-2*phase)*.65*self.water).astype(np.float32)
        rippled=cv2.remap(canvas,(x-dx),(y-dy),cv2.INTER_LINEAR,
                          borderMode=cv2.BORDER_REPLICATE)
        canvas=canvas*(1-self.water[:,:,None]*.65)+rippled*self.water[:,:,None]*.65

        # Original sun remains ONE fixed sun. Change only light intensity,
        # not its location or add an artificial second sky image.
        rays=np.exp(-((u-.245)**2/.035+(v-.20)**2/.037))
        sunf=.028*np.sin(phase+.7)+.012*np.sin(2*phase+1.1)
        canvas+=(rays*sunf*(1-self.locked))[:,:,None] * np.array([24,39,55])

        # Dust drifts continuously along wind, ascending through the cave
        # entrance. Wrap *outside* the visible illuminated shaft and fade at
        # both ends: there is no oscillating left-right particle motion.
        for i in range(72):
            seed=(i*.61803398875)%1
            path=(seed+t*(.67+.06*(i%5)))%1
            vx=.663+.11*((i*.7548776662)%1)+.045*path
            vy=.60-.30*path+.007*np.sin(phase+i*1.14)
            a=math.sin(math.pi*path)**2
            px=int(vx*self.w)
            py=int(vy*self.h)
            if 1<=px<self.w-1 and 1<=py<self.h-1:
                opacity=.23*a*(.55+.45*((i*13)%11)/10.)
                radius=1 if i%6 else 2
                cv2.circle(canvas,(px,py),radius,
                           tuple(float(min(255.,c)) for c in (
                               canvas[py,px,0]+(244-canvas[py,px,0])*opacity,
                               canvas[py,px,1]+(223-canvas[py,px,1])*opacity,
                               canvas[py,px,2]+(177-canvas[py,px,2])*opacity)),
                           -1,lineType=cv2.LINE_AA)
        return np.clip(canvas,0,255).astype(np.uint8)


def generate(master,dest,width,fps,seconds):
    approved=cv2.imread(str(master),cv2.IMREAD_COLOR)
    if approved is None:
        raise FileNotFoundError("Locked approved menu not available: "+str(master))
    if approved.shape[:2]!=(768,1664):
        raise ValueError("Original locked 1664x768 menu must not change")
    height=2*round(width*768/1664/2)
    approved=cv2.resize(approved,(width,height),interpolation=cv2.INTER_AREA)
    animator=SingleSceneAnimator(approved)
    dest.parent.mkdir(parents=True,exist_ok=True)
    p=subprocess.Popen(["ffmpeg","-hide_banner","-loglevel","error","-y",
        "-f","rawvideo","-pixel_format","bgr24","-video_size",f"{width}x{height}",
        "-framerate",str(fps),"-i","pipe:0","-an",
        "-c:v","libx264","-preset","veryfast","-crf","21",
        "-profile:v","baseline","-level","3.1","-pix_fmt","yuv420p",
        "-movflags","+faststart","-g",str(fps*2),str(dest)],stdin=subprocess.PIPE)
    try:
        for k in range(fps*seconds):
            p.stdin.write(animator.frame(k,fps*seconds).tobytes())
            if k%fps==0:print("Animated seconds:",k//fps,flush=True)
    except BaseException:
        p.kill()
        raise
    finally:
        if p.stdin and not p.stdin.closed:p.stdin.close()
    if p.wait()!=0 or not dest.exists() or dest.stat().st_size<90_000:
        raise RuntimeError("Genuinely animated single-scene MP4 failed")
    print("APPROVED SINGLE-SCENE LOOP READY",dest,dest.stat().st_size,flush=True)

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--input",type=Path,default=MASTER)
    parser.add_argument("--output",type=Path,default=DEST)
    parser.add_argument("--width",type=int,default=1152)
    parser.add_argument("--fps",type=int,default=24)
    parser.add_argument("--seconds",type=int,default=6)
    args=parser.parse_args()
    if not 320<=args.width<=1920 or args.width%2 or not 12<=args.fps<=60 or not 3<=args.seconds<=15:
        parser.error("unsupported video parameters")
    generate(args.input,args.output,args.width,args.fps,args.seconds)
