<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger monitor execute command

The paused-only `E` command requests execution from the selected CPU PC through
the application's normal resume path. Dispatch preserves the complete CPU
state and master tick. The host runner reports mapped breakpoint stops before
executing the target instruction; the application pauses and refreshes the
debugger view at that stop. The contract checks strict parsing, closed and
running-window gating, register/time preservation, execution, and return to a
later breakpoint.

The source-push workflow builds the full Release/Sokol application and runs the
contract through CTest on all five native targets. The matching driver manifest
pins the contract and production inputs. The latest hosted result is
[debugger-monitor-exit-contract.json](../../test-results/debugger-monitor-exit-contract.json).
