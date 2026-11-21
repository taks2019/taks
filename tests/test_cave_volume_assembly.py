"""Compile and run new volumetric 3D entrance checks, not just source-string checks."""
import subprocess
import tempfile
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
SCENE=(ROOT/"app/src/main/cpp/FastScene.cpp").read_text(encoding="utf-8")
CAVE=(ROOT/"app/src/main/cpp/CaveVolumeReview.h").read_text(encoding="utf-8")
MAIN=(ROOT/"app/src/main/java/hu/taks/world/MainActivity.java").read_text(encoding="utf-8")

class ThreeDimensionalCaveAssembly(unittest.TestCase):
    def test_genuine_tunnel_and_no_front_card(self):
        self.assertIn('taks::CaveVolume::build(',SCENE)
        self.assertIn('matId==16',SCENE)
        self.assertIn('matId==17',SCENE)
        self.assertIn('photoDolomite(wPos,wNorm)',SCENE)
        self.assertNotIn('if(cliffOpen(midX,midY))continue',SCENE)
        self.assertIn('void build(Emit emit,Ground ground)',CAVE)
        self.assertIn('float archCeiling(float x)',CAVE)
        self.assertIn('backZ=11.75f',CAVE)

    def test_native_geometry_and_walkable_opening(self):
        with tempfile.TemporaryDirectory() as t:
            exe=Path(t)/"cave-volume-test"
            subprocess.run(["g++","-std=c++17","-O2","-Wall","-Wextra","-Werror",
                            str(ROOT/"tests/test_cave_volume_review.cpp"),"-o",str(exe)],
                           check=True)
            output=subprocess.run([str(exe)],capture_output=True,text=True,check=True)
            print(output.stdout)

    def test_menu_and_saves_remain_separate(self):
        self.assertIn("RestoredStoneMenu.load(this",MAIN)
        self.assertIn("private volatile boolean cavePreviewWanted=false;",MAIN)
        self.assertNotIn("CaveVolumeReview",MAIN)

if __name__=="__main__":
    unittest.main()
