#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

"""Acquire the pinned Fuse source privately and run canonical conformance."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

PINNED_COMMIT = "9cbab635f5c9dfdfc5cb769aa89048c7e624d6b7"
REPOSITORY = "https://git.code.sf.net/p/fuse-emulator/fuse"


def run(command: list[str], cwd: Path) -> None:
    completed = subprocess.run(command, cwd=cwd, check=False)
    if completed.returncode != 0:
        raise RuntimeError(f"command failed with exit code {completed.returncode}: {command[0]}")


def find_runner(root: Path) -> Path:
    candidates = sorted(
        path for path in root.rglob("wz_fuse_ed_runner*")
        if path.is_file() and path.suffix.lower() in ("", ".exe")
    )
    if len(candidates) != 1:
        raise RuntimeError(f"expected one Fuse ED runner under {root}, found {len(candidates)}")
    return candidates[0]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--runner-root", required=True, type=Path)
    parser.add_argument("--private-root", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    args = parser.parse_args()

    project = Path.cwd().resolve()
    private_root = args.private_root.resolve()
    if project not in private_root.parents:
        parser.error("private root must stay inside the project directory")
    source = private_root / "fuse-source"
    private_root.mkdir(parents=True, exist_ok=True)
    if not (source / ".git").is_dir():
        run(["git", "clone", REPOSITORY, str(source)], project)
    run(["git", "fetch", "origin", PINNED_COMMIT], source)
    run(["git", "checkout", "--detach", PINNED_COMMIT], source)
    resolved = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=source, text=True
    ).strip()
    if resolved != PINNED_COMMIT:
        raise RuntimeError("resolved Fuse revision does not match the pin")

    runner = find_runner(args.runner_root.resolve())
    command = [
        sys.executable,
        str(project / "tools" / "harness" / "run_fuse_ed_corpus.py"),
        "--runner", str(runner),
        "--corpus", str(source / "z80" / "tests"),
        "--manifest", str(args.manifest.resolve()),
        "--commit", PINNED_COMMIT,
    ]
    run(command, project)
    indexed_command = command.copy()
    indexed_command[indexed_command.index(str(args.manifest.resolve()))] = str(
        args.manifest.resolve().with_name("fuse-indexed-cb-manifest.json")
    )
    indexed_command.extend(["--selection", "indexed-cb"])
    run(indexed_command, project)
    cb_command = command.copy()
    cb_command[cb_command.index(str(args.manifest.resolve()))] = str(
        args.manifest.resolve().with_name("fuse-cb-rotate-shift-manifest.json")
    )
    cb_command.extend(["--selection", "cb-rotate-shift"])
    run(cb_command, project)
    stack_command = command.copy()
    stack_command[stack_command.index(str(args.manifest.resolve()))] = str(
        args.manifest.resolve().with_name("fuse-stack-subroutine-manifest.json")
    )
    stack_command.extend(["--selection", "stack-subroutine"])
    run(stack_command, project)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
