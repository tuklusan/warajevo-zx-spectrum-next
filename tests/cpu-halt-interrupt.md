<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned HALT and interrupt acceptance

The hosted acceptance checks repeated HALT M1 fetches and refresh increments,
then IM 1 wake-up with exact acknowledge and stack-write cycles. It separately
checks NMI wake-up, stack order, IFF transfer, and the five-T-state internal
phase. An EI/DI sequence checks the one-instruction EI delay, rejected early
acceptance, and later IM 1 acceptance. Each case checks CPU state, master ticks,
bus cycles, and corresponding TIMING_FULL events.

Hosted proof: [cpu-halt-interrupt.json](../test-results/cpu-halt-interrupt.json).
