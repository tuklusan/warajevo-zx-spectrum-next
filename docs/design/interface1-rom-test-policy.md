<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Interface 1 ROM test and distribution policy

This policy resolves Task 201 without making an unverified rights claim.

## Distribution

- Do not check Interface 1 ROM bytes into the repository or include them in
  public source, test, build, or release artifacts.
- Runtime firmware is supplied by the user. The application must not silently
  substitute or fetch firmware.
- Do not embed or redistribute Interface 1 firmware unless written rights for
  the intended distribution have been established and recorded.

## Automated tests

- Ordinary automated tests use deterministic synthetic 8192-byte fixtures for
  size validation, identity plumbing, paging, state preservation, and failure
  atomicity. Such tests do not claim authentic firmware compatibility.
- Tests that execute an authentic ROM require an operator-provided OLD or NEW
  image outside the repository tree. Before execution, validate its exact
  length, declared revision, and SHA-256 against the test invocation's pinned
  metadata.
- Never upload ROM bytes in workflow artifacts or logs. Proof may retain the
  revision, digest, runner, tested commit, and results.
- If the required authentic image is unavailable, the real-ROM test is
  unavailable; it must not be reported as passing.

## Release

Public release artifacts exclude Interface 1 ROM bytes. Any future change to
that rule requires recorded redistribution rights before packaging changes.
