#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Audit host runtime dependencies of a native Linux or macOS build."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import subprocess


def run(*command: str) -> str:
    return subprocess.check_output(command, text=True, stderr=subprocess.STDOUT)


def linux_dependencies(binary: Path) -> list[dict[str, str]]:
    output = run("ldd", str(binary))
    dependencies = []
    for raw_line in output.splitlines():
        line = raw_line.strip()
        if not line or line.startswith("linux-vdso"):
            continue
        if "not found" in line:
            raise ValueError(f"unresolved runtime dependency: {line}")
        if "=>" in line:
            name, target = (part.strip() for part in line.split("=>", 1))
            path = target.split(" ", 1)[0]
        else:
            match = re.match(r"(/\S+)\s+\(", line)
            if not match:
                raise ValueError(f"unrecognized ldd output: {line}")
            path = match.group(1)
            name = Path(path).name
        resolved = os.path.realpath(path)
        if not resolved.startswith(("/lib/", "/lib64/", "/usr/lib/")):
            raise ValueError(f"non-system runtime dependency path: {resolved}")
        dependencies.append({"name": name, "path": resolved})
    if not dependencies:
        raise ValueError("ldd did not report any runtime dependencies")
    return sorted(dependencies, key=lambda item: item["name"])


def macos_dependencies(binary: Path) -> list[dict[str, str]]:
    output = run("otool", "-L", str(binary))
    dependencies = []
    for raw_line in output.splitlines()[1:]:
        line = raw_line.strip()
        path = line.split(" (compatibility version", 1)[0]
        if not path:
            continue
        if not path.startswith(("/System/Library/", "/usr/lib/")):
            raise ValueError(f"non-system runtime dependency path: {path}")
        dependencies.append({"name": Path(path).name, "path": path})
    if not dependencies:
        raise ValueError("otool did not report any runtime dependencies")
    return sorted(dependencies, key=lambda item: item["name"])


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--runner-id", required=True)
    parser.add_argument("--expected-architecture", choices=("x64", "arm64"), required=True)
    parser.add_argument("--proof-out", required=True, type=Path)
    args = parser.parse_args()

    if not args.binary.is_file():
        raise SystemExit(f"release executable not found: {args.binary}")
    actual_architecture = platform.machine().casefold()
    accepted_architectures = {
        "x64": {"x86_64", "amd64"},
        "arm64": {"aarch64", "arm64"},
    }
    if actual_architecture not in accepted_architectures[args.expected_architecture]:
        raise SystemExit(
            f"expected {args.expected_architecture}, got {actual_architecture}"
        )

    if platform.system() == "Linux":
        dependencies = linux_dependencies(args.binary)
    elif platform.system() == "Darwin":
        dependencies = macos_dependencies(args.binary)
    else:
        raise SystemExit(f"unsupported audit host: {platform.system()}")

    binary_bytes = args.binary.read_bytes()
    proof = {
        "runnerId": args.runner_id,
        "os": platform.system(),
        "architecture": actual_architecture,
        "binary": args.binary.name,
        "binarySha256": hashlib.sha256(binary_bytes).hexdigest(),
        "dependencies": dependencies,
    }
    args.proof_out.parent.mkdir(parents=True, exist_ok=True)
    args.proof_out.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(f"PASS {args.runner_id}: {len(dependencies)} resolved system dependencies")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
