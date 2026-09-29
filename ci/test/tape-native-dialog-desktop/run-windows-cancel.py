#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

import ctypes
import subprocess
import sys
import time
from ctypes import wintypes


def find_dialog():
    user32 = ctypes.windll.user32
    found = []
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND,
                                       wintypes.LPARAM)

    def visit(window, _):
        if not user32.IsWindowVisible(window):
            return True
        length = user32.GetWindowTextLengthW(window)
        if length:
            title = ctypes.create_unicode_buffer(length + 1)
            user32.GetWindowTextW(window, title, length + 1)
            if title.value == "Open / Run":
                found.append(window)
                return False
        return True

    user32.EnumWindows(callback_type(visit), 0)
    return found[0] if found else None


def main():
    process = subprocess.Popen([sys.argv[1]], stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, text=True)
    try:
        deadline = time.monotonic() + 20
        window = None
        while time.monotonic() < deadline and process.poll() is None:
            window = find_dialog()
            if window:
                break
            time.sleep(0.1)
        if not window:
            raise RuntimeError("native open dialog did not appear")
        user32 = ctypes.windll.user32
        if not user32.SetForegroundWindow(window):
            raise RuntimeError("native open dialog could not receive focus")
        time.sleep(0.1)
        user32.keybd_event(0x1B, 0, 0, 0)
        user32.keybd_event(0x1B, 0, 0x0002, 0)
        stdout, stderr = process.communicate(timeout=15)
        if process.returncode != 0:
            raise RuntimeError((stdout + stderr).strip())
        if "PASS: desktop chooser cancellation" not in stdout:
            raise RuntimeError("native chooser contract did not report success")
        print(stdout.strip())
    finally:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)


if __name__ == "__main__":
    main()
