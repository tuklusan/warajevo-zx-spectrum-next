#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

"""Best-effort UDP syslog forwarding for approved harness evidence trees."""

from __future__ import annotations

import argparse
import base64
import json
import os
import socket
from pathlib import Path


HOST = "sanyalnet-oracle-vps2.duckdns.org"
PORT = 65514
FACILITY = 1
SEVERITY = 6
MAX_PACKET = 1024
PREFIX = f"<{FACILITY * 8 + SEVERITY}>1 - wzsn-harness - - - - "
CHUNK_BYTES = 480


def enabled() -> bool:
    return os.environ.get("WZ_TRACE_FORWARD") in {"Y", "1"}


def _packet(payload: dict[str, object]) -> bytes:
    return (PREFIX + json.dumps(payload, separators=(",", ":"), sort_keys=True)).encode("utf-8")


def _send(sock: socket.socket, payload: dict[str, object]) -> bool:
    packet = _packet(payload)
    if len(packet) > MAX_PACKET:
        return False
    try:
        sock.sendto(packet, (HOST, PORT))
    except (OSError, socket.timeout):
        return False
    return True


def forward_tree(tree: Path, run_id: str, lane: str, phase: str) -> dict[str, int]:
    counts = {"files": 0, "packets": 0, "failed": 0}
    if not enabled():
        return counts
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setblocking(False)
    except OSError:
        return counts
    try:
        for path in sorted(p for p in tree.rglob("*") if p.is_file()):
            counts["files"] += 1
            try:
                data = path.read_bytes()
            except OSError:
                counts["failed"] += 1
                continue
            encoded = base64.b64encode(data)
            total = max(1, (len(encoded) + CHUNK_BYTES - 1) // CHUNK_BYTES)
            for index in range(total):
                payload = {
                    "run_id": run_id,
                    "lane": lane,
                    "phase": phase,
                    "path": path.as_posix(),
                    "encoding": "base64",
                    "chunk": index + 1,
                    "chunks": total,
                    "data": encoded[index * CHUNK_BYTES:(index + 1) * CHUNK_BYTES].decode("ascii"),
                }
                if _send(sock, payload):
                    counts["packets"] += 1
                else:
                    counts["failed"] += 1
    finally:
        sock.close()
    return counts


def main() -> int:
    parser = argparse.ArgumentParser(description="Forward a harness evidence tree over UDP syslog.")
    parser.add_argument("--tree", required=True, type=Path)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--lane", required=True)
    parser.add_argument("--phase", required=True)
    args = parser.parse_args()
    result = forward_tree(args.tree, args.run_id, args.lane, args.phase)
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
