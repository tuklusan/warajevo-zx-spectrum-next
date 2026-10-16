<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned 16-bit flag edge vectors

The hosted test crosses 15 values around nibble, 12-bit half-carry, sign,
and full-carry boundaries, with both carry inputs. It checks ADD HL,BC,
ADC HL,BC, SBC HL,BC, ADD IX,BC, and ADD IY,BC: 2,250 cases. An independent
arithmetic reference computes carry, half carry, signed overflow, zero,
sign, preserved flags, and undocumented X/Y bits. Each case also checks the
result register, WZ/memptr, PC, and T-state length.

Hosted proof: [cpu-wide-flags.json](../test-results/cpu-wide-flags.json).
