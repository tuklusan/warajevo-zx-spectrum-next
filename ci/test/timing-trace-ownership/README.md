<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Timing trace process ownership contract

This standalone hosted contract launches eight independent processes that race
to create one production 16 MiB trace path. Exactly one process must own and
freeze the file; the other seven must be denied without changing the winner's
session identity or committed event. The existing trace-file harness is not
modified.
