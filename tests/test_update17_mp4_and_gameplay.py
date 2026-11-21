"""Regression guards for ONE simultaneous native-game + animated MP4 UPDATE.

This must not bypass the previously approved logo, menu and APK signing rules.
"""
import subprocess
from pathlib import Path
import unittest

MAIN=Path("app/src/main/java/hu/taks/world/MainActivity.java").read_text()
VIDEO=Path("app/src/main/java/hu/taks/world/ApprovedMenuMotion.java").read_text()
SCENE=Path("app/src/main/cpp/FastScene.cpp").read_text()
NATIVE=Path("app/src/main/cpp/native_main.cpp").read_text()
FRAME=Path("app/src/main/cpp/FastScene.h").read_text()
GRADLE=Path("app/build.gradle").read_text()
CLIP=Path("app/src/main/assets/menu/wonderwerk_loop.mp4")


class ActualGameplayAndMenuMotion(unittest.TestCase):
    def test_original_menu_still_in_control(self):
        self.assertIn("RestoredStoneMenu.load(this",MAIN)
        self.assertNotIn("menuMotion=new MenuMotionView(this)",MAIN)
        self.assertIn("installApprovedMotionOnCurrentMenu();",MAIN)
        self.assertIn("menuCover.getChildAt(0) instanceof RestoredStoneMenu",MAIN)
        self.assertIn("menuCover.addView(moving,new FrameLayout.LayoutParams(-1,-1))",MAIN)
        self.assertIn("setClickable(false)",VIDEO)
        self.assertIn("target.setOnClickListener(v->{",VIDEO)
        self.assertIn("this::newWorldFromMenu,this::showSettings",MAIN)
        self.assertIn("setFocusable(false)",VIDEO)
        self.assertIn("mp.setLooping(true)",VIDEO)
        self.assertIn("MEDIA_INFO_VIDEO_RENDERING_START",VIDEO)
        self.assertIn("menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png",
                      Path("app/src/main/java/hu/taks/world/RestoredStoneMenu.java").read_text())

    def test_genuine_native_inside_cave_camera_does_not_reset_world(self):
        self.assertIn("BARLANG BELSEJE",MAIN)
        self.assertIn("VISSZA A TÁJHOZ",MAIN)
        self.assertIn("private void toggleCaveInspection()",MAIN)
        self.assertIn("nativeSetCaveInspection(caveInspecting)",MAIN)
        self.assertIn("Java_hu_taks_world_MainActivity_nativeSetCaveInspection",NATIVE)
        self.assertIn("f.inspectCave=caveInspection.load",NATIVE)
        self.assertIn("bool inspectCave=false",FRAME)
        self.assertIn("const float effectiveZoom=f.inspectCave?7.5f:f.zoom",SCENE)
        self.assertIn("Vec3{0.f,surfaceHeight(0.f,4.8f)+.95f,4.8f}",SCENE)
        self.assertIn("f.inspectCave?0.f",SCENE)
        self.assertNotIn("world=taks::World();\n    caveInspection",NATIVE)
        self.assertIn("for(View v:gameHud)v.setVisibility(visible?",MAIN)

    def test_inplace_update_package_unchanged(self):
        self.assertIn("applicationIdSuffix '.menufixedgame'",GRADLE)
        self.assertIn("versionCode 18",GRADLE)
        self.assertIn("versionName '0.15.3'",GRADLE)

    @unittest.skipUnless(CLIP.is_file(),"Generated in GitHub Actions before test")
    def test_video_is_actually_moving_not_static_png_reencoded(self):
        def frame(t):
            return subprocess.run(["ffmpeg","-hide_banner","-loglevel","error",
                                   "-ss",str(t),"-i",str(CLIP),"-frames:v","1",
                                   "-vf","scale=192:88","-pix_fmt","rgb24",
                                   "-f","rawvideo","pipe:1"],
                                  capture_output=True,check=True).stdout
        a=frame(.15);b=frame(1.75)
        self.assertEqual(len(a),len(b))
        from array import array
        # Count significantly changed brightness and colour pixels.
        big=sum(abs(x-y)>5 for x,y in zip(a,b))
        print("Actually changed MP4 colour channels (>5):",big,
              "/",len(a),flush=True)
        self.assertGreater(big,350,
            "Rejected: MP4 holds nearly the same static frame")

if __name__=="__main__":
    unittest.main()
