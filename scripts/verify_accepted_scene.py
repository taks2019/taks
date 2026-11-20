#!/usr/bin/env python3
"""Bájthű ellenőrzés a 2026-11-19-én elfogadott TAKS World képi referenciához.

Csak az eredeti forrásképet azonosítja; a belőle gyártott/animált 3D assetek
minőségét, archéológiai hitelességét vagy működését nem tanúsítja.
"""
import hashlib
import struct
import sys
from pathlib import Path

SHA256 = "980ea229cb87bda9f18891681a9227db111a01ec56ad2e5149c6426b21b63aa5"
SIZE = 3946758
WIDTH, HEIGHT = 1536, 1024
PNG_SIG = b"\x89PNG\r\n\x1a\n"


def check(path: Path) -> tuple[bool, str]:
    if not path.is_file():
        return False, f"Nem található az eredeti referencia: {path}"
    digest = hashlib.sha256()
    size = 0
    header = b""
    with path.open("rb") as handle:
        while True:
            block = handle.read(1024 * 1024)
            if not block:
                break
            if not header:
                header = block[:24]
            digest.update(block)
            size += len(block)
    if len(header) < 24 or header[:8] != PNG_SIG or header[12:16] != b"IHDR":
        return False, "Nem eredeti vagy sérült PNG."
    width, height = struct.unpack(">II", header[16:24])
    if size != SIZE or (width, height) != (WIDTH, HEIGHT) or digest.hexdigest() != SHA256:
        return False, "Eltérés az elfogadott eredeti képtől; ne írd felül a referencia mastert."
    return True, "A kép bájtról bájtra megegyezik az elfogadott 2026-11-19-i eredetivel."


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Használat: python3 scripts/verify_accepted_scene.py <eredeti.png>")
        sys.exit(2)
    success, message = check(Path(sys.argv[1]))
    print(message)
    sys.exit(0 if success else 1)
