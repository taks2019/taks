#!/usr/bin/env python3
"""Import a genuinely rigged CC0 humanoid and animation clips for TAKS GLES3.

The exported TKS1 asset keeps the source skin weights and 53-joint skeleton;
it does not bake the body into a static pose. Source files are pinned to the
upstream CharacterCreator commit so CI does not silently change model data.
"""
from __future__ import annotations

import hashlib
import io
import json
import math
import os
from pathlib import Path
import struct
from tempfile import TemporaryDirectory

import numpy as np
from PIL import Image, ImageDraw, ImageFilter
import requests

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "app/src/main/assets/models"
SOURCE = "https://raw.githubusercontent.com/kblood/CharacterCreator/852e48f316b90a0937234886630e4fb4cdfd1f78/output/"
CLIPS = ("idle", "idle_breathe", "idle_fidget", "walk", "run", "strafe_left", "strafe_right")
MAX_VERTEX_COUNT = 180_000
TILE = 384


def fetch(path: str, destination: Path, session: requests.Session) -> None:
    url = SOURCE + path
    response = session.get(url, timeout=(15, 120))
    response.raise_for_status()
    destination.write_bytes(response.content)


def read_glb(path: Path):
    data = path.read_bytes()
    if len(data) < 20 or data[:4] != b"glTF":
        raise RuntimeError(f"Invalid GLB: {path}")
    version, total = struct.unpack_from("<II", data, 4)
    if version != 2 or total != len(data):
        raise RuntimeError(f"Unsupported or truncated GLB: {path}")
    offset = 12
    document = None
    binary = None
    while offset < len(data):
        length, kind = struct.unpack_from("<II", data, offset)
        offset += 8
        chunk = data[offset:offset + length]
        offset += length
        if kind == 0x4E4F534A:
            document = json.loads(chunk.decode("utf-8"))
        elif kind == 0x004E4942:
            binary = chunk
    if document is None or binary is None:
        raise RuntimeError(f"GLB is missing JSON or binary payload: {path}")
    return document, binary


COMPONENTS = {
    5120: (np.dtype("i1"), 1), 5121: (np.dtype("u1"), 1),
    5122: (np.dtype("<i2"), 2), 5123: (np.dtype("<u2"), 2),
    5125: (np.dtype("<u4"), 4), 5126: (np.dtype("<f4"), 4),
}
WIDTHS = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}


def accessor(document, binary, index):
    item = document["accessors"][index]
    component, unit = COMPONENTS[item["componentType"]]
    width = WIDTHS[item["type"]]
    count = item["count"]
    if "bufferView" not in item:
        values = np.zeros((count, width), dtype=component)
    else:
        view = document["bufferViews"][item["bufferView"]]
        stride = view.get("byteStride", width * unit)
        start = view.get("byteOffset", 0) + item.get("byteOffset", 0)
        raw = np.ndarray((count, width), dtype=component, buffer=binary,
                         offset=start, strides=(stride, unit))
        values = np.asarray(raw).copy()
    if item.get("normalized") and component != np.dtype("<f4"):
        values = values.astype(np.float32)
        if component.kind == "u":
            values /= np.iinfo(component).max
        else:
            values = np.maximum(values / np.iinfo(component).max, -1.)
    return values


def embedded_images(document, binary):
    images = []
    for image in document.get("images", []):
        if "bufferView" in image:
            view = document["bufferViews"][image["bufferView"]]
            start = view.get("byteOffset", 0)
            images.append(Image.open(io.BytesIO(binary[start:start + view["byteLength"]])).convert("RGBA"))
        else:
            images.append(None)
    return images


def material_image(document, images, material_index):
    material = document.get("materials", [])[material_index]
    pbr = material.get("pbrMetallicRoughness", {})
    texture = pbr.get("baseColorTexture")
    alpha = material.get("alphaMode", "OPAQUE")
    factor = pbr.get("baseColorFactor", [1., 1., 1., 1.])
    if texture is not None:
        texture_index = texture["index"]
        image_index = document["textures"][texture_index]["source"]
        image = images[image_index]
        if image is None:
            raise RuntimeError("Only embedded images are supported in rigged GLB")
        return image, alpha
    color = tuple(int(round(max(0., min(1., float(c))) * 255)) for c in factor)
    return Image.new("RGBA", (64, 64), color), alpha


def hide_texture():
    """Small project-original rough hide pattern, without a modern fabric weave."""
    size = 512
    image = Image.new("RGB", (size, size), (91, 62, 39))
    draw = ImageDraw.Draw(image)
    # Irregular warm and dark hide patches create material variation; the real
    # skinned garment mesh remains the CC0 source geometry.
    for i in range(155):
        x = (i * 197 + 23) % size
        y = (i * 313 + 41) % size
        rx = 8 + ((i * 11) % 43)
        ry = 6 + ((i * 17) % 32)
        c = (111 + (i * 7) % 44, 72 + (i * 5) % 32, 42 + (i * 3) % 21)
        points = []
        for j in range(9):
            a = 2 * math.pi * j / 9
            wobble = .77 + ((i * 13 + j * 7) % 23) / 100
            points.append((x + math.cos(a) * rx * wobble, y + math.sin(a) * ry * wobble))
        draw.polygon(points, fill=c)
    return image.filter(ImageFilter.GaussianBlur(2)).convert("RGBA")


def main():
    session = requests.Session()
    session.headers["User-Agent"] = "TAKSWorld-CC0-RiggedHumanoid/1.0"
    local = os.environ.get("TAKS_CC0_SOURCE_DIR")
    with TemporaryDirectory() as temporary:
        work = Path(temporary)
        if local:
            base = Path(local)
            body_path = base / "base_body_anim.glb"
            anim_dir = base / "animations"
            clothes_paths = [base / "clothing_shirt.glb", base / "clothing_shorts.glb"]
        else:
            body_path = work / "base_body_anim.glb"
            fetch("base_body_anim.glb", body_path, session)
            anim_dir = work / "animations"
            anim_dir.mkdir()
            for name in CLIPS:
                fetch(f"animations/{name}.json", anim_dir / f"{name}.json", session)
            clothes_paths = []
            for name in ("clothing_shirt", "clothing_shorts"):
                target = work / f"{name}.glb"
                fetch(f"{name}.glb", target, session)
                clothes_paths.append(target)

        body_doc, body_bin = read_glb(body_path)
        skin = body_doc["skins"][0]
        joint_nodes = skin["joints"]
        joint_names = [body_doc["nodes"][index]["name"] for index in joint_nodes]
        if len(joint_names) != 53 or len(set(joint_names)) != len(joint_names):
            raise RuntimeError("Expected a distinct source 53-bone humanoid rig")
        node_to_joint = {node: i for i, node in enumerate(joint_nodes)}
        node_parent = {}
        for parent, node in enumerate(body_doc["nodes"]):
            for child in node.get("children", []):
                node_parent[child] = parent

        inverse_bind = accessor(body_doc, body_bin, skin["inverseBindMatrices"])
        if inverse_bind.shape != (53, 16):
            raise RuntimeError("The source inverse bind pose does not match its joint list")
        joint_records = []
        for i, node_index in enumerate(joint_nodes):
            node = body_doc["nodes"][node_index]
            parent = node_to_joint.get(node_parent.get(node_index, -1), -1)
            translation = node.get("translation", [0., 0., 0.])
            rotation = node.get("rotation", [0., 0., 0., 1.])
            joint_records.append((parent, translation, rotation, inverse_bind[i].tolist()))

        # Textured source body details plus two CC0 skin-weighted garment meshes.
        # Garment texture is a muted project-original hide material, while this
        # imported contemporary garment cut remains provisional for the era.
        body_images = embedded_images(body_doc, body_bin)
        hide = hide_texture()
        materials = []
        material_lookup = {}
        vertex_groups = []
        source_mesh_documents = [(body_doc, body_bin, body_images, True)]
        for clothes_path in clothes_paths:
            doc, binary = read_glb(clothes_path)
            names = [doc["nodes"][index]["name"] for index in doc["skins"][0]["joints"]]
            if names != joint_names:
                raise RuntimeError(f"Garment rig mismatch: {clothes_path.name}")
            source_mesh_documents.append((doc, binary, embedded_images(doc, binary), False))

        def add_material(image):
            digest = hashlib.sha256(image.tobytes()).hexdigest()
            if digest not in material_lookup:
                material_lookup[digest] = len(materials)
                materials.append(image.convert("RGBA"))
            return material_lookup[digest]

        # Determine body normalization once and use it for matching clothes.
        body_positions = []
        for primitive in body_doc["meshes"][0]["primitives"]:
            body_positions.append(accessor(body_doc, body_bin, primitive["attributes"]["POSITION"]))
        raw_body = np.concatenate(body_positions)
        low = raw_body.min(axis=0)
        high = raw_body.max(axis=0)
        height = float(high[1] - low[1])
        if not 1.35 < height < 1.9:
            raise RuntimeError(f"Unexpected humanoid height {height}")
        scale = 1.70 / height
        center_x = float((low[0] + high[0]) * .5)
        center_z = float((low[2] + high[2]) * .5)

        for doc, binary, images, is_body in source_mesh_documents:
            for mesh in doc.get("meshes", []):
                for primitive in mesh.get("primitives", []):
                    attrs = primitive["attributes"]
                    required = ("POSITION", "NORMAL", "TEXCOORD_0", "JOINTS_0", "WEIGHTS_0")
                    if any(key not in attrs for key in required):
                        raise RuntimeError("A humanoid or garment primitive has no skin weights")
                    pos = accessor(doc, binary, attrs["POSITION"]).astype(np.float32)
                    norm = accessor(doc, binary, attrs["NORMAL"]).astype(np.float32)
                    uv = accessor(doc, binary, attrs["TEXCOORD_0"]).astype(np.float32)
                    joints = accessor(doc, binary, attrs["JOINTS_0"]).astype(np.uint16)
                    weights = accessor(doc, binary, attrs["WEIGHTS_0"]).astype(np.float32)
                    if "indices" in primitive:
                        indices = accessor(doc, binary, primitive["indices"]).reshape(-1).astype(np.int64)
                    else:
                        indices = np.arange(len(pos), dtype=np.int64)
                    if len(indices) % 3 or indices.max(initial=0) >= len(pos):
                        raise RuntimeError("Invalid humanoid triangle index buffer")
                    weights /= np.maximum(weights.sum(axis=1, keepdims=True), 1e-8)
                    bad_joint = np.any(joints[indices] >= len(joint_names))
                    if bad_joint:
                        raise RuntimeError("Humanoid has a vertex weight outside the 53-joint rig")
                    p = pos.copy()
                    p[:, 0] = (p[:, 0] - center_x) * scale
                    p[:, 1] = (p[:, 1] - low[1]) * scale
                    p[:, 2] = (p[:, 2] - center_z) * scale

                    if is_body:
                        image, alpha_mode = material_image(doc, images, primitive.get("material", 0))
                        if alpha_mode == "BLEND":
                            # The source cornea is intentionally translucent;
                            # the GLES alpha test discards it like its source opacity.
                            pass
                    else:
                        image = hide
                    slot = add_material(image)
                    vertex = np.column_stack((p[indices], norm[indices], uv[indices],
                                              joints[indices], weights[indices]))
                    if not np.isfinite(vertex).all():
                        raise RuntimeError("NaN or infinity in source humanoid mesh")
                    vertex_groups.append((vertex, slot))

        if not vertex_groups:
            raise RuntimeError("No skinned source triangles were imported")
        vertex_count = sum(len(v) for v, _ in vertex_groups)
        if vertex_count > MAX_VERTEX_COUNT:
            raise RuntimeError(f"Humanoid exceeds the mobile vertex budget: {vertex_count}")

        cols = math.ceil(math.sqrt(len(materials)))
        rows = math.ceil(len(materials) / cols)
        atlas = Image.new("RGBA", (cols * TILE, rows * TILE), (0, 0, 0, 0))
        for slot, image in enumerate(materials):
            fitted = image.resize((TILE - 4, TILE - 4), Image.Resampling.LANCZOS)
            atlas.paste(fitted, ((slot % cols) * TILE + 2, (slot // cols) * TILE + 2))

        OUT.mkdir(parents=True, exist_ok=True)
        destination = OUT / "makehuman_rigged.tks"
        texture_path = OUT / "makehuman_rigged.png"
        serialized_roots = []
        with destination.open("wb") as stream:
            stream.write(struct.pack("<4sIII", b"TKS1", vertex_count, len(joint_names), len(CLIPS)))
            vertex_struct = struct.Struct("<8f4H4f")
            for vertices, slot in vertex_groups:
                uv = vertices[:, 6:8].copy()
                uv[:, 1] = 1. - uv[:, 1]
                uv[:, 0] = ((slot % cols) * TILE + 2 + uv[:, 0] * (TILE - 4)) / (cols * TILE)
                uv[:, 1] = ((slot // cols) * TILE + 2 + uv[:, 1] * (TILE - 4)) / (rows * TILE)
                vertices[:, 6:8] = uv
                for row in vertices:
                    stream.write(vertex_struct.pack(*row[:8],
                        *(int(value) for value in row[8:12]), *row[12:16]))
            for parent, translation, rotation, bind in joint_records:
                stream.write(struct.pack("<i3f4f16f", parent, *translation, *rotation, *bind))
            for clip_name in CLIPS:
                clip_path = anim_dir / f"{clip_name}.json"
                if not clip_path.exists():
                    fetch(f"animations/{clip_name}.json", clip_path, session)
                clip = json.loads(clip_path.read_text(encoding="utf-8"))
                if clip.get("format") != "charactercreator.humanoid-clip" or clip.get("rig") != "mpfb-game_engine":
                    raise RuntimeError(f"Animation rig mismatch: {clip_name}")
                # The source stores a closed loop: one endpoint sample follows
                # the declared duration so the final interval also interpolates.
                frames = len(clip.get("root", []))
                fps = float(clip["fps"])
                bone_by_name = {name: i for i, name in enumerate(joint_names)}
                semantic_to_bone = clip.get("boneMap", {})
                mapped = {}
                for semantic, bone in semantic_to_bone.items():
                    if bone in bone_by_name and semantic in clip.get("joints", {}):
                        mapped[bone_by_name[bone]] = np.asarray(clip["joints"][semantic], dtype=np.float32)
                if not frames or not mapped:
                    raise RuntimeError(f"Animation {clip_name} has no usable skeletal channels")
                root = np.asarray(clip.get("root", [[0., 0., 0.]] * frames), dtype=np.float32)
                if root.shape != (frames, 3):
                    raise RuntimeError(f"Malformed root track: {clip_name}")
                serialized_roots.append((clip_name, root.copy()))
                stream.write(struct.pack("<16sIf", clip_name.encode("ascii"), frames, fps))
                rotation_frames = []
                for frame_index in range(frames):
                    rotations = np.zeros((len(joint_names), 4), dtype=np.float32)
                    rotations[:, 3] = 1.
                    for bone_index, track in mapped.items():
                        if track.shape[0] < frames or track.shape[1] != 4:
                            raise RuntimeError(f"Malformed animation track: {clip_name}")
                        rotations[bone_index] = track[frame_index]
                    rotation_frames.append(rotations)
                # TKS1 stores each clip as one contiguous quaternion block,
                # followed by its root-motion block. The GLES reader uses
                # those offsets to decode every frame on the CPU.
                stream.write(np.asarray(rotation_frames, dtype="<f4").tobytes())
                stream.write(root.astype("<f4").tobytes())
        atlas.save(texture_path, "PNG", optimize=True)

        # Verify the serialized offsets against the source data before the
        # asset is allowed into an Android build. This catches interleaving a
        # root sample between quaternion frames, which scrambles skinning.
        encoded = destination.read_bytes()
        cursor = 16 + vertex_count * 56 + len(joint_names) * 96
        for expected_name, expected_root in serialized_roots:
            name, frame_count, _fps = struct.unpack_from("<16sIf", encoded, cursor)
            cursor += struct.calcsize("<16sIf")
            if name.rstrip(b"\0").decode("ascii") != expected_name or frame_count != len(expected_root):
                raise RuntimeError(f"Serialized animation header mismatch: {expected_name}")
            cursor += frame_count * len(joint_names) * 4 * 4
            actual_root = np.frombuffer(encoded, dtype="<f4", count=frame_count * 3,
                                        offset=cursor).reshape(frame_count, 3)
            if not np.array_equal(actual_root, expected_root):
                raise RuntimeError(f"Serialized root-motion offsets are corrupt: {expected_name}")
            cursor += frame_count * 3 * 4
        if cursor != len(encoded):
            raise RuntimeError("TKS1 output has unexpected trailing animation data")

        # Pin attribution + the remaining human-history uncertainty in the build.
        (OUT / "makehuman_rigged_source.json").write_text(json.dumps({
            "model": "MakeHuman CC0 base_body_anim",
            "source_repository": "https://github.com/kblood/CharacterCreator",
            "source_commit": "852e48f316b90a0937234886630e4fb4cdfd1f78",
            "source_license": "CC0 MakeHuman system assets",
            "rig": "53-joint mpfb-game_engine",
            "clips": list(CLIPS),
            "garment_meshes": ["clothing_shirt.glb", "clothing_shorts.glb"],
            "garment_license": "CC0 source garment meshes; hide-look material is project-original",
            "historical_clothing_authenticity": "UNVERIFIED; modern garment cuts are provisional",
            "anatomical_species_authenticity": "UNVERIFIED; general MakeHuman model, not Homo erectus",
            "vertex_count": vertex_count,
            "joint_count": len(joint_names),
            "texture_atlas_px": [atlas.width, atlas.height],
            "model_sha256": hashlib.sha256(destination.read_bytes()).hexdigest(),
            "texture_sha256": hashlib.sha256(texture_path.read_bytes()).hexdigest(),
        }, ensure_ascii=False, indent=2), encoding="utf-8")
        print(f"REAL SKINNED HUMANOID: {vertex_count} vertices, {len(joint_names)} joints, "
              f"{len(CLIPS)} clips, {atlas.width}x{atlas.height} atlas, "
              f"{destination.stat().st_size} bytes", flush=True)


if __name__ == "__main__":
    main()
