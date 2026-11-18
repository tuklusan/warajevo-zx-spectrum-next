<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Multi-instance stress suite

`run-stress.py` is run only on the four hosted project runner families. It uses
twelve synchronized processes per contention case, checks lock release and
retry recovery, and records per-runner output for proof aggregation. The test
worker in `tests/multi-instance-stress.c` invokes the project's Control Port,
host configuration, media ownership, Telnet screenshot, and exclusive-output
implementations directly. Its command-registry dependency also links the
thread-identity module used to enforce application-command ownership.
