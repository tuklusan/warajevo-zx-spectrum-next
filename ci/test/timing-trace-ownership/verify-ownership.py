#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Prove that one process exclusively owns a production trace path."""

import argparse
import struct
import subprocess
import time
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixture", required=True, type=Path)
    parser.add_argument("--workdir", required=True, type=Path)
    args = parser.parse_args()
    args.workdir.mkdir(parents=True, exist_ok=True)
    for path in args.workdir.iterdir():
        if path.is_file():
            path.unlink()
        else:
            raise SystemExit(f"unexpected directory in test output: {path.name}")

    trace_path = args.workdir / "shared.wztrace"
    start_path = args.workdir / "start.signal"
    release_path = args.workdir / "release.signal"
    workers = []
    processes = []
    try:
        for index in range(8):
            result_path = args.workdir / f"worker-{index}.result"
            process = subprocess.Popen([
                str(args.fixture), str(trace_path), str(start_path),
                str(release_path), str(result_path), str(100 + index),
            ])
            processes.append(process)
            workers.append((index, result_path, process))

        start_path.touch()
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            if all(result_path.exists() for _, result_path, _ in workers):
                break
            if any(process.poll() not in (None, 0)
                   for _, _, process in workers):
                raise SystemExit("a trace contender exited unsuccessfully")
            time.sleep(0.02)
        else:
            raise SystemExit("trace contenders did not all report ownership")

        outcomes = [result_path.read_text(encoding="ascii").strip()
                    for _, result_path, _ in workers]
        if outcomes.count("OWNER") != 1 or outcomes.count("DENIED") != 7:
            raise SystemExit(f"unexpected trace ownership outcomes: {outcomes}")

        release_path.touch()
        for _, _, process in workers:
            if process.wait(timeout=30) != 0:
                raise SystemExit("a trace contender failed to exit cleanly")

        header = trace_path.read_bytes()[:256]
        owner_index = outcomes.index("OWNER")
        if len(header) != 256 or header[:7] != b"WZSNTRC":
            raise SystemExit("the winning process did not create a valid trace")
        if struct.unpack_from("<Q", header, 24)[0] != 100 + owner_index:
            raise SystemExit("trace header does not identify its sole owner")
        if header[72] != 1 or struct.unpack_from("<Q", header, 104)[0] != 1:
            raise SystemExit("winning trace was not frozen with one complete event")

        print("PASS one of eight concurrent processes exclusively owned and froze the shared trace path")
    finally:
        release_path.touch(exist_ok=True)
        for process in processes:
            if process.poll() is None:
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.terminate()
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()


if __name__ == "__main__":
    main()
