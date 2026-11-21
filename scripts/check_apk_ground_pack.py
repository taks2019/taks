#!/usr/bin/env python3
"""Fail a PC-built TAKS APK unless the SAME six verified approved textures shipped.

This checks the assembled APK bytes, not source folder presence. It never
claims that image slices constitute an approved photorealistic 3D cave.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

APPROVED_ORIGINAL = "71600a427e5d17cc0334a07ca9a0a827ad747f2caaed326efaa2603c30d55660"
APPROVED_MENU = "9369d3668b69424d31132e70accbfdb76282d7d9b3f3adfaa51c7c3ca1ea7bc8"
FILES = ("dry.png", "red.png", "stone.png", "sand.png", "mud.png", "transition.png")
PREFIX = "assets/approved_ground/"

def validate_apk(apk: Path) -> dict:
    if not apk.is_file():
        raise FileNotFoundError(f"Az APK nem létezik: {apk}")
    with zipfile.ZipFile(apk) as archive:
        available = set(archive.namelist())
        menu = "assets/menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png"
        if menu not in available or hashlib.sha256(archive.read(menu)).hexdigest() != APPROVED_MENU:
            raise ValueError("A korábban elfogadott főmenükép hiányzik vagy megváltozott.")
        manifest_name = PREFIX + "approved_ground_manifest.json"
        if manifest_name not in available:
            raise ValueError("A hat eredeti SuperImage-talaj NINCS az APK-ban (barna paca visszatérhet).")
        manifest = json.loads(archive.read(manifest_name).decode("utf-8"))
        if manifest.get("source_sha256") != APPROVED_ORIGINAL:
            raise ValueError("Az APK más forrásból készült talajgrafikákat tartalmaz.")
        rows = manifest.get("tiles", [])
        if len(rows) != len(FILES) or {r["file"] for r in rows} != set(FILES):
            raise ValueError("A hat talajminta listája hiányos vagy hibás.")
        for item in rows:
            name = item["file"]
            full_path = PREFIX + name
            if full_path not in available:
                raise ValueError("Az APK nem tartalmazza a talajt: " + name)
            data = archive.read(full_path)
            if hashlib.sha256(data).hexdigest() != item["sha256"]:
                raise ValueError("Módosult PNG az APK-ban: " + name)
            if len(data) < 24 or not data.startswith(b"\x89PNG\r\n\x1a\n"):
                raise ValueError("Nem PNG textúra: " + name)
            if struct.unpack(">II",data[16:24]) != (512,512):
                raise ValueError("Nem 512x512-as, Androidhoz méretezett textúra: " + name)
        if "lib/arm64-v8a/libtaks_engine.so" not in available:
            raise ValueError("Az APK nem tartalmazza a Galaxy A56 ARM64 3D motorját.")
        return {"png_count":6, "menu_unchanged":True,
                "source_sha256":APPROVED_ORIGINAL,
                "native_arm64_present":True,
                "user_graphic_approval":"review_only"}

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--apk",type=Path,required=True)
    args=parser.parse_args()
    result=validate_apk(args.apk)
    print("APK BYTE-CHECK OK:", json.dumps(result,ensure_ascii=False))
    print("NEM jelenti a végleges grafika jóváhagyását vagy a valós telefonos FPS igazolását.")

if __name__=="__main__":
    main()
