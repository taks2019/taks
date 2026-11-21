"""Regression gate: gameplay uses the genuine depth-tested 3D cave by default.
The photographic eight-angle PNG remains a separately opt-in review tool.
No texture, approved menu, APK identity, or save schema is modified.
"""
from pathlib import Path
import unittest
import re

ROOT = Path(__file__).resolve().parents[1]
JAVA = (ROOT / "app/src/main/java/hu/taks/world/MainActivity.java").read_text(encoding="utf-8")
NATIVE = (ROOT / "app/src/main/cpp/native_main.cpp").read_text(encoding="utf-8")
SCENE = (ROOT / "app/src/main/cpp/FastScene.cpp").read_text(encoding="utf-8")
GRADLE = (ROOT / "app/build.gradle").read_text(encoding="utf-8")


class Real3DByDefault(unittest.TestCase):
    def test_flat_cave_never_enabled_automatically(self):
        self.assertRegex(JAVA, r"(?m)^\s*private volatile boolean cavePreviewWanted=false;\s*$")
        self.assertNotIn("private volatile boolean cavePreviewWanted=true;", JAVA)
        self.assertIn("std::atomic<bool> pngCavePreview{false}", NATIVE)
        self.assertIn("nativeSetCavePreview(cavePreviewReady && cavePreviewWanted)", JAVA)

    def test_2d_reference_remains_explicit_and_reversible(self):
        self.assertIn('8 irányú barlang PNG - TESZT előnézet BE/KI', JAVA)
        self.assertIn("cavePreviewWanted=!cavePreviewWanted", JAVA)
        self.assertIn("nativeSetCavePreview(cavePreviewWanted && cavePreviewReady)", JAVA)

    def test_real_geometry_is_not_replaced_by_camera_facing_card(self):
        self.assertIn("sculptedTerrainGrid(terrainMesh", SCENE)
        self.assertIn("glDrawArrays(GL_TRIANGLES,0,(GLsizei)caveMesh.size());", SCENE)
        self.assertIn("glDrawArrays(GL_TRIANGLES,0,(GLsizei)terrainMesh.size());", SCENE)
        self.assertIn("const float oldCaveAlpha = 1.f - pngCover;", SCENE)
        self.assertIn("const float pngCover = f.pngCavePreview", SCENE)

    def test_approved_menu_and_android_identity_are_untouched(self):
        self.assertIn("RestoredStoneMenu.load(this", JAVA)
        self.assertIn("applicationIdSuffix '.menufixedgame'", GRADLE)


if __name__ == "__main__":
    unittest.main()
