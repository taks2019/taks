#!/usr/bin/env python3
"""No graphics can bypass the exact-owner-approval APK audit."""
import hashlib
import json
import tempfile
import unittest
from pathlib import Path
from zipfile import ZipFile
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from verify_approved_apk import audit


class ApprovalTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.apk = self.root / 'app.apk'
        self.manifest = self.root / 'approvals.json'
        self.content = b'owner-reviewed-texture-bytes'
        self.doc = {
            'schema': 'TAKS_VISUAL_APPROVAL_V1',
            'default_policy': 'deny',
            'entries': [{
                'path': 'wonderwerk/approved.png',
                'status': 'approved',
                'sha256': hashlib.sha256(self.content).hexdigest(),
                'approval_reference': 'owner explicitly reviewed original approved.png',
            }],
        }

    def write(self, data=None, entries=None):
        self.manifest.write_text(json.dumps(self.doc), encoding='utf-8')
        if data is None:
            data = self.content
        with ZipFile(self.apk, 'w') as out:
            out.writestr('lib/arm64-v8a/libtaks_engine.so', b'native-symbols-for-test')
            out.writestr('assets/wonderwerk/approved.png', data)
            for path, contents in (entries or {}).items():
                out.writestr(path, contents)

    def test_matching_approved_asset_and_native_3d_pass(self):
        self.write()
        self.assertEqual(audit(self.apk, self.manifest), [])

    def test_unknown_generated_model_is_blocked(self):
        self.write(entries={'assets/models/generated_tree.glb': b'not approved'})
        self.assertTrue(any('generated_tree.glb' in e for e in audit(self.apk, self.manifest)))

    def test_modified_texture_is_blocked(self):
        self.write(data=b'changed')
        self.assertTrue(any('differ' in e for e in audit(self.apk, self.manifest)))

    def test_pending_is_blocked(self):
        self.doc['entries'][0]['status'] = 'pending'
        self.write()
        self.assertTrue(any('Not owner-approved' in e for e in audit(self.apk, self.manifest)))

    def test_android_resource_bypass_is_blocked(self):
        self.write(entries={'res/drawable-anydpi/icon.png': b'forbidden'})
        self.assertTrue(any('Android visual resource' in e for e in audit(self.apk, self.manifest)))

    def test_missing_native_engine_is_blocked(self):
        self.manifest.write_text(json.dumps(self.doc), encoding='utf-8')
        with ZipFile(self.apk, 'w') as out:
            out.writestr('assets/wonderwerk/approved.png', self.content)
        self.assertTrue(any('Missing native' in e for e in audit(self.apk, self.manifest)))

    def test_unknown_status_is_blocked(self):
        self.doc['entries'][0]['status'] = 'automatic'
        self.write()
        self.assertNotEqual(audit(self.apk, self.manifest), [])


if __name__ == '__main__':
    unittest.main()
