# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

import argparse
import queue
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

WRITERS = 8
ATOMIC_ITERATIONS = 16
PAYLOAD_SIZE = 128 * 1024


def payload(writer: int, iteration: int) -> bytes:
    value = (writer * 31 + iteration * 17) & 0xFF
    data = bytes((value + index * 13) & 0xFF for index in range(PAYLOAD_SIZE))
    prefix = f"writer={writer};iteration={iteration};".encode()
    return prefix + data[len(prefix):]


def run_group(worker: Path, mode: str, destination: Path, iterations: int):
    expected_payloads = {
        payload(writer, iteration)
        for writer in range(WRITERS)
        for iteration in range(iterations)
    }
    processes = [
        subprocess.Popen([str(worker), mode, str(destination), str(writer),
                          str(iterations)], stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
        for writer in range(WRITERS)
    ]
    observed: set[bytes] = set()
    stop = threading.Event()
    read_errors: queue.SimpleQueue[str] = queue.SimpleQueue()

    def observe_atomic_destination():
        while not stop.is_set():
            try:
                content = destination.read_bytes()
            except FileNotFoundError:
                continue
            except OSError as error:
                read_errors.put(str(error))
                return
            if len(content) != PAYLOAD_SIZE or content not in expected_payloads:
                read_errors.put("reader observed a partial or mixed output")
                return
            observed.add(content)
            time.sleep(0.0005)

    observer = None
    if mode == "atomic":
        observer = threading.Thread(target=observe_atomic_destination, daemon=True)
        observer.start()
    results = []
    try:
        for process in processes:
            stdout, stderr = process.communicate(timeout=120)
            results.append((process.returncode, stdout, stderr))
    except subprocess.TimeoutExpired:
        for process in processes:
            if process.poll() is None:
                process.kill()
        for process in processes:
            process.communicate()
        raise RuntimeError("writer process exceeded the 120-second limit")
    if observer is not None:
        stop.set()
        observer.join(timeout=5)
    if not read_errors.empty():
        raise RuntimeError(read_errors.get())
    return results, observed


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("worker", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    worker = args.worker.resolve()
    if not worker.exists() and worker.with_suffix(".exe").exists():
        worker = worker.with_suffix(".exe")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="wz-host-output-", dir=output) as temp:
        root = Path(temp)
        atomic_path = root / "shared-snapshot.bin"
        results, observed = run_group(worker, "atomic", atomic_path,
                                      ATOMIC_ITERATIONS)
        if any(code != 0 for code, _, _ in results):
            raise RuntimeError(f"atomic writers failed: {[r[0] for r in results]}")
        if atomic_path.read_bytes() not in {
                payload(writer, iteration) for writer in range(WRITERS)
                for iteration in range(ATOMIC_ITERATIONS)}:
            raise RuntimeError("final atomic output is not a complete writer payload")
        if not observed:
            raise RuntimeError("observer did not capture a published atomic output")
        if list(root.glob("shared-snapshot.bin.tmp.*")):
            raise RuntimeError("atomic write left temporary files behind")
        print(f"PASS atomic shared-path writes: {WRITERS} processes, "
              f"{ATOMIC_ITERATIONS} replacements each, {len(observed)} "
              "complete versions observed")

        exclusive_path = root / "shared-export.bin"
        exclusive_results, _ = run_group(worker, "exclusive", exclusive_path, 1)
        codes = [code for code, _, _ in exclusive_results]
        if codes.count(0) != 1 or codes.count(3) != WRITERS - 1:
            raise RuntimeError(f"exclusive claim results were {codes}")
        if exclusive_path.read_bytes() not in {
                payload(writer, 0) for writer in range(WRITERS)
        }:
            raise RuntimeError("exclusive output is not one complete writer payload")
        print(f"PASS exclusive destination claim: {WRITERS} processes, "
              "one complete winner, no clobber")

    print("PASS no temporary output remains after concurrent writes")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print(f"FAIL host output atomic contract: {error}", file=sys.stderr)
        sys.exit(1)
