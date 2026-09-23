<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Multi-instance host-service stress

The hosted four-runner matrix starts twelve processes at a shared barrier and
exercises Control Port allocation, concurrent atomic settings writes, lock
contention and recovery, exclusive writable-media claims and release, and
concurrent screenshot generation through the Telnet screenshot service. Each
run validates that settings files contain one complete writer snapshot, media
has one owner at a time, and every generated PNG has a unique path and valid
signature. Evidence is pinned in `test-results/multi-instance-stress.json`.
