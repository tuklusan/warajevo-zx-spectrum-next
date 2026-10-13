#!/usr/bin/env bash
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Commercial Use and use for AI/ML model training are
# prohibited unless separately authorized. Attribution is required. See
# LICENSE for full terms.
set -euo pipefail

readonly FUSE_REVISION=7edc2418ce11837d9ede799f7e0445440beead1d
readonly LIBSPECTRUM_REVISION=ebac4e5f55ba434549cd7970d257fb9a0f1e2f63
readonly SOURCE_ROOT="$(git rev-parse --show-toplevel)"
readonly BUILD_ROOT="${RUNNER_TEMP:?RUNNER_TEMP is required}/wzsn-fuse-visual"
readonly INSTALL_ROOT="$BUILD_ROOT/install"
readonly FUSE_SOURCE="$BUILD_ROOT/fuse-src"
readonly SPECTRUM_SOURCE="$BUILD_ROOT/libspectrum-src"
readonly OUTPUT_ROOT="$SOURCE_ROOT/dist/fuse-visual-reference"
readonly ROM_IMAGE="$SOURCE_ROOT/roms/48.rom"

fetch_revision() {
    local remote="$1"
    local revision="$2"
    local destination="$3"
    git init "$destination"
    git -C "$destination" remote add origin "$remote"
    git -C "$destination" fetch --no-tags --depth=1 origin "$revision"
    git -C "$destination" checkout --detach FETCH_HEAD
    test "$(git -C "$destination" rev-parse HEAD)" = "$revision"
}

mkdir -p "$BUILD_ROOT" "$OUTPUT_ROOT"
fetch_revision https://git.code.sf.net/p/fuse-emulator/libspectrum \
    "$LIBSPECTRUM_REVISION" "$SPECTRUM_SOURCE"
fetch_revision https://git.code.sf.net/p/fuse-emulator/fuse \
    "$FUSE_REVISION" "$FUSE_SOURCE"

(
    cd "$SPECTRUM_SOURCE"
    ./autogen.sh
    ./configure --prefix="$INSTALL_ROOT"
    make --jobs=2
    make install
)

(
    cd "$FUSE_SOURCE"
    ./autogen.sh
    export PKG_CONFIG_PATH="$INSTALL_ROOT/lib/pkgconfig:$INSTALL_ROOT/lib64/pkgconfig"
    ./configure --prefix="$INSTALL_ROOT" --with-null-ui --enable-automation
    make --jobs=2
    make install
)

test -s "$ROM_IMAGE"
test -s "$INSTALL_ROOT/bin/fuse"
export LD_LIBRARY_PATH="$INSTALL_ROOT/lib:$INSTALL_ROOT/lib64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
"$INSTALL_ROOT/bin/fuse" --version | tee "$OUTPUT_ROOT/fuse-version.txt"
"$INSTALL_ROOT/bin/fuse" --help | grep -F -- '--automation-capture-screen'
timeout --signal=TERM 120s "$INSTALL_ROOT/bin/fuse" \
    --machine 48 \
    --rom-48 "$ROM_IMAGE" \
    --no-sound \
    --automation-output "$OUTPUT_ROOT" \
    --automation-frames 200 \
    --automation-capture-screen
test -s "$OUTPUT_ROOT/screen.png"
test -s "$OUTPUT_ROOT/result.json"
python3 ci/test/fuse-visual-equivalence/write-reference-proof.py \
    --rom "$ROM_IMAGE" \
    --result "$OUTPUT_ROOT/result.json" \
    --screen "$OUTPUT_ROOT/screen.png" \
    --proof-out "$OUTPUT_ROOT/reference-proof.json" \
    --fuse-revision "$FUSE_REVISION" \
    --libspectrum-revision "$LIBSPECTRUM_REVISION"
