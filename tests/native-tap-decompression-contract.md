<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Warajevo-native TAP decompression contract

The standalone contract exercises production conversion of native TAP type-5
compressed records into standard TAP blocks. It independently checks a
literal-only stream, a long match using the original distance coding, a
short-distance run, and rejection of malformed signed lengths.

The exact-source five-target Release/Sokol run `44254470964` passed all four
cases on Windows x64, Linux x64, Linux ARM64, macOS ARM64, and macOS Intel.
Each target reports `native-tap-decompression-contract cases=4 status=pass`.
Sampled type-4 records and native maintenance decompression remain outside
this contract.
