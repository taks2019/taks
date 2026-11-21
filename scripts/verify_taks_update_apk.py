#!/usr/bin/env python3
"""APK-to-APK proof that TAKS World installs as an UPDATE, not another app.

Run against the actually-installed previous APK and the newly signed update.
Private keystores are never stored in Git or printed to the build logs.
"""
import argparse
import hashlib
import re
import subprocess
import zipfile
from pathlib import Path

PINNED_MENU = "9369d3668b69424d31132e70accbfdb76282d7d9b3f3adfaa51c7c3ca1ea7bc8"
APP_ID = "hu.taks.world.menufixedgame"
PINNED_SIGNER_SHA256 = "ddf758c3c311da162ff49f9876591284526a42c7daa7e918994a3a57de67299c"
ANGLES = ("000","045","090","135","180","225","270","315")


def run(argv):
    result = subprocess.run(argv,text=True,capture_output=True)
    if result.returncode:
        raise AssertionError("Tool failed: " + " ".join(str(x) for x in argv)
                             + "\n" + result.stdout + result.stderr)
    return result.stdout


def certificate(apk, apksigner):
    output=run([*apksigner,"verify","--print-certs",str(apk)])
    found=re.search(r"Signer #1 certificate SHA-256 digest:\s*([a-f0-9]+)",
                    output,re.IGNORECASE)
    assert found, "APK signature missing"
    digest=found.group(1).lower()
    assert digest==PINNED_SIGNER_SHA256, (
        "UPDATE BLOCKED: signing certificate changed. "
        "Re-sign with the ORIGINAL test signing key, do not uninstall!")
    return digest


def metadata(apk,aapt):
    output=run([str(aapt),"dump","badging",str(apk)])
    p=re.search(r"package: name='([^']+)' versionCode='(\d+)'",output)
    assert p, "Failed to read the actual APK manifest"
    assert p.group(1)==APP_ID, (
        "UPDATE BLOCKED: wrong package. The original saved games will not be "
        "accessible from a different app ID.")
    return int(p.group(2))


def approved_assets(apk,previous=None):
    with zipfile.ZipFile(apk) as z:
        assert z.testzip() is None, "APK ZIP corruption"
        names=set(z.namelist())
        menu="assets/menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png"
        assert menu in names, "Missing original menu master"
        assert hashlib.sha256(z.read(menu)).hexdigest()==PINNED_MENU, (
            "UPDATE BLOCKED: original menu artwork changed")
        assert "lib/arm64-v8a/libtaks_engine.so" in names, (
            "UPDATE BLOCKED: native gameplay engine missing")
        cave_hashes={}
        for a in ANGLES:
            name=f"assets/wonderwerk_png/{a}.png"
            assert name in names, "UPDATE BLOCKED: prior cave PNG lost: "+name
            cave_hashes[name]=hashlib.sha256(z.read(name)).hexdigest()
        if previous:
            with zipfile.ZipFile(previous) as old:
                for name,checksum in cave_hashes.items():
                    assert name in old.namelist(), "Old cave image missing"
                    assert hashlib.sha256(old.read(name)).hexdigest()==checksum, (
                        "UPDATE BLOCKED: previous cave image changed "
                        "without separate visual approval: "+name)


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("previous",type=Path)
    ap.add_argument("update",type=Path)
    ap.add_argument("--apksigner-jar",type=Path,default=None)
    ap.add_argument("--aapt",type=Path,required=True)
    args=ap.parse_args()
    for file in (args.previous,args.update):
        if not file.is_file():
            ap.error("Missing APK: "+str(file))
    signer=["java","-jar",str(args.apksigner_jar)] if args.apksigner_jar else ["apksigner"]
    previous_certificate=certificate(args.previous,signer)
    next_certificate=certificate(args.update,signer)
    original_code=metadata(args.previous,args.aapt)
    next_code=metadata(args.update,args.aapt)
    assert next_code > original_code, (
        f"UPDATE BLOCKED: versionCode {next_code} must exceed {original_code}")
    approved_assets(args.update,args.previous)
    print("TAKS UPDATE VALID: same package / certificate / original menu "
          "/ unchanged 8 PNGs; versionCode "
          f"{original_code} -> {next_code}; "
          f"signer SHA256 {previous_certificate}",flush=True)


if __name__=="__main__":
    main()
