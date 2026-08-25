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


corpus = load("fuse_corpus", ROOT / "tools" / "harness" / "run_fuse_ed_corpus.py")


class FuseHarnessGateTests(unittest.TestCase):
    def write_baseline(self, data):
        directory = tempfile.TemporaryDirectory()
        path = Path(directory.name) / "baseline.json"
        path.write_text(json.dumps(data), encoding="ascii")
        self.addCleanup(directory.cleanup)
        return path

    def test_baseline_accepts_unique_names_at_the_pinned_revision(self):
        path = self.write_baseline({"commit": corpus.PINNED_COMMIT, "case_names": ["37_2"]})
        self.assertEqual(corpus.load_unresolved_baseline(path, corpus.PINNED_COMMIT), {"37_2"})

    def test_baseline_rejects_wrong_pin_and_duplicate_case_names(self):
        wrong_pin = self.write_baseline({"commit": "wrong", "case_names": []})
        duplicate = self.write_baseline({"commit": corpus.PINNED_COMMIT, "case_names": ["b8", "b8"]})
        with self.assertRaises(ValueError):
            corpus.load_unresolved_baseline(wrong_pin, corpus.PINNED_COMMIT)
        with self.assertRaises(ValueError):
            corpus.load_unresolved_baseline(duplicate, corpus.PINNED_COMMIT)
