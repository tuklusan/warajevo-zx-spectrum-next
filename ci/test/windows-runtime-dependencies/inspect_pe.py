#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Audit the Windows executable's normal and delay-loaded PE imports."""

import argparse
import hashlib
import json
import os
from datetime import datetime, timezone
from pathlib import Path
import struct
import subprocess


SYSTEM_DLLS = frozenset(
    name.casefold()
    for name in (
        "advapi32.dll", "bcrypt.dll", "combase.dll", "comdlg32.dll",
        "d3d11.dll", "d3d12.dll", "d3dcompiler_47.dll", "dwmapi.dll",
        "dxgi.dll", "gdi32.dll", "imm32.dll", "kernel32.dll",
        "kernelbase.dll", "msvcp140.dll", "msvcp140_1.dll",
        "msvcp_win.dll", "ntdll.dll", "ole32.dll", "oleaut32.dll",
        "opengl32.dll", "shcore.dll", "shell32.dll", "ucrtbase.dll",
        "user32.dll", "vcruntime140.dll", "vcruntime140_1.dll",
        "version.dll", "vulkan-1.dll", "winmm.dll", "ws2_32.dll",
    )
)


def imports_from_pe(image: bytes) -> tuple[int, list[str]]:
    if image[:2] != b"MZ":
        raise ValueError("application is missing the DOS executable signature")
    pe_offset = struct.unpack_from("<I", image, 0x3C)[0]
    if image[pe_offset:pe_offset + 4] != b"PE\0\0":
        raise ValueError("application is missing the PE signature")

    machine = struct.unpack_from("<H", image, pe_offset + 4)[0]
    section_count = struct.unpack_from("<H", image, pe_offset + 6)[0]
    optional_size = struct.unpack_from("<H", image, pe_offset + 20)[0]
    optional_offset = pe_offset + 24
    magic = struct.unpack_from("<H", image, optional_offset)[0]
    if magic == 0x20B:
        directory_offset = optional_offset + 112
        image_base = struct.unpack_from("<Q", image, optional_offset + 24)[0]
        directory_count_offset = optional_offset + 108
    elif magic == 0x10B:
        directory_offset = optional_offset + 96
        image_base = struct.unpack_from("<I", image, optional_offset + 28)[0]
        directory_count_offset = optional_offset + 92
    else:
        raise ValueError(f"unsupported PE optional-header magic: {magic:#x}")

    directory_count = struct.unpack_from("<I", image, directory_count_offset)[0]
    section_offset = optional_offset + optional_size
    sections = []
    for index in range(section_count):
        offset = section_offset + index * 40
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from(
            "<IIII", image, offset + 8
        )
        sections.append((virtual_address, max(virtual_size, raw_size), raw_offset))

    def rva_to_offset(rva: int) -> int:
        for address, size, raw_offset in sections:
            if address <= rva < address + size:
                offset = raw_offset + rva - address
                if offset >= len(image):
                    break
                return offset
        raise ValueError(f"PE RVA is not mapped to file data: {rva:#x}")

    def read_name(rva: int) -> str:
        offset = rva_to_offset(rva)
        end = image.find(b"\0", offset)
        if end < 0:
            raise ValueError("unterminated PE import name")
        return image[offset:end].decode("ascii").casefold()

    found: set[str] = set()

    def read_import_directory(directory_index: int, delay: bool) -> None:
        if directory_count <= directory_index:
            return
        rva, size = struct.unpack_from("<II", image, directory_offset + 8 * directory_index)
        if rva == 0 or size == 0:
            return
        offset = rva_to_offset(rva)
        entry_size = 32 if delay else 20
        for index in range(size // entry_size + 1):
            entry_offset = offset + index * entry_size
            if delay:
                attributes, name_value = struct.unpack_from("<II", image, entry_offset)
                if attributes == 0 and name_value == 0:
                    break
                name_rva = name_value if attributes & 1 else name_value - image_base
            else:
                descriptor = struct.unpack_from("<IIIII", image, entry_offset)
                if not any(descriptor):
                    break
                name_rva = descriptor[3]
            found.add(read_name(name_rva))
        else:
            raise ValueError("PE import directory has no terminator")

    read_import_directory(1, delay=False)
    read_import_directory(13, delay=True)
    return machine, sorted(found)


def committed_fixture(path: str) -> dict[str, str]:
    data = subprocess.check_output(["git", "show", f"HEAD:{path}"])
    return {"path": path, "sha256": hashlib.sha256(data).hexdigest()}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--driver", required=True, type=Path)
    parser.add_argument("--proof-out", required=True, type=Path)
    args = parser.parse_args()

    driver = json.loads(args.driver.read_text(encoding="utf-8"))
    fixtures = []
    for fixture in driver["fixtures"]:
        actual = committed_fixture(fixture["path"])
        if fixture["sha256"] != actual["sha256"]:
            raise SystemExit(f"pinned fixture changed: {fixture['path']}")
        fixtures.append(actual)

    image = args.binary.read_bytes()
    machine, imports = imports_from_pe(image)
    if machine != 0x8664:
        raise SystemExit(f"expected x86-64 PE executable, got machine {machine:#x}")
    if "kernel32.dll" not in imports:
        raise SystemExit("executable import table is missing kernel32.dll")
    unexpected = [
        name for name in imports
        if name not in SYSTEM_DLLS
        and not name.startswith("api-ms-win-")
        and not name.startswith("ext-ms-win-")
    ]
    if unexpected:
        raise SystemExit("non-system runtime imports detected: " + ", ".join(unexpected))

    proof = {
        "testId": driver["testId"],
        "status": "pass",
        "commit": os.environ["GITHUB_SHA"],
        "runId": os.environ["GITHUB_RUN_ID"],
        "runner": f"{os.environ.get('RUNNER_OS', 'Windows')}/{os.environ.get('RUNNER_ARCH', 'X64')}",
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "binary": args.binary.name,
        "binarySha256": hashlib.sha256(image).hexdigest(),
        "peMachine": f"{machine:#06x}",
        "imports": imports,
        "fixturePolicy": "only Windows system DLLs and Windows API-set DLLs are imported",
        "fixtures": fixtures,
    }
    args.proof_out.parent.mkdir(parents=True, exist_ok=True)
    args.proof_out.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(f"PASS {len(imports)} imported DLLs are Windows system/API-set libraries")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
