<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Networking dirty Microdrive resolution contract

The hosted contract links the production Networking command, UI radio-group
dispatch, Microdrive eject resolver, and core transport. It passed 42
assertions on Linux x64 in run `39388401909`, at source
`3cdee4d491d0c3d5099f51639191bc72531695cd`; proof is pinned in
[`networking-dirty-media.json`](../test-results/networking-dirty-media.json).

With dirty Interface-1 media, the Networking command is disabled with the
stable local-resolution reason, and attempted mode changes leave the mode,
mounted image, and dirty bytes intact. Cancel and failed-save outcomes also
preserve the dirty media and keep the mode change disabled. Successful Save
persists the sector before eject and then permits leaving Interface-1;
explicit Discard leaves backing bytes unchanged and then permits the mode
change. The existing networking, reconfiguration, and Microdrive UI contracts
were not modified.
