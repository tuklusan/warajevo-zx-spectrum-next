<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Hosted media corpus ingestion

The isolated hosted contract parses each TAP and TZX in `test-media/` through
the production core parser, including timed-segment expansion and TZX block
classification. It reports only aggregate counts; media contents and
per-media paths are never written to proofs or uploaded artifacts. Unsupported
TZX timing is counted explicitly. Malformed or unreadable files fail the run.

This contract establishes parser/ingestion coverage only. The normal-ROM loader
and GUI execution of every program remain separate acceptance requirements.
