# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Hosted static checks for the deterministic-core dependency boundary."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from datetime import datetime, timezone
from pathlib import Path
import re
import sys


REPO = Path(__file__).resolve().parents[3]
FORBIDDEN_INCLUDE_PATTERNS = tuple(
    re.compile(pattern, re.IGNORECASE)
    for pattern in (
        r"(?:^|[/\\])sokol_[^/\\]+\.h$",
        r"(?:^|[/\\])(?:sys/)?socket\.h$",
        r"(?:^|[/\\])winsock2?\.h$",
        r"(?:^|[/\\])ws2tcpip\.h$",
        r"(?:^|[/\\])windows\.h$",
        r"(?:^|[/\\])X11(?:[/\\]|\.h)",
        r"(?:^|[/\\])xcb(?:[/\\]|\.h)",
        r"(?:^|[/\\])(?:sys/time|mach/mach_time)\.h$",
        r"(?:^|[/\\])time\.h$",
        r"(?:^|[/\\])pthread\.h$",
        r"(?:^|[/\\])(?:Cocoa|AppKit|UIKit|Carbon)/",
    )
)
FORBIDDEN_HOST_CALLS = re.compile(
    r"\b(?:clock_gettime|timespec_get|clock|time|mach_absolute_time|"
    r"mach_continuous_time|QueryPerformanceCounter|GetTickCount64?|"
    r"SDL_GetTicks|SDL_GetPerformanceCounter|WSAStartup|closesocket|"
    r"socket|bind|listen|accept|connect|send|recv)\s*\(",
    re.IGNORECASE,
)
INCLUDE_LINE = re.compile(r"^\s*#\s*include\s*[<\"]([^>\"]+)[>\"]", re.MULTILINE)


def forbidden_include(path: str) -> bool:
    normalized = path.replace("\\", "/")
    return any(pattern.search(normalized) for pattern in FORBIDDEN_INCLUDE_PATTERNS)


def hash_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def core_sources() -> list[Path]:
    return sorted(
        path
        for path in (REPO / "src/core").rglob("*")
        if path.is_file() and path.suffix.lower() in {".c", ".h"}
    )


def check_cmake_report(report_path: Path) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    if not report_path.is_file():
        return ["CMake target inspection report is missing"], []
    report = {}
    for line in report_path.read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("=")
        if not separator or key in report:
            errors.append("CMake target inspection report is malformed")
            continue
        report[key] = value

    source_entries = report.get("core_sources", "").split(";")
    source_entries = [entry for entry in source_entries if entry]
    if not source_entries:
        errors.append("wz_core has no resolved source files")
    core_root = (REPO / "src/core").resolve()
    for entry in source_entries:
        source = Path(entry).resolve()
        try:
            source.relative_to(core_root)
        except ValueError:
            errors.append(f"wz_core source escapes src/core: {entry}")

    expected_sources = {
        path.resolve() for path in core_root.rglob("*.c") if path.is_file()
    }
    actual_sources = {Path(entry).resolve() for entry in source_entries}
    if actual_sources != expected_sources:
        missing = sorted(str(path) for path in expected_sources - actual_sources)
        unexpected = sorted(str(path) for path in actual_sources - expected_sources)
        errors.append(f"wz_core source inventory mismatch: missing={missing}, unexpected={unexpected}")

    core_links = set(filter(None, report.get("core_links", "").split(";")))
    if core_links != {"wz_warnings", "ZLIB::ZLIB"}:
        errors.append("wz_core direct link dependencies differ from the approved set")
    core_interface = set(filter(None, report.get("core_interface_links", "").split(";")))
    if core_interface != {"wz_warnings", "ZLIB::ZLIB"}:
        errors.append("wz_core interface link dependencies differ from the approved set")
    headless_links = list(filter(None, report.get("headless_links", "").split(";")))
    if headless_links != ["wz_core"]:
        errors.append("wz_headless must link directly to wz_core only")
    return errors, source_entries


def inspect(cmake_report: Path) -> tuple[list[str], list[dict[str, str]]]:
    errors: list[str] = []
    fixtures: list[dict[str, str]] = []
    sources = core_sources()
    if not sources:
        errors.append("no deterministic-core C or header files were found")

    for path in sources:
        relative = path.relative_to(REPO).as_posix()
        text = path.read_text(encoding="utf-8")
        fixtures.append({"path": relative, "sha256": hash_file(path)})
        for include in INCLUDE_LINE.findall(text):
            if forbidden_include(include):
                errors.append(f"forbidden host include in {relative}: {include}")
        if FORBIDDEN_HOST_CALLS.search(text):
            errors.append(f"forbidden host API call in {relative}")

    cmake_path = REPO / "src/cmake/CMakeLists.txt"
    fixtures.append(
        {"path": cmake_path.relative_to(REPO).as_posix(), "sha256": hash_file(cmake_path)}
    )
    cmake_errors, _ = check_cmake_report(cmake_report)
    errors.extend(cmake_errors)
    fixtures.append(
        {
            "path": cmake_report.resolve().relative_to(REPO).as_posix(),
            "sha256": hash_file(cmake_report),
        }
    )

    # Exercise both sides of the include rule so an accidentally weakened
    # pattern is detected by the hosted check itself.
    forbidden_controls = (
        "sokol_app.h",
        "sys/socket.h",
        "winsock2.h",
        "windows.h",
        "X11/Xlib.h",
        "xcb/xcb.h",
        "sys/time.h",
        "mach/mach_time.h",
        "time.h",
        "pthread.h",
        "AppKit/AppKit.h",
    )
    for include in forbidden_controls:
        if not forbidden_include(include):
            errors.append(f"negative control failed to detect forbidden include {include}")
    if forbidden_include("stdint.h"):
        errors.append("positive control incorrectly rejected a standard C header")
    return errors, fixtures


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cmake-report", required=True, type=Path)
    parser.add_argument("--proof-out", required=True, type=Path)
    arguments = parser.parse_args()

    errors, fixtures = inspect(arguments.cmake_report)
    run_id = os.environ.get("GITHUB_RUN_ID", "local")
    proof = {
        "testId": "core-dependency-boundary",
        "status": "pass" if not errors else "fail",
        "commit": os.environ.get("GITHUB_SHA", "uncommitted"),
        "runner": f"{os.environ.get('RUNNER_OS', 'local')}/{os.environ.get('RUNNER_ARCH', 'unknown')}",
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "runId": run_id,
        "fixtures": fixtures,
        "errors": errors,
    }
    arguments.proof_out.parent.mkdir(parents=True, exist_ok=True)
    arguments.proof_out.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    if errors:
        for error in errors:
            print(f"FAIL {error}", file=sys.stderr)
        return 1
    print(f"PASS checked {len(fixtures) - 2} core sources and the CMake dependency graph")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
