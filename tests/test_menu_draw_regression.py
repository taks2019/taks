"""Regression: no completely brown Wonderwerk menu / single visible pickup button.
These guards must pass before distributing another APK to the phone.
"""
from pathlib import Path
import unittest

MENU=Path("app/src/main/java/hu/taks/world/RestoredStoneMenu.java").read_text(encoding="utf-8")
MAIN=Path("app/src/main/java/hu/taks/world/MainActivity.java").read_text(encoding="utf-8")
GRADLE=Path("app/build.gradle").read_text(encoding="utf-8")

class RestoredApprovedMenu(unittest.TestCase):
    def test_draws_master_directly_across_full_screen(self):
        self.assertIn('setWillNotDraw(false)',MENU)
        self.assertIn('canvas.drawBitmap(accepted,0f,0f,imagePaint)',MENU)
        self.assertIn('float scale',MENU)
        self.assertIn('Math.min(width/(float)DESIGN_W,height/(float)DESIGN_H)',MENU)
        self.assertNotIn('artwork=new FrameLayout',MENU)
        self.assertNotIn('artwork = new FrameLayout',MENU)
        self.assertNotIn('ImageView.ScaleType.FIT_XY',MENU)
    def test_buttons_have_accessible_touches_and_top_z_order(self):
        for text in ('ÚJ JÁTÉK','BEÁLLÍTÁSOK','KÉSZÍTŐ','KILÉPÉS'):
            self.assertIn(text,MENU)
        self.assertIn('target.setOnClickListener(v->actions[index].run())',MENU)
        self.assertIn('menuCover.setElevation(dp(24))',MAIN)
        self.assertIn('menuCover.bringToFront()',MAIN)
        self.assertIn('modalOverlay.setElevation(dp(48))',MAIN)
        self.assertIn('if(pickupButton!=null)pickupButton.setVisibility(View.GONE)',MAIN)
        self.assertIn('RestoredStoneMenu.load(this',MAIN)
        self.assertIn('if(surface!=null)surface.setVisibility(View.INVISIBLE);',MAIN)
        self.assertIn('surface.setVisibility(View.VISIBLE);',MAIN)
        self.assertIn('surface.setZOrderOnTop(false)',MAIN)
        self.assertIn('surface.setZOrderMediaOverlay(false)',MAIN)
        self.assertIn("applicationIdSuffix '.menufixedgame'",GRADLE)
    def test_exact_original_asset_required_not_synthetic_mp4(self):
        self.assertIn('menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png',MENU)
        self.assertNotIn('menuMotion=new MenuMotionView(this)',MAIN)

if __name__ == '__main__':
    unittest.main()
