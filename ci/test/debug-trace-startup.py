#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Exercise Debug Sokol trace startup/shutdown and validate the frozen file."""

import argparse
import json
import os
import re
import subprocess
import tempfile
from pathlib import Path

TRACE_SIZE = 16_777_216
TRACE_MAGIC = b"WZSNTRC"
TRACE_FROZEN_OFFSET = 72
TRACE_RECORD_COUNT_OFFSET = 104


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--executable", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    executable = args.executable.resolve()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="wzsn-trace-startup-",
                                      dir=args.output) as temp:
        temp_path = Path(temp).resolve()
        environment = os.environ.copy()
        environment["TMPDIR"] = str(temp_path)
        environment["WZSN_TRACE_TEST_FRAMES"] = "1"
        completed = subprocess.run(
            [str(executable)],
            cwd=executable.parent,
            env=environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=90,
            check=False,
        )
        match = re.search(r"^WZSN timing trace: (.+)$",
                          completed.stderr, re.MULTILINE)
        if completed.returncode != 0 or match is None:
            raise SystemExit(
                f"Debug host did not complete trace startup/shutdown: "
                f"exit={completed.returncode} stderr={completed.stderr[-2000:]}")
        trace_path = Path(match.group(1).strip())
        if trace_path.parent.resolve() != temp_path.resolve():
            raise SystemExit("trace path escaped the configured temporary directory")
        if not trace_path.is_file() or trace_path.stat().st_size != TRACE_SIZE:
            raise SystemExit("trace file was not created at the fixed production size")
        raw_header = trace_path.read_bytes()[:TRACE_RECORD_COUNT_OFFSET + 8]
        if (raw_header[: len(TRACE_MAGIC)] != TRACE_MAGIC or
                raw_header[TRACE_FROZEN_OFFSET] == 0 or
                int.from_bytes(raw_header[TRACE_RECORD_COUNT_OFFSET:
                                         TRACE_RECORD_COUNT_OFFSET + 8], "little") == 0):
            raise SystemExit("trace header is not frozen with records")
        output = {
            "testId": "debug-trace-startup",
            "status": "pass",
            "exitCode": completed.returncode,
            "fileBytes": trace_path.stat().st_size,
            "frozen": True,
            "recordCount": int.from_bytes(
                raw_header[TRACE_RECORD_COUNT_OFFSET:TRACE_RECORD_COUNT_OFFSET + 8],
                "little"),
            "stderrTraceLine": "WZSN timing trace: " + trace_path.name,
        }
        print(json.dumps(output, separators=(",", ":")))


if __name__ == "__main__":
    main()
