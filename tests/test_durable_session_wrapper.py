"""
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
"""

from pathlib import Path


ROOT = Path(__file__).parents[1]
SOURCE = (ROOT / "tools" / "harness" / "run_durable_session.py").read_text(encoding="utf-8")


def test_start_event_and_state_precede_command_wait():
    assert '"state": "running"' in SOURCE
    assert '"event": "session_started"' in SOURCE
    assert "atomic_write(state_path, record)" in SOURCE


def test_stop_has_platform_termination_paths():
    assert 'os.killpg(pid, signal.SIGTERM)' in SOURCE
    assert '"taskkill", "/PID"' in SOURCE


def test_command_output_is_redirected_to_files():
    assert "stdout=output" in SOURCE and "stderr=error" in SOURCE
    assert "output_path" in SOURCE and "error_path" in SOURCE
