#!/usr/bin/env python3
"""Offline TAKS World terrain bake: NEVER rewrite the owner's SuperImage.zip.

Six original, approved atlas regions become 512x512 review-only texture
COPIES. They use native 3D geometry but are not themselves 3D models.
"""
from __future__ import annotations
import argparse
import hashlib
import io
import json
from pathlib import Path
import zipfile

from PIL import Image

ORIGINAL = "966856_x16_drawing.png"
SOURCE_HASH = "71600a427e5d17cc0334a07ca9a0a827ad747f2caaed326efaa2603c30d55660"
SHEET_SIZE = (6144, 4096)
NAMES = ("dry.png", "red.png", "stone.png", "sand.png", "mud.png", "transition.png")
COL = (0, 1, 3, 0, 1, 3)
ROW = (0, 0, 0, 1, 1, 1)

def derive(source):
    with zipfile.ZipFile(source) as archive:
        matches = [e for e in archive.infolist()
                   if not e.is_dir() and Path(e.filename).name == ORIGINAL]
        if len(matches) != 1:
            raise ValueError("Az elfogadott 966856_x16_drawing.png hiányzik vagy többször szerepel.")
        original_bytes = archive.read(matches[0])
    if hashlib.sha256(original_bytes).hexdigest() != SOURCE_HASH:
        raise ValueError("Nem az elfogadott eredeti atlasz. Semmit nem írunk felül.")
    with Image.open(io.BytesIO(original_bytes)) as im:
        if im.size != SHEET_SIZE:
            raise ValueError("A forrásméret nem 6144x4096.")
        half = im.convert("RGB").resize((3072, 2048), Image.Resampling.BILINEAR)
    tiles = {}
    descriptions = []
    for name, col, row in zip(NAMES, COL, ROW):
        x, y = col * 768 + 74, row * 1024 + 132
        with half.crop((x, y, x + 620, y + 620)) as patch:
            tile = patch.resize((512, 512), Image.Resampling.LANCZOS)
        try:
            result = io.BytesIO()
            tile.save(result, format="PNG", optimize=True)
            data = result.getvalue()
        finally:
            tile.close()
        tiles[name] = data
        descriptions.append({"file": name, "crop_in_halfsize": [x, y, 620, 620],
                             "sha256": hashlib.sha256(data).hexdigest()})
    manifest = {"source_png": ORIGINAL, "source_sha256": SOURCE_HASH,
                "source_zip_unchanged": True, "owner_final_graphics_approval": False,
                "source_image_dimensions": list(SHEET_SIZE),
                "derivative_texture_dimensions": [512, 512], "tiles": descriptions}
    return tiles, manifest

def bake(source, output):
    tiles, manifest = derive(source)
    output.mkdir(parents=True, exist_ok=True)
    entries = dict(tiles)
    entries["approved_ground_manifest.json"] = (
        json.dumps(manifest, sort_keys=True, indent=2, ensure_ascii=False) + "\n"
    ).encode("utf-8")
    for name, data in entries.items():
        target = output / name
        if target.exists() and target.read_bytes() != data:
            raise FileExistsError(f"Másik grafika már létezik, nem írom felül: {target}")
    for name, data in entries.items():
        target = output / name
        if not target.exists():
            target.write_bytes(data)
    return manifest

def main():
    parser = argparse.ArgumentParser(description="TAKS World: eredeti SuperImage.zip -> 6 változatlan forrású talajmásolat")
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--out", type=Path, default=Path("app/src/main/assets/approved_ground"))
    args = parser.parse_args()
    bake(args.source, args.out)
    print("KÉSZ: 6 elfogadott SuperImage-talajminta másolata, 512x512; eredeti ZIP változatlan.")
    print("Ezek csak ellenőrzendő játékbeli textúrák; a végső grafika nincs jóváhagyva.")

if __name__ == "__main__":
    main()
