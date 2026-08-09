#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

NOTICE_LINES = (
    "Warajevo ZX Spectrum Next",
    "Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.",
    "New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.",
    "Upstream Warajevo and third-party material retain their applicable copyrights and licenses.",
    "See LICENSE.txt and NOTICE.md for complete terms and provenance.",
)

HEADER_FILE_SUFFIXES = {
    ".c",
    ".cmake",
    ".h",
    ".m",
    ".md",
    ".ps1",
    ".py",
    ".sh",
    ".txt",
    ".yaml",
    ".yml",
}

TEXT_FILE_SUFFIXES = HEADER_FILE_SUFFIXES | {
    ".gitignore",
    ".gitattributes",
    ".json",
}

HEADER_FILE_NAMES = {
    "CMakeLists.txt",
    ".gitattributes",
    ".gitignore",
}

TEXT_FILE_NAMES = HEADER_FILE_NAMES

SKIP_DIRS = {
    ".git",
    ".idea",
    ".vs",
    "__pycache__",
    "build",
    "cmake-build-debug",
    "cmake-build-release",
    "dist",
    "out",
}

STATUS_VALUES = {"open", "in_progress", "closed", "blocked"}

BANNED_TERMS = tuple(
    "".join(chr(code) for code in codes)
    for codes in (
        (67, 108, 97, 117, 100, 101),
        (79, 112, 101, 110, 65, 73),
        (67, 111, 100, 101, 120),
    )
)


def should_skip(path: Path) -> bool:
    return any(part in SKIP_DIRS for part in path.parts)


def is_header_file(path: Path) -> bool:
    return path.name in HEADER_FILE_NAMES or path.suffix.lower() in HEADER_FILE_SUFFIXES


def is_text_file(path: Path) -> bool:
    return path.name in TEXT_FILE_NAMES or path.suffix.lower() in TEXT_FILE_SUFFIXES


def normalize_comment_line(line: str) -> str:
    text = line.strip()

    if not text:
        return ""

    if text.startswith("#!"):
        return ""

    for marker in ("<!--", "-->", "/*", "*/"):
        text = text.replace(marker, " ")

    while True:
        original = text

        if text.startswith("//"):
            text = text[2:].lstrip()
        elif text[:1] in {"#", "*", ";"}:
            text = text[1:].lstrip()

        if text == original:
            break

    return text.strip()


def has_notice_header(path: Path) -> bool:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except UnicodeDecodeError:
        return False

    normalized = []
    for line in lines[:16]:
        candidate = normalize_comment_line(line)
        if candidate:
            normalized.append(candidate)

    return tuple(normalized[: len(NOTICE_LINES)]) == NOTICE_LINES


def find_banned_terms(path: Path) -> list[str]:
    try:
        content = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        return []

    lowered = content.casefold()
    return [term for term in BANNED_TERMS if term.casefold() in lowered]


def validate_change_requests(path: Path) -> list[str]:
    problems: list[str] = []

    data = json.loads(path.read_text(encoding="utf-8"))

    notice = data.get("project_notice")
    if notice != list(NOTICE_LINES):
        problems.append(f"{path}: project_notice does not match the canonical notice text")

    change_requests = data.get("change_requests")
    if not isinstance(change_requests, list):
        problems.append(f"{path}: change_requests must be a JSON array")
        return problems

    for entry in change_requests:
        if not isinstance(entry, dict):
            problems.append(f"{path}: every change request entry must be an object")
            continue

        cr_number = entry.get("cr_number", "")
        status = entry.get("status", "")

        if not re.fullmatch(r"CR-\d{4}", cr_number):
            problems.append(f"{path}: invalid CR number {cr_number!r}")

        if status not in STATUS_VALUES:
            problems.append(f"{path}: invalid status {status!r} for {cr_number!r}")

    return problems


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    problems: list[str] = []

    for path in sorted(root.rglob("*")):
        if not path.is_file() or should_skip(path):
            continue

        relative_path = path.relative_to(root)

        if is_header_file(relative_path) and not has_notice_header(path):
            problems.append(f"{relative_path}: missing canonical notice header")

        if is_text_file(relative_path):
            for term in find_banned_terms(path):
                problems.append(f"{relative_path}: contains banned term {term!r}")

    issues_path = root / "issues" / "change-requests.json"
    if issues_path.exists():
        problems.extend(validate_change_requests(issues_path))
    else:
        problems.append("issues/change-requests.json: missing change-request tracker")

    if problems:
        for problem in problems:
            print(problem)
        return 1

    print("repository gates passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
