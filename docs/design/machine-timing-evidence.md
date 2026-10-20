<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Machine timing and ROM evidence

This record freezes the nominal PAL timing profiles for the required 48K and
base 128K machines. It is an Architecture #1/#2 record; the separate
network/router architecture is outside its scope.

## Authority and limits

The machine profile and event-order contract are defined by
[Architecture #1, Sections 9.2 and 10.4](01-warajevo-zx-spectrum-next-architecture.md),
and the detailed device contract is in
[the T-state description](../../reference/t-state-description.md). The profile
values below follow the cited [48K technical reference](https://worldofspectrum.org/faq/reference/48kreference.htm)
and [128K technical reference](https://worldofspectrum.org/faq/reference/128kreference.htm).
The references describe reported hardware behavior; no physical Spectrum or
audio capture is available to this project. The pinned Fuse emulator is the
reproducible software comparison source, not a claim of physical-hardware
certification. The 48K source, 128K contention, and master-time contracts are
tracked in `test-results/`.

The 48K reference reports timing variants that differ by one T-state on some
machines. The supported profiles freeze the nominal values in this file; other
ULA revisions are not certified by that choice.

## Frozen nominal clock and frame values

| Profile | CPU frequency | Master frequency | Master ticks / CPU T | T / line | Lines / frame | T / frame | Frame rate |
|---|---:|---:|---:|---:|---:|---:|---:|
| ZX Spectrum 48K PAL | 3,500,000 Hz | 7,000,000 Hz | 2 | 224 | 312 | 69,888 | 50.0801 Hz |
| ZX Spectrum 128K PAL | 3,546,900 Hz | 7,093,800 Hz | 2 | 228 | 311 | 70,908 | 50.0100 Hz |

The rate is derived from the profile's integer frequency and frame length; it
does not schedule machine events. `master_tick` remains the sole scheduler
timestamp. The expected master-tick spans are 139,776 per 48K frame and 141,816
per 128K frame.

## Raster, interrupt, contention, and floating bus

The nominal profiles freeze these timing inputs:

| Quantity | 48K PAL | 128K PAL | Evidence |
|---|---:|---:|---|
| Raster positions / line | 448 | 456 | Two raster positions per CPU T-state; profile geometry |
| Interrupt assertion T-state | 0 | 0 | ULA-synchronized frame interrupt; profile contract |
| Interrupt deassertion T-state | 32 | 32 | Frozen profile pulse window |
| First ULA fetch / contention T-state | 14,335 | 14,361 | 48K and 128K technical references |
| ULA fetch lines | 192 | 192 | Active bitmap region |
| Fetch window / line | 128 T | 128 T | 32 fetches at four-T intervals |
| Attribute-fetch offset | 1 T | 1 T | Profile value and fetch contract |
| Contention delay sequence | 6,5,4,3,2,1,0,0 | 6,5,4,3,2,1,0,0 | 48K/128K references |
| Frame T-states | 69,888 | 70,908 | Technical references |

For the 48K profile, lower 16K RAM and applicable I/O operations contend only
during ULA fetch windows. The 128K profile applies contention to RAM banks 1,
3, 5, and 7, with its first fetch shifted to T-state 14,361 and a 228-T line.
Reads from floating I/O space observe ULA screen/attribute data while that
data is present on the bus; otherwise the idle bus value is used. The bus
contract and behavior are described in the technical references and
`reference/t-state-description.md` Sections 4–6.

For events sharing a master tick, Architecture #1 Section 10.4 requires an
explicit profile ordering rule and forbids C statement order from choosing
visibility. A profile-specific, evidence-backed event-order table is not yet
recorded. This remains an open Phase-0/acceptance item; source inspection and
general raster tests do not close it.

The current proof set includes the canonical core regression, master-time
contract, contention contract, and Fuse visual comparison. The latter is a
software reference for the visible 48K screen only. It does not certify analog
output, ULA-revision timing variants, or the physical 128K raster.

## ROM identity baselines

Hashes identify the approved development/test ROM bytes without embedding them
in a new public proof or final application package. Hashes do not grant
redistribution rights.

| Profile / ROM bank | Bytes | SHA-256 | Runtime FNV-1a 64 identity | Source |
|---|---:|---|---|
| 48K ROM | 16,384 | `d55daa439b673b0e3f5897f99ac37ecb45f974d1862b4dadb85dec34af99cb42` | `0x0a927c36965e6e82` | `roms/48.rom`; matches Fuse revision `7edc2418ce11837d9ede799f7e0445440beead1d` |
| 128K ROM 0 | 16,384 | `3ba308f23b9471d13d9ba30c23030059a9ce5d4b317b85b86274b132651d1425` | `0x167a39561d9acbe6` | Fuse revision `7edc2418ce11837d9ede799f7e0445440beead1d`, `roms/128-0.rom` |
| 128K ROM 1 | 16,384 | `8d93c3342321e9d1e51d60afcd7d15f6a7afd978c231b43435a7c0757c60b9a3` | `0x42501740662a4075` | Fuse revision `7edc2418ce11837d9ede799f7e0445440beead1d`, `roms/128-1.rom` |

The ROM distribution notice is `roms/README.copyright`; final public packages
remain ROM-free. The production loader verifies two external 16K images as a
pair and selects the visible bank using the 128K paging register. ROM bytes
remain user supplied and are never copied into proof or package files. Local
machine states record firmware identities and require the same external ROM
pair to be loaded before restoring that state; hosted proof uses synthetic
bank images and publishes no ROM bytes.
