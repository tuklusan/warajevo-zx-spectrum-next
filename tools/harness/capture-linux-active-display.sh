#!/usr/bin/env bash
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "usage: capture-linux-active-display.sh <output-path-within-project>" >&2
    exit 2
fi

project_root="$(pwd -P)"
output_path="$(
python3 - "$project_root" "$1" <<'PY'
from pathlib import Path
import sys

project_root = Path(sys.argv[1]).resolve()
candidate = Path(sys.argv[2])
if not candidate.is_absolute():
    candidate = project_root / candidate

resolved = candidate.resolve()
resolved.relative_to(project_root)
print(resolved)
PY
)"

mkdir -p "$(dirname "$output_path")"

xauthority_path="${WZSN_CAPTURE_XAUTHORITY:-${XAUTHORITY:-$HOME/.Xauthority}}"
display_value="${WZSN_CAPTURE_DISPLAY:-${DISPLAY:-}}"

if [ -z "$display_value" ]; then
    display_value=":0"
fi

if [ -z "$display_value" ]; then
    echo "no active X11 display could be resolved for screenshot capture" >&2
    exit 1
fi

if command -v scrot >/dev/null 2>&1; then
    DISPLAY="$display_value" XAUTHORITY="$xauthority_path" scrot "$output_path"
elif command -v import >/dev/null 2>&1; then
    DISPLAY="$display_value" XAUTHORITY="$xauthority_path" import -window root "$output_path"
else
    echo "no supported Linux screenshot tool is installed" >&2
    exit 1
fi

echo "SCREENSHOT_PATH=$output_path"
echo "DISPLAY_USED=$display_value"
echo "XAUTHORITY_USED=$xauthority_path"
