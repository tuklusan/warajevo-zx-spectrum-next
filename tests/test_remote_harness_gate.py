#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

import importlib.util
import hashlib
import json
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
    def test_active_machine_registry_contains_only_remaining_hosts(self):
        self.assertEqual(set(remote.REMOTE_MACHINES), {"linux-x64-lxqt", "windows-10-reference"})
        self.assertEqual(remote.REMOTE_MACHINES["linux-x64-lxqt"]["ssh_target"], "sanyalnet@192.168.4.76")
        self.assertEqual(remote.REMOTE_MACHINES["windows-10-reference"]["ssh_target"], "sanyalnet@192.168.4.75")

    def test_retired_machine_names_are_rejected_before_execution(self):
        for machine in ("windows-11-laptop", "macos-bigsur-lab"):
            with self.subTest(machine=machine):
                self.assertNotIn(machine, remote.REMOTE_MACHINES)

    def test_run_id_rejects_local_traversal_and_remote_shell_metacharacters(self):
        for value in ("../escape", "name'; Write-Output injected; '", "with space", "a" * 65):
            with self.subTest(value=value), self.assertRaises(SystemExit):
                remote.validate_run_id(value)
        self.assertEqual(remote.validate_run_id("20260811T120000Z-smoke_1"), "20260811T120000Z-smoke_1")

    def test_python_selection_rejects_python_two_and_resolves_python_three(self):
        def available(command):
            return command if command in {"python3", "python"} else None

        results = [Result(text_stdout=""), Result(text_stdout="C:\\Python311\\python.exe\n")]
        results[0].returncode = 0
        results[1].returncode = 0
        with patch.object(smoke.shutil, "which", side_effect=available), \
             patch.object(smoke.subprocess, "run", side_effect=results) as run_mock:
            self.assertEqual(smoke.choose_python_command(), r"C:\Python311\python.exe")
        self.assertTrue(all(call.kwargs["timeout"] == 30 for call in run_mock.call_args_list))

    def test_python_selection_falls_back_after_execution_error(self):
        def available(command):
            return command if command in {"python3", "python"} else None

        fallback = Result(text_stdout="/usr/bin/python3\n")
        fallback.returncode = 0
        with patch.object(smoke.shutil, "which", side_effect=available), \
             patch.object(smoke.subprocess, "run", side_effect=[OSError("blocked"), fallback]):
            self.assertEqual(smoke.choose_python_command(), "/usr/bin/python3")

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
                 Result(text_stdout="head\n"), Result(text_stdout="head\trefs/heads/main\n"),
                 Result(stdout=b"diff")
             ]):
            with self.assertRaises(SystemExit):
                remote.require_code_review_pass(ROOT)

    def test_unpublished_reviewed_head_blocks(self):
        receipt = '{"verdict":"PASS","review_complete":true,"snapshot_id":"git:base..head:sha256:bad"}'
        with patch.object(Path, "is_file", return_value=True), \
             patch.object(Path, "read_text", return_value=receipt), \
             patch.object(remote.subprocess, "run", side_effect=[
                 Result(text_stdout="head\n"), Result(text_stdout="older\trefs/heads/main\n")
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

    def test_protocol_four_receipt_remains_compatible(self):
        diff = b"reviewed diff"
        digest = hashlib.sha256(diff).hexdigest()
        head = "a" * 40
        original_read_bytes = Path.read_bytes
        original_read_text = Path.read_text
        authority = remote.validate_code_receipt.__globals__
        cr_number = "CR-0344"
        preflight_path = ROOT / "design" / "cr-preflight" / f"{cr_number}.md"
        preflight_data = preflight_path.read_bytes()
        review_base = next(
            line.split(":", 1)[1].strip()
            for line in preflight_data.decode("utf-8").splitlines()
            if line.startswith("Review-Base:")
        )
        requirement_path = "design/review-gate.md"
        requirement_digest = hashlib.sha256((ROOT / requirement_path).read_bytes()).hexdigest()
        requirement_sources = [{"source": requirement_path, "sha256": requirement_digest}]
        excerpt = (ROOT / requirement_path).read_text(encoding="utf-8").splitlines()[0]
        excerpts = [{
            "requirement_id": "REQ-REMOTE-HARNESS",
            "source": requirement_path,
            "start": 1,
            "end": 1,
            "excerpt_sha256": hashlib.sha256(excerpt.encode()).hexdigest(),
        }]
        review_map_path = "test-artefacts/reviewer/requirements/CR-test-remote-harness-map.json"
        review_map_data = b"{}\n"
        receipt_data = {
            "schema_version": 4,
            "review_protocol_version": 4,
            "project_id": authority["PROJECT_ID"],
            "review_profile_hash": authority["profile_hash"](ROOT),
            "cr_number": cr_number,
            "preflight_source": preflight_path.relative_to(ROOT).as_posix(),
            "preflight_sha256": hashlib.sha256(preflight_data).hexdigest(),
            "review_base": review_base,
            "requirements_manifest_hash": hashlib.sha256(authority["cj"]({
                "sources": requirement_sources, "excerpts": excerpts
            }).encode()).hexdigest(),
            "requirement_sources": requirement_sources,
            "requirement_excerpt_bindings": excerpts,
            "scope_private_source": None,
            "review_map_source": review_map_path,
            "review_map_sha256": hashlib.sha256(review_map_data).hexdigest(),
            "verdict": "PASS",
            "review_complete": True,
            "snapshot_id": f"git:{review_base}..{head}:sha256:{digest}",
        }
        tracker_data = (ROOT / "issues" / "change-requests.json").read_bytes()
        tracker = json.loads(tracker_data)
        cr = next(item for item in tracker["change_requests"]
                  if item["cr_number"] == cr_number)
        scope = {
            "cr_number": cr_number,
            "title": cr.get("title"),
            "status": cr.get("status"),
            "source_authority": cr.get("source_authority", []),
            "notes": cr.get("notes", ""),
            "tracker_source": "issues/change-requests.json",
            "tracker_sha256": hashlib.sha256(tracker_data).hexdigest(),
            "record_sha256": hashlib.sha256(authority["cj"](cr).encode()).hexdigest(),
            "preflight_source": preflight_path.relative_to(ROOT).as_posix(),
            "preflight_sha256": hashlib.sha256(preflight_data).hexdigest(),
            "review_base": review_base,
            "operator_authorized_gate_change": False,
        }
        receipt_data["scope_manifest_hash"] = hashlib.sha256(
            authority["cj"](scope).encode()
        ).hexdigest()
        receipt = json.dumps(receipt_data)

        def read_bytes(path):
            if Path(path).resolve() == (ROOT / review_map_path).resolve():
                return review_map_data
            return original_read_bytes(path)

        def read_text(path, *args, **kwargs):
            if Path(path).resolve() == (ROOT / "test-artefacts" / "reviewer" / "code-pass.json").resolve():
                return receipt
            return original_read_text(path, *args, **kwargs)

        with patch.object(Path, "is_file", return_value=True), \
             patch.object(Path, "read_text", autospec=True, side_effect=read_text), \
             patch.object(Path, "read_bytes", autospec=True, side_effect=read_bytes), \
             patch.object(remote.subprocess, "run", side_effect=[
                 Result(text_stdout=f"{head}\n"), Result(text_stdout=f"{head}\trefs/heads/main\n"),
                 Result(stdout=diff),
             ]):
            remote.require_code_review_pass(ROOT)

    def test_protocol_two_authority_change_blocks_remote_execution(self):
        receipt = {
            "review_protocol_version": 2,
            "cr_number": "CR-0020",
            "requirement_sources": [{"source": "design/review-gate.md", "sha256": "stale"}],
            "requirements_manifest_hash": "stale",
            "scope_manifest_hash": "stale",
            "scope_private_source": None,
        }
        with self.assertRaises(SystemExit):
            remote.validate_review_authority(ROOT, receipt)

    def test_review_pending_record_proves_no_remote_execution_started(self):
        record = remote.review_pending_record(
            "windows-11-laptop", "CR0267-WIN11-R1",
            "remote smoke blocked: review is not published",
        )
        self.assertEqual(record["classification"], "review_pending")
        self.assertFalse(record["remote_execution_started"])
        self.assertEqual(record["machine"], "windows-11-laptop")
        self.assertEqual(record["run_id"], "CR0267-WIN11-R1")


if __name__ == "__main__":
    unittest.main()
