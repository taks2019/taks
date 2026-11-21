#!/usr/bin/env python3
"""Immutable user-approved TAKS World assets and IN-PLACE Android UPDATE gate.

The first baseline is the *actually installed* menu-repair branch, NOT a
recent AI mockup. The gate blocks unapproved scene/menu/logo replacements,
changes in button geometry or routing, reinstall-as-another-app, and accidental
downgrades. Changes to other engine files are allowed as non-destructive
incremental work; original saves remain in the same Android app sandbox.
"""
import argparse
import hashlib
import re
import subprocess
from pathlib import Path

APPROVED_BASE_COMMIT = "1e81a26b4774a2db85ee6596cc81326452247333"
APPROVED_MENU_SOURCE = "app/src/main/java/hu/taks/world/RestoredStoneMenu.java"
APPROVED_MAIN = "app/src/main/java/hu/taks/world/MainActivity.java"
APPROVED_GRAPHICS = {
    "app/src/main/assets/menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png":
        "9369d3668b69424d31132e70accbfdb76282d7d9b3f3adfaa51c7c3ca1ea7bc8"
}
EXPECTED_PACKAGE = "hu.taks.world.menufixedgame"
FIRST_UPDATE_CODE = 16
# Additional assets may be added with explicit per-asset review. These are
# never editable implicitly, even when a newer engine branch is merged.
PROTECTED_PREFIXES = (
    "app/src/main/assets/logo/",
    "app/src/main/assets/menu/",
    "art/reference_crops/",
    "assets/original_approved/",
)


def git(*args):
    command = subprocess.run(["git", *args], text=True,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
    if command.returncode != 0:
        raise AssertionError("Cannot inspect approved baseline commit: "
                             + " ".join(args) + "\n" + command.stderr)
    return command.stdout


def repo_file_at(revision, path):
    return git("show", f"{revision}:{path}")


def menu_subsection(java):
    # The previous approved menu routine is untouched. This catches
    # unannounced font/layout/navigation changes that alter the screenshot.
    a = java.find("    private void showMainMenu(){")
    b = java.find("    @Override public void onBackPressed()", a)
    assert a >= 0 and b > a, "Cannot locate the original complete main menu"
    return java[a:b]


def ensure_source_unchanged(path):
    old = repo_file_at(APPROVED_BASE_COMMIT, path)
    now = Path(path).read_text("utf-8")
    assert old == now, (
        "APPROVAL BLOCK: The existing menu source was modified: " + path
        + ". Restore the original source; only reviewed additive features"
        + " outside the menu may be updated.")


def ensure_menu_routine_unchanged():
    old = repo_file_at(APPROVED_BASE_COMMIT, APPROVED_MAIN)
    now = Path(APPROVED_MAIN).read_text("utf-8")
    assert menu_subsection(old) == menu_subsection(now), (
        "APPROVAL BLOCK: do not change the approved menu layout or routing.")


def ensure_graphics_unchanged():
    for path, checksum in APPROVED_GRAPHICS.items():
        target = Path(path)
        # The CI stage restores the approved bytes from the immutable
        # archive before this script runs. Fail closed on any missing art.
        assert target.is_file(), "APPROVAL BLOCK: missing source art " + path
        actual = hashlib.sha256(target.read_bytes()).hexdigest()
        assert actual == checksum, (
            "APPROVAL BLOCK: approved artwork bytes were replaced: "
            + path + ". Expected " + checksum + ", got " + actual)
    # Never overwrite any previously committed asset in a protected folder.
    diff = git("diff", "--name-status", APPROVED_BASE_COMMIT, "HEAD", "--",
               *PROTECTED_PREFIXES).splitlines()
    for line in diff:
        fields = line.split("\t")
        if not fields:
            continue
        status = fields[0]
        if status and status[0] in "MDRTC":
            raise AssertionError(
                "APPROVAL BLOCK: existing protected file was modified or "
                f"removed: {line}")


def ensure_stable_package_and_update():
    gradle = Path("app/build.gradle").read_text("utf-8")
    prefix = re.search(r"applicationId\s+['\"]([^'\"]+)['\"]", gradle)
    suffix = re.search(r"applicationIdSuffix\s+['\"]([^'\"]+)['\"]",
                       gradle)
    code = re.search(r"versionCode\s+(\d+)", gradle)
    assert prefix and suffix and code, "UPDATE BLOCK: APK identity absent"
    actual = prefix.group(1) + suffix.group(1)
    assert actual == EXPECTED_PACKAGE, (
        "UPDATE BLOCK: do not create a new app or uninstall the old one: "
        + actual + " != " + EXPECTED_PACKAGE)
    assert int(code.group(1)) >= FIRST_UPDATE_CODE, (
        "UPDATE BLOCK: versionCode must be higher than the installed APK")
    baseline = repo_file_at(APPROVED_BASE_COMMIT, "app/build.gradle")
    old = re.search(r"versionCode\s+(\d+)", baseline)
    assert old and int(code.group(1)) > int(old.group(1)), (
        "UPDATE BLOCK: versionCode must monotonically increase")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--no-git-check", action="store_true",
                        help="Only for testing extracted APK, not for CI")
    args = parser.parse_args()
    if not args.no_git_check:
        ensure_source_unchanged(APPROVED_MENU_SOURCE)
        ensure_menu_routine_unchanged()
    ensure_graphics_unchanged()
    ensure_stable_package_and_update()
    print("TAKS UPDATE PASS: exact menu, original logo and scene protected;"
          " same Android app ID; no save migration / reset.", flush=True)


if __name__ == "__main__":
    main()
