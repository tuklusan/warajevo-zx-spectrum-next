<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Timing-trace behavior contract

This hosted regression compares identical 48K PAL machines with tracing off,
enabled, and toggled. It requires identical canonical state hashes, verifies
enabling/disabling, ring wrap, and freeze do not mutate state, confirms a
trace-file creation error does not mutate or stop the machine, and compares the
ordered event digest from two independent runs. Together with the file
contract, it covers trace isolation, ordering, wrap, freeze, and recovery
without putting trace contents in the proof artifact.
