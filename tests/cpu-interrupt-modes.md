<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned IM 0 and IM 2 acceptance

The hosted acceptance injects RST 38h through an IM 0 acknowledge and checks
the internal phase, stack writes, return address, refresh register, master
ticks, and TIMING_FULL bus/instruction/interrupt records. It also supplies an
IM 2 vector, checks table reads and stack writes in order, and checks the
resolved PC and matching trace events. Together with
[HALT and interrupt acceptance](cpu-halt-interrupt.md), these cover IM 0,
IM 1, IM 2, and NMI entry paths for Task 58.

Hosted proof: [cpu-interrupt-modes.json](../test-results/cpu-interrupt-modes.json).
