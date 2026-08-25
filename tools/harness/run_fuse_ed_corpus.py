#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

"""Run a selected pinned Fuse vector family through the project-owned runner."""

from __future__ import annotations

import argparse
import json
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path

PINNED_COMMIT = "9cbab635f5c9dfdfc5cb769aa89048c7e624d6b7"
REGISTER_COUNT = 13
STACK_SUBROUTINE_OPCODES = frozenset({
    "c0", "c1", "c4", "c5", "c7", "c8", "c9", "cc", "cd", "cf",
    "d0", "d1", "d4", "d5", "d7", "d8", "dc", "df",
    "e0", "e1", "e4", "e5", "e7", "e8", "ec", "ef",
    "f0", "f1", "f4", "f5", "f7", "f8", "fc", "ff",
})
BRANCH_OPCODES = frozenset({
    "10", "18", "20", "28", "30", "38", "c2", "c3", "ca", "d2",
    "da", "e2", "e9", "ea", "f2", "fa",
})
INC_DEC_OPCODES = frozenset({
    "04", "05", "0c", "0d", "14", "15", "1c", "1d",
    "24", "25", "2c", "2d", "34", "35", "3c", "3d",
})


@dataclass
class FuseCase:
    name: str
    source: str
    registers: list[int]
    auxiliary: list[int]
    memory: dict[int, int]
    events: list[str]


def _is_register_line(line: str) -> bool:
    fields = line.split()
    if len(fields) != REGISTER_COUNT:
        return False
    try:
        [int(field, 16) for field in fields]
    except ValueError:
        return False
    return True


def _memory_lines(lines: list[str]) -> dict[int, int]:
    memory: dict[int, int] = {}
    for line in lines:
        fields = line.split()
        if not fields or fields[0] == "-1":
            continue
        address = int(fields[0], 16)
        for field in fields[1:]:
            if field == "-1":
                break
            memory[address & 0xFFFF] = int(field, 16)
            address += 1
    return memory


def parse_inputs(path: Path) -> dict[str, FuseCase]:
    lines = path.read_text(encoding="ascii").splitlines()
    cases: dict[str, FuseCase] = {}
    index = 0
    while index < len(lines):
        if not lines[index].strip():
            index += 1
            continue
        name = lines[index].strip()
        start = index
        registers = [int(value, 16) for value in lines[index + 1].split()]
        auxiliary = [int(value, 16) for value in lines[index + 2].split()[:2]]
        auxiliary.extend(int(value, 10) for value in lines[index + 2].split()[2:])
        index += 3
        memory_lines: list[str] = []
        while index < len(lines):
            memory_lines.append(lines[index])
            index += 1
            if memory_lines[-1].strip() == "-1":
                break
        source = "\n".join(lines[start:index]) + "\n"
        cases[name] = FuseCase(name, source, registers, auxiliary,
                               _memory_lines(memory_lines), [])
    return cases


def parse_expected(path: Path) -> dict[str, FuseCase]:
    lines = path.read_text(encoding="ascii").splitlines()
    cases: dict[str, FuseCase] = {}
    index = 0
    while index < len(lines):
        if not lines[index].strip():
            index += 1
            continue
        name = lines[index].strip()
        index += 1
        events: list[str] = []
        while index < len(lines) and not _is_register_line(lines[index]):
            events.append(lines[index].strip())
            index += 1
        if index >= len(lines):
            raise ValueError(f"missing register result for {name}")
        registers = [int(value, 16) for value in lines[index].split()]
        index += 1
        fields = lines[index].split()
        auxiliary = [int(value, 16) for value in fields[:2]]
        auxiliary.extend(int(value, 10) for value in fields[2:])
        index += 1
        memory_lines: list[str] = []
        while index < len(lines) and lines[index].strip():
            memory_lines.append(lines[index])
            index += 1
        cases[name] = FuseCase(name, "", registers, auxiliary,
                               _memory_lines(memory_lines), events)
    return cases


def parse_runner_output(output: str) -> tuple[list[int], list[int], dict[int, int]]:
    registers: list[int] = []
    auxiliary: list[int] = []
    memory: dict[int, int] = {}
    for line in output.splitlines():
        fields = line.split()
        if not fields:
            continue
        if fields[0] == "RESULT":
            registers = [int(value, 16) for value in fields[1:]]
        elif fields[0] == "AUX":
            auxiliary = [int(value, 16) for value in fields[1:3]]
            auxiliary.extend(int(value, 10) for value in fields[3:])
        elif fields[0] == "MEM":
            memory[int(fields[1], 16)] = int(fields[2], 16)
    if len(registers) != REGISTER_COUNT or len(auxiliary) != 7:
        raise ValueError("case runner returned incomplete state")
    return registers, auxiliary, memory


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--runner", required=True, type=Path)
    parser.add_argument("--corpus", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--selection", choices=(
        "complete", "ed", "indexed-cb", "cb-rotate-shift", "stack-subroutine", "branch",
        "inc-dec", "halt"
    ),
                        default="complete")
    args = parser.parse_args()
    if args.commit != PINNED_COMMIT:
        parser.error("corpus commit does not match the pinned identity")

    inputs = parse_inputs(args.corpus / "tests.in")
    expected = parse_expected(args.corpus / "tests.expected")
    if args.selection == "complete":
        names = sorted(inputs)
        expected_count = len(inputs)
        selection_description = "every pinned Fuse Z80 vector from tests.in"
    elif args.selection == "ed":
        names = sorted(name for name in inputs if name.startswith("ed"))
        expected_count = 109
        selection_description = "all case names beginning with ed"
    elif args.selection == "indexed-cb":
        names = sorted(name for name in inputs
                       if name.startswith("ddcb") or name.startswith("fdcb"))
        expected_count = 512
        selection_description = "all case names beginning with ddcb or fdcb"
    elif args.selection == "cb-rotate-shift":
        names = sorted(name for name in inputs
                       if len(name) == 4 and name.startswith("cb") and
                       int(name[2:], 16) < 0x40)
        expected_count = 64
        selection_description = "all CB rotate/shift cases from cb00 through cb3f"
    elif args.selection == "stack-subroutine":
        names = sorted(name for name in inputs
                       if name.split("_", 1)[0] in STACK_SUBROUTINE_OPCODES)
        expected_count = 50
        selection_description = (
            "all CALL, RET, RST, PUSH, and POP cases including conditional paths"
        )
    elif args.selection == "branch":
        names = sorted(name for name in inputs
                       if name.split("_", 1)[0] in BRANCH_OPCODES)
        expected_count = 28
        selection_description = (
            "all JR, JP, and DJNZ cases including taken and not-taken paths"
        )
    elif args.selection == "inc-dec":
        names = sorted(name for name in inputs
                       if name.split("_", 1)[0] in INC_DEC_OPCODES)
        expected_count = 16
        selection_description = "all primary INC and DEC register and memory cases"
    else:
        names = ["76"] if "76" in inputs else []
        expected_count = 1
        selection_description = "the primary HALT state and timing case"
    results: list[dict[str, object]] = []
    with tempfile.TemporaryDirectory(prefix=f"wzsn-fuse-{args.selection}-") as temporary:
        case_path = Path(temporary) / "case.in"
        for name in names:
            item: dict[str, object] = {
                "name": name,
                "event_assertions": "not_applicable",
                "event_reason": "Exact external bus-event projection is delivered by Phase-2 task 059; final CPU state, timing, and memory remain applicable.",
            }
            if name not in expected:
                item.update(status="failed", reason="missing expected case")
                results.append(item)
                continue
            case_path.write_text(inputs[name].source, encoding="ascii")
            completed = subprocess.run(
                [str(args.runner), str(case_path)],
                check=False,
                capture_output=True,
                text=True,
            )
            if completed.returncode != 0:
                item.update(status="failed", reason=completed.stderr.strip())
            else:
                actual = parse_runner_output(completed.stdout)
                reference = expected[name]
                mismatches: list[str] = []
                if actual[0] != reference.registers:
                    mismatches.append("registers")
                if actual[1] != reference.auxiliary:
                    mismatches.append("auxiliary/timing")
                if actual[2] != reference.memory:
                    mismatches.append("memory")
                item.update(status="passed" if not mismatches else "failed",
                            mismatches=mismatches)
            results.append(item)

    passed = sum(item["status"] == "passed" for item in results)
    manifest = {
        "schema_version": 1,
        "corpus": "Fuse Z80",
        "commit": PINNED_COMMIT,
        "suite_path": "z80/tests",
        "selection": selection_description,
        "total": len(results),
        "passed": passed,
        "failed": len(results) - passed,
        "silent_skips": 0,
        "cases": results,
    }
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.manifest.write_text(json.dumps(manifest, indent=2) + "\n", encoding="ascii")
    print(json.dumps({key: manifest[key] for key in
                      ("commit", "total", "passed", "failed", "silent_skips")}))
    return 0 if len(results) == expected_count and passed == len(results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
