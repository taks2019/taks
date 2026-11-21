"""Fail a screenshot with a blank native world behind colourful HUD."""
import sys, unittest
from pathlib import Path
from PIL import Image, ImageDraw

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"scripts"))
from visual_surface_gate import neutral_blank_fraction, require_visible_world, world_view_difference

class NativeWorldScreenshot(unittest.TestCase):
    def test_rejects_blank_world_even_when_hud_draws(self):
        img=Image.new("RGB",(2400,1080),(48,48,48))
        brush=ImageDraw.Draw(img)
        brush.rectangle((0,0,2399,140),fill=(165,134,72))
        brush.rectangle((0,930,2399,1079),fill=(30,120,150))
        self.assertGreater(neutral_blank_fraction(img),.98)
        with self.assertRaisesRegex(AssertionError,"szürke"):
            require_visible_world(img)

    def test_accepts_actual_varied_scene_not_merely_hud(self):
        img=Image.new("RGB",(1000,500))
        px=img.load()
        for y in range(img.height):
            for x in range(img.width):
                px[x,y]=(32+(x//5)%75,21+(y//4)%65,10+(x//13)%20)
        self.assertLess(neutral_blank_fraction(img),.01)
        require_visible_world(img)

    def test_also_rejects_neutral_untextured_plane(self):
        img=Image.new("RGB",(840,480),(55,55,55))
        self.assertGreater(neutral_blank_fraction(img),.9)
        with self.assertRaises(AssertionError):
            require_visible_world(img)


    def test_camera_orbit_comparison_excludes_static_hud(self):
        old=Image.new("RGB",(1000,500),(71,58,36))
        new=old.copy()
        brush=ImageDraw.Draw(new)
        brush.rectangle((0,0,1000,60),fill=(230,55,25))
        brush.rectangle((0,420,1000,500),fill=(25,160,200))
        self.assertAlmostEqual(world_view_difference(old,new),0.0)
        brush.rectangle((340,150,620,335),fill=(120,140,35))
        self.assertGreater(world_view_difference(old,new),8.0)

if __name__=="__main__":
    unittest.main()
