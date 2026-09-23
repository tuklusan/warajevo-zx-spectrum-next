# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms, warranty disclaimer, termination,
# patent, trademark, and governing-law provisions.

"""Compare one hosted performance run with its frozen Phase-16 baseline."""

import json
import sys
from pathlib import Path

MAX_SLOWDOWN = 0.40


def load(path: Path) -> dict:
    with path.open(encoding="utf-8") as stream:
        value = json.load(stream)
    if value.get("schema") != 1 or value.get("status") != "pass":
        raise ValueError(f"{path} is not a passing schema-1 record")
    return value


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: compare.py CURRENT.json BASELINE.json", file=sys.stderr)
        return 2

    try:
        current = load(Path(sys.argv[1]))
        baseline = load(Path(sys.argv[2]))
        if current.get("runner") != baseline.get("runner") or \
                current.get("architecture") != baseline.get("architecture"):
            raise ValueError("current run does not match its runner baseline")
        current_metrics = {item["name"]: item for item in current["metrics"]}
        baseline_metrics = {item["name"]: item for item in baseline["metrics"]}
        if not current_metrics or current_metrics.keys() != baseline_metrics.keys():
            raise ValueError("current and baseline metric sets differ")

        failed = False
        for name, metric in current_metrics.items():
            reference = baseline_metrics[name]
            if metric["unit"] != reference["unit"] or \
                    metric["operations"] != reference["operations"]:
                raise ValueError(f"{name}: workload shape differs from baseline")
            if metric["fingerprint"] != reference["fingerprint"]:
                print(f"FAIL {name}: deterministic fingerprint changed")
                failed = True
                continue
            reference_rate = float(reference["operations_per_second"])
            current_rate = float(metric["operations_per_second"])
            if reference_rate <= 0.0 or current_rate <= 0.0:
                raise ValueError(f"{name}: throughput must be positive")
            change = current_rate / reference_rate - 1.0
            state = "FAIL" if change < -MAX_SLOWDOWN else "PASS"
            print(f"{state} {name}: {change:+.1%} vs baseline")
            failed = failed or state == "FAIL"
        return 1 if failed else 0
    except (KeyError, OSError, TypeError, ValueError, json.JSONDecodeError) as error:
        print(f"invalid performance record: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
