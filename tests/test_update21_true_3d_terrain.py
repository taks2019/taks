"""Proof that the TAKS Wonderwerk terrain is actual CPU/GPU 3D geology,
not just another brown texture, billboard or screen-sized quad.
"""
import subprocess
import tempfile
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
ENGINE=(ROOT/"app/src/main/cpp/FastScene.cpp").read_text(encoding="utf-8")
MODEL=(ROOT/"app/src/main/cpp/RealModels.cpp").read_text(encoding="utf-8")
TERRAIN=(ROOT/"app/src/main/cpp/WorldTerrain.h").read_text(encoding="utf-8")
GRADLE=(ROOT/"app/build.gradle").read_text(encoding="utf-8")
MENU=(ROOT/"app/src/main/java/hu/taks/world/MainActivity.java").read_text(encoding="utf-8")

class Generated3DGround(unittest.TestCase):
    def test_real_height_mesh_and_two_geometric_lods(self):
        self.assertIn("sculptedTerrainGrid(terrainMesh,-120.f,120.f,-120.f,120.f",ENGINE)
        self.assertIn("sculptedTerrainGrid(terrainMesh,-28.f,28.f,-28.f,40.f",ENGINE)
        self.assertIn("glDrawArrays(GL_TRIANGLES,0,(GLsizei)terrainMesh.size());",ENGINE)
        self.assertIn("WorldTerrain::normal",ENGINE)
        self.assertNotIn("quad(terrainMesh,{-160.f,-.485f,-160.f}",ENGINE)
        self.assertIn("photoDolomite",ENGINE)
        self.assertIn("geologicalScree",ENGINE)
        self.assertIn("wetChannel",ENGINE)
        self.assertIn("mudCoverage",ENGINE)
        self.assertIn("soilField(wPos.xz)",ENGINE)

    def test_cave_actors_and_mesh_share_one_world_height(self):
        self.assertIn('#include "WorldTerrain.h"',ENGINE)
        self.assertIn('#include "WorldTerrain.h"',MODEL)
        self.assertIn("return taks::WorldTerrain::height(x,z);",ENGINE)
        self.assertIn("return taks::WorldTerrain::height(x,z);",MODEL)
        self.assertIn("gaussian(x,z,-6.8f,6.2f",TERRAIN)
        self.assertIn("gaussian(x,z,6.5f,6.8f",TERRAIN)
        self.assertIn("const float corridor=gaussian",TERRAIN)
        self.assertIn("return y;",TERRAIN)

    def test_real_compiled_geometry_math_and_walkability(self):
        with tempfile.TemporaryDirectory() as td:
            executable=Path(td)/"terrain-native-test"
            subprocess.run([
                "g++","-std=c++17","-O2","-Wall","-Wextra","-Werror",
                str(ROOT/"tests/test_world_terrain_3d.cpp"),"-o",str(executable)
            ],check=True)
            result=subprocess.run([str(executable)],capture_output=True,text=True,check=True)
            print(result.stdout)

    def test_inplace_update_not_a_new_app_or_redesigned_menu(self):
        self.assertIn("versionCode 21",GRADLE)
        self.assertIn("versionName '0.15.6'",GRADLE)
        self.assertIn("applicationIdSuffix '.menufixedgame'",GRADLE)
        self.assertIn("RestoredStoneMenu.load(this",MENU)
        self.assertIn("BARLANG BELSEJE",MENU)

if __name__=="__main__":
    unittest.main()
