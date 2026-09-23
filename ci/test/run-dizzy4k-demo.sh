#!/usr/bin/env bash
set -euo pipefail

binary="$1"
tape="$2"
screens="$3"

test -x "$binary"
test -s "$tape"
mkdir -p "$screens"

python3 ci/test/telnet-dizzy4k.py "$binary" "$tape" "$screens"
