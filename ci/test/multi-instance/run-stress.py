# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Exercise shared host services under concurrent process load."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import tempfile
import time


WORKERS = 12
HOLD_MILLISECONDS = 500
CONFIG_PAYLOAD_SIZE = 4 * 1024 * 1024
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def run_group(worker: Path, mode: str, directory: Path, count: int,
              hold_milliseconds: int, environment: dict[str, str]) -> list[str]:
    directory.mkdir(parents=True, exist_ok=True)
    processes = [
        subprocess.Popen(
            [str(worker), mode, str(directory), str(index),
             str(hold_milliseconds)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            env=environment,
        )
        for index in range(count)
    ]
    deadline = time.monotonic() + 45.0
    while not all((directory / f"ready-{index}").is_file()
                  for index in range(count)):
        exited = [process for process in processes if process.poll() is not None]
        if exited:
            break
        if time.monotonic() >= deadline:
            break
        time.sleep(0.02)
    ready = all((directory / f"ready-{index}").is_file()
                for index in range(count))
    (directory / "go").write_text("go", encoding="ascii")
    outputs: list[str] = []
    failures: list[str] = []
    for process in processes:
        try:
            stdout, stderr = process.communicate(timeout=45.0)
        except subprocess.TimeoutExpired:
            process.kill()
            stdout, stderr = process.communicate()
            failures.append(f"worker timed out: {stdout} {stderr}")
            continue
        outputs.append(stdout.strip())
        if process.returncode != 0:
            failures.append(
                f"worker exited {process.returncode}: {stdout} {stderr}"
            )
    if not ready:
        failures.append("not all workers reached the start barrier")
    if failures:
        raise RuntimeError("; ".join(failures))
    return outputs


def validate_atomic_settings(outputs: list[str], directory: Path) -> None:
    if sum(output == "CONFIG OK" for output in outputs) < 1:
        raise RuntimeError("no concurrent settings writer completed")
    target = directory / "settings.ini"
    if target.stat().st_size != CONFIG_PAYLOAD_SIZE:
        raise RuntimeError("settings file was not atomically published")
    with target.open("rb") as stream:
        expected = stream.read(1)
        if len(expected) != 1:
            raise RuntimeError("settings file is empty")
        remaining = CONFIG_PAYLOAD_SIZE - 1
        while remaining:
            chunk = stream.read(min(65536, remaining))
            if not chunk or any(value != expected[0] for value in chunk):
                raise RuntimeError("settings file contains a torn mixed write")
            remaining -= len(chunk)
    if target.with_name(target.name + ".lock").exists():
        raise RuntimeError("settings writer left its lock behind")


def run(worker: Path) -> None:
    worker = worker.resolve()
    if not worker.is_file() and Path(f"{worker}.exe").is_file():
        worker = Path(f"{worker}.exe")
    if not worker.is_file():
        raise RuntimeError(f"worker executable not found: {worker}")

    with tempfile.TemporaryDirectory(prefix="wzsn-multi-instance-") as raw:
        root = Path(raw)
        base_environment = os.environ.copy()

        control = run_group(
            worker, "control", root / "control", WORKERS,
            HOLD_MILLISECONDS, base_environment)
        ports = [int(output.removeprefix("CONTROL ")) for output in control]
        if (len(ports) != WORKERS or len(set(ports)) != WORKERS or
                any(port < 30740 or port > 32787 for port in ports)):
            raise RuntimeError("concurrent instances did not receive unique ports")
        print(f"PASS concurrent control ports: {len(ports)}")

        settings_dir = root / "settings"
        settings = run_group(
            worker, "settings", settings_dir, WORKERS, 0, base_environment)
        validate_atomic_settings(settings, settings_dir)
        print("PASS concurrent settings writes are atomic")

        blocked_dir = root / "settings-blocked"
        blocked_dir.mkdir()
        lock_path = blocked_dir / "settings.ini.lock"
        lock_path.write_bytes(b"held")
        blocked_environment = base_environment.copy()
        blocked_environment["WZSN_SETTINGS_DIR"] = str(blocked_dir)
        blocked = run_group(
            worker, "settings", root / "settings-blocked-attempt", 1, 0,
            blocked_environment)
        if blocked != ["CONFIG BUSY"]:
            raise RuntimeError("settings lock contention was not reported")
        if not lock_path.exists():
            raise RuntimeError("failed writer removed another process's lock")
        lock_path.unlink()
        recovered = run_group(
            worker, "settings", root / "settings-recovery", 1, 0,
            blocked_environment)
        if recovered != ["CONFIG OK"]:
            raise RuntimeError("settings write did not recover after contention")
        validate_atomic_settings(recovered, blocked_dir)
        print("PASS settings contention recovery")

        media_dir = root / "media"
        media_dir.mkdir()
        (media_dir / "media.img").write_bytes(b"writable media fixture")
        media = run_group(
            worker, "media", media_dir, WORKERS, HOLD_MILLISECONDS,
            base_environment)
        winners = sum(output == "MEDIA OK" for output in media)
        if winners != 1 or sum(output == "MEDIA BUSY" for output in media) != WORKERS - 1:
            raise RuntimeError("writable media was not exclusively owned")
        recovery_dir = root / "media-recovery"
        recovery_dir.mkdir()
        (recovery_dir / "media.img").write_bytes(b"writable media recovery fixture")
        recovery = run_group(
            worker, "media", recovery_dir, 1, 0,
            base_environment)
        if recovery != ["MEDIA OK"]:
            raise RuntimeError("writable media claim did not recover after release")
        print("PASS exclusive writable media contention")

        screenshot_dir = root / "screenshots"
        screenshot_dir.mkdir()
        environment = base_environment.copy()
        environment["WZSN_SCREENSHOT_DIR"] = str(screenshot_dir)
        screenshots = run_group(
            worker, "screenshot", root / "screenshots-barrier", WORKERS, 0,
            environment)
        paths = [Path(output.removeprefix("SCREENSHOT ")) for output in screenshots]
        if len(paths) != WORKERS or len({path.name for path in paths}) != WORKERS:
            raise RuntimeError("concurrent screenshots did not get unique names")
        if any(path.parent != screenshot_dir or
               not path.read_bytes().startswith(PNG_SIGNATURE) for path in paths):
            raise RuntimeError("a screenshot output is missing or invalid")
        print(f"PASS concurrent screenshot outputs: {len(paths)}")

    print("PASS multi-instance stress cases: 5")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("worker", type=Path)
    arguments = parser.parse_args()
    try:
        run(arguments.worker)
    except (OSError, RuntimeError, ValueError) as error:
        print(f"FAIL multi-instance stress: {error}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
