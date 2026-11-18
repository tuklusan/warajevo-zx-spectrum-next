<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger window live-machine contract

Hosted regression for the production debugger-window component. It opens the
window against one initialized machine and verifies snapshot data comes from
that exact shared instance, running mode rejects memory writes and stepping,
paused mode writes to the live machine, and closing clears the window's machine
reference.

It also exercises the production monitor parser with `R SETREG <register>
<hex-value>`, including mixed case, surrounding whitespace, primary and
alternate registers, invalid commands, width rejection, pause gating, and
register undo.
