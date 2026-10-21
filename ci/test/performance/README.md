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
(run `35888370795`). The main-branch full native build workflow runs these
workloads on the same five native runners as the product build and compares
each runner against its own pinned baseline. Linux x64 and ARM64 baselines were
initialized from the current-source measurements in run `39506023731`; no
earlier Linux throughput reference was available. The 40% allowance absorbs
normal hosted-runner variance; it is not a target for accepted optimizations.

When accepting an optimization, compare against the existing records first.
Update the affected runner baselines only after the optimization has passing
canonical correctness evidence and the before/after measurements have been
reviewed. Keep the old records in the change history.

The CPU execution and snapshot serialization fingerprint expectations were refreshed for state format v14 after AY divider phases became serialized. All four hosted runner artifacts on run `35952794245` agreed on the updated fingerprints; the pinned throughput measurements remain from the original baseline records. The state-format update passed the four-runner comparison on run `35953015461` against the unchanged throughput measurements.

State format v15 / extension v4 adds eight Microdrive transport records and
omits clean staged-sector bytes from canonical state while preserving dirty
sector data. The canonical CPU and snapshot fingerprints are now
`f49f759382ec8a34` and `7254cf8df4a13bdb`; hosted canonical correctness passed
on run `36820270996`. Snapshot throughput baselines were refreshed for Windows
ARM and both Mac runner families after the serializer optimization. Three
macOS Intel measurements in run `36820851431` also showed repeatable slowdowns
across the unchanged raster, audio, tape, and UI workloads; their median rates
are now the runner's reference profile. This is a runner-profile inference;
the Windows x64 throughput profile remains unchanged. Linux x64 and ARM64
throughput baselines were initialized from the successful measurements in run
`39506023731` at source `60a9c799bb67384f853a637a9e196d4a9879a79b`. The
integrated five-target Phase-16 gate passed all 30 fingerprint and throughput
comparisons in run `39506587064` at source
`b4ea7fdcbbf808aac35c1e61447fd6449b52943f`; the largest slowdown was 28.7% on
macOS ARM64 CPU execution, within the 40% gate. Current raw records are pinned
in `test-results/performance-current/` and
`test-results/phase16-performance-current.json`.
State extension v6 adds each drive's host write-protection state to canonical
serialization. Run `39526536113` built and tested the source on all five native
targets, then rejected the expected CPU and snapshot fingerprint changes. The
five delivered-platform fingerprints were refreshed without changing the
throughput references or 40% gate. Exact follow-up run `39526865899` passed the
clean full build, all 19 CTests per target, and all 30 Phase-16 fingerprint and
throughput checks. The largest slowdown was 31.0% on macOS Intel. Current
records and the aggregator are pinned in
`test-results/performance-current/` and
`test-results/phase16-performance-current.json`.

State extension v9 adds a canonical pending Interface 1 bus continuation. Run `39543047933` passed all 26 CTests per native target and produced the same new CPU and snapshot fingerprints on all five targets. Only those fingerprints were refreshed; throughput references and the 40% gate remain unchanged. A successful exact-head five-target rerun is required to close the current gate.
