#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

from __future__ import annotations

import json
import re
import subprocess
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

PRIVATE_MEDIA_DIR_NAMES = {
    "WZSN-PRIVATE-TEST-MEDIA",
    "WZSN_PRIVATE_TEST_MEDIA",
}

PRIVATE_MEDIA_PUBLIC_FILES = {
    ".gitignore",
    "README.md",
}

TEST_ARTIFACTS_DIR_NAME = "test-artefacts"

TEST_ARTIFACTS_PUBLIC_FILES = {
    "README.md",
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
    for index, part in enumerate(path.parts):
        if part in PRIVATE_MEDIA_DIR_NAMES:
            tail = path.parts[index + 1 :]
            return len(tail) != 1 or tail[0] not in PRIVATE_MEDIA_PUBLIC_FILES

        if part == TEST_ARTIFACTS_DIR_NAME:
            tail = path.parts[index + 1 :]
            return len(tail) != 1 or tail[0] not in TEST_ARTIFACTS_PUBLIC_FILES

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


def git_tracked_paths(root: Path, relative_path: str) -> list[str]:
    result = subprocess.run(
        ["git", "ls-files", "--", relative_path],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )

    if result.returncode != 0:
        return []

    return [line.strip() for line in result.stdout.splitlines() if line.strip()]


def validate_private_directories(root: Path) -> list[str]:
    problems: list[str] = []

    tracked_test_artefacts = git_tracked_paths(root, TEST_ARTIFACTS_DIR_NAME)
    allowed_test_artefacts = {f"{TEST_ARTIFACTS_DIR_NAME}/README.md"}
    for tracked in tracked_test_artefacts:
        if tracked not in allowed_test_artefacts:
            problems.append(
                f"{tracked}: test-artefacts may only publish README.md; all other contents must remain private"
            )

    for directory_name in PRIVATE_MEDIA_DIR_NAMES:
        tracked_private_media = git_tracked_paths(root, directory_name)
        allowed_private_media = {
            f"{directory_name}/README.md",
            f"{directory_name}/.gitignore",
        }
        for tracked in tracked_private_media:
            if tracked not in allowed_private_media:
                problems.append(
                    f"{tracked}: private media directory may only publish README.md and .gitignore"
                )

    return problems


def validate_forbidden_local_build_outputs(root: Path) -> list[str]:
    problems: list[str] = []

    forbidden_dirs = [
        root / "build",
        root / "Testing",
    ]
    forbidden_dirs.extend(root.glob("cmake-build-*"))

    for path in forbidden_dirs:
        if path.exists():
            problems.append(
                f"{path.relative_to(root)}: local build/test output directory exists, but local build/test execution is forbidden"
            )

    return problems


def validate_remote_only_ci_policy(root: Path) -> list[str]:
    problems: list[str] = []

    workflow_path = root / ".github" / "workflows" / "repository-gates.yml"
    if not workflow_path.exists():
        return problems

    content = workflow_path.read_text(encoding="utf-8")
    forbidden_patterns = (
        r"\bcmake\b",
        r"\bctest\b",
        r"\bmeson\b",
        r"\bninja\b",
        r"(^|\s)make(\s|$)",
        r"\bmsbuild\b",
        r"\bdevenv\b",
        r"\bpytest\b",
        r"\bcargo\s+build\b",
        r"\bcargo\s+test\b",
        r"\bdotnet\s+build\b",
        r"\bdotnet\s+test\b",
        r"\bgo\s+test\b",
        r"\bnpm\s+test\b",
        r"\bpnpm\s+test\b",
        r"\byarn\s+test\b",
    )

    for pattern in forbidden_patterns:
        if re.search(pattern, content, flags=re.MULTILINE):
            problems.append(
                f".github/workflows/repository-gates.yml: contains forbidden non-remote build/test command pattern {pattern!r}"
            )

    return problems


def validate_platform_smoke_workflow(root: Path) -> list[str]:
    problems: list[str] = []

    workflow_path = root / ".github" / "workflows" / "platform-smoke.yml"
    if not workflow_path.exists():
        problems.append(".github/workflows/platform-smoke.yml: hosted platform smoke workflow is missing")
        return problems

    content = workflow_path.read_text(encoding="utf-8")
    required_snippets = (
        "ubuntu-24.04",
        "ubuntu-24.04-arm",
        "windows-2025",
        "macos-15",
        "macos-15-intel",
        "python tools/harness/run_cmake_smoke.py",
        "actions/upload-artifact",
    )

    for snippet in required_snippets:
        if snippet not in content:
            problems.append(
                f".github/workflows/platform-smoke.yml: missing required hosted smoke snippet {snippet!r}"
            )

    return problems


def validate_required_workflow_documents(root: Path) -> list[str]:
    problems: list[str] = []

    required_files = {
        "WORKFLOW.md": (
            "Never create any project item or artifact above the project directory.",
            "Never build or test on this local machine.",
            "GitHub-hosted runners used by this repository's tracked workflows are approved",
            "test-artefacts/",
            "Whenever a PowerShell command is about to be executed on the local machine or",
            "tools/Test-PowerShellSyntax.ps1",
        ),
        "test-artefacts/README.md": (
            "linux-x64-lxqt",
            "sanyalnet@192.168.4.76",
            "~/SOFTWARE-DEVELOPMENT/Warajevo-Spectrum-Next",
            "windows-10-reference",
            "sanyalnet@192.168.4.75",
            r"D:\WarajevoSpectrum.Next",
            "windows-11-laptop",
            "vagab@192.168.4.35",
            r"C:\Users\vagab\WarajevoSpectrum.Next",
            "Never build or test on this local machine.",
            "test-artefacts/remote-runs/<machine>/<run-id>/",
            ".github/workflows/platform-smoke.yml",
            "Before executing any PowerShell command locally or on either remote Windows",
            "powershell -NoProfile -File tools/Test-PowerShellSyntax.ps1 -CommandText '<command>'",
        ),
        "README-GIT-GITHUB.md": (
            "tools/harness/invoke_remote_harness.py",
            ".github/workflows/platform-smoke.yml",
            "ubuntu-24.04",
            "ubuntu-24.04-arm",
            "windows-2025",
            "macos-15",
            "macos-15-intel",
        ),
    }

    tracked_files = set(git_tracked_paths(root, "."))
    required_tracked_files = {
        ".github/workflows/platform-smoke.yml",
        "WORKFLOW.md",
        "tools/Test-PowerShellSyntax.ps1",
        "tools/harness/README.md",
        "tools/harness/run_cmake_smoke.py",
        "tools/harness/stream_zip_tree.py",
        "tools/harness/invoke_remote_harness.py",
        "tools/harness/capture-linux-active-display.sh",
        "tools/harness/Capture-WindowsDesktopScreenshot.ps1",
        "test-artefacts/README.md",
        "WZSN-PRIVATE-TEST-MEDIA/README.md",
        "WZSN-PRIVATE-TEST-MEDIA/.gitignore",
    }

    for relative_name, required_snippets in required_files.items():
        path = root / relative_name
        if not path.exists():
            problems.append(f"{relative_name}: required workflow document is missing")
            continue

        content = path.read_text(encoding="utf-8")
        for snippet in required_snippets:
            if snippet not in content:
                problems.append(
                    f"{relative_name}: missing required workflow snippet {snippet!r}"
                )

    for tracked_name in sorted(required_tracked_files):
        if tracked_name not in tracked_files:
            problems.append(
                f"{tracked_name}: required public workflow file exists but is not tracked by git"
            )

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

    problems.extend(validate_private_directories(root))
    problems.extend(validate_forbidden_local_build_outputs(root))
    problems.extend(validate_remote_only_ci_policy(root))
    problems.extend(validate_platform_smoke_workflow(root))
    problems.extend(validate_required_workflow_documents(root))

    if problems:
        for problem in problems:
            print(problem)
        return 1

    print("repository gates passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
