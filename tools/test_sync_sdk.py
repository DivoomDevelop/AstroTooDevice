"""Exercise replacement failure paths using disposable directories only."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from operation_lock import operation_lock
from build_app import application_link_inputs
from sync_sdk import APPLICATION, FIXED, GENERATED, aligned_source_list, install, snapshot


class ReplacementTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.base = Path(self.temp.name)
        self.root = self.base / "project"
        self.candidate = self.base / "candidate"
        self.backup = self.base / "run/backup"
        self.root.mkdir()
        self.candidate.mkdir()
        for rel in FIXED + GENERATED + APPLICATION:
            self.put(self.root, rel, "old-" + rel)
        for rel in FIXED:
            self.put(self.candidate, rel, "new-" + rel)
        self.before = {rel: snapshot(self.root / rel) for rel in FIXED + GENERATED}
        self.expected = {rel: snapshot(self.candidate / rel) for rel in FIXED}
        self.app = {rel: snapshot(self.root / rel) for rel in APPLICATION}

    def tearDown(self):
        self.temp.cleanup()

    @staticmethod
    def put(root, rel, content):
        path = root / rel
        if rel in ("sdk", "toolchain", "licenses/vendor", "build", "out", "src"):
            path = path / "payload.txt"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)

    def apply(self, validate):
        install(self.root, self.candidate, self.backup, self.before, self.expected, validate)

    def assert_app_unchanged(self):
        for rel in APPLICATION:
            self.assertEqual(snapshot(self.root / rel), self.app[rel])

    def test_success_backs_up_old_sdk_and_keeps_app(self):
        def validate():
            self.put(self.root, "out", "new-image")
        self.apply(validate)
        for rel in FIXED:
            self.assertEqual(snapshot(self.root / rel), self.expected[rel])
        for rel in FIXED + GENERATED:
            self.assertEqual(snapshot(self.backup / rel), self.before[rel])
        self.assert_app_unchanged()

    def test_failed_build_restores_sdk_and_previous_outputs(self):
        def validate():
            self.put(self.root, "build", "failed-object")
            self.put(self.root, "out", "failed-image")
            raise RuntimeError("simulated compiler failure")
        with self.assertRaisesRegex(RuntimeError, "previous SDK restored"):
            self.apply(validate)
        for rel in FIXED + GENERATED:
            self.assertEqual(snapshot(self.root / rel), self.before[rel])
        self.assertTrue((self.base / "run/rejected/out/payload.txt").exists())
        self.assert_app_unchanged()

    def test_failed_build_without_old_outputs_retains_failure_artifacts(self):
        for rel in GENERATED:
            (self.root / rel / "payload.txt").unlink()
            (self.root / rel).rmdir()
            self.before[rel] = None
        def validate():
            self.put(self.root, "out", "failed-image")
            raise RuntimeError("simulated compiler failure")
        with self.assertRaisesRegex(RuntimeError, "previous SDK restored"):
            self.apply(validate)
        self.assertFalse((self.root / "out").exists())
        self.assertTrue((self.base / "run/rejected/out/payload.txt").exists())

    def test_edit_before_install_is_not_overwritten(self):
        self.put(self.root, "toolchain", "external-change")
        with self.assertRaises(RuntimeError):
            self.apply(lambda: None)
        self.assertEqual((self.root / "toolchain/payload.txt").read_text(), "external-change")
        self.assertEqual(snapshot(self.root / "sdk"), self.before["sdk"])
        self.assert_app_unchanged()

    def test_edit_after_install_stops_rollback(self):
        def validate():
            self.put(self.root, "sdk", "external-change")
            raise RuntimeError("simulated failure")
        with self.assertRaisesRegex(RuntimeError, "rollback stopped"):
            self.apply(validate)
        self.assertEqual((self.root / "sdk/payload.txt").read_text(), "external-change")
        self.assertEqual(snapshot(self.backup / "sdk"), self.before["sdk"])
        self.assert_app_unchanged()

    def test_process_lock_blocks_independent_build_and_allows_child(self):
        if sys.platform != "linux":
            self.skipTest("Linux flock")
        code = ("from pathlib import Path\nfrom operation_lock import operation_lock\n"
                f"with operation_lock(Path({str(self.root)!r})):\n print('acquired')\n")
        with operation_lock(self.root):
            inherited = subprocess.run([sys.executable, "-c", code], capture_output=True,
                                       cwd=Path(__file__).parent)
            self.assertEqual(inherited.returncode, 0, inherited.stderr)
            environment = dict(os.environ)
            environment.pop("ASTROTOO_OPERATION_TOKEN", None)
            blocked = subprocess.run([sys.executable, "-c", code], env=environment, capture_output=True,
                                     cwd=Path(__file__).parent)
            self.assertNotEqual(blocked.returncode, 0)
        with operation_lock(self.root):
            pass

    def test_source_list_alignment_preserves_comments_and_custom_sources(self):
        text = "# custom build\napp.c\nmoved_into_lib.c\ncustom.c\n"
        updated, added, removed = aligned_source_list(text, {"app.c": {}, "moved_into_lib.c": {}},
                                                     {"app.c": {}, "new_app.c": {}})
        self.assertIn("# custom build", updated)
        self.assertIn("custom.c", updated)
        self.assertNotIn("moved_into_lib.c", updated)
        self.assertEqual(added, ["new_app.c"])
        self.assertEqual(removed, ["moved_into_lib.c"])

    def test_optional_source_list_is_rolled_back_together_with_sdk(self):
        fixed = FIXED + ("app_sources.txt",)
        self.put(self.candidate, "app_sources.txt", "updated-list")
        self.before["app_sources.txt"] = snapshot(self.root / "app_sources.txt")
        self.expected["app_sources.txt"] = snapshot(self.candidate / "app_sources.txt")
        def validate():
            self.assertEqual((self.root / "app_sources.txt").read_text(), "updated-list")
            raise RuntimeError("simulated compiler failure")
        with self.assertRaisesRegex(RuntimeError, "previous SDK restored"):
            install(self.root, self.candidate, self.backup, self.before, self.expected, validate, fixed=fixed)
        self.assert_app_unchanged()

    def test_frozen_object_keeps_link_order_and_custom_source_is_included(self):
        items = ["platform.a", {"application_sources": ["a.c"]}, "frozen.a",
                 {"application_sources": ["b.c"], "include_extra": True}, "bundle.a"]
        actual = application_link_inputs(items, ["a.c", "b.c", "custom.c"],
                                         ["a.o", "b.o", "custom.o"], lambda x: x)
        self.assertEqual(actual, ["platform.a", "a.o", "frozen.a", "b.o", "custom.o", "bundle.a"])

    def test_old_sdk_link_format_still_supported(self):
        actual = application_link_inputs(["platform.a", "{APP_OBJECTS}"], ["a.c"], ["a.o"], lambda x: x)
        self.assertEqual(actual, ["platform.a", "a.o"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
