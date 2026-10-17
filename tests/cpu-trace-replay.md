<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# CPU timing trace replay and byte coverage

The hosted test runs CP/SCF/NOP/CCF, repeated DD/FD prefixes, indexed DDCB,
ED, CB, HALT with a refetch, NMI, and a post-NMI fetch. It freezes the
production 16 MiB circular trace, recovers it, and reconstructs the CPU
state from absolute/delta chunks. All 10 instruction-boundary states are
compared field by field with live state, including Q and prior Q. All 28
recovered bus events match the bus observer in order, tick, address, value,
and duration. The 21 instruction-stream bytes include indexed displacement
and the non-M1 final DDCB opcode; 17 M1 opcode records and NMI acceptance
are checked. Sequence and tick order must be monotonic.

Source coverage: every CPU bus request goes through `wz_z80_bus` (or its ED
wrapper), which calls `wz_machine_bus_request`; that function emits the
CPU bus event including contention delay. Every M1 path uses `wz_z80_bus`,
which additionally emits an opcode-byte event. The indexed-CB displacement
and final opcode are memory-read bus events. An IM 0 injected opcode comes
from an interrupt-acknowledge bus event. Instruction boundaries emit the
five register-state chunks; interrupt sample, accept, and acknowledge paths
are separately checked by the hosted interrupt-mode acceptance. The
5,000-fetch HALT test checks periodic absolute sync for wrapped recovery.

Hosted proof: [cpu-trace-replay.json](../test-results/cpu-trace-replay.json).
