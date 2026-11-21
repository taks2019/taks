"""Never silently replace the accepted menu controls or 3D world with a still fake."""
from pathlib import Path
import ast
import subprocess
import shutil
import unittest
import numpy as np

MAIN = Path("app/src/main/java/hu/taks/world/MainActivity.java").read_text(encoding="utf-8")
VIDEO = Path("app/src/main/java/hu/taks/world/MenuMotionView.java").read_text(encoding="utf-8")
SCRIPT = Path("scripts/build_wonderwerk_menu_video.py").read_text(encoding="utf-8")
CLIP = Path("app/src/main/assets/menu/wonderwerk_loop.mp4")


class MenuAnimation(unittest.TestCase):
    def test_menu_buttons_and_fallback_unchanged(self):
        self.assertIn('String[] labels={"ÚJ JÁTÉK","BEÁLLÍTÁSOK","KÉSZÍTŐK","KILÉPÉS"};', MAIN)
        self.assertIn('menuCover.addView(backdrop,new FrameLayout.LayoutParams(-1,-1));', MAIN)
        self.assertIn('new MenuMotionView(this)', MAIN)
        self.assertIn('menuCover.addView(menuMotion,new FrameLayout.LayoutParams(-1,-1));', MAIN)
        self.assertIn('menuCover.addView(stack,p)', MAIN)
        self.assertNotIn('gameplayLogo', MAIN)

    def test_real_video_not_fake_static_android_overlay(self):
        self.assertIn('extends TextureView', VIDEO)
        self.assertIn('mp.setLooping(true)', VIDEO)
        self.assertIn('mp.prepareAsync()', VIDEO)
        self.assertIn('MEDIA_INFO_VIDEO_RENDERING_START', VIDEO)
        self.assertIn('if(menuMotion!=null)menuMotion.setPlaying(false)', MAIN)
        self.assertIn('if(menuMotion!=null)menuMotion.setPlaying(true)', MAIN)
        self.assertIn('menuMotion.release()', MAIN)
        self.assertNotIn('root.addView(caveOverlay', MAIN)

    def test_video_generator_is_valid_python(self):
        ast.parse(SCRIPT)
        self.assertIn('shifted = base[yy', SCRIPT)
        self.assertIn('Image.alpha_composite', SCRIPT)
        self.assertIn('"libx264"', SCRIPT)

    @unittest.skipUnless(CLIP.is_file() and shutil.which('ffmpeg'), "MP4 is generated during APK build")
    def test_menu_clip_has_actual_temporal_movement(self):
        def frame(second):
            data = subprocess.check_output([
                'ffmpeg', '-hide_banner', '-loglevel', 'error', '-ss', str(second),
                '-i', str(CLIP), '-frames:v', '1', '-f', 'rawvideo',
                '-pix_fmt', 'rgb24', 'pipe:1'
            ])
            return np.frombuffer(data, np.uint8).astype(np.int16)
        earlier, later = frame(0), frame(1.5)
        self.assertEqual(len(earlier), len(later))
        self.assertGreater(np.abs(earlier - later).mean(), .8)


if __name__ == "__main__":
    unittest.main()
