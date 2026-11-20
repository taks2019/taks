"""Regression guard: game HUD must use the same evolving stone visual system as menu.
These checks do not approve any new visual content and do not replace the 3D world.
"""
from pathlib import Path
import unittest

MAIN=Path("app/src/main/java/hu/taks/world/MainActivity.java")

class UnifiedHudSourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source=MAIN.read_text(encoding="utf-8")
    def test_live_skins_are_used_for_each_gameplay_control(self):
        for key in ('skin(gameplayLogo,"LOGO")',
                    'skin(hint,"HUD_STATUS")',
                    'skin(notice,"HUD_STATUS")',
                    'skin(proximity,"HUD_STATUS")',
                    'skin(movePad,"MOVEMENT")',
                    'skin(sprint,"BUTTON")',
                    'skin(bottom,"CARD")'):
            self.assertIn(key,self.source,key)
        self.assertNotIn('movePad.setBackground(panelBackground(',self.source)
        self.assertNotIn('sprint.setBackground(panelBackground(',self.source)
    def test_status_does_not_cover_entire_game(self):
        self.assertIn('safeStatusWidth',self.source)
        self.assertIn('Gravity.BOTTOM|Gravity.CENTER_HORIZONTAL',self.source)
        self.assertIn('notice.setMaxLines(3)',self.source)
    def test_actual_rendering_remains_native_and_separate_from_menu_plate(self):
        self.assertIn('root.addView(surface,new FrameLayout.LayoutParams(-1,-1));',self.source)
        self.assertIn('private Bitmap menuBackground()',self.source)
        self.assertIn('surface.setEGLContextClientVersion(3)',self.source)
    def test_settings_header_logo_not_clipped_again(self):
        self.assertIn('dp(132),dp(67)',self.source)

if __name__=="__main__":
    unittest.main()
