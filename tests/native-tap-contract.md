<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Warajevo-native TAP contract

The standalone native TAP contract exercises the production parser and writer
on all four supported record classes, verifies linked-record round trips,
converts uncompressed records 0–3 into standard TAP blocks for timed playback,
and rejects malformed links, lengths, and unknown record types. The exact-source
five-target Release build and CTest passed in native run `44057710617` (#1330)
at source `06a546145110dc43c40e4cd87e4907b472afe5c8`. Each target reports
`native-tap-contract cases=3 status=pass`.
The contract covers parser/writer compatibility only; host file loading and
broader native-media corpus coverage remain open.
