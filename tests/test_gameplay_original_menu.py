"""Regression guards: preserve the approved menu, test the actual game renderer."""
from pathlib import Path
import unittest
MAIN = Path("app/src/main/java/hu/taks/world/MainActivity.java").read_text()
RESTORED = Path("app/src/main/java/hu/taks/world/RestoredStoneMenu.java").read_text()
SPRITE = Path("app/src/main/cpp/CaveSprite.cpp").read_text()

class GameplayAndOriginalMenu(unittest.TestCase):
    def test_rejected_mp4_is_not_launched(self):
        self.assertIn("RestoredStoneMenu.load(this", MAIN)
        self.assertNotIn("menuMotion=new MenuMotionView(this)", MAIN)
        self.assertIn("WONDERWERK_APPROVED_WIDE_NO_TEXT.png", RESTORED)
    def test_exactly_four_buttons_run_native_actions(self):
        self.assertIn('{"ÚJ JÁTÉK","BEÁLLÍTÁSOK","KÉSZÍTŐ","KILÉPÉS"}', RESTORED)
        self.assertIn("this::newWorldFromMenu,this::showSettings", MAIN)
        self.assertIn("this::showCredits,this::confirmExit", MAIN)
    def test_one_grounded_png_at_a_time_without_double_silhouettes(self):
        self.assertIn("vec4 texel=mix(first,second",SPRITE)
        self.assertIn("drawTexture(textures[index0],billboard,alpha,tint)",SPRITE)
        self.assertNotIn("drawTexture(textures[index1]",SPRITE)
        self.assertIn("sourceFootRow=444.f/512.f",SPRITE) 
        self.assertIn("int index1=index0",SPRITE) 
        self.assertIn("CaveBillboard::aligned",SPRITE)

if __name__=="__main__":unittest.main()
