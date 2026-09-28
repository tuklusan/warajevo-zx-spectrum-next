#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Create and audit deterministic Windows or macOS desktop archives."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import plistlib
import re
import struct
import zipfile

ROOT = Path(__file__).resolve().parents[2]
PRODUCT = "Warajevo-ZX-Spectrum-Next"
NUKLEAR_COMMIT = "a53ad2c658151071501372a5e0e5e978153835aa"
SOKOL_COMMIT = "1847290135f95e57e6d220b0a41208306aafc0dd"
PLATFORMS = {
    "windows-x86_64": ("windows", "x86_64", 0x8664),
    "macos-x86_64": ("macos", "x86_64", 0x01000007),
    "macos-arm64": ("macos", "arm64", 0x0100000C),
}


def third_party_notices(nuklear_license: Path, sokol_header: Path) -> bytes:
    nuklear = nuklear_license.read_text(encoding="utf-8").strip()
    sokol_source = sokol_header.read_text(encoding="utf-8")
    match = re.search(r"(?ms)^\s*zlib/libpng license\s*(.*?)^\s*\*/", sokol_source)
    if match is None:
        raise ValueError("pinned Sokol helper license block was not found")
    sokol_license = match.group(1).strip()
    if "MIT License" not in nuklear or "Micha Mettke" not in nuklear:
        raise ValueError("Nuklear license does not match the pinned upstream text")
    if "Warren Merrifield" not in sokol_license or "redistribute it" not in sokol_license:
        raise ValueError("Sokol helper license does not match the pinned upstream text")
    text = (
        "# Third-party notices\n\n## Nuklear\n\n"
        f"Nuklear is pinned to commit `{NUKLEAR_COMMIT}`. Its upstream license\n"
        "offers the MIT License or public-domain dedication.\n\n"
        f"{nuklear}\n\n## Sokol Nuklear helper\n\n"
        f"`util/sokol_nuklear.h` is pinned to Sokol commit `{SOKOL_COMMIT}`.\n"
        f"Applicable upstream license:\n\n{sokol_license}\n"
    )
    return text.encode("utf-8")


def verify_executable(binary: bytes, platform: str, architecture: str) -> None:
    expected = PLATFORMS[f"{platform}-{architecture}"][2]
    if platform == "windows":
        if len(binary) < 0x40 or binary[:2] != b"MZ":
            raise ValueError("release executable is not a PE image")
        pe_offset = struct.unpack_from("<I", binary, 0x3C)[0]
        if pe_offset + 6 > len(binary) or binary[pe_offset:pe_offset + 4] != b"PE\0\0":
            raise ValueError("release executable has an invalid PE header")
        actual = struct.unpack_from("<H", binary, pe_offset + 4)[0]
    else:
        if len(binary) < 8:
            raise ValueError("release executable is too short to be Mach-O")
        magic = binary[:4]
        if magic == b"\xcf\xfa\xed\xfe":
            actual = struct.unpack_from("<I", binary, 4)[0]
        elif magic == b"\xfe\xed\xfa\xcf":
            actual = struct.unpack_from(">I", binary, 4)[0]
        else:
            raise ValueError("release executable is not a 64-bit Mach-O image")
    if actual != expected:
        raise ValueError(
            f"executable architecture mismatch: expected {expected:#x}, got {actual:#x}"
        )


def zip_entry(archive: zipfile.ZipFile, name: str, data: bytes, mode: int) -> None:
    info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
    info.compress_type = zipfile.ZIP_DEFLATED
    info.create_system = 3
    info.external_attr = (mode & 0xFFFF) << 16
    archive.writestr(info, data)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--platform", required=True, choices=PLATFORMS)
    parser.add_argument("--nuklear-license", required=True, type=Path)
    parser.add_argument("--sokol-header", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    binary_path = args.binary.resolve(strict=True)
    if not binary_path.is_file():
        raise SystemExit("release executable is not a regular file")
    platform, architecture, _machine = PLATFORMS[args.platform]
    binary = binary_path.read_bytes()
    verify_executable(binary, platform, architecture)
    readme = (ROOT / "ci/build/desktop-package-README.md").read_bytes()
    license_text = (ROOT / "LICENSE").read_bytes()
    notices = third_party_notices(
        args.nuklear_license.resolve(strict=True),
        args.sokol_header.resolve(strict=True),
    )

    if platform == "windows":
        executable_name = f"{PRODUCT}.exe"
        entries = {
            f"{PRODUCT}/{executable_name}": (binary, 0o755),
            f"{PRODUCT}/README.md": (readme, 0o644),
            f"{PRODUCT}/LICENSE": (license_text, 0o644),
            f"{PRODUCT}/THIRD-PARTY-NOTICES.md": (notices, 0o644),
        }
    else:
        app = "Warajevo ZX Spectrum Next.app"
        app_root = f"{PRODUCT}/{app}"
        executable_name = PRODUCT
        info = plistlib.dumps({
            "CFBundleExecutable": executable_name,
            "CFBundleIdentifier": "net.sanyalnet.warajevo-zx-spectrum-next",
            "CFBundleName": "Warajevo ZX Spectrum Next",
            "CFBundlePackageType": "APPL",
            "CFBundleShortVersionString": "1.0",
            "CFBundleVersion": "1.0",
            "LSApplicationCategoryType": "public.app-category.games",
            "NSHighResolutionCapable": True,
        }, fmt=plistlib.FMT_XML, sort_keys=True)
        entries = {
            f"{app_root}/Contents/Info.plist": (info, 0o644),
            f"{app_root}/Contents/MacOS/{executable_name}": (binary, 0o755),
            f"{PRODUCT}/README.md": (readme, 0o644),
            f"{PRODUCT}/LICENSE": (license_text, 0o644),
            f"{PRODUCT}/THIRD-PARTY-NOTICES.md": (notices, 0o644),
        }

    args.output.mkdir(parents=True, exist_ok=True)
    archive_path = args.output / f"{PRODUCT}-{args.platform}.zip"
    with zipfile.ZipFile(archive_path, "w", compression=zipfile.ZIP_DEFLATED,
                         compresslevel=9) as archive:
        for name, (contents, mode) in entries.items():
            zip_entry(archive, name, contents, mode)

    expected = set(entries)
    with zipfile.ZipFile(archive_path) as archive:
        actual = set(archive.namelist())
        if actual != expected or len(actual) != len(entries):
            raise SystemExit(f"archive inventory mismatch: {sorted(actual)}")
        if archive.testzip() is not None:
            raise SystemExit("archive contains a corrupt member")
        if platform == "macos":
            app_executable = next(name for name in expected if name.endswith(
                f"/Contents/MacOS/{PRODUCT}"))
            info_path = next(name for name in expected if name.endswith("/Contents/Info.plist"))
            if not archive.getinfo(app_executable).external_attr >> 16 & 0o111:
                raise SystemExit("macOS app executable is not marked executable")
            if plistlib.loads(archive.read(info_path))["CFBundleExecutable"] != PRODUCT:
                raise SystemExit("macOS app Info.plist does not identify its executable")

    digest = hashlib.sha256(archive_path.read_bytes()).hexdigest()
    checksum = args.output / f"{archive_path.name}.sha256"
    checksum.write_text(f"{digest}  {archive_path.name}\n", encoding="ascii")
    manifest = {
        "platform": args.platform,
        "architecture": architecture,
        "archive": archive_path.name,
        "archiveSha256": digest,
        "binarySha256": hashlib.sha256(binary).hexdigest(),
        "members": sorted(expected),
        "signed": False if platform == "macos" else None,
        "notarized": False if platform == "macos" else None,
    }
    (args.output / f"{archive_path.name}.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"PASS PACKAGE {args.platform} SHA256={digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
