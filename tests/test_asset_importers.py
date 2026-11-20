#!/usr/bin/env python3
"""Validate source GLTF skin color and imported mobile plant dimensions."""
from __future__ import annotations

import json
from pathlib import Path
import struct
import sys

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from import_cc0_rigged_human import apply_base_color_factor


def main() -> None:
    base = ROOT / "app/src/main/assets/models"
    source = json.loads((base / "makehuman_rigged_source.json")
                        .read_text(encoding="utf-8"))
    factor = source["skin_base_color_factor"]
    assert len(factor) == 4 and factor[0] > factor[1] > factor[2]
    sample = apply_base_color_factor(Image.new("RGBA", (1, 1), (255, 255, 255, 255)), factor)
    red, green, blue, alpha = sample.getpixel((0, 0))
    assert red > green > blue and alpha == 255, (red, green, blue, alpha)

    manifest = json.loads((base / "foliage_manifest.json").read_text(encoding="utf-8"))
    grass = next(model for model in manifest["models"] if model["name"] == "grass_bermuda_01")
    assert .40 <= grass["normalized_height_m"] <= .44, grass
    assert .98 <= grass["normalized_horizontal_span_m"] <= 1.02, grass
    encoded = (base / "grass_bermuda_01.tkm").read_bytes()
    assert encoded[:4] == b"TKM1"
    count = struct.unpack_from("<I", encoded, 4)[0]
    vertices = np.frombuffer(encoded, dtype="<f4", offset=8).reshape(count, 8)
    height = float(vertices[:, 1].max() - vertices[:, 1].min())
    width = float(max(np.ptp(vertices[:, 0]), np.ptp(vertices[:, 2])))
    assert .40 <= height <= .44 and .98 <= width <= 1.02, (height, width)
    print(f"PASS: GLTF skin factor={factor}; grass={width:.2f}m x {height:.2f}m")


if __name__ == "__main__":
    main()
