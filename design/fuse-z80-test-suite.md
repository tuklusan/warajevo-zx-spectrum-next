<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Fuse Z80 Test Suite

This document pins the external Fuse Z80 unit-test source required by the core
architecture. The repository does not copy the external suite into public
source control.

## Pinned identity

```text
Repository: https://git.code.sf.net/p/fuse-emulator/fuse
Reference: refs/heads/master
Commit: e7fe9a0f3625b0aef649b114fc98323c7d241840
Suite path: z80/tests/
```

The pinned source commit was resolved on 2026-08-11 and its tree was verified
again on 2026-08-20. The suite includes the Fuse Z80 core test vectors under
`z80/tests/`. The commit identity,
not a moving download URL, is the certification input.

## Acquisition

Remote test machines or hosted runners may acquire the suite into a private,
run-local directory under `test-artefacts/`, for example:

```text
git clone --filter=blob:none https://git.code.sf.net/p/fuse-emulator/fuse fuse-source
git -C fuse-source checkout e7fe9a0f3625b0aef649b114fc98323c7d241840
```

The runner must verify `git -C fuse-source rev-parse HEAD` equals the pinned
commit before reporting a suite result. The fetched source and generated test
outputs are test artefacts, not project source and must not be committed.

## Integration policy

Fuse vectors certify CPU semantics using an unloaded, writable fixture image.
The Fuse fixture runner explicitly disables hardware ULA I/O decoding so its
synthetic I/O provider can supply vector-defined values; ordinary machine
execution keeps hardware ULA decoding enabled by default.
They do not certify the 48K hardware memory map: project-owned ROM-map tests
must install an external ROM image and independently verify that emulated
writes to `0000-3fff` are rejected while `4000-ffff` remains writable.

The eventual `wz_tests` integration must:

- report the pinned commit in its machine-readable test manifest;
- distinguish passed, failed, not-applicable, and reviewed-waived cases;
- never silently skip a case because the current CPU implementation is
  incomplete;
- preserve the external suite's applicable copyright and license notices;
- run only through approved remote or hosted test machines.

This pin authorizes acquisition and integration work. It does not claim that
the WZSN CPU currently passes the suite.
