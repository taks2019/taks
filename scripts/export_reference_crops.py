"""Re-export the ten original isometric reference regions at their source resolution."""
import json
from pathlib import Path

from PIL import Image

directory = Path(__file__).resolve().parents[1] / "art" / "reference_crops"
regions = json.loads((directory / "regions.json").read_text(encoding="utf-8"))
with Image.open(directory / "Wonderwerk_etalon.jpg") as source:
    for name, bounds in regions.items():
        source.crop(bounds).save(directory / name, format="PNG")
