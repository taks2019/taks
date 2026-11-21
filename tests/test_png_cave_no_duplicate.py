"""Static guards for standalone visual-review Android APK.
These tests document non-approval and the user-requested main-menu invariants.
"""
import unittest
from pathlib import Path

JAVA=Path("app/src/main/java/hu/taks/world/MainActivity.java").read_text(encoding="utf-8")
OVERLAY=Path("app/src/main/java/hu/taks/world/IsometricCaveOverlay.java").read_text(encoding="utf-8")
SCENE=Path("app/src/main/cpp/FastScene.cpp").read_text(encoding="utf-8")
HEADER=Path("app/src/main/cpp/FastScene.h").read_text(encoding="utf-8")
BRIDGE=Path("app/src/main/cpp/native_main.cpp").read_text(encoding="utf-8")

class SingleCavePreview(unittest.TestCase):
    def test_start_menu_untouched_and_game_logo_removed(self):
        self.assertIn('menuCover.addView(backdrop,new FrameLayout.LayoutParams(-1,-1));',JAVA)
        self.assertIn('logoFrame.addView(originalLogoOrTitle()',JAVA)
        self.assertNotIn('gameplayLogo',JAVA)
        self.assertNotIn('header.addView(caption("TAKS ENGINE',JAVA)

    def test_native_only_renders_one_cave_at_far_zoom(self):
        self.assertIn('std::vector<Vertex> terrainMesh,caveMesh,roofMesh',SCENE)
        self.assertIn('const float oldCaveAlpha = 1.f - pngCover;',SCENE)
        self.assertIn('if(oldCaveAlpha > .001f',SCENE)
        self.assertIn('const float roofOpacity=f.pngCavePreview?0.f:',SCENE)
        self.assertIn('bool pngCavePreview=false',HEADER)
        self.assertIn('pngCavePreview.store(enabled==JNI_TRUE',BRIDGE)
        self.assertIn('nativeSetCavePreview(caveOverlay.isPreviewEnabled())',JAVA)

    def test_smooth_8_direction_rotation(self):
        self.assertIn('final double direction = (double)yaw / (Math.PI/4.0)',OVERLAY)
        self.assertIn('canvas.drawBitmap(views[first],null,dst,paint)',OVERLAY)
        self.assertIn('canvas.drawBitmap(views[second],null,dst,paint)',OVERLAY)
        self.assertIn('getHeight() / Math.max(zoom * .99f',OVERLAY)
        self.assertIn('float rightX=-(float)Math.cos(yaw)',OVERLAY)
        self.assertNotIn('Math.round((float)Math.toDegrees(yaw)/45f)',OVERLAY)

    def test_preview_is_not_a_saved_game_state(self):
        self.assertNotIn('pngCavePreview',Path("engine/include/CauseWorld.h").read_text(encoding="utf-8") if Path("engine/include/CauseWorld.h").exists() else "")
        self.assertIn('applicationIdSuffix \'.pngfixed\'',Path('app/build.gradle').read_text(encoding='utf-8'))

if __name__ == "__main__":
    unittest.main()
