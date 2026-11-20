"""Fail on changed preserved artwork; inspect APK bytes, not just source paths."""
from pathlib import Path
from hashlib import sha256
import json,sys,zipfile,subprocess
root=Path(__file__).resolve().parents[1]
base="3bc921a991afcafa46a5efb87f3417b108d4d46d"
protected=["app/src/main/assets/logo/TAKS_WORLD_LOGO.webp", "app/src/main/assets/logo/TAKS_WORLD_RUNTIME.webp", "app/src/main/java/hu/taks/world/AcceptedWonderwerkMenu.java", "app/src/main/java/hu/taks/world/LivingWonderwerkOverlay.java"]
report={"baseline_commit":base,"protected_files":{},"original_standalone_png":"NOT VERIFIED: the approved 7109006c... PNG is not in this repository/archive", "0.17_reference":"NOT FOUND; preserving accepted branch artwork without visual edits", "missing_eight_direction_sets":["cave exterior/interior","ground","animals","NPC-specific","plants"]}
for name in protected:
    original=subprocess.check_output(["git","show",base+":"+name],cwd=root)
    current=(root/name).read_bytes()
    assert (original==current if not name.endswith(".java") else original.decode().splitlines()==current.decode().splitlines()),"Preserved file changed: "+name
    report["protected_files"][name]=sha256(current).hexdigest()
with zipfile.ZipFile(root/"TAKS_WORLD_GITHUB_ANDROID_JATEK_ASSETS.zip") as z:
    manifest=json.loads(z.read("MANIFEST.json"))["files_sha256"]
    for name,digest in manifest.items():assert sha256(z.read(name)).hexdigest()==digest,name
    report["archive_verified"]=True
    if len(sys.argv)>1:
        with zipfile.ZipFile(sys.argv[1]) as apk:
            for name in manifest:
                if name.startswith("approved/characters/") or name.endswith("WONDERWERK_APPROVED_WIDE_NO_TEXT.png"):
                    target="assets/"+name.removeprefix("approved/")
                    assert apk.read(target)==z.read(name),target
            for name in protected[:2]:assert apk.read(name.removeprefix("app/src/main/"))==(root/name).read_bytes()
            assert not any(n.endswith(".mp4") for n in apk.namelist())
            assert "lib/arm64-v8a/libtaks_engine.so" in apk.namelist()
        report["apk_sha256"]=sha256(Path(sys.argv[1]).read_bytes()).hexdigest()
        report["apk_verified"]=True
Path("audio-release-verification.json").write_text(json.dumps(report,indent=2),encoding="utf-8")
print(json.dumps(report,indent=2))
