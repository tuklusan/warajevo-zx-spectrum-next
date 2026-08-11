<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# External Review Gate

Status: mandatory before Phase 2.

The project uses an independent DeepSeek reviewer as an advisory correctness
gate. Project ownership and final technical decisions remain with the project
developer, but every BLOCKER or HIGH finding must be either corrected or
recorded as disputed with precise authoritative evidence before a complete new
review of the corrected snapshot.

## Workflow

1. Complete a CR implementation without running it on designated test systems.
2. Commit the immutable candidate snapshot locally.
3. Run one `CODE` review invocation over the complete CR range.
4. Batch-correct all valid serious findings and repeat the complete review.
5. After `PASS`, run required documentation review and then remote/hosted tests.
6. Analyze the private artifacts and obtain a `TEST_ARTIFACT` second opinion.
7. Any resulting code correction invalidates the earlier code pass.

The SSH smoke orchestrator enforces a private CODE PASS receipt bound to the
current commit. Receipts, telemetry, raw artifacts, and secrets remain under
`test-artefacts/` and are not published.

## Review Architecture

Each normal invocation uses three independent specialist passes followed by an
adversarial consolidation. Adjudication is added only for material ambiguity.
The supported review types are `CODE`, `DOCUMENTATION`, and `TEST_ARTIFACT`.
All passes use one immutable snapshot, report only BLOCKER/HIGH findings, and
return one compact consolidated result to the caller.

Large inputs are partitioned deterministically and every partition is reviewed.
No failed, malformed, incomplete, truncated, unavailable, or inconclusive API
operation can produce PASS. The runtime key is read only from the exact
case-sensitive environment variable `DeepSeek_API_key` and is never persisted.

## Commands

```text
python tools/reviewer/deepseek_gate.py review --type CODE --base <base> --head <head> --requirements design/deepseek-review-gate.md
python tools/reviewer/deepseek_gate.py review --type DOCUMENTATION --requirements design/deepseek-review-gate.md --path <document>
python tools/reviewer/deepseek_gate.py review --type TEST_ARTIFACT --requirements design/deepseek-review-gate.md --path <artifact>
python tools/reviewer/deepseek_gate.py health-check --requirements design/deepseek-review-gate.md
```

The retained `legacy_bootstrap_gate.py` exists only to independently audit
changes to the normal review implementation and to preserve the initial
bootstrap evidence. It is not the normal review path.

## Acceptance Contract

The gate must preserve all three review types and execute every mandatory
specialist even when an earlier pass reports defects. Specialist responses are
schema-validated JSON, continue after the first defect, perform a private
self-challenge, and contain no hidden reasoning. Consolidation rechecks all
findings against the immutable material, groups root causes, removes only
unsupported or duplicate findings, and cannot majority-vote away a unique
valid finding. Material uncertainty invokes adjudication; ordinary clear
results do not.

Normal review makes no separate model liveness request. The first substantive
request establishes availability. Transient transport/service failures and
empty or malformed outputs receive bounded internal retries; permanent
configuration/authentication failures, retry exhaustion, truncation,
incomplete passes, invalid schemas, and missing context fail closed. A manual
health check remains separate.

The centralized adapter uses the configured API endpoint, `deepseek-v4-pro`,
thinking enabled, high reasoning effort, non-streaming JSON output, and a large
output allowance without unsupported sampling controls. Cache hit/miss usage,
calls, retries, pass names, adjudication, token usage, verdict, finding count,
snapshot, and duration are recorded in bounded private telemetry without
prompts, source, reasoning, authorization data, or the key.

Code snapshots bind base commit, head commit, and diff hash. Documentation and
artifact snapshots bind sorted paths and SHA-256 hashes. Every internal pass
uses identical snapshot material. Oversized material is detected and reviewed
in deterministic shards, followed by one global consolidation. No material is
silently truncated and callers see only one compact final result.

Every corrected snapshot receives a complete new review. Prior findings may be
recorded as resolved, open, or disputed with evidence, and later rounds remain
free to identify new serious defects. There is no finding or review-round cap.
Any code correction after PASS invalidates the code gate; material document
changes invalidate documentation review; new test runs create new artifact
snapshots. A serious post-test code finding reopens the complete workflow.

Remote execution is blocked for FAIL, INCONCLUSIVE, REVIEW_UNAVAILABLE, stale
receipts, and missing receipts. PASS permits the next required stage but does
not transfer ownership of the code or final disposition decisions away from
the developer. Controlled fixtures must cover configuration, request shape,
retry/fail-closed behavior, schemas, all review types, consolidation,
adjudication, snapshot identity, subsequent rounds, sharding, compact output,
gate integration, telemetry, and secret non-disclosure.
