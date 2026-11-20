#!/usr/bin/env python3
"""Reassemble approved user ZIP from immutable Git blobs using base64 text.
Each blob is one EXACT unmodified sequential base64 slice. Do not regenerate
any artwork, logos, video, water/fern masks or character images.
"""
from pathlib import Path
import base64, hashlib

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'assets/approved-transfer'
TARGET=ROOT/'TAKS_WORLD_GITHUB_ANDROID_JATEK_ASSETS.zip'
EXPECTED='3ee394b05cc3c0125823198f72951175246de1cf25f3049717cb1a27a635535a'
PART_COUNT=36
CHUNK_LENGTH=524288
files=[SOURCE/f'chunk_{i:02d}.b64' for i in range(PART_COUNT)]
for p in files:
    if not p.is_file():raise SystemExit(f'Missing Git chunk: {p}')
with TARGET.open('wb') as output:
    # The first 35 parts are divisible by four and decode independently.
    for i,p in enumerate(files):
        encoded=p.read_bytes()
        if i<PART_COUNT-1 and len(encoded)!=CHUNK_LENGTH:
            raise SystemExit(f'Broken base64 chunk length {i}: {len(encoded)}')
        try: output.write(base64.b64decode(encoded,validate=True))
        except Exception as e:raise SystemExit(f'Corrupt base64 chunk {i}: {e}')
raw=TARGET.read_bytes()
digest=hashlib.sha256(raw).hexdigest()
if digest!=EXPECTED:raise SystemExit(f'Original ZIP is NOT byte-identical: {digest}')
print(f'PASS: original approved images/video ZIP recovered, {len(raw)} bytes, SHA256 {digest}')
