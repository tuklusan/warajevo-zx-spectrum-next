<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Multi-instance stress suite

`run-stress.py` is run only on the four hosted project runner families. It uses
twelve synchronized processes per contention case, checks lock release and
retry recovery, and records per-runner output for proof aggregation. The test
worker in `tests/multi-instance-stress.c` invokes the project's Control Port,
host configuration, media ownership, Telnet screenshot, and exclusive-output
implementations directly.
