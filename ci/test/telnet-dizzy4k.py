#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# Proprietary rights reserved except as expressly licensed herein.
#
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
#
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms, warranty disclaimer, termination,
# patent, trademark, and governing-law provisions.
"""Runner-only GUI/telnet DIZZY4K smoke proof."""

import pathlib
import os
import re
import socket
import subprocess
import sys
import time


def command(sock, text):
    sock.sendall((text + "\n").encode("ascii"))
    data = bytearray()
    deadline = time.time() + 10
    while time.time() < deadline:
        try:
            part = sock.recv(4096)
        except socket.timeout:
            continue
        if not part:
            break
        data.extend(part.replace(b"\xff", b""))
        if b"\n" in data and (b"OK " in data or b"ERR " in data):
            break
    result = data.decode("utf-8", "replace")
    if "ERR " in result:
        raise RuntimeError(result.strip())
    return result


def find_port():
    for port in range(30740, 32788):
        sock = socket.socket()
        sock.settimeout(0.25)
        try:
            sock.connect(("127.0.0.1", port))
            return sock, port
        except OSError:
            sock.close()
    return None, None


def main():
    binary = pathlib.Path(sys.argv[1]).resolve()
    tape = pathlib.Path(sys.argv[2]).resolve()
    output = pathlib.Path(sys.argv[3]).resolve()
    output.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment["WZSN_ROM_PATH"] = str(tape.parent.parent / "roms" / "48.rom")
    environment["WZSN_TAPE_PATH"] = str(tape)
    process = subprocess.Popen([str(binary)], env=environment,
                               stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    try:
        sock = None
        for _ in range(120):
            sock, port = find_port()
            if sock:
                break
            time.sleep(1)
        if not sock:
            raise RuntimeError("GUI control port did not become available")
        sock.settimeout(1)
        command(sock, "HELP")
        time.sleep(60)
        response = command(sock, "SCREENSHOT")
        match = re.search(
            r"OK (?:SCREENSHOT\s+|DO host\.screenshot\.temp\s+PATH=)\"?([^\"\r\n]+)",
            response,
        )
        if not match:
            raise RuntimeError(f"Screenshot path missing from response: {response!r}")
        source = pathlib.Path(match.group(1).strip()).resolve()
        destination = output / f"dizzy4k-{port}.png"
        destination.write_bytes(source.read_bytes())
        print(f"DIZZY4K screenshot: {destination}")
    finally:
        if 'sock' in locals() and sock:
            sock.close()
        process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


if __name__ == "__main__":
    main()
