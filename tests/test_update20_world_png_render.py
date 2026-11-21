"""The Android game must render the existing approved-look 512px cave PNG at
native image aspect, with the alpha foot planted on the terrain. Menu unchanged.
"""
import subprocess
import tempfile
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
CAVE=(ROOT/"app/src/main/cpp/CaveSprite.cpp").read_text()
SCENE=(ROOT/"app/src/main/cpp/FastScene.cpp").read_text()
MAIN=(ROOT/"app/src/main/java/hu/taks/world/MainActivity.java").read_text()
GEOMETRY=(ROOT/"app/src/main/cpp/CaveBillboardGeometry.h").read_text()
GRADLE=(ROOT/"app/build.gradle").read_text()

class CameraAlignedCave(unittest.TestCase):
    def test_portable_proportions_for_eight_views_and_all_pitches(self):
        with tempfile.TemporaryDirectory() as temporary:
            binary=Path(temporary)/"geometry-test"
            subprocess.run(["g++","-std=c++17","-Wall","-Wextra",
                            "-Werror","-O2",str(ROOT/"tests/test_cave_billboard_geometry.cpp"),
                            "-o",str(binary)],check=True)
            subprocess.run([str(binary)],check=True)

    def test_single_source_cave_at_20m_never_stretches(self):
        self.assertIn("CaveBillboard::aligned(",CAVE)
        self.assertIn("sourceFootRow=444.f/512.f",CAVE)
        self.assertIn("constexpr float widthM=20.f",CAVE)
        self.assertNotIn("heightM=10.7f",CAVE)
        self.assertIn("int index1=index0",CAVE)
        self.assertIn("float fade=0.f",CAVE) if "float fade=0.f" in CAVE else self.assertIn("const float fade=0.f",CAVE)
        self.assertIn("glEnable(GL_DEPTH_TEST);",CAVE)
        self.assertIn("taks::CaveSprite::render(vp.a,f.yaw,f.pitch",SCENE)
        self.assertIn("camera",GEOMETRY)

    def test_world_terrain_photographic_stochastic_sampling(self):
        self.assertIn("gravelNoTile",SCENE)
        self.assertIn("gravelOffset",SCENE)
        self.assertIn("u_groundTexture",SCENE)
        self.assertIn("u_rain",SCENE)
        self.assertIn("u_mud",SCENE)
        self.assertNotIn("wPos.xz*1.41",SCENE)

    def test_ui_and_installed_app_preserved(self):
        self.assertIn("RestoredStoneMenu.load(this",MAIN)
        self.assertIn("this::newWorldFromMenu,this::showSettings",MAIN)
        self.assertIn("this::showCredits,this::confirmExit",MAIN)
        self.assertIn("nativeSetCaveInspection(caveInspecting)",MAIN)
        self.assertIn("versionCode 20",GRADLE)
        self.assertIn("applicationIdSuffix '.menufixedgame'",GRADLE)

if __name__=="__main__":
    unittest.main()
