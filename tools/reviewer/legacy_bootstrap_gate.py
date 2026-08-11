#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

"""Retained independent one-pass gate for reviewer-harness bootstrap audits."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from deepseek_gate import DeepSeekClient, Telemetry, code_packet, resolve_inside


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base", required=True)
    parser.add_argument("--head", required=True)
    parser.add_argument("--requirements", required=True)
    args = parser.parse_args()
    root = Path.cwd().resolve()
    snapshot, records = code_packet(root, args.base, args.head)
    requirements = resolve_inside(root, args.requirements).read_text(encoding="utf-8")
    prompt = (
        "Return JSON only. Exhaustively review the complete immutable change for high-confidence BLOCKER/HIGH "
        "correctness, security, reliability, integration, and test-validity defects. Continue after each finding "
        "and silently self-challenge before final output. Never emit hidden reasoning.\n"
        f"SNAPSHOT={snapshot}\nREQUIREMENTS={requirements}\nCHANGE={records[0][1]}\n"
        "Return {\"review_complete\":true,\"verdict\":\"PASS|FAIL\",\"findings\":[]} as JSON."
    )
    telemetry = Telemetry("CODE", snapshot)
    result = DeepSeekClient().request("You are the trusted bootstrap software review gate. JSON only.", prompt, telemetry)
    valid = (result.get("review_complete") is True and result.get("verdict") in {"PASS", "FAIL"}
             and isinstance(result.get("findings"), list))
    if not valid:
        result = {"review_complete": False, "verdict": "INCONCLUSIVE", "reason": "invalid bootstrap response"}
    print(json.dumps({"review_type": "CODE", "snapshot_id": snapshot, **result},
                     separators=(",", ":"), sort_keys=True))
    return 0 if result.get("verdict") == "PASS" else 2


if __name__ == "__main__":
    sys.exit(main())
