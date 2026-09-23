#!/usr/bin/env bash
set -euo pipefail

manifest="$1"
case "$(uname -s)" in
  Darwin)
    command -v brew >/dev/null
    brew update
    brew install cmake ninja python
    ;;
  Linux)
    sudo apt-get update
    sudo apt-get install -y cmake ninja-build python3 xvfb libx11-dev libx11-xcb-dev libxrandr-dev libxi-dev libxcursor-dev libgl1-mesa-dev
    ;;
  *)
    printf 'Unsupported Unix runner: %s\n' "$(uname -s)" >&2
    exit 1
    ;;
esac

test -s "$manifest"
