"""Visual gate for the actual native game WORLD, excluding Android controls.

A real screenshot formerly passed because HUD text/buttons contributed enough
contrast while the entire background was exactly #303030. Only check the broad
central playfield; HUD decorations must never be counted as visible 3D.
This detects blank-screen regressions, not photorealism or asset approval.
"""
from PIL import Image

def neutral_blank_fraction(frame):
    if not isinstance(frame, Image.Image):
        raise TypeError("Expected an actual Pillow screenshot image")
    image=frame.convert("RGB")
    w,h=image.size
    if w<320 or h<180:
        raise AssertionError("Android gameplay screenshot too small")
    # Excludes top labels, left movement pad and the bottom HUD.
    scene=image.crop((int(w*.18),int(h*.20),int(w*.82),int(h*.73)))
    sampled=scene.resize((160,90),Image.Resampling.NEAREST)
    colors=sampled.getdata()
    count=0
    for rgb in colors:
        r,g,b=rgb
        if max(rgb)-min(rgb)<=3 and 24 <= (r+g+b)/3 <=96:
            count+=1
    return count/(160*90)

def require_visible_world(frame):
    blank=neutral_blank_fraction(frame)
    if blank>.87:
        raise AssertionError(
            "A játéktér továbbra is üres/szürke; a HUD nem bizonyítja, "
            f"hogy a valós 3D világ megjelent. Üres képrész: {blank:.1%}"
        )
    return blank

def world_view_difference(a,b):
    """Compare native game pixels, excluding static Android HUD."""
    from PIL import ImageChops, ImageStat
    if a.size!=b.size:
        raise AssertionError("Device changed orientation during orbit")
    w,h=a.size
    bounds=(int(w*.26),int(h*.19),int(w*.74),int(h*.71))
    diff=ImageChops.difference(a.convert("RGB").crop(bounds),
                               b.convert("RGB").crop(bounds))
    return float(sum(ImageStat.Stat(diff).mean)/3)
