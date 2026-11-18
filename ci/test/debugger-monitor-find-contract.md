<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger monitor find command

`F <start> <end> <pattern...>` searches an inclusive, non-wrapping logical
address range. One- or two-digit tokens add bytes; three- or four-digit tokens
add little-endian words; `-1` adds a wildcard byte. The first match selects the
memory view address. No match and patterns too long for the range report
not-found without changing machine state or selection.

The 15-assertion CTest contract covers byte and word matches, wildcards, range
edges, invalid input, no-match behavior, selection, and read-only state. The
source-push workflow runs it on all five native Release/Sokol targets. Proof:
[debugger-monitor-find-contract.json](../../test-results/debugger-monitor-find-contract.json).
