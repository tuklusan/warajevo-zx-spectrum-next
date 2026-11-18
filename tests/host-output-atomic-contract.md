<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Concurrent managed-output contract

The hosted four-runner regression starts eight independent processes that
repeatedly atomically replace one shared destination while a reader checks that
every visible version is a complete writer payload. A second race verifies that
exclusive creation of one logical destination has exactly one complete winner.
Both cases check that temporary siblings are removed after publication.

Snapshots, printer exports, and other managed persistent outputs use the same
atomic/exclusive host-output primitives. Their call sites are fixture-pinned by
the driver. Conversion commands remain explicitly unavailable, so there is no
conversion writer to race. Proof is pinned in
`test-results/host-output-atomic-contract.json`.
