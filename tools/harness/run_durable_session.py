#!/usr/bin/env python3
"""
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
"""

from __future__ import annotations

import argparse
import json
import os
import secrets
import signal
import subprocess
import sys
import time
from pathlib import Path


def atomic_write(path: Path, value: dict[str, object]) -> None:
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    temporary.replace(path)


def new_session_id() -> str:
    return f"{int(time.time() * 1000)}-{os.getpid()}-{secrets.token_hex(6)}"


def start(args: argparse.Namespace) -> int:
    state_path = Path(args.state).resolve()
    output_path = Path(args.output).resolve()
    error_path = Path(args.error).resolve()
    state_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    error_path.parent.mkdir(parents=True, exist_ok=True)
    command = args.command
    with output_path.open("ab") as output, error_path.open("ab") as error:
        process = subprocess.Popen(
            command,
            stdout=output,
            stderr=error,
            cwd=args.cwd or None,
            start_new_session=(os.name != "nt"),
            creationflags=(subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0),
        )
        record: dict[str, object] = {
            "schema_version": 1,
            "session_id": new_session_id(),
            "pid": process.pid,
            "command": command,
            "cwd": str(Path(args.cwd).resolve()) if args.cwd else None,
            "state": "running",
            "started_unix": time.time(),
            "output_path": str(output_path),
            "error_path": str(error_path),
        }
        atomic_write(state_path, record)
        print(json.dumps({"event": "session_started", "session_id": record["session_id"],
                          "pid": process.pid, "state_path": str(state_path)}, separators=(",", ":")),
              flush=True)
        return_code = process.wait()
    record["state"] = "completed" if return_code == 0 else "failed"
    record["returncode"] = return_code
    record["finished_unix"] = time.time()
    atomic_write(state_path, record)
    return return_code


def stop(args: argparse.Namespace) -> int:
    state_path = Path(args.state).resolve()
    record = json.loads(state_path.read_text(encoding="utf-8"))
    if record.get("state") != "running":
        print(json.dumps({"event": "session_not_running", "state": record.get("state")}), flush=True)
        return 0
    pid = int(record["pid"])
    if os.name == "nt":
        subprocess.run(["taskkill", "/PID", str(pid), "/T", "/F"], check=False,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    else:
        os.killpg(pid, signal.SIGTERM)
    record["state"] = "terminated"
    record["finished_unix"] = time.time()
    atomic_write(state_path, record)
    print(json.dumps({"event": "session_terminated", "session_id": record["session_id"]},
                     separators=(",", ":")), flush=True)
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Run a command with durable session state.")
    subparsers = parser.add_subparsers(dest="action", required=True)
    starter = subparsers.add_parser("start")
    starter.add_argument("--state", required=True)
    starter.add_argument("--output", required=True)
    starter.add_argument("--error", required=True)
    starter.add_argument("--cwd")
    starter.add_argument("command", nargs=argparse.REMAINDER)
    stopper = subparsers.add_parser("stop")
    stopper.add_argument("--state", required=True)
    args = parser.parse_args()
    if args.action == "start":
        if args.command and args.command[0] == "--":
            args.command = args.command[1:]
        if not args.command:
            parser.error("start requires a command")
        return start(args)
    return stop(args)


if __name__ == "__main__":
    sys.exit(main())
