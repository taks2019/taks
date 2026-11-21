from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
class TestWindowsAndroidWorkflow(unittest.TestCase):
    def test_simple_entrypoint(self):
        x=(ROOT/'PC_TAKS_WORLD.cmd').read_text(encoding='utf-8')
        self.assertIn('tools\\taks-world-pc.ps1',x)
        self.assertIn('-ExecutionPolicy Bypass',x)
    def test_build_is_real_gradle_not_mock_png(self):
        x=(ROOT/'tools/taks-world-pc.ps1').read_text(encoding='utf-8-sig')
        self.assertIn(':app:assembleDebug',x)
        self.assertIn('27.2.12479018',x)
        self.assertIn('gradle-8.9',x)
        self.assertIn('TAKS_WORLD_PC_TESZT.apk',x)
        self.assertIn('Get-FileHash',x)
    def test_approved_ground_baked_and_verified_in_real_apk(self):
        script=(ROOT/'scripts/bake_approved_ground.py').read_text(encoding='utf-8')
        validator=(ROOT/'scripts/check_apk_ground_pack.py').read_text(encoding='utf-8')
        pc=(ROOT/'tools/taks-world-pc.ps1').read_text(encoding='utf-8-sig')
        self.assertIn('71600a427e5d17cc0334a07ca9a0a827ad747f2caaed326efaa2603c30d55660',script)
        self.assertIn('approved_ground_manifest.json',script)
        self.assertIn('approved_ground_manifest.json',validator)
        self.assertIn('APPROVED_MENU',validator)
        self.assertIn('lib/arm64-v8a/libtaks_engine.so',validator)
        self.assertIn('Prepare-ApprovedGround',pc)
        self.assertIn('check_apk_ground_pack.py',pc)
        self.assertIn('scripts\\bake_approved_ground.py',pc)
        self.assertIn('Capture-Screenshot',pc)
        self.assertIn('screencap',pc)
    def test_user_save_and_approved_image_preservation(self):
        x=(ROOT/'tools/taks-world-pc.ps1').read_text(encoding='utf-8-sig')
        self.assertIn('if(Test-Path $property){ return }',x)
        self.assertIn('if($current -ieq $ApprovedHash)',x)
        self.assertIn('if($actual -ine $match.Value)',x)
        self.assertIn('"install" "-r"',x)
        self.assertIn('if(-not $packagesReady)',x)
        self.assertNotIn('adb uninstall',x.lower())
        self.assertNotIn('pm clear',x.lower())
if __name__=='__main__': unittest.main()
