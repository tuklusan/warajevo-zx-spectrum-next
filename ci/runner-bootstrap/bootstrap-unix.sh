#!/usr/bin/env bash
set -euo pipefail

manifest="$1"
case "$(uname -s)" in
  Darwin)
    command -v brew >/dev/null
    brew update
    command -v cmake >/dev/null || brew install cmake
    command -v ninja >/dev/null || brew install ninja
    command -v python3 >/dev/null || brew install python
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
