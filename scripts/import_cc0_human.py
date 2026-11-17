#!/usr/bin/env python3
"""Try to import actual CC0 MakeHuman neutral base-body model, NO Blender or generated primitive.

Source: CharacterCreator neutral base_body.glb, MakeHuman CC0 system assets.
The existing TAKS offline TKM1 renderer currently bakes a static pose; this
import does NOT imply prehistoric anatomical authentication or rig playback.
"""
from __future__ import annotations
from pathlib import Path
from tempfile import TemporaryDirectory
import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry

from import_cc0_foliage import bake

URL="https://raw.githubusercontent.com/kblood/CharacterCreator/master/output/base_body.glb"

def main():
    s=requests.Session()
    s.mount("https://",HTTPAdapter(max_retries=Retry(total=5,connect=5,read=5,
                               status=5,backoff_factor=1.0,status_forcelist=[429,500,502,503,504])))
    s.headers["User-Agent"]="TAKSWorld-RealHumanCC0/0.9"
    with TemporaryDirectory() as td:
        file=Path(td)/"human.glb"
        with s.get(URL,timeout=(15,120),stream=True) as r:
            r.raise_for_status()
            with file.open("wb") as out:
                total=0
                for block in r.iter_content(1024*1024):
                    if block:
                        total+=len(block)
                        if total>35*1024*1024:raise RuntimeError("Unreasonably large human model")
                        out.write(block)
        if file.read_bytes()[:4]!=b"glTF":
            raise RuntimeError("Invalid GLB body; refuse placeholder")
        print("Downloaded real CC0 MakeHuman body:",total,"bytes",flush=True)
        result=bake(file,"makehuman_cc0",{
            "target_height_m":1.70,
            "allow_authored_solid_materials":True,
            "target_span_m":.75,
            "source":URL,
        })
        if result["triangles"]<5000:
            raise RuntimeError("Human model detail unexpectedly low")
        print("GENUINE MAKEHUMAN ASSET IMPORT:",result,flush=True)

if __name__=="__main__":
    main()
