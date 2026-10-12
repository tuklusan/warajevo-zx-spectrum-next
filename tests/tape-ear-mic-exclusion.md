<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Tape Ear+Mic signal exclusion

The hosted contract verifies that while the upstream core networking state is
`EAR_MIC`, ordinary tape transport cannot control or advance a mounted tape and
the tape EAR signal remains low. It covers rejected motor changes and rewind,
no-op advance, unchanged transport position, and the EAR level. It does not
implement Architecture #3 bootstrap or networking operations.

Run on Linux with:

```text
cmake -S ci/test/tape-ear-mic-exclusion -B dist/tape-ear-mic-exclusion -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build dist/tape-ear-mic-exclusion --config Release --target wz_tape_ear_mic_exclusion --parallel 2
dist/tape-ear-mic-exclusion/wz_tape_ear_mic_exclusion
```

Hosted proof is recorded in `test-results/tape-ear-mic-exclusion.json`.
