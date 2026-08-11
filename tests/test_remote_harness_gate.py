#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

import importlib.util
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


remote = load("remote_harness", ROOT / "tools" / "harness" / "invoke_remote_harness.py")
smoke = load("cmake_smoke", ROOT / "tools" / "harness" / "run_cmake_smoke.py")


class Result:
    def __init__(self, stdout=b"", text_stdout=""):
        self.stdout = text_stdout if text_stdout else stdout


class HarnessGateTests(unittest.TestCase):
    def test_python_selection_rejects_python_two_and_resolves_python_three(self):
        def available(command):
            return command if command in {"python3", "python"} else None

        results = [Result(text_stdout=""), Result(text_stdout="C:\\Python311\\python.exe\n")]
        results[0].returncode = 0
        results[1].returncode = 0
        with patch.object(smoke.shutil, "which", side_effect=available), \
             patch.object(smoke.subprocess, "run", side_effect=results):
            self.assertEqual(smoke.choose_python_command(), r"C:\Python311\python.exe")

    def test_python_path_is_pinned_for_cmake(self):
        self.assertEqual(smoke.python_cmake_definition(r"C:\Python\python.exe"),
                         "-DPython3_EXECUTABLE=C:/Python/python.exe")

    def test_missing_receipt_blocks_smoke(self):
        with patch.object(Path, "is_file", return_value=False):
            with self.assertRaises(SystemExit):
                remote.require_code_review_pass(ROOT)

    def test_receipt_head_or_digest_mismatch_blocks(self):
        receipt = '{"verdict":"PASS","review_complete":true,"snapshot_id":"git:base..head:sha256:bad"}'
        with patch.object(Path, "is_file", return_value=True), \
             patch.object(Path, "read_text", return_value=receipt), \
             patch.object(remote.subprocess, "run", side_effect=[
                 Result(text_stdout="head\n"), Result(text_stdout="head\n"),
                 Result(stdout=b"diff")
             ]):
            with self.assertRaises(SystemExit):
                remote.require_code_review_pass(ROOT)

    def test_unpublished_reviewed_head_blocks(self):
        receipt = '{"verdict":"PASS","review_complete":true,"snapshot_id":"git:base..head:sha256:bad"}'
        with patch.object(Path, "is_file", return_value=True), \
             patch.object(Path, "read_text", return_value=receipt), \
             patch.object(remote.subprocess, "run", side_effect=[
                 Result(text_stdout="head\n"), Result(text_stdout="older\n")
             ]):
            with self.assertRaises(SystemExit):
                remote.require_code_review_pass(ROOT)

    def test_missing_git_blocks_cleanly(self):
        receipt = '{"verdict":"PASS","review_complete":true,"snapshot_id":"git:base..head:sha256:bad"}'
        with patch.object(Path, "is_file", return_value=True), \
             patch.object(Path, "read_text", return_value=receipt), \
             patch.object(remote.subprocess, "run", side_effect=FileNotFoundError()):
            with self.assertRaises(SystemExit):
                remote.require_code_review_pass(ROOT)


if __name__ == "__main__":
    unittest.main()
