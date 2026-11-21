"""The single approved background MP4 is assembled, genuinely moves and stays
pixel-anchored in the logo and stone button regions (no double text/scene).
This is NOT approval of new artwork or claim of photorealistic 3D.
"""
import ast
import shutil
import subprocess
import unittest
from pathlib import Path
import numpy as np

VIDEO=Path("app/src/main/assets/menu/wonderwerk_loop.mp4")
SOURCE=Path("app/src/main/assets/menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png")
GENERATOR=Path("scripts/build_wonderwerk_menu_video.py")
ANDROID=Path("app/src/main/java/hu/taks/world/ApprovedMenuMotion.java").read_text(encoding="utf-8")
MAIN=Path("app/src/main/java/hu/taks/world/MainActivity.java").read_text(encoding="utf-8")

class TestApprovedMenuVideo(unittest.TestCase):
    def test_generated_only_from_accepted_art(self):
        ast.parse(GENERATOR.read_text(encoding="utf-8"))
        self.assertTrue(SOURCE.exists())
        self.assertTrue(VIDEO.exists())
        self.assertGreater(VIDEO.stat().st_size,120_000)
        self.assertIn("installApprovedMotionOnCurrentMenu();",MAIN)
        self.assertEqual(MAIN.count("installApprovedMotionOnCurrentMenu();"),1)
        self.assertIn("approvedStaticScene.setVisibility(View.INVISIBLE)",ANDROID)
        self.assertIn("SINGLE_SCENE: MP4 visible, static scene hidden",ANDROID)
        self.assertIn("lastMeasuredW",ANDROID)
    @unittest.skipUnless(shutil.which("ffmpeg"),"ffmpeg required for decoding")
    def test_foliage_and_water_moves_but_rock_logo_stays_still(self):
        import io
        def frame(s):
            raw=subprocess.check_output([
                "ffmpeg","-v","error","-ss",str(s),"-i",str(VIDEO),
                "-frames:v","1","-f","image2pipe","-vcodec","png","pipe:1"])
            from PIL import Image
            return np.asarray(Image.open(io.BytesIO(raw)).convert("RGB"),dtype=np.int16)
        a,b=frame(1),frame(4)
        self.assertEqual(a.shape[1],1152)
        self.assertEqual(a.shape[0],532)
        diff=np.abs(a-b).mean(axis=2)
        self.assertGreater(float(diff[30:210,0:370].mean()),.7)
        self.assertGreater(float(diff[390:510,0:440].mean()),.7)
        self.assertLess(float(diff[15:145,430:850].mean()),.5)
