<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Warajevo-native TAP contract

The standalone native TAP contract exercises the production parser and writer
on all four supported record classes, verifies linked-record round trips, and
rejects malformed links, lengths, and unknown record types. The exact-source
five-target Release build and CTest passed in native run `43993717917` attempt
2 at source `0666bd1a1d220c8fc83ba718b8abab6fa77ba01e`; the proof-only run was
`43995587257`. Each target reported `native-tap-contract cases=2 status=pass`.
The contract covers parser/writer compatibility only; host file loading and
broader native-media corpus coverage remain open.
