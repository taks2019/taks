#!/usr/bin/env python3
"""Fetch real CC0 photo-scanned stone meshes and bake them for offline TAKS Engine.

This is a BUILD-TIME tool, not a runtime downloader. GitHub Actions embeds both
the photogrammetry triangles and the authentic diffuse texture in the Android APK.
License: the selected individual Poly Haven models are CC0 (see source pages).
"""
from __future__ import annotations
import hashlib
import json
import os
from pathlib import Path
import struct
from tempfile import TemporaryDirectory
from urllib.parse import urlparse
import requests
import numpy as np
import trimesh
from PIL import Image

API = "https://api.polyhaven.com"
OUT = Path("app/src/main/assets/models")
SELECTION = {
    # CC0, real photo-based rocks; NOT a scan/reconstruction of Wonderwerk itself.
    "rock_07": {"target_span_m": .45, "source": "https://polyhaven.com/a/rock_07"},
    "namaqualand_boulders_01": {
        "target_span_m": 1.5,
        "source": "https://polyhaven.com/a/namaqualand_boulders_01"
    },
    "namaqualand_cliff_02": {
        "target_span_m": 7.0,
        "source": "https://polyhaven.com/a/namaqualand_cliff_02"
    },
}
MAX_MB = 96
PBR_MATERIALS = {
    "sandy_gravel_02": "ground_sandy_gravel",
    "cliff_side": "wall_cliff_side"
}
MAX_TRIANGLES = 180000

def download(url: str, dest: Path, session: requests.Session, md5=None):
    assert url.startswith("https://"), "Only trusted HTTPS asset download URLs"
    dest.parent.mkdir(parents=True, exist_ok=True)
    size=0
    digest=hashlib.md5()
    with session.get(url, stream=True, timeout=(20,150)) as r:
        r.raise_for_status()
        with dest.open("wb") as out:
            for block in r.iter_content(1024*1024):
                if block:
                    size += len(block)
                    if size>MAX_MB*1024*1024:
                        raise RuntimeError(f"Unexpected huge model dependency: {url}")
                    digest.update(block)
                    out.write(block)
    if md5 and digest.hexdigest().lower()!=md5.lower():
        raise RuntimeError(f"Source checksum mismatch! {dest.name}")
    print(f"Downloaded {dest.name}, {size/1024/1024:.2f} MiB", flush=True)

def validate_path(base: Path, path: str) -> Path:
    p=Path(path.replace("\\","/"))
    if p.is_absolute() or ".." in p.parts:
        raise ValueError(f"Unsafe GLTF include path: {path}")
    dest=(base/p).resolve()
    if not dest.is_relative_to(base.resolve()):
        raise ValueError(f"GLTF include path escapes directory: {path}")
    return dest

def choose_gltf(metadata: dict) -> dict:
    formats=metadata.get("gltf")
    if not isinstance(formats,dict):
        raise RuntimeError("The asset has no glTF model download")
    for resolution in ("1k","2k","4k"):
        item=formats.get(resolution)
        if isinstance(item,dict) and "gltf" in item:
            entry=item["gltf"]
            if isinstance(entry,dict) and entry.get("url"):
                return entry
    raise RuntimeError(f"Unrecognized Poly Haven glTF data: {list(formats)}")

def collect_asset(slug: str, settings: dict, session: requests.Session, workdir: Path):
    r=session.get(f"{API}/files/{slug}",timeout=40)
    r.raise_for_status()
    variants=r.json()
    model=choose_gltf(variants)
    assetdir=workdir/slug
    assetdir.mkdir(exist_ok=True)
    url=model["url"]
    name=Path(urlparse(url).path).name
    main=assetdir/name
    download(url,main,session,model.get("md5"))
    includes=model.get("include") or {}
    print(f"{slug}: {len(includes)} dependencies",flush=True)
    for rel,item in includes.items():
        if isinstance(item,str):
            depurl=item;check=None
        else:
            depurl=item.get("url");check=item.get("md5")
        if not depurl:raise RuntimeError(f"Missing URL for {rel}")
        download(depurl,validate_path(assetdir,rel),session,check)
    gltf=json.loads(main.read_text(encoding="utf-8"))
    # All GLTF-referenced textures / bin files must actually exist.
    for spec in list(gltf.get("buffers",[]))+list(gltf.get("images",[])):
        uri=spec.get("uri")
        if uri and not uri.startswith("data:"):
            validate_path(assetdir,uri).relative_to(assetdir)
            if not validate_path(assetdir,uri).is_file():
                raise FileNotFoundError(f"{slug} glTF references missing: {uri}")
    return main

def convert_model(gltf: Path, slug: str, settings: dict):
    scene=trimesh.load(str(gltf),force="scene",process=False,maintain_order=True)
    meshes=[m for m in scene.dump() if isinstance(m,trimesh.Trimesh)]
    meshes.sort(key=lambda m:len(m.faces),reverse=True)
    meshes=[m for m in meshes if m.faces.shape[0]>0]
    if not meshes:raise RuntimeError(f"No triangle geometry in {slug}")
    # Single textured submesh for first milestone; no hidden stand-in primitives.
    best=None
    for mesh in meshes:
        uv=getattr(mesh.visual,"uv",None)
        material=getattr(mesh.visual,"material",None)
        image=getattr(material,"baseColorTexture",None) if material is not None else None
        if image is None and material is not None:
            image=getattr(material,"image",None)
        if uv is not None and len(uv)==len(mesh.vertices) and image is not None:
            best=(mesh,np.asarray(uv),image)
            break
    if best is None:
        raise RuntimeError(f"{slug}: no properly UV-mapped, textured 3D mesh: not accepted as a real textured model")
    mesh,uv,image=best
    total=int(len(mesh.faces))
    if total>MAX_TRIANGLES:
        # Do not silently hollow the mesh by dropping triangles. Require a real LOD.
        raise RuntimeError(f"{slug}: {total} faces exceed mobile import budget {MAX_TRIANGLES}; choose a real LOD")
    print(f"{slug}: real textured mesh {len(mesh.vertices)} vertices, {total} triangles",flush=True)
    pos=np.asarray(mesh.vertices,dtype=np.float32)
    normals=np.asarray(mesh.vertex_normals,dtype=np.float32)
    tex=np.asarray(uv,dtype=np.float32)
    xyzSpan=np.maximum(pos.max(axis=0)-pos.min(axis=0),1.e-5)
    # Poly Haven models are in meters; normalize to a stable local X/Z footprint.
    maxspan=float(max(xyzSpan[0],xyzSpan[2]))
    scale=settings["target_span_m"]/maxspan
    pos=pos*scale
    pos[:,0]-=.5*(pos[:,0].min()+pos[:,0].max())
    pos[:,2]-=.5*(pos[:,2].min()+pos[:,2].max())
    pos[:,1]-=pos[:,1].min()
    # glTF UV is top-origin; BitmapFactory pixel uploads are top-first.
    tex[:,1]=1.-tex[:,1]
    ids=np.asarray(mesh.faces,dtype=np.int64).reshape(-1)
    packed=np.concatenate((pos[ids],normals[ids],tex[ids]),axis=1).astype("<f4")
    if not np.isfinite(packed).all():
        raise RuntimeError(f"{slug}: non-finite vertex data")
    data=b"TKM1"+struct.pack("<I",len(ids))+packed.tobytes()
    OUT.mkdir(parents=True,exist_ok=True)
    model_path=OUT/f"{slug}.tkm"
    model_path.write_bytes(data)

    image=Image.fromarray(np.asarray(image)).convert("RGB")
    image.thumbnail((1024,1024),Image.Resampling.LANCZOS)
    # Pack a real UV-dependent diffuse map for offline Android use.
    png=OUT/f"{slug}.png"
    image.save(png,"PNG",optimize=True)
    print(f"  baked {model_path}: {len(data)/1024/1024:.1f} MB; {png}: {png.stat().st_size/1024:.0f} KB",flush=True)
    if image.width<256 or image.height<256:
        raise RuntimeError(f"{slug}: texture is unexpectedly low-resolution")
    return {"name":slug,"url":settings["source"],"license":"CC0",
            "triangles":total,"material":"UV mapped photogrammetry albedo",
            "texture_px":[image.width,image.height],"model_sha256":hashlib.sha256(data).hexdigest(),
            "texture_sha256":hashlib.sha256(png.read_bytes()).hexdigest()}

def fetch_terrain_material(slug: str, name: str, session: requests.Session, base: Path):
    """Import an actually tileable photograph-based PBR albedo, not a color fill."""
    r=session.get(f"{API}/files/{slug}",timeout=50)
    r.raise_for_status()
    info=r.json()
    diffuse=next((k for k in info if k.lower() in ("diffuse","diff","albedo","basecolor")),None)
    if diffuse is None:
        raise RuntimeError(f"{slug}: no diffuse PBR map available: {list(info)}")
    detail=info[diffuse]
    variant=detail.get("1k") or detail.get("2k")
    if not isinstance(variant,dict):
        raise RuntimeError(f"{slug}: no 1k/2k ground material map")
    entry=variant.get("jpg") or variant.get("png")
    if not isinstance(entry,dict) or not entry.get("url"):
        raise RuntimeError(f"{slug}: no photographic JPG/PNG texture URL")
    target=base/(name+"_origin"+Path(urlparse(entry["url"]).path).suffix)
    download(entry["url"],target,session,entry.get("md5"))
    image=Image.open(target).convert("RGB")
    image.thumbnail((1024,1024),Image.Resampling.LANCZOS)
    namefile=OUT/f"{name}.png"
    image.save(namefile,"PNG",optimize=True)
    if image.width<500 or image.height<500:
        raise RuntimeError(f"{slug}: real terrain texture quality too low")
    print(f"True CC0 terrain photograph: {slug}, {image.width}x{image.height}",flush=True)
    return {"name":name,"source":f"https://polyhaven.com/a/{slug}","license":"CC0",
            "image_px":[image.width,image.height],
            "texture_sha256":hashlib.sha256(namefile.read_bytes()).hexdigest()}

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    session=requests.Session()
    session.headers.update({"User-Agent":"TAKSWorld-OfflineAssets/0.8 (GitHub Actions; non-commercial research prototype)"})
    manifest={"version":2,"purpose":"authentic scanned mobile geology props, NOT authentic Wonderwerk survey","models":[],"materials":[]}
    with TemporaryDirectory() as scratch:
        for slug,settings in SELECTION.items():
            model=collect_asset(slug,settings,session,Path(scratch))
            manifest["models"].append(convert_model(model,slug,settings))
        for slug,name in PBR_MATERIALS.items():
            manifest["materials"].append(fetch_terrain_material(slug,name,session,Path(scratch)))
    (OUT/"manifest.json").write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding="utf-8")
    assert len(manifest["models"])==3
    assert len(manifest["materials"])==2
    print("TAKS REAL CC0 SCANNED 3D MODEL IMPORT PASSED",flush=True)

if __name__=="__main__":
    main()
