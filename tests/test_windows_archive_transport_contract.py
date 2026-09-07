"""
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
"""

from pathlib import Path


ROOT = Path(__file__).parents[1]
SOURCE = (ROOT / "tools" / "harness" / "invoke_remote_harness.py").read_text(encoding="utf-8")


def test_windows_commands_default_to_tty():
    assert "allocate_tty: bool = True" in SOURCE
    assert "ssh_base(root, machine, allocate_tty=allocate_tty)" in SOURCE


def test_windows_archive_pull_is_noninteractive():
    start = SOURCE.index("def pull_windows(")
    end = SOURCE.index("\n\ndef extract_zip", start)
    pull = SOURCE[start:end]
    assert "run_windows(machine, script_text, root, allocate_tty=False)" in pull
