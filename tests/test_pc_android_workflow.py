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
    def test_user_save_and_approved_image_preservation(self):
        x=(ROOT/'tools/taks-world-pc.ps1').read_text(encoding='utf-8-sig')
        self.assertIn('if(Test-Path $property){ return }',x)
        self.assertIn('if($current -ieq $ApprovedHash)',x)
        self.assertIn('if($actual -ine $match.Value)',x)
        self.assertIn('"install" "-r"',x)
        self.assertNotIn('adb uninstall',x.lower())
        self.assertNotIn('pm clear',x.lower())
if __name__=='__main__': unittest.main()
