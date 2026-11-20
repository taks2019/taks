#!/usr/bin/env python3
"""Fail closed: only individually approved, hash-pinned visual assets may ship in TAKS World.

This is a technical gate, NOT proof that the owner approved an image. Review references
must be added only after the owner explicitly approves the exact preview/version.
Unapproved items can remain in review and screenshot artifacts, never an APK.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path, PurePosixPath

VISUAL_DIRS = ("logo", "menu", "wonderwerk", "models")
VISUAL_EXTENSIONS = {
    ".png", ".jpg", ".jpeg", ".webp", ".ktx", ".ktx2", ".avif",
    ".glb", ".gltf", ".obj", ".fbx", ".tkm", ".tks", ".anim", ".binmesh",
}
STATUS = {"pending", "changes_requested", "rejected", "approved"}
HASH_FORMAT = re.compile(r"^[0-9a-f]{64}$")


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def valid_relative_path(value: object) -> bool:
    if not isinstance(value, str) or not value or "\\" in value:
        return False
    path = PurePosixPath(value)
    return not path.is_absolute() and ".." not in path.parts and "." not in path.parts


def find_visuals(asset_root: Path) -> set[str]:
    result: set[str] = set()
    for directory in VISUAL_DIRS:
        root = asset_root / directory
        if not root.exists():
            continue
        for file in root.rglob("*"):
            if file.is_file() and file.suffix.lower() in VISUAL_EXTENSIONS:
                result.add(file.relative_to(asset_root).as_posix())
    return result


def audit(manifest_path: Path, asset_root: Path) -> tuple[bool, list[str]]:
    errors: list[str] = []
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return False, [f"Cannot read visual-approval manifest: {exc}"]
    if not isinstance(manifest, dict) or manifest.get("schema") != "TAKS_VISUAL_APPROVAL_V1":
        return False, ["Wrong approval schema"]
    if manifest.get("default_policy") != "deny":
        return False, ["Approval policy must be default-deny"]
    records = manifest.get("entries")
    if not isinstance(records, list):
        return False, ["entries must be an array"]
    seen: set[str] = set()
    for index, entry in enumerate(records):
        if not isinstance(entry, dict):
            errors.append(f"entries[{index}]: expected object")
            continue
        path = entry.get("path")
        if not valid_relative_path(path) or path.split("/")[0] not in VISUAL_DIRS:
            errors.append(f"entries[{index}]: unsafe or untracked asset path {path!r}")
            continue
        if path in seen:
            errors.append(f"Duplicate approval record: {path}")
            continue
        seen.add(path)
        state = entry.get("status")
        if state not in STATUS:
            errors.append(f"{path}: unknown status {state!r}")
            continue
        if state != "approved":
            errors.append(f"{path}: {state}; explicit owner approval missing")
            continue
        expected = entry.get("sha256")
        evidence = entry.get("approval_reference")
        if not isinstance(expected, str) or not HASH_FORMAT.fullmatch(expected):
            errors.append(f"{path}: approved asset needs exact SHA-256 of the reviewed file")
            continue
        if not isinstance(evidence, str) or len(evidence.strip()) < 12:
            errors.append(f"{path}: explicit owner approval reference required")
            continue
        file = asset_root / path
        if not file.is_file():
            errors.append(f"{path}: approved file absent from APK asset directory")
        elif digest(file) != expected:
            errors.append(f"{path}: changed since approval; requires fresh review")
    for path in sorted(find_visuals(asset_root) - seen):
        errors.append(f"{path}: not listed; blocked by default-deny policy")
    # Approved records cannot silently approve nonvisual paths or paths outside asset roots.
    return not errors, errors


def main() -> int:
    parser = argparse.ArgumentParser(description="TAKS World user visual-approval gate")
    parser.add_argument("--manifest", default="assets/visual_approvals.json", type=Path)
    parser.add_argument("--asset-root", default="app/src/main/assets", type=Path)
    parser.add_argument("--json-report", type=Path)
    args = parser.parse_args()
    ok, errors = audit(args.manifest, args.asset_root)
    if args.json_report:
        args.json_report.parent.mkdir(parents=True, exist_ok=True)
        args.json_report.write_text(
            json.dumps({"approved_for_apk": ok, "blockers": errors},
                       ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8")
    if ok:
        print("TAKS visual gate: APPROVED - every detected visual file is separately approved")
    else:
        print(f"TAKS visual gate: BLOCKED - {len(errors)} approval item(s) unresolved")
        for item in errors:
            print(" - " + item)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
