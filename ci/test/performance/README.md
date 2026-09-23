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

The initial records in `baselines/` were produced on the four pinned hosted
runner families at commit `dfaa8243a074f2b9268eeb7cd7381397577c2a9b`. The
performance workflow runs on `main`, repeats these workloads, and rejects a
fingerprint change or a throughput decrease greater than 40% against that
runner's own baseline. The 40% allowance absorbs normal hosted-runner variance;
it is not a target for accepted optimizations.

When accepting an optimization, compare against the existing records first.
Update the affected runner baselines only after the optimization has passing
canonical correctness evidence and the before/after measurements have been
reviewed. Keep the old records in the change history.
