<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Hosted media corpus ingestion

The isolated hosted contract parses each TAP and TZX in `test-media/` through
the production core parser and the tape reader used by Fuse's media stack.
It compares ingestion results and includes timed-segment expansion and TZX
block classification. It also contracts zero-duration TZX pause stop behavior.
It reports only aggregate counts; media contents and
per-media paths are never written to proofs or uploaded artifacts. Unsupported
TZX timing is counted explicitly. Malformed, unreadable, or parser-divergent
files fail the run.

This is an emulated software-reference check, not physical hardware evidence.
It establishes parser/ingestion coverage only. The normal-ROM loader, GUI
execution of every program, and broader software/reference behavior remain
separate acceptance work; physical hardware/audio evidence is out of scope.
