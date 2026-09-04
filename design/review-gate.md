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
The bounded caller stops waiting and fails closed when the remaining review time
expires even if a platform transport keeps a socket operation alive beyond its
requested timeout; a stalled transport can therefore never extend gate authority.

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
and elapsed time. It never records the key, authorization data, prompts, source,
or hidden reasoning. A private active-review status file prevents duplicate
normal reviews of the same snapshot while safely recovering stale locks.

## Commands

```text
python tools/reviewer/review_gate.py review --type CODE --cr CR-0020 --base <base> --head <head> --requirements design/review-gate.md --scope-file test-artefacts/reviewer/requirements/CR-0020-review-harness-hardening.local.txt
python tools/reviewer/review_gate.py review --type DOCUMENTATION --requirements design/review-gate.md --path <document>
python tools/reviewer/review_gate.py review --type TEST_ARTIFACT --requirements design/review-gate.md --run-id <test-run-id> --build-id <build-identity> --path <artifact>
python tools/reviewer/review_gate.py health-check --requirements design/review-gate.md --deadline-seconds 60
```

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
