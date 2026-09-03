#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

from __future__ import annotations

import argparse
import json
import os
import platform
import queue
import shlex
import shutil
import subprocess
import sys
import threading
import time
from datetime import datetime, timezone
from pathlib import Path


def utc_now() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def resolve_within_project(project_root: Path, candidate: str) -> Path:
    path = Path(candidate)
    if not path.is_absolute():
        path = project_root / path

    resolved = path.resolve()

    try:
        resolved.relative_to(project_root)
    except ValueError as exc:
        raise SystemExit(f"path must remain within the project directory: {candidate}") from exc

    return resolved


def safe_write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def tool_record(command: str | None) -> dict[str, str | None]:
    if not command:
        return {"path": None, "version": None}

    version_commands = {
        "cmake": [command, "--version"],
        "ctest": [command, "--version"],
        "git": [command, "--version"],
        "python": [command, "--version"],
        "python3": [command, "--version"],
        "py": [command, "-3", "--version"],
        "ninja": [command, "--version"],
        "make": [command, "--version"],
        "cc": [command, "--version"],
        "gcc": [command, "--version"],
        "clang": [command, "--version"],
        "clang-cl": [command, "--version"],
        "cl": [command],
        "msbuild": [command, "-version"],
        "xcodebuild": [command, "-version"],
    }

    version_command = version_commands.get(command, [command, "--version"])
    path = shutil.which(command)
    version = None

    if path:
        result = subprocess.run(
            version_command,
            check=False,
            capture_output=True,
            text=True,
        )
        output = result.stdout.strip() or result.stderr.strip()
        if output:
            version = output.splitlines()[0]

    return {"path": path, "version": version}


def choose_python_command() -> str | None:
    for candidate in ("python3", "python"):
        if not shutil.which(candidate):
            continue
        try:
            result = subprocess.run(
                [candidate, "-c", "import sys; print(sys.executable if sys.version_info.major == 3 else '')"],
                check=False, capture_output=True, text=True, timeout=30,
            )
        except (OSError, subprocess.SubprocessError):
            continue
        if result.returncode == 0 and result.stdout.strip():
            return result.stdout.strip()
    launcher = shutil.which("py")
    if launcher:
        try:
            result = subprocess.run(
                [launcher, "-3", "-c", "import sys; print(sys.executable)"],
                check=False, capture_output=True, text=True, timeout=30,
            )
        except (OSError, subprocess.SubprocessError):
            return None
        if result.returncode == 0 and result.stdout.strip():
            return result.stdout.strip()
    return None


def choose_compiler(system_name: str, requested: str | None = None) -> str | None:
    if requested:
        return requested if shutil.which(requested) else None
    if system_name == "Windows":
        candidates = ["cl", "clang-cl", "clang", "gcc"]
    else:
        candidates = ["cc", "gcc", "clang"]

    for candidate in candidates:
        if shutil.which(candidate):
            return candidate

    if system_name == "Windows":
        llvm_bin = Path(os.environ.get("ProgramFiles", r"C:\Program Files")) / "LLVM" / "bin"
        for candidate in ("clang.exe", "clang-cl.exe"):
            compiler_path = llvm_bin / candidate
            if compiler_path.exists():
                return str(compiler_path)

    return None


def choose_build_helper(system_name: str) -> str | None:
    candidates = ["ninja", "msbuild"] if system_name == "Windows" else ["ninja", "make"]
    for candidate in candidates:
        if shutil.which(candidate):
            return candidate
    return None


def choose_windows_sdk_tool(tool_name: str) -> str | None:
    tool_path = shutil.which(tool_name)
    if tool_path:
        return tool_path

    kits_root = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / "Windows Kits" / "10" / "bin"
    if not kits_root.exists():
        return None

    version_dirs = sorted(
        (path for path in kits_root.iterdir() if path.is_dir()),
        reverse=True,
    )
    for version_dir in version_dirs:
        candidate = version_dir / "x64" / tool_name
        if candidate.exists():
            return str(candidate)

    return None


def cmake_path_string(path: str) -> str:
    return path.replace("\\", "/")


def python_cmake_definition(path: str) -> str:
    return f"-DPython3_EXECUTABLE={cmake_path_string(path)}"


def command_text(command: list[str]) -> str:
    return shlex.join(command)


def run_logged(command: list[str], log_path: Path, environment: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("w", encoding="utf-8") as log:
        header = f"$ {command_text(command)}"
        print(header, flush=True)
        log.write(header + "\n\n")
        log.flush()
        process = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            env=environment,
        )
        lines: queue.Queue[str | None] = queue.Queue()

        def read_output() -> None:
            assert process.stdout is not None
            for line in process.stdout:
                lines.put(line)
            lines.put(None)

        reader = threading.Thread(target=read_output, daemon=True)
        reader.start()
        reader_done = False
        while not reader_done:
            try:
                line = lines.get(timeout=15)
            except queue.Empty:
                print(f"[wzsn-smoke] still running: {command[0]}", flush=True)
                continue
            if line is None:
                reader_done = True
                continue
            print(line, end="", flush=True)
            log.write(line)
            log.flush()
        returncode = process.wait()
        reader.join(timeout=5)
    return subprocess.CompletedProcess(command, returncode)


def windows_developer_environment() -> dict[str, str]:
    environment = os.environ.copy()
    vswhere = Path(environment.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / \
        "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    devcmd = None
    installation_root = None
    if vswhere.is_file():
        install = subprocess.run(
            [str(vswhere), "-products", "*", "-latest", "-property", "installationPath"],
            check=False, capture_output=True, text=True,
        )
        candidate = Path(install.stdout.strip()) / "Common7" / "Tools" / "VsDevCmd.bat"
        if install.returncode == 0 and candidate.is_file():
            devcmd = candidate
            installation_root = candidate.parents[2]
    if devcmd is None:
        roots = [
            Path(environment.get("ProgramFiles", r"C:\Program Files")),
            Path(r"D:\Program Files"),
        ]
        for program_files in roots:
            root = program_files / "Microsoft Visual Studio"
            for version in ("18", "17"):
                for edition in ("Community", "Professional", "Enterprise", "BuildTools"):
                    candidate = root / version / edition / "Common7" / "Tools" / "VsDevCmd.bat"
                    if candidate.is_file():
                        devcmd = candidate
                        installation_root = candidate.parents[2]
                        break
                if devcmd is not None:
                    break
            if devcmd is not None:
                break
    if devcmd is not None:
        loaded = subprocess.run(
            ["cmd", "/d", "/s", "/c", f'call "{devcmd}" -arch=x64 >nul && set'],
            check=False, capture_output=True, text=True, env=environment,
        )
        if loaded.returncode == 0:
            for line in loaded.stdout.splitlines():
                key, separator, value = line.partition("=")
                if separator and key:
                    environment[key] = value
    if installation_root is None:
        roots = [
            Path(environment.get("ProgramFiles", r"C:\Program Files")),
            Path(r"D:\Program Files"),
        ]
        for program_files in roots:
            root = program_files / "Microsoft Visual Studio"
            for version in ("18", "17"):
                for edition in ("Community", "Professional", "Enterprise", "BuildTools"):
                    candidate = root / version / edition
                    if (candidate / "VC" / "Tools" / "MSVC").is_dir():
                        installation_root = candidate
                        break
                if installation_root is not None:
                    break
            if installation_root is not None:
                break
    if installation_root is None:
        return environment
    msvc_root = installation_root / "VC" / "Tools" / "MSVC"
    if msvc_root.is_dir():
        versions = sorted((path for path in msvc_root.iterdir() if path.is_dir()), reverse=True)
        if versions:
            library = versions[0] / "lib" / "x64"
            if library.is_dir():
                environment["LIB"] = str(library) + ";" + environment.get("LIB", "")
                environment["WZSN_MSVC_LIBRARY_PATH"] = str(library)
    return environment


def windows_linker_paths(environment: dict[str, str]) -> list[Path]:
    paths = []
    msvc_library = environment.get("WZSN_MSVC_LIBRARY_PATH")
    if msvc_library:
        paths.append(Path(msvc_library))
    if not msvc_library:
        visual_studio_root = Path(r"D:\Program Files\Microsoft Visual Studio")
        for version in ("18", "17"):
            for edition in ("Community", "Professional", "Enterprise", "BuildTools"):
                msvc_root = visual_studio_root / version / edition / "VC" / "Tools" / "MSVC"
                if not msvc_root.is_dir():
                    continue
                versions = sorted((path for path in msvc_root.iterdir() if path.is_dir()), reverse=True)
                if versions:
                    library = versions[0] / "lib" / "x64"
                    if library.is_dir():
                        paths.append(library)
                        return paths
    kit_root = Path(environment.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / "Windows Kits" / "10" / "Lib"
    if kit_root.is_dir():
        for version in sorted((path for path in kit_root.iterdir() if path.is_dir()), reverse=True):
            candidates = (version / "um" / "x64", version / "ucrt" / "x64")
            if all(candidate.is_dir() for candidate in candidates):
                paths.extend(candidates)
                break
    return paths


def load_generator_name(build_dir: Path) -> str:
    cache_path = build_dir / "CMakeCache.txt"
    if not cache_path.exists():
        return ""

    for line in cache_path.read_text(encoding="utf-8").splitlines():
        if line.startswith("CMAKE_GENERATOR:INTERNAL="):
            return line.split("=", 1)[1].strip()

    return ""


def is_multi_config(generator_name: str) -> bool:
    lowered = generator_name.lower()
    return any(
        token in lowered
        for token in (
            "visual studio",
            "xcode",
            "multi-config",
        )
    )


def write_json(path: Path, payload: object) -> None:
    safe_write_text(path, json.dumps(payload, indent=2, sort_keys=True) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the shared CMake smoke harness.")
    parser.add_argument("--artifact-dir", required=True, help="Artifact directory within the project tree.")
    parser.add_argument(
        "--build-config",
        default="Debug",
        help="Multi-config build configuration name. Defaults to Debug.",
    )
    parser.add_argument("--compiler", help="Require and use this C compiler executable.")
    parser.add_argument("--sanitizers", action="store_true", help="Enable address and undefined-behavior sanitizers.")
    parser.add_argument("--sokol-host", action="store_true",
                        help="Also configure and build the opt-in Sokol host target.")
    parser.add_argument(
        "--probe-only",
        action="store_true",
        help="Record tool inventory and summary without configuring or building.",
    )
    args = parser.parse_args()

    project_root = Path.cwd().resolve()
    artifact_dir = resolve_within_project(project_root, args.artifact_dir)
    build_dir = artifact_dir / "build"
    temp_dir = artifact_dir / "tmp"
    artifact_dir.mkdir(parents=True, exist_ok=True)
    temp_dir.mkdir(parents=True, exist_ok=True)
    os.environ["TMPDIR"] = str(temp_dir)
    os.environ["TEMP"] = str(temp_dir)
    os.environ["TMP"] = str(temp_dir)

    system_name = platform.system()
    build_environment = windows_developer_environment() if system_name == "Windows" else os.environ.copy()
    python_command = choose_python_command()
    compiler_command = choose_compiler(system_name, args.compiler)
    build_helper = choose_build_helper(system_name)

    tools = {
        "cmake": tool_record("cmake"),
        "ctest": tool_record("ctest"),
        "git": tool_record("git"),
        "python": tool_record(python_command),
        "compiler": tool_record(compiler_command),
        "build_helper": tool_record(build_helper),
        "ninja": tool_record("ninja"),
        "make": tool_record("make"),
        "msbuild": tool_record("msbuild"),
        "mt": tool_record(choose_windows_sdk_tool("mt.exe")) if system_name == "Windows" else tool_record(None),
        "rc": tool_record(choose_windows_sdk_tool("rc.exe")) if system_name == "Windows" else tool_record(None),
        "xcodebuild": tool_record("xcodebuild"),
    }

    missing_tools = [
        name
        for name in ("cmake", "ctest", "git", "python", "compiler")
        if not tools[name]["path"]
    ]

    if not build_helper:
        missing_tools.append("build_helper")

    inventory = {
        "generated_at_utc": utc_now(),
        "platform": {
            "system": system_name,
            "release": platform.release(),
            "version": platform.version(),
            "machine": platform.machine(),
        },
        "project_root": str(project_root),
        "artifact_dir": str(artifact_dir),
        "temp_dir": str(temp_dir),
        "tools": tools,
        "missing_tools": missing_tools,
        "environment": {
            "CI": os.environ.get("CI", ""),
        },
    }
    write_json(artifact_dir / "inventory.json", inventory)

    summary = {
        "generated_at_utc": utc_now(),
        "status": "probe_only" if args.probe_only else "pending",
        "artifact_dir": str(artifact_dir),
        "build_dir": str(build_dir),
        "temp_dir": str(temp_dir),
        "missing_tools": missing_tools,
        "commands": [],
    }

    if args.probe_only:
        write_json(artifact_dir / "summary.json", summary)
        return 0

    if missing_tools:
        summary["status"] = "missing_tools"
        write_json(artifact_dir / "summary.json", summary)
        safe_write_text(
            artifact_dir / "missing-tools.txt",
            "\n".join(missing_tools) + "\n",
        )
        return 2

    compiler_path = tools["compiler"]["path"]
    configure_command = [
        "cmake",
        "-S",
        str(project_root),
        "-B",
        str(build_dir),
        "-DWZSN_ENABLE_WARNINGS=ON",
        f"-DWZSN_ENABLE_SANITIZERS={'ON' if args.sanitizers else 'OFF'}",
        f"-DWZSN_BUILD_SOKOL_HOST={'ON' if args.sokol_host else 'OFF'}",
    ]

    if tools["python"]["path"]:
        configure_command.append("-DWZSN_ENABLE_REVIEW_GATE_TESTS=ON")
        configure_command.append(python_cmake_definition(tools["python"]["path"]))

    if compiler_path:
        configure_command.append(f"-DCMAKE_C_COMPILER={cmake_path_string(compiler_path)}")

    if system_name == "Windows" and compiler_path:
        compiler_name = Path(compiler_path).name.lower()
        if compiler_name in ("clang.exe", "clang-cl.exe"):
            if tools["rc"]["path"]:
                configure_command.append(f"-DCMAKE_RC_COMPILER={cmake_path_string(tools['rc']['path'])}")
            if tools["mt"]["path"]:
                configure_command.append(f"-DCMAKE_MT={cmake_path_string(tools['mt']['path'])}")
            linker_paths = windows_linker_paths(build_environment)
            if linker_paths:
                linker_flags = " ".join(
                    f'-Xlinker "/libpath:{cmake_path_string(str(path))}"' for path in linker_paths
                )
                configure_command.append(f"-DCMAKE_EXE_LINKER_FLAGS={linker_flags}")

    if tools["ninja"]["path"]:
        configure_command.extend(["-G", "Ninja"])

    summary["commands"].append(configure_command)
    configure_result = run_logged(configure_command, artifact_dir / "configure.log", build_environment)
    if configure_result.returncode != 0:
        summary["status"] = "configure_failed"
        summary["configure_returncode"] = configure_result.returncode
        write_json(artifact_dir / "summary.json", summary)
        return configure_result.returncode

    generator_name = load_generator_name(build_dir)
    summary["generator"] = generator_name
    multi_config = is_multi_config(generator_name)

    build_command = ["cmake", "--build", str(build_dir)]
    if multi_config:
        build_command.extend(["--config", args.build_config])

    summary["commands"].append(build_command)
    build_result = run_logged(build_command, artifact_dir / "build.log", build_environment)
    if build_result.returncode != 0:
        summary["status"] = "build_failed"
        summary["build_returncode"] = build_result.returncode
        write_json(artifact_dir / "summary.json", summary)
        return build_result.returncode

    ctest_command = ["ctest", "--test-dir", str(build_dir), "--output-on-failure"]
    if multi_config:
        ctest_command.extend(["-C", args.build_config])

    summary["commands"].append(ctest_command)
    ctest_result = run_logged(ctest_command, artifact_dir / "ctest.log", build_environment)
    if ctest_result.returncode != 0:
        summary["status"] = "tests_failed"
        summary["ctest_returncode"] = ctest_result.returncode
        write_json(artifact_dir / "summary.json", summary)
        return ctest_result.returncode

    summary["status"] = "passed"
    write_json(artifact_dir / "summary.json", summary)
    return 0


if __name__ == "__main__":
    sys.exit(main())
