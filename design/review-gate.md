<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# External Review Gate

Status: mandatory before Phase 2.

The project uses an independent external reviewer as an advisory correctness
gate. The developer owns implementation and final technical decisions, while
the harness acts as an evidence court: reviewer suspicion is inexpensive, but
only a positively established `BLOCKER` or `HIGH` defect may block execution.

## Workflow

1. Complete the current change request without executing it on a test system.
2. Commit the candidate and require a clean working tree.
3. Run `CODE` review with the current CR identity, base, head, exact requirement
   sources, and any authorized private scope file.
4. Correct every confirmed serious defect, commit a new immutable candidate,
   and repeat the complete review without a round limit.
5. After `PASS`, complete required documentation review.
6. Publish the exact reviewed commit to `origin/main`.
7. Run approved remote and hosted tests. The SSH orchestrator verifies the
   private receipt, current head, authoritative remote head, and diff hash.
8. Analyze private artifacts and obtain the required `TEST_ARTIFACT` second
   opinion. A resulting code correction reopens the complete CODE gate.

At the start of every CODE attempt, the harness removes any older CODE PASS
receipt. Only a complete current PASS recreates it. Unavailable, failed,
inconclusive, exceptional, and human-decision results leave no authorization.
Immediately before receipt creation, the harness revalidates current `HEAD` and
the clean working tree. Repository mutation during review invalidates the PASS.

## Immutable Evidence

Normal CODE review requires an active CR, an ancestor base, current `HEAD` as
the reviewed head, and no staged, unstaged, or untracked non-ignored changes.
Ignored private artifacts do not dirty the candidate.

The packet is constructed from committed Git objects and contains:

- a NUL-safe deterministic change manifest with status, old/new paths, object
  mode/type, size, identity, and text/binary classification;
- the original 80-line-context diff used by the public snapshot identity;
- complete head content for added, modified, copied, and renamed text files;
- complete base content for deleted text files;
- metadata, but no invented semantics, for binary files, symlinks, and
  gitlinks; and
- a deterministic internal packet-manifest hash.

The public snapshot form remains
`git:<base>..<head>:sha256:<diff-sha256>` for remote-harness compatibility.

## Scope And Authority

CODE review requires `--cr CR-XXXX`. The active record in
`issues/change-requests.json` supplies current scope metadata. A private scope
file inside the project may provide detailed acceptance material when the
public tracker is intentionally concise. Current CR scope is distinct from
future architecture and unrelated backlog.
The exact tracker identity, selected CR-record identity, optional private scope
identity, and a deterministic scope-manifest hash bind the review and receipt
to the current task definition.

Each requirement is an exact record containing project-relative source path,
SHA-256 identity, and original content. Model-generated summaries are never
authority. Every candidate must cite an exact source and quote; the harness
rejects a quote that does not occur in that source before another model call.

## Candidate And Proof Pipeline

One combined discovery pass per review unit discovers candidates, not findings.
For CODE, that single pass must explicitly cover requirements and functional
correctness; runtime, failure paths, safety, hostile input, lifecycle,
ownership, concurrency, and recovery; and integration, regression,
compatibility, and test adequacy. DOCUMENTATION and TEST_ARTIFACT use equivalent
combined lenses. Candidates state current-scope applicability, a concrete
failure scenario, causal path, evidence, assumptions, and bounded context
requests. Missing context is never promoted to HIGH.

The harness then:

1. validates candidate structure, requirement provenance, current paths, and
   duplicates deterministically;
2. resolves bounded exact PATH or SYMBOL evidence from the same immutable head;
3. sends remaining candidates to a high-effort hostile falsifier instructed to
    assume each allegation is false;
4. requires one `CONFIRMED`, `REJECTED`, `NON_BLOCKING`, or `UNRESOLVED`
   decision per ID, with independently confirmed BLOCKER/HIGH severity; and
5. synthesizes blockers in Python from the exact confirmed candidate IDs.

A real issue below HIGH is `NON_BLOCKING`, not falsely labeled `REJECTED`. A
falsifier-discovered suspicion re-enters the same validation and proof path.
No majority vote, grouping helper, or final model may add, drop, resurrect, or
promote an ID. `UNRESOLVED` cannot become HIGH and prevents PASS when material.
A falsifier-rejected or non-blocking allegation is not resurrected merely
because its own requested proof was unavailable. Material unresolved results
include the exact candidate claim, authority, location, decision reason, and
resolved-context record rather than only an opaque ID.

## Disputes And Ambiguity

Prior records use `OPEN`, `RESOLVED`, or `DISPUTED` and retain exact evidence.
They do not bias independent discovery; only matching candidates receive the
relevant evidence during falsification or adjudication.

One structured dispute may provide decisive source evidence. Max-effort
adjudication is exceptional and is bounded to that candidate, exact requirement,
original evidence, falsifier decision, newly acquired context, and dispute.
Conflicting authoritative sources return `HUMAN_DECISION_REQUIRED`; the same
ambiguous evidence is not forced into a model-generated answer.

## Review Types And Artifacts

The default bounded deadline for a CODE, DOCUMENTATION, or TEST_ARTIFACT
review is 3,600 seconds (one hour). A caller may select a shorter explicit
deadline when its immutable packet is known to require less time; expiration
remains fail-closed and never authorizes remote or hosted execution.

`CODE`, `DOCUMENTATION`, and `TEST_ARTIFACT` use the same candidate/proof
discipline. Documentation style and future detail do not block current work.

Artifact inputs are classified before review:

- valid UTF-8 text is reviewed as text with byte/hash identity;
- an approved deterministic extractor may provide a provenance-bound textual
  representation of a structured binary format;
- images are never represented as text or claimed to have been visually
  inspected by the text-only reviewer; and
- unknown binary or visual-only evidence without an approved description is
  `EVIDENCE_INSUFFICIENT` and produces an inconclusive result when semantics
  are required.

Artifact conclusions distinguish product defects, test defects, insufficient
evidence, and interpretation errors.

`TEST_ARTIFACT` reviews use `--evidence-root` rather than a caller-selected
subset when claiming a complete hosted matrix. The gate derives the expected
20 lane IDs from the immutable `platform-smoke.yml`, recursively indexes every
file under the canonical root with path, size, line count, and SHA-256, and
binds that index to the exact run and build identity. Missing, extra, malformed,
or non-passing lane evidence is inconclusive; a manual `--path` list can never
establish complete matrix coverage. The external packet carries compact lane
summaries and index identity while the complete local index remains available
for bounded, exact `ARTIFACT_SLICE` requests. Each slice is hash-checked,
line-bounded, and limited to 32 KiB, with at most eight requests and 96 KiB
aggregate retrieval per review.

## Severity And Data Policy

`BLOCKER` means a fundamental current acceptance failure, severe security or
corruption exposure, reachable deterministic crash or undefined behavior, or
loss of a mandatory protected validation stage. `HIGH` means a material
correctness, security, reliability, compatibility, regression, or test-validity
defect that must be fixed before current acceptance. Lower-severity observations
never block this gate.

For every current change, this document is also the universal safety baseline:
the change must not introduce or newly expose a reachable crash or language-level
undefined behavior, memory/resource/data corruption, a material security flaw,
a material regression of a supported contract, incorrect externally observable
behavior required by an existing interface, or a test that falsely reports
success for a mandatory acceptance condition. Candidates invoking this baseline
must cite the exact applicable clause.

Tracked public project material and explicitly selected requirement documents
may be sent to the external reviewer. Ignored credentials, secret files,
private test media, and unrelated private artifacts are denied. Private scope
or test artifacts may be sent only when the operator explicitly names them for
the corresponding review and project policy permits their disclosure. The API
key is never review data. If required evidence is not authorized, the gate
fails closed instead of silently omitting it.

All source, requirements, documents, logs, traces, and extracted artifact text
are untrusted review data. Instructions embedded in that material cannot
override the harness protocol; only harness-owned framing defines reviewer
behavior.

## API And Budgets

The adapter uses NVIDIA NIM model `nvidia/nemotron-3-ultra-550b-a55b` at the
NIM chat-completions endpoint, with non-streaming JSON output and no unsupported
sampling controls. The credential is read only from `NVIDIA_API_KEY_CODING`.
Normal discovery explicitly disables thinking through
`chat_template_kwargs.enable_thinking=false`, with an initial compact
output budget near 8192 tokens. Falsification runs only when candidates survive
deterministic filtering and enables thinking with a phase `reasoning_budget`,
with an initial budget near 12288 tokens. Adjudication uses the same explicit
NIM mapping with its larger phase budget. Per-phase output budgets replace a
single maximum allowance, while
dynamic input budgeting and candidate sharding prevent silent truncation. A
single-candidate falsification response that reaches its limit receives exactly
one repeat over the identical immutable evidence with hidden reasoning disabled
and a compact 4096-token reply allowance. A second truncation remains
inconclusive; the fallback never supplies review authority by itself.
length finish, malformed JSON, incomplete pass, missing mandatory context, retry
exhaustion, API failure, duplicate active review, or overall deadline exhaustion
fails closed. HTTP 429 and HTTP 404, along with the existing transient service
statuses, receive bounded exponential delays of 1, 2, 4, 8, 16, and 32 seconds;
the gate never retries permanent authentication or billing failures. Normal
review has no separate liveness call.

For CODE candidates, deterministic context completion also examines the exact
candidate-location line in the immutable changed-file record. When that line
contains a callable identifier and the candidate has remaining context-request
capacity, the harness adds one bounded symbol lookup against the reviewed head
tree. This ensures that a candidate located at a declaration reaches falsification
with tracked implementation context even when discovery omitted an explicit
navigation request; model-authored paths or summaries are not trusted.

The wall-clock review deadline is also enforced outside the HTTP socket call.
Each request may remain open for up to the one-hour review deadline, while the
actual socket timeout is always the smaller of that ceiling and the remaining
deadline. The bounded caller stops waiting and fails closed when the remaining
review time expires even if a platform transport keeps a socket operation alive
beyond its requested timeout; a stalled transport can therefore never extend
gate authority.

Stable scope, requirement, and immutable evidence prefixes are ordered before
pass-specific instructions for cache reuse. The review-unit budget is calculated
after the stable prefix and fails closed as `REQUIREMENT_SCOPE_TOO_BROAD` when
the selected requirements leave too little useful source-evidence space. A
one-unit multi-file CODE packet performs per-file and cross-file analysis inside
the combined discovery pass; a separate integration discovery call is used only
when genuine multi-unit splitting requires it.

Telemetry records every API call's phase, thinking setting, reasoning effort,
input bytes, tokens, cache usage, elapsed time, retry index, and result class,
plus review-level call count, discovery units, integration need, candidate
count, falsification batches, adjudications, final verdict, snapshot, manifests,
and elapsed time. For indexed test evidence it additionally records the local
byte universe, indexed-file count, compact packet size, anomaly count, result
groups, and bounded slice retrieval totals. It never records the key,
authorization data, prompts, source, or hidden reasoning. A private
active-review status file prevents duplicate normal reviews of the same
snapshot while safely recovering stale locks.

## Commands

```text
python tools/reviewer/review_gate.py review --type CODE --cr CR-0020 --base <base> --head <head> --requirements design/review-gate.md --scope-file test-artefacts/reviewer/requirements/CR-0020-review-harness-hardening.local.txt --review-map test-artefacts/reviewer/requirements/CR-0020-review-map.json
python tools/reviewer/review_gate.py review --type DOCUMENTATION --requirements design/review-gate.md --path <document> --review-map test-artefacts/reviewer/requirements/CR-0020-review-map.json
python tools/reviewer/review_gate.py review --type TEST_ARTIFACT --requirements design/review-gate.md --run-id <test-run-id> --build-id <build-identity> --evidence-root test-artefacts/github/CR####
python tools/reviewer/review_gate.py health-check --requirements design/review-gate.md --deadline-seconds 60
```

### Requirement-linked review packets

Every normal CODE and DOCUMENTATION review must use `--review-map`. The map is a
small JSON file containing one or more links. Each link names a minimal but
complete requirement excerpt (`requirement.source`, `start`, `end`, and the
full-source `sha256`) and the exact related file and line range (`related.path`,
`start`, `end`, and the current-file `sha256`). CODE links are read from the
immutable HEAD object. A link with a `prior` range must also provide the
base-object hash and receives the exact bounded file diff. Documentation links
use the same structure and may provide `prior` plus `--base` and `--head` when a
previous tracked document section exists.

The packet contract is **Minimal BUT Sufficient+Accurate**. For every link, the
requirement excerpt must state the complete acceptance intent for that link,
and the related excerpt must contain the exact implementation or document
section that can satisfy or violate it. The map author must not use a generic
file excerpt when a narrower symbol or section is sufficient, and must not
omit neighboring control logic needed to understand the stated behavior. The
link ID, requirement source/range, related path/range, and optional prior
range are one immutable correspondence; they are not independent snippets.

The emitted record carries separate full-source hashes and excerpt hashes for
the requirement, current related content, and optional prior content. When a
prior exists, the record also carries a hash of the bounded diff and a binding
to the exact path, base object, head object, and base/head ranges used to
produce it. The gate recomputes all of these values from the selected Git
objects before transmission. A stale hash, invalid range, missing required
prior diff, path mismatch, empty bounded diff, or packet field that does not
match its map is rejected before any model call. This makes the requirement,
code/document excerpt, and diff internally consistent rather than merely
individually plausible.

The transmitted packet contains only the linked requirement excerpt, related
excerpt, and applicable prior excerpt/diff plus the integrity metadata above.
Missing, stale, broad, or malformed maps fail closed; complete requirement
files and complete changed-file packets are not valid substitutes for a
normal review map. If a reviewer returns `CODE-DISCOVERY OutputError`, the
first recovery action is to inspect and regenerate the packet map and verify
these bindings; retrying an unchanged inconsistent packet is not a valid
recovery.

The retained bootstrap gate is an architecturally independent one-pass audit
path for maintenance of the normal gate. It consumes the complete bounded
immutable packet and remains available after cutover. Its full prompt has a
separate conservative 2 MB input ceiling so a complete gate-maintenance change
is not silently reduced to the normal per-unit budget. It uses the same
non-thinking, 8,192-token bounded discovery mode as normal candidate discovery:
the bootstrap contract requires one complete independently auditable verdict,
not hidden-reasoning output that can consume the response allowance before a
result is emitted. A larger reserve can delay service scheduling without adding
review authority.
Transport-successful but schema-invalid bootstrap output receives at most two
fresh repair attempts and then remains inconclusive.

Bootstrap maintenance may authorize remote validation only through an exact
`wzsn/maintenance/CR-####` published ref, an immutable matching bootstrap PASS
receipt, and an unchanged `design/review-gate.md` authority digest.
This path is unavailable to normal smoke runs and never authorizes `main`.

## Verdict Contract

- `PASS`: complete review, no confirmed serious defect, and no material
  unresolved acceptance issue.
- `FAIL`: complete review and at least one independently confirmed serious
  defect with the complete evidence contract.
- `INCONCLUSIVE`: incomplete proof, context, or evidence; never authorization.
- `REVIEW_UNAVAILABLE`: infrastructure or configuration prevented review.
- `HUMAN_DECISION_REQUIRED`: exact authoritative intent conflicts or cannot be
  decided from immutable evidence.

Corrected snapshots always receive another complete review. Later valid
serious defects remain allowed, and no arbitrary maximum number of rounds or
confirmed findings exists.

## Protocol v4 authoritative correction â€” zero-gap gate remediation

This section is authoritative for review protocol v4 and supersedes any conflicting earlier packet, reasoning, lock, hosted-run, bootstrap, or acceptance wording in this document.

- Every mandatory CODE, DOCUMENTATION, and TEST_ARTIFACT review is bound to one active CR and its `Status: APPROVED_FOR_IMPLEMENTATION` preflight. The preflight records `Review-Base: <commit-ish>`; normally this is a stable tag created at the committed preflight baseline, so no self-referential future SHA is required. Gate/harness/governance surgery additionally records `Operator-Authorized-Gate-Change: YES`.
- CODE `--base` must resolve exactly to the active CR Review-Base. The complete Git change universe (including delete/rename/copy/non-text/mode/type identity) is deterministically bound locally. The review map must cover every changed hunk (including deletion/rename/copy sides and mode/type/non-text changes) and every explicit/CR authority source; a map is a semantic index, not a replacement for the full universe.
- CODE keeps the public snapshot form `git:<base>..<head>:sha256:<canonical-80-line-diff-sha256>` for protected-execution compatibility. Packet/review-map identities are additional independent receipt fields.
- Requirement excerpts have stable identities containing source path, full-source SHA-256, exact line range and excerpt SHA-256. Distinct excerpts from one source never collapse into one requirement identity. Receipts separately bind unique full authority sources and exact excerpt identities.
- Candidate locations always use original immutable source `path:line`, never synthetic packet-record coordinates. Deletion/rename/copy context preserves base_path/head_path separately. Acceptance-relevant non-text semantics require an approved deterministic text representation or the review is INCONCLUSIVE.
- A clean linked semantic unit is never arbitrary character-sharded. If a semantic unit cannot fit, regenerate a narrower source/symbol/section map or fail closed. Split CODE discovery requires a real cross-unit integration review.
- Discovery must explicitly return its phase and `review_complete:true`. Material `uncertainties` or `evidence_requests` cannot be ignored. Malformed serious candidates, invalid proof provenance, or missing evidence fail closed rather than disappearing into PASS.
- Hostile falsification receives only candidate-relevant, exact hash-bound evidence and uses structured `proof_refs`; Python validates decision/evidence algebra. There is no non-thinking semantic authority fallback.
- Reviewer reasoning profile is provider-bound by `tools/reviewer/review-profile-v4.json`: health/pure formatting `none`; CODE/DOCUMENTATION/TEST discovery `medium`; integration/falsification/adjudication `high`. `reasoning_budget` is strictly below `max_tokens`. HTTP 202 is polled by requestId; transport retry and schema/semantic failure are separate.
- One repository-global atomic review lock prevents parallel packet review. A live process is not aged out by a fixed 15-minute timer; the lease covers the requested deadline plus grace.
- DOCUMENTATION is bound to the exact CODE-reviewed implementation commit and exact requested document set. Review-map `related.role=document` identifies reviewed documents and `related.role=context` supplies immutable implementation/support context without expanding the reviewed document set. Filesystem/private inputs are hash-bound and revalidated before PASS.
- TEST_ARTIFACT consumes signed publication provenance and one canonical lane-evidence schema. Its pinned Fuse unresolved baseline is read from the tested commit Git object, never from a later working tree. Exactly one validated embedded lane manifest is required for each of the exact 20 hosted lanes. Retry artifact names are transport metadata only. Structured source hashes, Fuse invariants, Sokol evidence, artifact classification and bounded retrieval are deterministic prerequisites.
- Protected hosted execution starts only from `workflow_dispatch` after verification of a short-lived HMAC-signed CODE-PASS authorization for the exact commit/profile. No push/PR event runs the protected product matrix. All 20 lane IDs/labels remain mandatory and unchanged. No live run may be cancelled/replaced. Local-lab evidence supplements but never substitutes for hosted publication acceptance.
- Normal CODE review refuses protected gate/governance changes with `BOOTSTRAP_REQUIRED`. The standalone bootstrap imports no normal-gate packet/client/schema/decision code. Bootstrap surgery itself requires the documented one-time root-of-trust record and independent review of complete unscoped change bytes.
- Protocols below v4 do not authorize protected execution. PASS receipts bind review profile, CR/preflight/scope, canonical snapshot, review-map, unique full requirement sources and excerpt identities.

- Successfully completed inference that fails the application schema is fail-closed. Schema normalization may repair syntax/shape only if semantics are frozen; protocol v4 does not launch an unconstrained second semantic reviewer under the label of formatting repair.
