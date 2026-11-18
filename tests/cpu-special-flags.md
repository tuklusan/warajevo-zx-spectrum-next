<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned special-flag instruction vectors

The hosted acceptance checks DAA after addition and subtraction, CPL,
SCF/CCF with previous Q clear, SCF/CCF after CP sets Q, and SCF after a
following NOP clears Q. Each vector checks A, F, PC, refresh R, and cycles.
The Q-dependent X/Y expectations follow the pinned Fuse implementation.

Hosted proof: [cpu-special-flags.json](../test-results/cpu-special-flags.json).
