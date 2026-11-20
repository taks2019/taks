"""Offline regression tests for image-by-image APK release approval."""
import hashlib
import json
import tempfile
import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from check_visual_approvals import audit


class VisualApprovalTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.assets = self.root / "app/src/main/assets"
        self.image = self.assets / "wonderwerk/stone.png"
        self.image.parent.mkdir(parents=True)
        self.image.write_bytes(b"original approved image bytes")
        self.manifest = self.root / "approvals.json"
        self.record = {
            "path": "wonderwerk/stone.png",
            "kind": "texture",
            "status": "pending",
            "sha256": None,
            "approval_reference": None,
        }

    def check(self):
        self.manifest.write_text(
            json.dumps({"schema": "TAKS_VISUAL_APPROVAL_V1",
                        "default_policy": "deny", "entries": [self.record]}),
            encoding="utf-8")
        return audit(self.manifest, self.assets)

    def approve(self):
        self.record["status"] = "approved"
        self.record["sha256"] = hashlib.sha256(self.image.read_bytes()).hexdigest()
        self.record["approval_reference"] = "Owner approval: GitHub PR review #123"

    def test_unapproved_image_fails(self):
        allowed, reasons = self.check()
        self.assertFalse(allowed)
        self.assertTrue(any("pending" in r for r in reasons))

    def test_individual_approved_image_passes(self):
        self.approve()
        allowed, reasons = self.check()
        self.assertTrue(allowed, reasons)

    def test_image_changed_after_approval_fails(self):
        self.approve()
        self.image.write_bytes(b"unreviewed replacement")
        allowed, reasons = self.check()
        self.assertFalse(allowed)
        self.assertTrue(any("changed since approval" in r for r in reasons))

    def test_added_image_is_blocked(self):
        self.approve()
        (self.assets / "wonderwerk/new.png").write_bytes(b"new visual")
        allowed, reasons = self.check()
        self.assertFalse(allowed)
        self.assertTrue(any("not listed" in r for r in reasons))

    def test_missing_approval_evidence_is_blocked(self):
        self.approve()
        self.record["approval_reference"] = None
        self.assertFalse(self.check()[0])

    def test_unsafe_paths_are_blocked(self):
        self.approve()
        self.record["path"] = "wonderwerk/../../secrets.png"
        self.assertFalse(self.check()[0])

    def test_default_allow_policy_is_blocked(self):
        self.approve()
        self.manifest.write_text(json.dumps({
            "schema": "TAKS_VISUAL_APPROVAL_V1",
            "default_policy": "allow",
            "entries": [self.record],
        }), encoding="utf-8")
        self.assertFalse(audit(self.manifest, self.assets)[0])


if __name__ == "__main__":
    unittest.main()
