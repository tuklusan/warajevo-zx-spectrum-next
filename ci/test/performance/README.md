<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms, warranty disclaimer, termination,
patent, trademark, and governing-law provisions. -->

# Phase-16 performance baselines

`baseline.c` measures deterministic Release workloads for CPU instruction
execution, canonical raster generation, AY/beeper mixing, tape playback,
machine snapshot serialization, and the UI status plus raster-presentation
handoff. It repeats each workload three times, checks stable output fingerprints,
and reports the median process CPU time. Snapshot serialization also restores
the saved state and verifies the canonical state hash.

The pre-optimization records were produced on the four pinned hosted runner
families at commit `574714b6be131e086f117676159efca4eb58a273` (run
`35886051010`). After the first accepted raster optimization, the current
records in `baselines/` were captured at `350181913777165a4a52fb47318b1b54269101f7`
(run `35888370795`). The original records remain in Git history. The performance
workflow runs on `main`, repeats these workloads, and rejects a fingerprint
change or a throughput decrease greater than 40% against that runner's own
current baseline. The 40% allowance absorbs normal hosted-runner variance; it
is not a target for accepted optimizations.

When accepting an optimization, compare against the existing records first.
Update the affected runner baselines only after the optimization has passing
canonical correctness evidence and the before/after measurements have been
reviewed. Keep the old records in the change history.

The CPU execution and snapshot serialization fingerprint expectations were refreshed for state format v14 after AY divider phases became serialized. All four hosted runner artifacts on run `35952794245` agreed on the updated fingerprints; the pinned throughput measurements remain from the original baseline records. The state-format update passed the four-runner comparison on run `35953015461` against the unchanged throughput measurements.
