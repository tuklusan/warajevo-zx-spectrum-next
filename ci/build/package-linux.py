#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Create and audit the frozen four-file Linux release archive."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import io
from pathlib import Path
import re
import struct
import tarfile


ROOT = Path(__file__).resolve().parents[2]
PRODUCT = "Warajevo-ZX-Spectrum-Next"
NUKLEAR_COMMIT = "a53ad2c658151071501372a5e0e5e978153835aa"
SOKOL_COMMIT = "1847290135f95e57e6d220b0a41208306aafc0dd"
ELF_MACHINE = {"x86_64": 62, "aarch64": 183}


def read_third_party_notices(nuklear_license: Path, sokol_header: Path) -> str:
    nuklear = nuklear_license.read_text(encoding="utf-8").strip()
    sokol_source = sokol_header.read_text(encoding="utf-8")
    match = re.search(
        r"(?ms)^\s*zlib/libpng license\s*(.*?)^\s*\*/",
        sokol_source,
    )
    if match is None:
        raise ValueError("pinned Sokol helper license block was not found")
    sokol_license = match.group(1).strip()
    if "MIT License" not in nuklear or "Micha Mettke" not in nuklear:
        raise ValueError("Nuklear license text does not match the pinned license")
    if "Warren Merrifield" not in sokol_license or "redistribute it" not in sokol_license:
        raise ValueError("Sokol helper license text does not match its pinned header")
    return (
        "# Third-party notices\n\n"
        "## Nuklear\n\n"
        f"Nuklear is pinned to commit `{NUKLEAR_COMMIT}`. Its upstream license\n"
        "offers the choice of the MIT License or public-domain dedication.\n\n"
        f"{nuklear}\n\n"
        "## Sokol Nuklear helper\n\n"
        f"`util/sokol_nuklear.h` is pinned to Sokol commit `{SOKOL_COMMIT}`.\n"
        "Applicable upstream license:\n\n"
        f"{sokol_license}\n"
    )


def check_elf(binary: Path, architecture: str) -> None:
    header = binary.read_bytes()[:20]
    if len(header) < 20 or header[:4] != b"\x7fELF":
        raise ValueError("release executable is not an ELF binary")
    if header[4] != 2 or header[5] != 1:
        raise ValueError("release executable must be a 64-bit little-endian ELF")
    machine = struct.unpack_from("<H", header, 18)[0]
    if machine != ELF_MACHINE[architecture]:
        raise ValueError(f"ELF machine {machine} does not match {architecture}")


def add_file(archive: tarfile.TarFile, source: Path, name: str, mode: int) -> None:
    contents = source.read_bytes()
    info = tarfile.TarInfo(f"{PRODUCT}/{name}")
    info.size = len(contents)
    info.mode = mode
    info.uid = 0
    info.gid = 0
    info.uname = ""
    info.gname = ""
    info.mtime = 0
    archive.addfile(info, io.BytesIO(contents))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--architecture", required=True, choices=ELF_MACHINE)
    parser.add_argument("--nuklear-license", required=True, type=Path)
    parser.add_argument("--sokol-header", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    binary = args.binary.resolve(strict=True)
    if not binary.is_file() or not binary.stat().st_mode & 0o111:
        raise SystemExit("release executable is missing or not executable")
    check_elf(binary, args.architecture)
    notices = read_third_party_notices(
        args.nuklear_license.resolve(strict=True),
        args.sokol_header.resolve(strict=True),
    ).encode("utf-8")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    archive_name = f"{PRODUCT}-linux-{args.architecture}.tar.gz"
    archive_path = args.output / archive_name
    readme = ROOT / "ci/build/linux-package-README.md"
    license_file = ROOT / "LICENSE"
    files = {
        PRODUCT: (binary, 0o755),
        "README.md": (readme, 0o644),
        "LICENSE": (license_file, 0o644),
    }
    for path, _mode in files.values():
        if not path.is_file():
            raise SystemExit(f"required release file is missing: {path}")

    with archive_path.open("wb") as raw:
        with gzip.GzipFile(filename="", mode="wb", fileobj=raw, mtime=0) as compressed:
            with tarfile.open(fileobj=compressed, mode="w", format=tarfile.USTAR_FORMAT) as archive:
                for name, (source, mode) in files.items():
                    add_file(archive, source, name, mode)
                info = tarfile.TarInfo(f"{PRODUCT}/THIRD-PARTY-NOTICES.md")
                info.size = len(notices)
                info.mode = 0o644
                info.uid = info.gid = info.mtime = 0
                info.uname = info.gname = ""
                archive.addfile(info, io.BytesIO(notices))

    expected = {
        f"{PRODUCT}/{PRODUCT}",
        f"{PRODUCT}/README.md",
        f"{PRODUCT}/LICENSE",
        f"{PRODUCT}/THIRD-PARTY-NOTICES.md",
    }
    with tarfile.open(archive_path, "r:gz") as archive:
        actual = {member.name for member in archive.getmembers()}
        if actual != expected or len(archive.getmembers()) != len(expected):
            raise SystemExit(f"archive contents mismatch: {sorted(actual)}")
        if any(not archive.extractfile(name) for name in actual):
            raise SystemExit("archive contains an unreadable file")

    digest = hashlib.sha256(archive_path.read_bytes()).hexdigest()
    checksum = args.output / f"{archive_name}.sha256"
    checksum.write_text(f"{digest}  {archive_name}\n", encoding="ascii")
    print(f"PASS package {archive_path.name} SHA256={digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
