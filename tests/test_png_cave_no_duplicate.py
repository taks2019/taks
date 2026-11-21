"""Native Android regression guards for the grounded Wonderwerk preview.
The preview does NOT mark the generated art as accepted or edit the main menu.
"""
from pathlib import Path
import unittest

def read(p): return Path(p).read_text(encoding="utf-8")
MAIN=read("app/src/main/java/hu/taks/world/MainActivity.java")
NATIVE=read("app/src/main/cpp/native_main.cpp")
RENDER=read("app/src/main/cpp/FastScene.cpp")
SPRITE=read("app/src/main/cpp/CaveSprite.cpp")
CM=read("app/src/main/cpp/CMakeLists.txt")

class GroundedCave(unittest.TestCase):
    def test_original_menu_unchanged_and_gameplay_logo_removed(self):
        self.assertIn("menuCover.addView(backdrop,new FrameLayout.LayoutParams(-1,-1));",MAIN)
        self.assertIn("logoFrame.addView(originalLogoOrTitle()",MAIN)
        self.assertNotIn("gameplayLogo",MAIN)
        self.assertNotIn('header.addView(caption("TAKS ENGINE',MAIN)
    def test_cave_no_longer_overlays_the_screen(self):
        self.assertNotIn("new IsometricCaveOverlay(this)",MAIN)
        self.assertNotIn("caveOverlay.setCamera(",MAIN)
        self.assertNotIn("root.addView(caveOverlay",MAIN)
        self.assertIn("taks::CaveSprite::render(vp.a",RENDER)
        self.assertIn("glEnable(GL_DEPTH_TEST);",SPRITE)
        self.assertIn("glDepthMask(GL_FALSE);",SPRITE)
        self.assertIn("float bottomY=caveGroundY",SPRITE)
        self.assertIn("footprintZ=2.2f",SPRITE)
        self.assertIn('CaveSprite.cpp',CM)
    def test_eight_angle_crossfade_and_ground_contact(self):
        self.assertIn("textures[8]",SPRITE)
        self.assertIn("int index1=(index0+1)%8",SPRITE)
        self.assertIn("drawTexture(textures[index0]",SPRITE)
        self.assertIn("glBindTexture(GL_TEXTURE_2D,textures[index1])",SPRITE)
        self.assertIn("glUniform1f(blendUniform,fade)",SPRITE)
        self.assertIn("sharedOpaqueFootRow=444.f",SPRITE)
        self.assertIn("glTexImage2D(GL_TEXTURE_2D",SPRITE)
        self.assertIn("shadowTexture",SPRITE)
        self.assertIn('nativeUploadCaveView(i,argb,width,height)',MAIN)
    def test_preview_only_never_approves_assets_or_saves_flag(self):
        self.assertIn("nativeSetCavePreview(cavePreviewReady && cavePreviewWanted)",MAIN)
        self.assertIn("std::atomic<bool> pngCavePreview{false}",NATIVE)
        self.assertIn("f.pngCavePreview=pngCavePreview.load",NATIVE)
        self.assertIn("applicationIdSuffix '.menufixedgame'",read("app/build.gradle"))
        self.assertIn('bool pngCavePreview=false',read("app/src/main/cpp/FastScene.h"))
        self.assertNotIn("pngCavePreview",read("engine/CauseWorld.cpp"))
if __name__=="__main__":
    unittest.main()
