#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

from __future__ import annotations

import base64
import io
import sys
import zipfile
from pathlib import Path


def resolve_within_project(project_root: Path, candidate: str) -> Path:
    path = Path(candidate)
    if not path.is_absolute():
        path = project_root / path

    resolved = path.resolve()

    try:
        resolved.relative_to(project_root)
    except ValueError as exc:
        raise SystemExit(f"path must remain within the project directory: {candidate}") from exc

    return resolved


def main() -> int:
    if len(sys.argv) not in {2, 3}:
        print("usage: stream_zip_tree.py [--base64] <path-within-project>", file=sys.stderr)
        return 2

    emit_base64 = False
    target_arg = sys.argv[1]
    if sys.argv[1] == "--base64":
        emit_base64 = True
        target_arg = sys.argv[2]

    project_root = Path.cwd().resolve()
    target_dir = resolve_within_project(project_root, target_arg)

    if not target_dir.exists():
        print(f"missing path: {target_dir}", file=sys.stderr)
        return 1

    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, mode="w", compression=zipfile.ZIP_DEFLATED) as archive:
        if target_dir.is_file():
            archive.write(target_dir, arcname=target_dir.relative_to(project_root).as_posix())
        else:
            for path in sorted(target_dir.rglob("*")):
                if path.is_file():
                    archive.write(path, arcname=path.relative_to(project_root).as_posix())

    payload = buffer.getvalue()
    if emit_base64:
        sys.stdout.write(base64.b64encode(payload).decode("ascii"))
    else:
        sys.stdout.buffer.write(payload)
    return 0


if __name__ == "__main__":
    sys.exit(main())
