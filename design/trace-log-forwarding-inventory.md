<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Trace and Log Forwarding Inventory

This inventory is the CR-0253 upstream-comparison and implementation gate. It
separates deterministic producers from host presentation and diagnostic output;
it does not claim that legacy text labels are machine-state behavior.

## Producer Inventory

| Producer or path | Current role | CR-0253 disposition |
|---|---|---|
| `src/core/wz_trace.c` and `src/core/wz_trace.h` | Structured canonical trace sink and sequence assignment | Forward approved structured trace events through the shared host bridge; preserve caller-visible sequencing |
| `src/core/wz_machine.c`, `src/core/wz_bus.c`, `src/core/wz_z80.c`, and `src/core/wz_debugger.c` | Emit structured machine, bus, CPU, interrupt, and debugger events | No producer-local sockets; retain the narrow sink boundary |
| `src/diagnostics/wz_trace_file.c` | Private bounded binary circular trace file | Local-only; never forward full binary records or sensitive state |
| `src/diagnostics/wz_trace_dump_main.c` | Host tool text output and recovery errors | Local-only command-line output; no direct network path |
| `src/diagnostics/wz_syslog_udp.c` | Existing bounded UDP Unix syslog transport | Sole network transport; retain fixed destination and best-effort failure behavior |
| `src/diagnostics/wz_diagnostics_router.c` | Shared trace fan-out and application-log forwarding boundary | Sole approved route from product diagnostics to the transport; local sink remains private |
| `src/app/wz_headless_main.c` | Application bootstrap output | Route application diagnostics through the shared host diagnostic boundary when enabled |
| Test and harness `stdout`/`stderr` | Test protocol, assertions, and harness control output | Local/CI evidence only; never forwarded by product code |
| Preserved upstream `SPECSIM.ASM` error/status strings | Legacy host and configuration diagnostics | Compatibility reference only; map only approved application diagnostics, never raw source text or host paths |

## Required Routing Invariants

- Every product trace or application diagnostic selected for forwarding has one
  path to `wz_syslog_udp`; no producer opens a network socket.
- The private binary trace, test protocol output, screenshots, host paths,
  machine memory, ROM content, credentials, and personal data are excluded.
- UDP destination is fixed at `sanyalnet-oracle-vps2.duckdns.org:65514`.
- Forwarding is opt-in, bounded, non-blocking, and observational; DNS or send
  failure cannot alter canonical state or stop emulation.
- Tests must prove routing and exclusion, not merely format one isolated packet.
- The router must preserve the trace sink's sequence assignment and invoke the
  local sink even when forwarding is disabled or the network transport fails.

## Upstream Zero-Gap Result

The preserved trace/debug references were scanned across `SPECSIM.ASM`,
`SPECINT.ASM`, `SPECMON.ASM`, and the named architecture trace sections. No
additional structured trace producer or approved application-log channel was
found beyond the paths listed above. Legacy text output remains explicitly
classified as host-local or mapped through the new boundary; it is not silently
treated as a second transport.

## Harness Diagnostics

The approved harness producer scope is the complete hosted lane result tree
under `.wzsn-harness/github/<lane-id>` and each local remote session tree under
`test-artefacts/remote-runs/<machine>/<run-id>`. The shared
`tools/harness/forward_syslog.py` adapter forwards their logs, traces,
screenshots, and raw test artifacts as bounded base64 RFC 5424-style records
over the same fixed UDP endpoint. Transport credentials are not payload data.
Forwarding is best effort and never changes the build, test, or publication
result.

Forwarding is disabled by default and activates only when
`WZ_TRACE_FORWARD` is exactly `Y` or `1`; validation environments set it
explicitly.

The bounded circular product trace file uses
`wz_trace_file_set_forwarder` to invoke the same forwarding boundary after
each successful record and header commit; it does not wait for freeze or
shutdown.
