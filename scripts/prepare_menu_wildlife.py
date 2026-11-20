#!/usr/bin/env python3
"""Derive a living photographic grazer from the APPROVED source, keeping it intact.

No source art is overwritten. Exact logo/stone pixels must remain identical.
The living scene hides the already-painted distant animal in a tiny region and
draws that SAME photographic animal as a separate transparently-masked subject.
"""
from pathlib import Path
import sys
import cv2
import numpy as np

root=Path(sys.argv[1]) if len(sys.argv)>1 else Path("app/src/main/assets/menu")
original_path=root/"WONDERWERK_APPROVED_WIDE_NO_TEXT.png"
original=cv2.imread(str(original_path),cv2.IMREAD_COLOR)
if original is None or original.shape[:2]!=(768,1664):
    raise SystemExit("APPROVED ORIGINAL MISSING; cannot substitute invented art")
x,y,w,h=465,378,71,48
roi=original[y:y+h,x:x+w].copy()
mask=np.full((h,w),cv2.GC_PR_BGD,np.uint8)
mask[:3,:]=mask[-3:,:]=cv2.GC_BGD
mask[:,:3]=mask[:,-3:]=cv2.GC_BGD
body=np.array([(12,29),(17,19),(24,14),(36,11),(47,13),(54,18),
               (57,27),(61,33),(59,36),(56,34),(55,41),(51,42),
               (47,35),(45,31),(31,31),(30,39),(25,42),(22,39),
               (22,31),(19,30),(18,38),(15,40),(12,38)],np.int32)
cv2.fillPoly(mask,[body],cv2.GC_PR_FGD)
cv2.ellipse(mask,(37,24),(14,7),0,0,360,cv2.GC_FGD,-1)
background=np.zeros((1,65),np.float64)
foreground=np.zeros((1,65),np.float64)
cv2.grabCut(roi,mask,None,background,foreground,5,cv2.GC_INIT_WITH_MASK)
alpha=np.where((mask==cv2.GC_FGD)|(mask==cv2.GC_PR_FGD),255,0).astype(np.uint8)
count,labels,stats,_=cv2.connectedComponentsWithStats(alpha,8)
if count>1:
    dominant=1+int(np.argmax(stats[1:,cv2.CC_STAT_AREA]))
    alpha=np.where(labels==dominant,255,0).astype(np.uint8)
alpha=cv2.GaussianBlur(alpha,(3,3),.78)
if int(np.count_nonzero(alpha))<90:
    raise SystemExit("No reliable photographic wildlife cutout; refuse fake animal")
rgba=cv2.cvtColor(roi,cv2.COLOR_BGR2BGRA)
rgba[:,:,3]=alpha
sprite_path=root/"WILDLIFE_GRAZER_1.png"
if not cv2.imwrite(str(sprite_path),rgba):
    raise SystemExit("Failed to write wildlife sprite")
full_mask=np.zeros((768,1664),np.uint8)
removed=cv2.dilate(np.where(alpha>10,255,0).astype(np.uint8),
                    np.ones((3,3),np.uint8),iterations=2)
full_mask[y:y+h,x:x+w]=removed
scene=cv2.inpaint(original,full_mask,9,cv2.INPAINT_TELEA)
# These exact original pixels are never retouched under any circumstances.
assert np.array_equal(scene[:335,650:1135],original[:335,650:1135])
assert np.array_equal(scene[335:,650:1135],original[335:,650:1135])
assert np.array_equal(scene[:,1135:],original[:,1135:])
live_path=root/"WONDERWERK_LIVE_BASE.png"
if not cv2.imwrite(str(live_path),scene):
    raise SystemExit("Failed to save living landscape background")
print("PASS: original stone logo/stone controls pixel-identical;",
      "one photographic grazer detached and inpainted in background.")
