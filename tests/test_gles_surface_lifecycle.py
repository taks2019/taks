"""Keep native GLES3 surface persistent across menu/game transitions.

SurfaceView INVISIBLE causes SurfaceFlinger/EGL surface teardown on Android.
An empty #303030 playfield must never be considered completed gameplay.
"""
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
MAIN=(ROOT/"app/src/main/java/hu/taks/world/MainActivity.java").read_text(encoding="utf-8")
MENU=(ROOT/"app/src/main/java/hu/taks/world/RestoredStoneMenu.java").read_text(encoding="utf-8")
class Continuous3DRenderer(unittest.TestCase):
    def test_3d_surface_not_torn_down_in_main_menu(self):
        self.assertNotIn("surface.setVisibility(View.INVISIBLE)",MAIN)
        self.assertNotIn("surface.setVisibility(View.GONE)",MAIN)
        self.assertIn("surface.setZOrderOnTop(false)",MAIN)
        self.assertIn("surface.setZOrderMediaOverlay(false)",MAIN)
        self.assertIn("menuCover.bringToFront()",MAIN)
    def test_frame_request_after_gameplay_menu_is_dismissed(self):
        start=MAIN[MAIN.index("private void startPlaying()"):MAIN.index("private void newWorldFromMenu()")]
        self.assertIn("root.removeView(menuCover)",start)
        self.assertIn("surface.postDelayed",start)
        self.assertIn("startFrameLoop()",start)
    def test_original_approved_art_unmodified_and_single_layer(self):
        self.assertIn("canvas.drawBitmap(accepted,0f,0f,imagePaint)",MENU)
        self.assertNotIn("installApprovedMotionOnCurrentMenu();",MAIN)
        self.assertIn("RestoredStoneMenu.load(this",MAIN)
if __name__=="__main__":unittest.main()
