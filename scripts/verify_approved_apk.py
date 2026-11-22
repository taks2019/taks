#!/usr/bin/env python3
"""Reject Android APKs containing unapproved visual assets or no native 3D engine.

This is a post-build audit: the source approval gate must run independently before
Gradle, because generated resources and packaging steps can change what ships.
The existing TAKS_VISUAL_APPROVAL_V1 JSON is intentionally fail-closed.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path, PurePosixPath
from zipfile import ZipFile, BadZipFile

VISUAL_EXTENSIONS = {
    '.png', '.jpg', '.jpeg', '.webp', '.gif', '.apng', '.avif', '.svg',
    '.ktx', '.ktx2', '.exr', '.hdr', '.basis', '.glb', '.gltf', '.obj',
    '.fbx', '.tkm', '.tks', '.anim', '.binmesh', '.mp4', '.webm', '.mov',
}
HASH = re.compile(r'^[0-9a-f]{64}$')


def audit(apk: Path, manifest_file: Path) -> list[str]:
    errors: list[str] = []
    try:
        manifest = json.loads(manifest_file.read_text(encoding='utf-8'))
    except (OSError, ValueError) as exc:
        return [f'Approval manifest unavailable or invalid: {exc}']
    if not isinstance(manifest, dict) or manifest.get('schema') != 'TAKS_VISUAL_APPROVAL_V1' or manifest.get('default_policy') != 'deny':
        return ['Expected TAKS_VISUAL_APPROVAL_V1 with default_policy=deny']
    records = manifest.get('entries')
    if not isinstance(records, list):
        return ['Approval entries must be an array']
    approved: dict[str, str] = {}
    listed: set[str] = set()
    for idx, record in enumerate(records):
        if not isinstance(record, dict):
            errors.append(f'Bad approval record {idx}')
            continue
        rel = record.get('path')
        if (not isinstance(rel, str) or not rel or '\\' in rel or
            PurePosixPath(rel).is_absolute() or '..' in PurePosixPath(rel).parts or
            PurePosixPath(rel).as_posix() != rel or PurePosixPath(rel).suffix.lower() not in VISUAL_EXTENSIONS):
            errors.append(f'Unsafe visual approval path: {rel!r}')
            continue
        if rel in listed:
            errors.append(f'Duplicate approval path: {rel}')
            continue
        listed.add(rel)
        state = record.get('status')
        if state != 'approved':
            errors.append(f'Not owner-approved: {rel} ({state})')
            continue
        sha = record.get('sha256')
        proof = record.get('approval_reference')
        if not isinstance(sha, str) or not HASH.fullmatch(sha):
            errors.append(f'Missing exact reviewed SHA-256 for {rel}')
            continue
        if not isinstance(proof, str) or len(proof.strip()) < 12:
            errors.append(f'Missing explicit owner approval evidence for {rel}')
            continue
        approved['assets/' + rel] = sha

    try:
        with ZipFile(apk) as package:
            names = package.namelist()
            if len(names) != len(set(names)):
                errors.append('Duplicate ZIP entries in APK')
            native = [p for p in names if re.fullmatch(r'lib/[^/]+/libtaks_engine\.so', p)]
            if not native:
                errors.append('Missing native TAKS Engine 3D renderer (libtaks_engine.so)')
            for name in names:
                path = PurePosixPath(name)
                if name.startswith('/') or '..' in path.parts:
                    errors.append(f'Unsafe APK entry: {name}')
                    continue
                if path.suffix.lower() not in VISUAL_EXTENSIONS:
                    continue
                if name.startswith('res/'):
                    errors.append(f'Unreviewed compiled Android visual resource: {name}')
                    continue
                if not name.startswith('assets/'):
                    errors.append(f'Visual content outside approved assets/: {name}')
                    continue
                expected = approved.get(name)
                if expected is None:
                    errors.append(f'Not allowlisted in installed APK: {name}')
                elif hashlib.sha256(package.read(name)).hexdigest() != expected:
                    errors.append(f'Packaged bytes differ from approved preview: {name}')
            for name in approved:
                if name not in names:
                    errors.append(f'Approved visual missing from APK: {name}')
    except (OSError, BadZipFile) as exc:
        errors.append(f'APK unavailable or invalid: {exc}')
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('apk', type=Path)
    parser.add_argument('--manifest', type=Path, default=Path('assets/visual_approvals.json'))
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    errors = audit(args.apk, args.manifest)
    result = {'approved_for_install': not errors, 'blockers': errors}
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 1 if errors else 0


if __name__ == '__main__':
    raise SystemExit(main())
