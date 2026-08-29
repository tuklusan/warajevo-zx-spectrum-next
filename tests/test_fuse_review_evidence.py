#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


summary = load("fuse_review_evidence", ROOT / "tools" / "harness" / "summarize_fuse_review_evidence.py")


class FuseReviewEvidenceTests(unittest.TestCase):
    def make_run(self, root, machine="linux-x64-lxqt", failed=False):
        run = root / machine
        manifest_path = run / "unzipped" / ".wzsn-harness" / "run" / "fuse-complete-manifest.json"
        manifest_path.parent.mkdir(parents=True)
        (run / "session.json").write_text(json.dumps({
            "action": "smoke", "machine": machine, "run_id": "run",
            "primary_returncode": 0, "pull_returncode": 0,
        }), encoding="ascii")
        cases = [{"name": "e3", "status": "failed" if failed else "passed"}]
        manifest_path.write_text(json.dumps({
            "commit": "pin", "total": 1, "passed": 0 if failed else 1,
            "failed": 1 if failed else 0, "silent_skips": 0,
            "known_unresolved": ["e3"] if failed else [],
            "unexpected_failures": [], "cases": cases,
        }), encoding="ascii")
        return run

    def test_summary_binds_complete_source_evidence(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "test-artefacts") as directory:
            root = Path(directory)
            baseline = root / "baseline.json"
            baseline.write_text(json.dumps({"commit": "pin", "case_names": []}), encoding="ascii")
            run = self.make_run(root)
            machine = summary.summarize_run(run, "pin", set(), {"e3"})
            self.assertEqual(machine["acceptance"]["required_cases"], {"e3": "passed"})
            self.assertTrue(machine["acceptance"]["remaining_failures_are_exactly_baseline"])
            self.assertIn("sha256", machine["source_artifacts"]["manifest"])

    def test_summary_rejects_inconsistent_baseline_accounting(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "test-artefacts") as directory:
            root = Path(directory)
            run = self.make_run(root, failed=True)
            with self.assertRaises(ValueError):
                summary.summarize_run(run, "pin", set(), {"e3"})


if __name__ == "__main__":
    unittest.main()
