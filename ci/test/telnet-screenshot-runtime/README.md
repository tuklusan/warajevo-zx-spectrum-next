<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Telnet screenshot runtime contract

The hosted test launches the production Release/Sokol host and exercises the
real Control Port. It checks the default OS temporary directory, absolute
timestamped filenames, successful PNG completion, unchanged running and paused
status, repeatable paused raster bytes, and an error without a path when output
cannot be created. Both application processes are terminated by the test.
