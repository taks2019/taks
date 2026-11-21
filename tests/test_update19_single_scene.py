"""Protects the user's frozen Wonderwerk menu and rejects double-scene MP4.
The accepted menu source file itself is never touched; this tests only added code.
"""
from pathlib import Path
import subprocess
import unittest
import cv2
import numpy as np

MENU=Path("app/src/main/java/hu/taks/world/ApprovedMenuMotion.java").read_text()
MAIN=Path("app/src/main/java/hu/taks/world/MainActivity.java").read_text()
GEN=Path("scripts/render_approved_wonderwerk_motion.py").read_text()
GRADLE=Path("app/build.gradle").read_text()
PNG=Path("app/src/main/assets/menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png")
MP4=Path("app/src/main/assets/menu/wonderwerk_loop.mp4")

class SingleSceneMenu(unittest.TestCase):
    def test_original_menu_is_not_edited_or_overwritten(self):
        self.assertIn("RestoredStoneMenu.load(this",MAIN)
        self.assertIn("WONDERWERK_APPROVED_WIDE_NO_TEXT.png",GEN)
        self.assertIn("menuCover.getChildAt(0)",MAIN)
        self.assertIn("ApprovedMenuMotion.create(this,",MAIN)
        self.assertIn("menuCover.getChildAt(0),this::newWorldFromMenu",MAIN)
        self.assertIn("applicationIdSuffix '.menufixedgame'",GRADLE)
        self.assertIn("versionCode 19",GRADLE)

    def test_single_scene_only_no_crossfade(self):
        self.assertIn("approvedStaticScene.setVisibility(View.INVISIBLE);",MENU)
        self.assertIn("setAlpha(1f);",MENU)
        self.assertIn('SINGLE_SCENE: MP4 visible, static scene hidden',MENU)
        self.assertIn("approvedStaticScene.setVisibility(View.VISIBLE);",MENU)
        self.assertNotIn('animate().alpha(1f).setDuration(220)',MENU)
        self.assertIn("setOnClickListener(v->{",MENU)
        for action in ("this::newWorldFromMenu","this::showSettings",
                       "this::showCredits","this::confirmExit"):
            self.assertIn(action,MAIN)

    def test_layer_cutout_avoids_ghost_image(self):
        self.assertIn("cv2.inpaint(original,removal",GEN)
        self.assertIn("self.draw_layer(canvas,layer,dx,dy)",GEN)
        self.assertIn("Foreground(alpha,self.source*alpha[:,:,None])",GEN)
        self.assertNotIn("source.astype(np.float32) * (1 - motion",GEN)
        self.assertIn("water",GEN)
        self.assertIn("elephant_head",GEN)
        self.assertIn("path=(seed+t*",GEN)

    @unittest.skipUnless(PNG.is_file() and MP4.is_file(),"Real MP4 built in CI before this test")
    def test_real_video_motion_without_duplicating_frozen_stone_logo(self):
        def frame(sec):
            cmd=["ffmpeg","-hide_banner","-loglevel","error","-ss",str(sec),
                 "-i",str(MP4),"-frames:v","1","-f","rawvideo",
                 "-pix_fmt","bgr24","pipe:1"]
            result=subprocess.run(cmd,capture_output=True,check=True).stdout
            master=cv2.imread(str(PNG))
            w=1152;h=2*round(w*768/1664/2)
            return np.frombuffer(result,dtype=np.uint8).reshape((h,w,3)).astype(np.int16)
        a,b=frame(.125),frame(1.625)
        diff=np.abs(b-a).mean(axis=2)
        visible=float(np.mean(diff))
        # The centre is the user's original stone logo and four stone buttons.
        central=float(np.mean(diff[int(diff.shape[0]*.08):int(diff.shape[0]*.90),
                                     int(diff.shape[1]*.42):int(diff.shape[1]*.59)]))
        print("Actual MP4 frame difference:",visible,"locked centre:",central,flush=True)
        self.assertGreater(visible,.25,"MP4 must actually change between frames")
        self.assertLess(central,1.45,"Approved logo/buttons must not move")

if __name__=="__main__":
    unittest.main()
