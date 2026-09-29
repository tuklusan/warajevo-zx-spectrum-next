#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

import os
import subprocess
import sys
import time


def main():
    environment = os.environ.copy()
    environment["GTK_USE_PORTAL"] = "0"
    environment["GDK_BACKEND"] = "x11"
    process = subprocess.Popen([sys.argv[1]], env=environment,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, text=True)
    try:
        deadline = time.monotonic() + 20
        window = None
        while time.monotonic() < deadline and process.poll() is None:
            search = subprocess.run(
                ["xdotool", "search", "--onlyvisible", "--name", "Open / Run"],
                check=False, capture_output=True, text=True, env=environment)
            matches = search.stdout.splitlines()
            if search.returncode == 0 and matches:
                window = matches[-1]
                break
            time.sleep(0.1)
        if window is None:
            raise RuntimeError("native open dialog did not appear")
        subprocess.run(["xdotool", "windowfocus", "--sync", window],
                       check=False, capture_output=True, env=environment)
        subprocess.run(["xdotool", "key", "Escape"], check=True,
                       capture_output=True, env=environment)
        stdout, stderr = process.communicate(timeout=15)
        if process.returncode != 0:
            raise RuntimeError((stdout + stderr).strip())
        if "PASS: native chooser cancellation" not in stdout:
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
