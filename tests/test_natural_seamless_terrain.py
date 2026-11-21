"""Native terrain compilation + seam and material checks. No rendered picture."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
WORLD=(ROOT/"app/src/main/cpp/WorldTerrain.h").read_text(encoding="utf-8")
SCENE=(ROOT/"app/src/main/cpp/FastScene.cpp").read_text(encoding="utf-8")
MAIN=(ROOT/"app/src/main/java/hu/taks/world/MainActivity.java").read_text(encoding="utf-8")

class SeamlessWonderwerkGround(unittest.TestCase):
    def test_real_hills_and_single_conforming_3d_mesh(self):
        self.assertIn("const float kopje=15.8f*gaussian", WORLD)
        self.assertIn("const float westernRidge=", WORLD)
        self.assertIn("const float distantShoulder=", WORLD)
        self.assertIn("const float microHummocks=", WORLD)
        self.assertIn('#include "WorldMeshGrid.h"', SCENE)
        self.assertIn("WorldMeshGrid::defaultAxes()", SCENE)
        self.assertIn("dst.reserve((xs.size()-1)*(zs.size()-1)*6);", SCENE)
        self.assertIn("sculptedTerrainGrid(terrainMesh);", SCENE)
        self.assertNotIn("skipCentralRegion", SCENE)
        self.assertNotIn("farCorner", SCENE)
        self.assertNotIn("push(farCorner);", SCENE)

    def test_smooth_material_blends_use_world_position_not_8_cut_tiles(self):
        self.assertIn("float soilField(vec2 p)", SCENE)
        self.assertIn("float broad=soilField(wPos.xz)", SCENE)
        self.assertIn("float stone=smoothstep", SCENE)
        self.assertIn("float basin=1.0-smoothstep", SCENE)
        self.assertIn("base=mix(base,weathered,stone*.92)", SCENE)
        self.assertNotIn("smoothstep(.43,.14", SCENE)
        self.assertIn('loadRealPbrSurface(0,"models/ground_sandy_gravel")', MAIN)
        self.assertIn('loadRealPbrSurface(1,"models/wall_cliff_side")', MAIN)
        self.assertNotIn('loadRealPbrSurface(0,"wonderwerk/talaj")', MAIN)
        self.assertIn("private volatile boolean cavePreviewWanted=false;", MAIN)

    def test_native_terrain_compiles_and_keeps_cave_walkable(self):
        with tempfile.TemporaryDirectory() as t:
            exe=Path(t)/"terrain-test"
            subprocess.run(["g++","-std=c++17","-O2","-Wall","-Wextra","-Werror",
                            str(ROOT/"tests/test_world_terrain_3d.cpp"),"-o",str(exe)],
                            check=True)
            result=subprocess.run([str(exe)],text=True,capture_output=True,check=True)
            self.assertIn("TRUE 3D TERRAIN PASS",result.stdout)

if __name__=="__main__":
    unittest.main()
