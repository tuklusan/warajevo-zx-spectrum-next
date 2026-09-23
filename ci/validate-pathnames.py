#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms, warranty disclaimer, termination,
# patent, trademark, and governing-law provisions.

"""Reject forbidden terms in every tracked pathname of a Git tree."""

import base64
import os
import subprocess
import sys


ENCODED_TERMS = (
    "Q2xhdWRl",
    "Q29kZXg=",
    "QW50aHJvcGl4",
    "T3BlbkFJ",
    "Q2hhdEdQVA==",
    "U1BEWC1MaWNlbnNlLUlkZW50aWZpZXI=",
    "U1BEWA==",
)
TERMS = tuple(base64.b64decode(value, validate=True).decode("ascii").casefold()
              for value in ENCODED_TERMS)


def main():
    tree = sys.argv[1] if len(sys.argv) == 2 else "HEAD"
    raw_paths = subprocess.check_output(
        ["git", "ls-tree", "-r", "--name-only", "-z", tree]
    )
    paths = [os.fsdecode(item) for item in raw_paths.split(b"\0") if item]
    violations = []
    for path in paths:
        folded = path.casefold()
        for term in TERMS:
            if term in folded:
                violations.append((path, term))

    if violations:
        for path, term in violations:
            print(f"Forbidden pathname match ({term}): {path}", file=sys.stderr)
        return 1
    print(f"Validated {len(paths)} tracked paths in {tree}.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
