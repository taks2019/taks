#!/usr/bin/env python3
"""Create a true Android UPDATE without dropping prior user-owned PNG assets.

Inputs:
- previous APK: installed TAKS World v15, signed using the owner's existing key
- compiled APK: GitHub v16 native build (not for user installation yet)
- PRIVATE signing keystore outside git, plus its signing tool

This step never uninstalls the old app or changes any menu byte. It copies
only the eight original cave visuals as required by the existing, unchanged
native loader. The new app is signed with exactly the old APK certificate.
"""
from __future__ import annotations
import argparse
import hashlib
import os
import subprocess
import zipfile
from pathlib import Path

APPROVED_MENU = "assets/menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png"
PINNED_MENU_SHA256 = "9369d3668b69424d31132e70accbfdb76282d7d9b3f3adfaa51c7c3ca1ea7bc8"
PINNED_CERT_SHA256 = "ddf758c3c311da162ff49f9876591284526a42c7daa7e918994a3a57de67299c"
ANGLES = ("000", "045", "090", "135", "180", "225", "270", "315")


def certificate(jar: Path, apk: Path):
    out=subprocess.run(["java","-jar",str(jar),"verify","--print-certs",str(apk)],
                       check=True,text=True,capture_output=True).stdout
    for line in out.splitlines():
        if "Signer #1 certificate SHA-256 digest:" in line:
            return line.split(":")[-1].strip().lower()
    raise RuntimeError("APK signing certificate missing")


def hashbytes(data):
    return hashlib.sha256(data).hexdigest()


def unchanged_assets(old: zipfile.ZipFile, new: zipfile.ZipFile):
    for name in (APPROVED_MENU,):
        assert name in old.namelist() and name in new.namelist(), (
            "Missing the original protected menu asset")
        assert old.read(name) == new.read(name), (
            "UPDATE BLOCKED: menu pixels were changed")
        assert hashbytes(new.read(name)) == PINNED_MENU_SHA256, (
            "UPDATE BLOCKED: the menu master is no longer approved")
    assert "lib/arm64-v8a/libtaks_engine.so" in new.namelist(), (
        "UPDATE BLOCKED: native gameplay library missing")


def build(previous: Path, compiled: Path, keystore: Path,
          signer: Path, output: Path, alias: str):
    for path in (previous,compiled,keystore,signer):
        if not path.is_file():
            raise FileNotFoundError(path)
    assert previous.resolve() != output.resolve() and compiled.resolve() != output.resolve()
    assert certificate(signer,previous) == PINNED_CERT_SHA256, (
        "UPDATE BLOCKED: the previous APK does not use the expected signed identity")
    # Ensure exact approved menu is unmodified by the new developer build.
    with zipfile.ZipFile(previous) as old, zipfile.ZipFile(compiled) as new:
        unchanged_assets(old,new)
        previous_png = {}
        for angle in ANGLES:
            name=f"assets/wonderwerk_png/{angle}.png"
            if name not in old.namelist():
                raise ValueError("UPDATE BLOCKED: old PNG missing: " + name)
            if name in new.namelist():
                if old.read(name) != new.read(name):
                    raise ValueError("UPDATE BLOCKED: a previous cave PNG was silently replaced: "+name)
            else:
                previous_png[name]=old.read(name)

    unsigned=output.with_suffix(".unsigned.apk")
    unsigned.write_bytes(compiled.read_bytes())
    with zipfile.ZipFile(unsigned,"a",allowZip64=True) as staged:
        for name,data in previous_png.items():
            staged.writestr(name,data,compress_type=zipfile.ZIP_DEFLATED,compresslevel=5)
    # Android v2/v3 signatures are invalid after ZIP modification; always
    # re-sign rather than pretending the original CI debug signer is valid.
    password=os.environ.get("TAKS_UPDATE_KEYSTORE_PASSWORD")
    if not password:
        raise RuntimeError("TAKS_UPDATE_KEYSTORE_PASSWORD must be set locally (never commit it)")
    args=["java","-jar",str(signer),"sign","--ks",str(keystore),
          "--ks-key-alias",alias,"--ks-pass","env:TAKS_UPDATE_KEYSTORE_PASSWORD",
          "--key-pass","env:TAKS_UPDATE_KEYSTORE_PASSWORD","--out",str(output),
          str(unsigned)]
    subprocess.run(args,check=True)
    if certificate(signer,output) != PINNED_CERT_SHA256:
        output.unlink(missing_ok=True)
        raise AssertionError("UPDATE BLOCKED: signing certificate differs from the installed app")
    with zipfile.ZipFile(previous) as old, zipfile.ZipFile(output) as new:
        assert new.testzip() is None, "Corrupted final APK"
        unchanged_assets(old,new)
        for angle in ANGLES:
            name=f"assets/wonderwerk_png/{angle}.png"
            assert new.read(name)==old.read(name), (
                "UPDATE BLOCKED: existing PNG was dropped or modified: " + name)
    unsigned.unlink(missing_ok=True)
    print("TRUE IN-PLACE APK UPDATE VERIFIED:",output,output.stat().st_size,"bytes")
    print("Original menu, 8 original PNG assets, app signer and native engine intact")


def main():
    p=argparse.ArgumentParser()
    p.add_argument("--previous",type=Path,required=True)
    p.add_argument("--compiled",type=Path,required=True)
    p.add_argument("--keystore",type=Path,required=True)
    p.add_argument("--apksigner-jar",type=Path,required=True)
    p.add_argument("--alias",default="androiddebugkey")
    p.add_argument("--output",type=Path,required=True)
    a=p.parse_args()
    build(a.previous,a.compiled,a.keystore,a.apksigner_jar,a.output,a.alias)


if __name__=="__main__":
    main()
