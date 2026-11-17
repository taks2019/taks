#!/usr/bin/env python3
"""Bake genuine Poly Haven CC0 multi-mesh plants to TAKS TKM1 + RGBA atlas.

No runtime download, no primitive replacement, no generic model generation.
Every authored textured mesh is included; oversized or unsupported assets FAIL
instead of silently discarding leaves. References aren't approved fauna/flora
for Pleistocene Wonderwerk until archaeological/biome review is complete.
"""
from __future__ import annotations
import hashlib
import json
import math
from pathlib import Path
import struct
from tempfile import TemporaryDirectory

import numpy as np
from PIL import Image
import requests
import trimesh

from import_cc0_rocks import collect_asset, OUT

SELECTION = {
    "grass_bermuda_01": {"target_span_m": 1.0, "source":"https://polyhaven.com/a/grass_bermuda_01",
                         "role":"living grazing biomass"},
    "fern_02": {"target_span_m": 1.4, "source":"https://polyhaven.com/a/fern_02",
                "role":"ferns (render only when species verified)"},
}
MAX_TRIANGLES = 180_000
MAX_PARTS = 256
TILE = 512


def bake(gltf: Path, slug: str, spec: dict):
    scene = trimesh.load(str(gltf), force="scene", process=False, maintain_order=True)
    parts = [mesh for mesh in scene.dump() if isinstance(mesh, trimesh.Trimesh) and len(mesh.faces)]
    if not parts:
        raise RuntimeError(f"{slug}: no actual triangles")
    if len(parts) > MAX_PARTS:
        raise RuntimeError(f"{slug}: {len(parts)} meshes exceed {MAX_PARTS}; needs expert LOD")
    total = sum(len(m.faces) for m in parts)
    if total > MAX_TRIANGLES:
        raise RuntimeError(f"{slug}: {total} triangles exceed {MAX_TRIANGLES} mobile budget; requires real LOD")
    if total < 100:
        raise RuntimeError(f"{slug}: too little geometry for requested detailed plants")

    entries = []
    for index,mesh in enumerate(parts):
        uv = getattr(mesh.visual, "uv", None)
        material = getattr(mesh.visual, "material", None)
        img = getattr(material, "baseColorTexture", None) if material is not None else None
        if img is None and material is not None:
            img = getattr(material, "image", None)
        if uv is None or img is None or len(uv) != len(mesh.vertices):
            raise RuntimeError(f"{slug}: part {index} has no real UV photographic texture")
        tex = np.asarray(uv, dtype=np.float32).copy()
        if not np.isfinite(tex).all() or np.min(tex) < -.001 or np.max(tex) > 1.001:
            raise RuntimeError(f"{slug}: part {index} requires complex repeat UVs; cannot atlas safely")
        texture = Image.fromarray(np.asarray(img)).convert("RGBA")
        if min(texture.size) < 32:
            raise RuntimeError(f"{slug}: part {index} texture is too small")
        entries.append((mesh, tex, texture))
    # Deduplicate image materials; preserve every genuine mesh.
    unique = {}
    group = []
    for _,_,im in entries:
        digest = hashlib.sha256(im.tobytes()).hexdigest()
        if digest not in unique: unique[digest] = (len(unique),im)
        group.append(unique[digest][0])
    cols = math.ceil(math.sqrt(len(unique)))
    rows = math.ceil(len(unique)/cols)
    if cols*TILE > 2048 or rows*TILE > 2048:
        raise RuntimeError(f"{slug}: too many individual photographed materials")
    atlas = Image.new("RGBA", (cols*TILE,rows*TILE), (0,0,0,0))
    for index,im in [item for item in unique.values()]:
        # One pixel bleed guard: UV stays within each tile.
        fitted = im.resize((TILE-4,TILE-4),Image.Resampling.LANCZOS)
        x=(index%cols)*TILE+2; y=(index//cols)*TILE+2
        atlas.paste(fitted,(x,y))
    all_positions = np.concatenate([np.asarray(m.vertices,dtype=np.float32) for m,_,_ in entries])
    bbox_min = all_positions.min(axis=0)
    bbox_max = all_positions.max(axis=0)
    span = np.maximum(bbox_max-bbox_min,1e-6)
    # Leaf-bearing models typically have Y-up, just like the existing TKM model path.
    scale = (spec["target_height_m"]/float(span[1]) if "target_height_m" in spec
             else spec["target_span_m"]/float(max(span[0],span[2])))
    anchor=(bbox_min+bbox_max)*0.5
    vertex_bytes = []
    alpha_present = False
    for (mesh,uv,texture),material_slot in zip(entries,group):
        pos = np.asarray(mesh.vertices,dtype=np.float32).copy()
        pos[:,0] = (pos[:,0]-anchor[0])*scale
        pos[:,2] = (pos[:,2]-anchor[2])*scale
        pos[:,1] = (pos[:,1]-bbox_min[1])*scale
        normals = np.asarray(mesh.vertex_normals,dtype=np.float32)
        tex = uv.copy()
        # Maintain the existing Android BitmapFactory/gltf V flip convention.
        tex[:,1] = 1.-tex[:,1]
        tex[:,0] = ((material_slot%cols)*TILE+2+tex[:,0]*(TILE-4))/(cols*TILE)
        tex[:,1] = ((material_slot//cols)*TILE+2+tex[:,1]*(TILE-4))/(rows*TILE)
        indices = np.asarray(mesh.faces,dtype=np.int64).reshape(-1)
        packed = np.concatenate((pos[indices],normals[indices],tex[indices]),axis=1).astype("<f4")
        if not np.isfinite(packed).all(): raise RuntimeError(f"{slug}: invalid vertex")
        vertex_bytes.append(packed)
        alpha_present |= texture.getextrema()[3][0] < 250
    vertex = np.concatenate(vertex_bytes,axis=0)
    if len(vertex) != total*3: raise RuntimeError(f"{slug}: lost triangles")
    data = b"TKM1" + struct.pack("<I",len(vertex)) + vertex.tobytes()
    OUT.mkdir(parents=True,exist_ok=True)
    model = OUT/f"{slug}.tkm"
    png = OUT/f"{slug}.png"
    model.write_bytes(data)
    atlas.save(png,"PNG",optimize=True)
    print(f"REAL FOLIAGE {slug}: {len(parts)} meshes, {total} triangles, "
          f"{len(unique)} photographed textures, alpha={alpha_present} "
          f"atlas={atlas.width}x{atlas.height}",flush=True)
    return {
        "name":slug,"source":spec["source"],"license":"CC0",
        "real_meshes":len(parts),"triangles":total,
        "material_images":len(unique),"texture_alpha":alpha_present,
        "texture_px":[atlas.width,atlas.height],
        "normalized_height_m":round(float(span[1]*scale),4),
        "material":"Photo-based authored multi-mesh RGBA atlas",
        "historical_species_authenticity":"UNVERIFIED",
        "model_sha256":hashlib.sha256(data).hexdigest(),
        "texture_sha256":hashlib.sha256(png.read_bytes()).hexdigest(),
    }


def main():
    session=requests.Session()
    session.headers["User-Agent"]="TAKSWorld-Causal3D/0.9 (+https://github.com/taks2019/taks)"
    report={"version":1,"type":"native visual ecology, NOT historical botanical proof",
            "license":"CC0","models":[],
            "deferred":[{"name":"tree_small_02",
                         "reason":"2,062,487 authored triangles require UV-preserving LOD before mobile import",
                         "source":"https://polyhaven.com/a/tree_small_02",
                         "not_substituted_with_primitives":True}]}
    with TemporaryDirectory() as working:
        for slug,spec in SELECTION.items():
            path=collect_asset(slug,spec,session,Path(working))
            report["models"].append(bake(path,slug,spec))
    output=OUT/"foliage_manifest.json"
    output.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")
    assert len(report["models"])==2
    print("AUTHENTIC CC0 FOLIAGE ASSET BAKING PASSED",flush=True)


if __name__=="__main__":
    main()
