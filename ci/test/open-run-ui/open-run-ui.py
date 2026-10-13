#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

import ctypes
import json
import os
import pathlib
import platform
import subprocess
import sys
import time
from ctypes import wintypes


TITLE = "Warajevo ZX Spectrum Next"


def windows_key(vk, down):
    user32 = ctypes.windll.user32
    class KEYBDINPUT(ctypes.Structure):
        _fields_ = [("wVk", wintypes.WORD), ("wScan", wintypes.WORD),
                    ("dwFlags", wintypes.DWORD),
                    ("time", wintypes.DWORD),
                    ("dwExtraInfo", ctypes.c_size_t)]

    class INPUTUNION(ctypes.Union):
        _fields_ = [("ki", KEYBDINPUT)]

    class INPUT(ctypes.Structure):
        _anonymous_ = ("u",)
        _fields_ = [("type", wintypes.DWORD), ("u", INPUTUNION)]

    event = INPUT(type=1, ki=KEYBDINPUT(vk, 0, 0 if down else 2, 0, 0))
    if user32.SendInput(1, ctypes.byref(event), ctypes.sizeof(INPUT)) != 1:
        raise ctypes.WinError()


def windows_text(value):
    user32 = ctypes.windll.user32
    for character in value:
        class KEYBDINPUT(ctypes.Structure):
            _fields_ = [("wVk", wintypes.WORD), ("wScan", wintypes.WORD),
                        ("dwFlags", wintypes.DWORD),
                        ("time", wintypes.DWORD),
                        ("dwExtraInfo", ctypes.c_size_t)]

        class INPUTUNION(ctypes.Union):
            _fields_ = [("ki", KEYBDINPUT)]

        class INPUT(ctypes.Structure):
            _anonymous_ = ("u",)
            _fields_ = [("type", wintypes.DWORD), ("u", INPUTUNION)]

        for flags in (4, 4 | 2):
            event = INPUT(type=1, ki=KEYBDINPUT(0, ord(character), flags,
                                                0, 0))
            if user32.SendInput(1, ctypes.byref(event),
                                ctypes.sizeof(INPUT)) != 1:
                raise ctypes.WinError()


def windows_press(vk):
    windows_key(vk, True)
    windows_key(vk, False)


def windows_mouse(flags):
    class MOUSEINPUT(ctypes.Structure):
        _fields_ = [("dx", wintypes.LONG), ("dy", wintypes.LONG),
                    ("mouseData", wintypes.DWORD),
                    ("dwFlags", wintypes.DWORD),
                    ("time", wintypes.DWORD),
                    ("dwExtraInfo", ctypes.c_size_t)]

    class INPUTUNION(ctypes.Union):
        _fields_ = [("mi", MOUSEINPUT)]

    class INPUT(ctypes.Structure):
        _anonymous_ = ("u",)
        _fields_ = [("type", wintypes.DWORD), ("u", INPUTUNION)]

    event = INPUT(type=0, mi=MOUSEINPUT(0, 0, 0, flags, 0, 0))
    if ctypes.windll.user32.SendInput(1, ctypes.byref(event),
                                      ctypes.sizeof(INPUT)) != 1:
        raise ctypes.WinError()


def mac_keys(script):
    subprocess.run(["osascript", "-e", script], check=True,
                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)


def activate_and_open_dialog(process):
    if os.name == "nt":
        user32 = ctypes.windll.user32
        user32.GetWindowTextLengthW.argtypes = [wintypes.HWND]
        user32.GetWindowTextLengthW.restype = ctypes.c_int
        user32.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR,
                                          ctypes.c_int]
        user32.GetWindowTextW.restype = ctypes.c_int
        user32.SetForegroundWindow.argtypes = [wintypes.HWND]
        user32.SetForegroundWindow.restype = wintypes.BOOL
        user32.GetWindowThreadProcessId.argtypes = [wintypes.HWND,
                                                    ctypes.POINTER(wintypes.DWORD)]
        user32.GetWindowThreadProcessId.restype = wintypes.DWORD
        user32.SetActiveWindow.argtypes = [wintypes.HWND]
        user32.SetFocus.argtypes = [wintypes.HWND]
        user32.AttachThreadInput.argtypes = [wintypes.DWORD, wintypes.DWORD,
                                             wintypes.BOOL]
        windows = []
        callback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND,
                                      wintypes.LPARAM)
        user32.EnumWindows.argtypes = [callback, wintypes.LPARAM]
        user32.EnumWindows.restype = wintypes.BOOL

        def visit(hwnd, _):
            length = user32.GetWindowTextLengthW(hwnd)
            owner = wintypes.DWORD()
            user32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
            if length:
                title = ctypes.create_unicode_buffer(length + 1)
                user32.GetWindowTextW(hwnd, title, length + 1)
                if title.value == TITLE or owner.value == process.pid:
                    windows.append(hwnd)
                    return False
            return True

        deadline = time.monotonic() + 30
        while time.monotonic() < deadline and not windows:
            user32.EnumWindows(callback(visit), 0)
            time.sleep(0.2)
        if not windows:
            raise RuntimeError(f"emulator window not found for process {process.pid}")
        hwnd = windows[0]
        user32.ShowWindow(hwnd, 9)
        user32.GetForegroundWindow.restype = wintypes.HWND
        foreground = user32.GetForegroundWindow()
        foreground_thread = user32.GetWindowThreadProcessId(
            foreground, None) if foreground else 0
        target_thread = user32.GetWindowThreadProcessId(hwnd, None)
        if foreground_thread and foreground_thread != target_thread:
            user32.AttachThreadInput(target_thread, foreground_thread, True)
        user32.SetForegroundWindow(hwnd)
        user32.SetActiveWindow(hwnd)
        user32.SetFocus(hwnd)
        if foreground_thread and foreground_thread != target_thread:
            user32.AttachThreadInput(target_thread, foreground_thread, False)
        focused = user32.GetForegroundWindow()
        focused_value = focused.value if hasattr(focused, "value") else focused
        hwnd_value = hwnd.value if hasattr(hwnd, "value") else hwnd
        if focused_value != hwnd_value:
            raise RuntimeError("emulator window could not receive desktop input")
        # Open / Run is the first toolbar button, centered near client (42,42).
        point = wintypes.POINT(42, 42)
        user32.ClientToScreen.argtypes = [wintypes.HWND,
                                          ctypes.POINTER(wintypes.POINT)]
        user32.ClientToScreen.restype = wintypes.BOOL
        user32.SetCursorPos.argtypes = [ctypes.c_int, ctypes.c_int]
        user32.SetCursorPos.restype = wintypes.BOOL
        if not user32.ClientToScreen(hwnd, ctypes.byref(point)) or not \
                user32.SetCursorPos(point.x, point.y):
            raise RuntimeError("could not position pointer on Open / Run")
        time.sleep(0.2)
        windows_mouse(0x0002)
        time.sleep(0.1)
        windows_mouse(0x0004)
        time.sleep(0.5)
        if not native_dialog_visible():
            # The same command is available as the first keyboard-menu action.
            windows_press(0x70)
            time.sleep(0.25)
            windows_press(0x0D)
    elif sys.platform == "darwin":
        mac_keys('tell application "System Events" to set frontmost of first application process whose name is "Warajevo-ZX-Spectrum-Next" to true')
        mac_keys('tell application "System Events" to key code 122')
        time.sleep(0.25)
        mac_keys('tell application "System Events" to key code 36')
    else:
        subprocess.run(["xdotool", "search", "--onlyvisible", "--name", TITLE,
                        "windowactivate", "--sync"], check=True)
        subprocess.run(["xdotool", "key", "F1", "Return"], check=True)
    time.sleep(2)


def native_dialog_visible():
    if os.name == "nt":
        user32 = ctypes.windll.user32
        found = []
        callback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND,
                                      wintypes.LPARAM)
        user32.EnumWindows.argtypes = [callback, wintypes.LPARAM]
        user32.EnumWindows.restype = wintypes.BOOL

        def visit(hwnd, _):
            length = user32.GetWindowTextLengthW(hwnd)
            if length:
                title = ctypes.create_unicode_buffer(length + 1)
                user32.GetWindowTextW(hwnd, title, length + 1)
                if title.value == "Open / Run":
                    found.append(hwnd)
                    return False
            return True

        user32.EnumWindows(callback(visit), 0)
        return bool(found)
    if sys.platform == "darwin":
        result = subprocess.run(
            ["osascript", "-e", 'tell application "System Events" to get name of every window of (first application process whose frontmost is true)'],
            check=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            text=True)
        return result.returncode == 0 and "Open / Run" in result.stdout
    return bool(subprocess.run(
        ["xdotool", "search", "--onlyvisible", "--name", "Open / Run"],
        check=False, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
        text=True).stdout.strip())


def wait_dialog_state(expected, timeout=20):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if native_dialog_visible() == expected:
            return
        time.sleep(0.2)
    state = "visible" if expected else "closed"
    raise RuntimeError(f"native Open / Run dialog did not become {state}")


def select_file(path):
    if os.name == "nt":
        # The production Windows shell dialog exposes the file-name field.
        windows_key(0x12, True)  # Alt+N
        windows_press(0x4E)
        windows_key(0x12, False)
        windows_text(str(path))
        windows_press(0x0D)
    elif sys.platform == "darwin":
        parent = str(path.parent)
        escaped = parent.replace("\\", "\\\\").replace('"', '\\"')
        mac_keys('tell application "System Events" to keystroke "g" using {command down, shift down}')
        time.sleep(0.5)
        mac_keys(f'tell application "System Events" to keystroke "{escaped}"')
        mac_keys('tell application "System Events" to key code 36')
        time.sleep(1)
        escaped_name = path.name.replace("\\", "\\\\").replace('"', '\\"')
        mac_keys(f'tell application "System Events" to keystroke "{escaped_name}"')
        time.sleep(0.25)
        try:
            mac_keys('tell application "System Events" to click button "Open" of window 1 of process "Warajevo-ZX-Spectrum-Next"')
        except subprocess.CalledProcessError:
            mac_keys('tell application "System Events" to key code 36')
    else:
        subprocess.run(["xdotool", "key", "ctrl+l"], check=True)
        subprocess.run(["xdotool", "type", "--clearmodifiers", str(path)],
                       check=True)
        subprocess.run(["xdotool", "key", "Return"], check=True)


def capture_screen(destination):
    if os.name == "nt":
        script = r'''
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
$bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen
$bitmap = New-Object System.Drawing.Bitmap($bounds.Width, $bounds.Height)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.CopyFromScreen($bounds.Left, $bounds.Top, 0, 0, $bitmap.Size)
$bitmap.Save($env:WZSN_SCREENSHOT_PATH, [System.Drawing.Imaging.ImageFormat]::Png)
$graphics.Dispose()
$bitmap.Dispose()
'''
        screenshot_environment = os.environ.copy()
        screenshot_environment["WZSN_SCREENSHOT_PATH"] = str(destination)
        subprocess.run(["powershell", "-NoProfile", "-Command", script,
                        ], check=True, env=screenshot_environment)
    elif sys.platform == "darwin":
        subprocess.run(["screencapture", "-x", str(destination)], check=True)
    else:
        subprocess.run(["import", "-window", "root", str(destination)],
                       check=True)
    if not destination.is_file() or destination.stat().st_size < 1024:
        raise RuntimeError("desktop screenshot was not captured")


def main():
    binary = pathlib.Path(sys.argv[1]).resolve()
    output = pathlib.Path(sys.argv[2]).resolve()
    output.mkdir(parents=True, exist_ok=True)
    tap = output / "open-run-smoke.tap"
    # One structurally valid, checksummed 3-byte standard TAP block.
    tap.write_bytes(bytes((3, 0, 0, 0, 0)))
    screenshot = output / "open-run-desktop.png"
    log_handle = (output / "emulator.log").open("wb")
    environment = os.environ.copy()
    environment["WZSN_ROM_PATH"] = str(pathlib.Path("roms/48.rom").resolve())
    process = subprocess.Popen([str(binary)], env=environment,
                               stdout=log_handle, stderr=subprocess.STDOUT)
    succeeded = False
    try:
        time.sleep(5)
        if process.poll() is not None:
            raise RuntimeError("emulator exited before Open / Run")
        activate_and_open_dialog(process)
        wait_dialog_state(True)
        select_file(tap)
        wait_dialog_state(False)
        time.sleep(3)
        if process.poll() is not None:
            log_handle.flush()
            log_handle.close()
            output_log = (output / "emulator.log").read_text(
                encoding="utf-8", errors="replace")
            raise RuntimeError(f"emulator exited after selection: {output_log}")
        capture_screen(screenshot)
        proof = {
            "status": "pass",
            "runner": platform.platform(),
            "action": ("Open / Run toolbar button/F1 keyboard menu > native file chooser"
                       if os.name == "nt" else
                       "F1 keyboard menu > Open / Run > native file chooser"),
            "selectedFixture": tap.name,
            "selectedSha256": __import__("hashlib").sha256(tap.read_bytes()).hexdigest(),
            "desktopScreenshot": screenshot.name,
            "desktopScreenshotSha256": __import__("hashlib").sha256(
                screenshot.read_bytes()).hexdigest(),
        }
        (output / "open-run-ui.json").write_text(
            json.dumps(proof, indent=2) + "\n", encoding="utf-8")
        succeeded = True
        print("PASS Open / Run native dialog selection; inspect desktop screenshot for Tape Stopped")
    finally:
        if not succeeded and not screenshot.exists():
            try:
                capture_screen(screenshot)
            except Exception:
                pass
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)
        if not log_handle.closed:
            log_handle.close()


if __name__ == "__main__":
    main()
