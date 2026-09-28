#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Exercise live model aliases and the shared command on a hosted GUI runner."""

import os
import pathlib
import re
import socket
import subprocess
import sys
import time


def send(sock: socket.socket, text: str) -> str:
    sock.sendall((text + "\n").encode("ascii"))
    data = bytearray()
    deadline = time.monotonic() + 10.0
    while time.monotonic() < deadline:
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
    if not result:
        raise RuntimeError(f"No response to {text!r}")
    return result


def expect(sock: socket.socket, command: str, response: str) -> str:
    actual = send(sock, command)
    if actual != response:
        raise RuntimeError(f"{command!r}: expected {response!r}, got {actual!r}")
    return actual


def status(sock: socket.socket, model: str, state: str, speed: str) -> None:
    actual = send(sock, "STATUS")
    expected = (rf"STATUS .*\bMODEL={model}\b .*\bSTATE={state}\b "
                rf"SPEED={speed}\b")
    if re.search(expected, actual) is None:
        raise RuntimeError(f"STATUS did not show {model}/{state}/{speed}: {actual!r}")


def connect(port_range: range) -> socket.socket:
    deadline = time.monotonic() + 120.0
    while time.monotonic() < deadline:
        for port in port_range:
            sock = socket.socket()
            sock.settimeout(1.0)
            try:
                sock.connect(("127.0.0.1", port))
                return sock
            except OSError:
                sock.close()
        time.sleep(0.5)
    raise RuntimeError("Control Port did not become available")


def main() -> None:
    binary, tape, rom = (pathlib.Path(value).resolve() for value in sys.argv[1:4])
    output = pathlib.Path(sys.argv[4]).resolve()
    output.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment["WZSN_TAPE_PATH"] = str(tape)
    environment["WZSN_ROM_PATH"] = str(rom)
    log_handle = None
    process = None
    sock = None
    try:
        log_handle = (output / "emulator.log").open("w", encoding="utf-8")
        process = subprocess.Popen([str(binary)], env=environment,
                                   stdout=log_handle, stderr=subprocess.STDOUT)
        sock = connect(range(30740, 32788))
        status(sock, "48K", "RUNNING", "100")
        for speed in ("25", "50", "100", "200", "400", "800", "UNLIMITED"):
            expect(sock, f"SPEED {speed}", f"OK SPEED {speed}\r\n")
            status(sock, "48K", "RUNNING", speed)
        expect(sock, "SPEED 200", "OK SPEED 200\r\n")
        expect(sock, "MODEL 128K", "OK MODEL 128K\r\n")
        status(sock, "128K", "RUNNING", "200")
        expect(sock, "MODEL 48K", "OK MODEL 48K\r\n")
        status(sock, "48K", "RUNNING", "200")

        expect(sock, "PAUSE", "OK PAUSE\r\n")
        status(sock, "48K", "PAUSED", "200")
        expect(sock, "PAUSE", "OK PAUSE\r\n")
        status(sock, "48K", "PAUSED", "200")
        expect(sock, "MODEL 128K", "OK MODEL 128K\r\n")
        status(sock, "128K", "PAUSED", "200")
        expect(sock, "MODEL 48K", "OK MODEL 48K\r\n")
        status(sock, "48K", "PAUSED", "200")
        expect(sock, "RESUME", "OK RESUME\r\n")
        status(sock, "48K", "RUNNING", "200")
        expect(sock, "RESUME", "OK RESUME\r\n")
        status(sock, "48K", "RUNNING", "200")
        status(sock, "48K", "RUNNING", "200")

        expect(sock, "MODEL 16K", "ERR BAD_MODEL\r\n")
        status(sock, "48K", "RUNNING", "200")
        expect(sock, "DO machine.model.set 128k",
               "OK DO machine.model.set 128k\r\n")
        status(sock, "128K", "RUNNING", "200")
        expect(sock, "DO machine.model.set 48k",
               "OK DO machine.model.set 48k\r\n")
        status(sock, "48K", "RUNNING", "200")
        expect(sock, "RESET", "OK RESET\r\n")
        status(sock, "48K", "RUNNING", "200")
        print("Telnet model switch live regression passed")
    finally:
        if sock is not None:
            sock.close()
        if process is not None:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
        if log_handle is not None:
            log_handle.close()


if __name__ == "__main__":
    main()
