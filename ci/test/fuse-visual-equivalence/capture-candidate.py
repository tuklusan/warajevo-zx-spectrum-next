#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Commercial Use and use for AI/ML model training are
# prohibited unless separately authorized. Attribution is required. See
# LICENSE for full terms.
"""Capture the production app's visible Spectrum viewport on a hosted desktop."""

import argparse
import ctypes
import hashlib
import json
import os
import pathlib
import platform
import re
import subprocess
import sys
import time
from ctypes import wintypes

from PIL import Image, ImageGrab, ImageStat


TITLE = "Warajevo ZX Spectrum Next"
PROCESS = "Warajevo-ZX-Spectrum-Next"
CANONICAL_SIZE = (448, 312)


def mac_window_bounds():
    script = '''
tell application "System Events"
    set p to first process whose name is "Warajevo-ZX-Spectrum-Next"
    set frontmost of p to true
    set w to window 1 of p
    set q to position of w
    set s to size of w
    return (item 1 of q as text) & "," & (item 2 of q as text) & "," & ¬
        (item 1 of s as text) & "," & (item 2 of s as text)
end tell
'''
    result = subprocess.run(["osascript", "-e", script], capture_output=True,
                            text=True, check=False)
    if result.returncode:
        return None
    values = [int(value) for value in re.findall(r"-?\d+", result.stdout)]
    return tuple(values) if len(values) == 4 else None


def windows_client_bounds(process_id):
    user32 = ctypes.windll.user32
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND,
                                       wintypes.LPARAM)
    user32.EnumWindows.argtypes = (callback_type, wintypes.LPARAM)
    user32.EnumWindows.restype = wintypes.BOOL
    user32.GetWindowThreadProcessId.argtypes = (wintypes.HWND,
                                                ctypes.POINTER(wintypes.DWORD))
    user32.GetWindowThreadProcessId.restype = wintypes.DWORD
    user32.IsWindowVisible.argtypes = (wintypes.HWND,)
    user32.IsWindowVisible.restype = wintypes.BOOL
    user32.IsIconic.argtypes = (wintypes.HWND,)
    user32.IsIconic.restype = wintypes.BOOL
    user32.GetWindowTextW.argtypes = (wintypes.HWND, wintypes.LPWSTR,
                                      ctypes.c_int)
    user32.GetWindowTextW.restype = ctypes.c_int
    user32.GetWindowRect.argtypes = (wintypes.HWND,
                                     ctypes.POINTER(wintypes.RECT))
    user32.GetWindowRect.restype = wintypes.BOOL
    user32.GetClientRect.argtypes = (wintypes.HWND,
                                     ctypes.POINTER(wintypes.RECT))
    user32.GetClientRect.restype = wintypes.BOOL
    user32.ClientToScreen.argtypes = (wintypes.HWND,
                                      ctypes.POINTER(wintypes.POINT))
    user32.ClientToScreen.restype = wintypes.BOOL
    user32.SetForegroundWindow.argtypes = (wintypes.HWND,)
    user32.SetForegroundWindow.restype = wintypes.BOOL
    try:
        user32.SetProcessDPIAware()
    except AttributeError:
        pass
    found = []
    def visit(hwnd, _):
        owner = wintypes.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value != process_id or not user32.IsWindowVisible(hwnd) or \
                user32.IsIconic(hwnd):
            return True
        title = ctypes.create_unicode_buffer(256)
        user32.GetWindowTextW(hwnd, title, len(title))
        if title.value != TITLE:
            return True
        rect = ctypes.wintypes.RECT()
        client = ctypes.wintypes.RECT()
        if not user32.GetWindowRect(hwnd, ctypes.byref(rect)) or \
                not user32.GetClientRect(hwnd, ctypes.byref(client)):
            return True
        origin = ctypes.wintypes.POINT(client.left, client.top)
        end = ctypes.wintypes.POINT(client.right, client.bottom)
        user32.ClientToScreen(hwnd, ctypes.byref(origin))
        user32.ClientToScreen(hwnd, ctypes.byref(end))
        user32.SetForegroundWindow(hwnd)
        found.append((origin.x, origin.y, end.x - origin.x,
                      end.y - origin.y))
        return False

    user32.EnumWindows(callback_type(visit), 0)
    return found[0] if found else None


def window_client_bounds(process):
    if os.name == "nt":
        return windows_client_bounds(process.pid)
    if sys.platform == "darwin":
        bounds = mac_window_bounds()
        if bounds is None:
            return None
        left, top, width, height = bounds
        title_bar = 28
        return left, top + title_bar, width, height - title_bar
    raise SystemExit(f"desktop capture is unsupported on {platform.system()}")


def windows_crop_owned_by_process(process_id, box):
    user32 = ctypes.windll.user32
    user32.WindowFromPoint.argtypes = (wintypes.POINT,)
    user32.WindowFromPoint.restype = wintypes.HWND
    user32.GetWindowThreadProcessId.argtypes = (wintypes.HWND,
                                                ctypes.POINTER(wintypes.DWORD))
    user32.GetWindowThreadProcessId.restype = wintypes.DWORD
    center = wintypes.POINT((box[0] + box[2]) // 2,
                            (box[1] + box[3]) // 2)
    window = user32.WindowFromPoint(center)
    owner = wintypes.DWORD()
    if not window:
        return False
    user32.GetWindowThreadProcessId(window, ctypes.byref(owner))
    return owner.value == process_id


def viewport_capture(bounds):
    left, top, width, height = bounds
    viewport_top = 56.0
    viewport_bottom = height - 48.0
    viewport_height = max(1.0, viewport_bottom - viewport_top)
    fit_scale = min(width / CANONICAL_SIZE[0],
                    viewport_height / CANONICAL_SIZE[1])
    display_width = CANONICAL_SIZE[0] * fit_scale
    display_height = CANONICAL_SIZE[1] * fit_scale
    display_left = (width - display_width) * 0.5
    display_top = viewport_top + (viewport_height - display_height) * 0.5
    box = (round(left + display_left), round(top + display_top),
           round(left + display_left + display_width),
           round(top + display_top + display_height))
    return ImageGrab.grab(bbox=box).convert("RGB"), box


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", type=pathlib.Path)
    parser.add_argument("rom", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    binary = args.binary.resolve()
    rom = args.rom.resolve()
    output = args.output.resolve()
    if not binary.is_file() or not rom.is_file():
        raise SystemExit("production application binary or certified ROM is missing")
    output.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment["WZSN_ROM_PATH"] = str(rom)
    log_path = output / "application.log"
    screenshot_path = output / "application.png"
    process = None
    try:
        with log_path.open("wb") as log:
            process = subprocess.Popen([str(binary)], env=environment,
                                       stdout=log, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 45
            bounds = None
            while time.monotonic() < deadline:
                if process.poll() is not None:
                    raise SystemExit("production application exited before opening its window")
                bounds = window_client_bounds(process)
                if bounds is not None:
                    break
                time.sleep(0.5)
            if bounds is None:
                raise SystemExit("production application window was not found")
            time.sleep(8)
            if process.poll() is not None:
                raise SystemExit("production application exited before display capture")
            for attempt in range(20):
                bounds = window_client_bounds(process)
                if bounds is None:
                    raise SystemExit("production application window disappeared")
                image, box = viewport_capture(bounds)
                if os.name != "nt" or windows_crop_owned_by_process(process.pid, box):
                    break
                time.sleep(0.5)
            else:
                raise SystemExit("production application viewport is obscured by another window")
            if image.width < CANONICAL_SIZE[0] or image.height < CANONICAL_SIZE[1]:
                raise SystemExit("captured production viewport is below full-frame dimensions")
            variance = sum(ImageStat.Stat(image).var) / 3.0
            if variance < 25.0:
                raise SystemExit("captured production viewport is blank or uniform")
            image.save(screenshot_path)
    finally:
        if process is not None and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)

    proof = {
        "status": "pass",
        "projectCommit": os.environ["GITHUB_SHA"],
        "runId": os.environ["GITHUB_RUN_ID"],
        "runner": f"{os.environ['RUNNER_OS']}-{os.environ['RUNNER_ARCH']}",
        "capturedAt": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "romSha256": hashlib.sha256(rom.read_bytes()).hexdigest(),
        "applicationSha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
        "screenshotSha256": hashlib.sha256(screenshot_path.read_bytes()).hexdigest(),
        "screenshotSize": [image.width, image.height],
        "desktopCrop": list(box),
    }
    (output / "capture-proof.json").write_text(
        json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(proof, sort_keys=True))


if __name__ == "__main__":
    main()
