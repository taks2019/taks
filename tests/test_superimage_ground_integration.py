"""Regression guards for the first real SuperImage terrain integration.

The original 6144x4096 PNG stays untouched inside the user's source ZIP.
This script intentionally does NOT claim visual acceptance or phone FPS proof.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
MAIN=(ROOT/"app/src/main/java/hu/taks/world/MainActivity.java").read_text(encoding="utf-8")
PACK=(ROOT/"app/src/main/java/hu/taks/world/ApprovedTerrainPack.java").read_text(encoding="utf-8")
SCENE=(ROOT/"app/src/main/cpp/FastScene.cpp").read_text(encoding="utf-8")
SCENE_H=(ROOT/"app/src/main/cpp/FastScene.h").read_text(encoding="utf-8")
JNI=(ROOT/"app/src/main/cpp/native_main.cpp").read_text(encoding="utf-8")
MENU=(ROOT/"app/src/main/java/hu/taks/world/RestoredStoneMenu.java").read_text(encoding="utf-8")

class ApprovedSuperImageIntegration(unittest.TestCase):
    def test_original_zip_not_overwritten_and_genuine_sheet_required(self):
        self.assertIn('ORIGINAL_SHEET="966856_x16_drawing.png"',PACK)
        self.assertIn('atlas.getWidth()!=3072||atlas.getHeight()!=2048',PACK)
        self.assertIn('options.inSampleSize=2',PACK)
        self.assertIn('Bitmap.createBitmap(atlas,x,y,620,620)',PACK)
        self.assertIn('ZipInputStream',PACK)
        self.assertIn('File root=context.getFilesDir()',PACK)
        self.assertNotIn('new FileOutputStream(selected',PACK)
        self.assertIn('if(!stage.renameTo(output))',PACK)
        self.assertIn('"dry.png","red.png","stone.png","sand.png","mud.png","transition.png"',PACK)
        self.assertIn('static final int[] COL={0,1,3,0,1,3}',PACK)
        self.assertIn('static final int[] ROW={0,0,0,1,1,1}',PACK)

    def test_native_seamless_3d_world_gets_six_new_materials(self):
        self.assertIn('GLuint terrainTexture[8]',SCENE)
        self.assertIn('slot<0||slot>7',SCENE)
        self.assertIn('void setApprovedTerrainActive(bool enable)',SCENE)
        self.assertIn('setApprovedTerrainActive(bool enabled)',SCENE_H)
        self.assertIn('nativeSetApprovedTerrainActive(',MAIN)
        self.assertIn('Java_hu_taks_world_MainActivity_nativeSetApprovedTerrainActive(',JNI)
        self.assertIn('approvedTriplanarRock',SCENE)
        self.assertIn('float soilField(vec2 p)',SCENE)
        self.assertIn('vec3 realSurface=',SCENE)
        self.assertIn('realSurface=mix(realSurface,wet,mudCoverage*.84);',SCENE)
        self.assertIn('glUniform1f(uApprovedTerrain,approvedTerrainActive?1.f:0.f);',SCENE)
        self.assertIn('restoreApprovedGroundOnGlThread()',MAIN)
        self.assertIn('surface.queueEvent(()->{',MAIN)
        for sampler in ("u_approvedDry","u_approvedRed","u_approvedRock",
                        "u_approvedSand","u_approvedMud","u_approvedTransition"):
            self.assertIn("uniform sampler2D "+sampler+";",SCENE)
        for slot in range(2,8):
            self.assertIn('terrainTexture[i+2]',SCENE)

    def test_camera_and_weather_frame_pacing_uses_android_vsync(self):
        self.assertIn('android.view.Choreographer.FrameCallback renderLoop',MAIN)
        self.assertIn('android.view.Choreographer.getInstance().postFrameCallback(renderLoop)',MAIN)
        self.assertIn('android.view.Choreographer.getInstance().removeFrameCallback(renderLoop)',MAIN)
        self.assertIn('private void startFrameLoop()',MAIN)
        self.assertIn('private void stopFrameLoop()',MAIN)
        self.assertNotIn('renderHandler.postDelayed(this,40)',MAIN)

    def test_same_original_menu_visible_in_settings_with_no_second_motion(self):
        self.assertIn('RestoredStoneMenu.load(this',MAIN)
        self.assertIn('"menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png"',MENU)
        self.assertIn('modalOverlay.setBackgroundColor(inMainMenu?0x32101411:0x88101411)',MAIN)
        self.assertIn('opacity(state.panelTop,220)',MAIN)
        self.assertIn('opacity(state.panelBottom,236)',MAIN)
        self.assertNotIn('installApprovedMotionOnCurrentMenu();',MAIN)
        self.assertNotIn('menuMotion=new MenuMotionView(this)',MAIN)

if __name__=="__main__": unittest.main()
