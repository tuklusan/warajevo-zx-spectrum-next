#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

"""Immutable, batched external-review gate with compact caller output."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import ssl
import subprocess
import sys
import time
import urllib.error
import urllib.request
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

API_URL = "https://api.deepseek.com/chat/completions"
MODEL = "deepseek-v4-pro"
KEY_NAME = "DeepSeek_API_key"
MAX_OUTPUT_TOKENS = 16384
SHARD_BYTES = 280_000
RETRY_DELAYS = (1, 3)
VERDICTS = {"PASS", "FAIL", "INCONCLUSIVE", "REVIEW_UNAVAILABLE"}
SEVERITIES = {"BLOCKER", "HIGH"}

NOTICE = [
    "Warajevo ZX Spectrum Next",
    "Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.",
    "New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.",
    "Upstream Warajevo and third-party material retain their applicable copyrights and licenses.",
    "See LICENSE.txt and NOTICE.md for complete terms and provenance.",
]

SPECIALISTS = {
    "CODE": (
        ("CODE-A", "requirements and functional correctness"),
        ("CODE-B", "runtime, failure paths, safety, hostile input, and recovery"),
        ("CODE-C", "integration, regression, compatibility, and test adequacy"),
    ),
    "DOCUMENTATION": (
        ("DOCUMENTATION-A", "technical and factual correctness"),
        ("DOCUMENTATION-B", "consistency and completeness"),
        ("DOCUMENTATION-C", "implementation and test readiness"),
    ),
    "TEST_ARTIFACT": (
        ("TEST-A", "direct evidence and explicit failure signals"),
        ("TEST-B", "hidden, masked, contradictory, or misinterpreted signals"),
        ("TEST-C", "acceptance-criteria correlation and proof sufficiency"),
    ),
}


class ReviewError(Exception):
    pass


class ConfigurationError(ReviewError):
    pass


class OutputError(ReviewError):
    pass


@dataclass
class Telemetry:
    review_type: str
    snapshot_id: str
    started: float = field(default_factory=time.monotonic)
    calls: int = 0
    retries: int = 0
    prompt_tokens: int = 0
    completion_tokens: int = 0
    cache_hit_tokens: int = 0
    cache_miss_tokens: int = 0
    passes: list[str] = field(default_factory=list)
    adjudication: bool = False
    api_status: str = "pending"


def canonical_json(value: Any) -> str:
    return json.dumps(value, ensure_ascii=True, separators=(",", ":"), sort_keys=True)


def run_git(root: Path, *args: str) -> str:
    try:
        result = subprocess.run(["git", *args], cwd=root, check=False, capture_output=True, text=True)
    except OSError as exc:
        raise ReviewError(f"git is unavailable: {type(exc).__name__}") from exc
    if result.returncode != 0:
        raise ReviewError(result.stderr.strip() or "git operation failed")
    return result.stdout


def resolve_inside(root: Path, value: str) -> Path:
    path = (root / value).resolve() if not Path(value).is_absolute() else Path(value).resolve()
    try:
        path.relative_to(root)
    except ValueError as exc:
        raise ReviewError(f"review path must be inside project: {value}") from exc
    if not path.is_file():
        raise ReviewError(f"review path is not a file: {value}")
    return path


def file_packet(root: Path, paths: list[str]) -> tuple[str, list[tuple[str, str]]]:
    records: list[tuple[str, str]] = []
    identities: list[dict[str, str]] = []
    for value in sorted(set(paths)):
        path = resolve_inside(root, value)
        data = path.read_bytes()
        relative = path.relative_to(root).as_posix()
        digest = hashlib.sha256(data).hexdigest()
        identities.append({"path": relative, "sha256": digest})
        records.append((relative, data.decode("utf-8", errors="replace")))
    identity = hashlib.sha256(canonical_json(identities).encode()).hexdigest()
    return f"files:{identity}", records


def code_packet(root: Path, base: str, head: str) -> tuple[str, list[tuple[str, str]]]:
    base_sha = run_git(root, "rev-parse", f"{base}^{{commit}}").strip()
    head_sha = run_git(root, "rev-parse", f"{head}^{{commit}}").strip()
    diff = run_git(root, "diff", "--no-ext-diff", "--unified=80", base_sha, head_sha)
    if not diff:
        raise ReviewError("code review range has no changes")
    digest = hashlib.sha256(diff.encode()).hexdigest()
    return f"git:{base_sha}..{head_sha}:sha256:{digest}", [("git-diff.patch", diff)]


def shard_records(records: list[tuple[str, str]]) -> list[str]:
    shards: list[str] = []
    current = ""
    for path, content in records:
        block = f"\n===== {path} =====\n{content}\n"
        if len(block.encode()) > SHARD_BYTES:
            if current:
                shards.append(current)
                current = ""
            remainder = content
            part = 1
            while remainder:
                header = f"\n===== {path} (part {part}) =====\n"
                capacity = SHARD_BYTES - len(header.encode()) - 1
                low, high = 1, len(remainder)
                while low < high:
                    middle = (low + high + 1) // 2
                    if len(remainder[:middle].encode()) <= capacity:
                        low = middle
                    else:
                        high = middle - 1
                if len(remainder[:low].encode()) > capacity:
                    raise ReviewError("single character exceeds shard byte limit")
                shards.append(header + remainder[:low] + "\n")
                remainder = remainder[low:]
                part += 1
        elif current and len((current + block).encode()) > SHARD_BYTES:
            shards.append(current)
            current = block
        else:
            current += block
    if current:
        shards.append(current)
    return shards


class DeepSeekClient:
    def __init__(self, key: str | None = None, opener: Any = None):
        self._key = key if key is not None else os.environ.get(KEY_NAME, "")
        if not self._key.strip():
            raise ConfigurationError(f"required environment variable {KEY_NAME} is missing or empty")
        self._opener = opener

    @staticmethod
    def _open(request: urllib.request.Request, timeout: int):
        try:
            import certifi
            context = ssl.create_default_context(cafile=certifi.where())
        except ImportError:
            context = ssl.create_default_context()
        return urllib.request.urlopen(request, timeout=timeout, context=context)

    def request(self, system: str, user: str, telemetry: Telemetry) -> dict[str, Any]:
        payload = {
            "model": MODEL,
            "messages": [{"role": "system", "content": system}, {"role": "user", "content": user}],
            "thinking": {"type": "enabled"},
            "reasoning_effort": "high",
            "stream": False,
            "response_format": {"type": "json_object"},
            "max_tokens": MAX_OUTPUT_TOKENS,
        }
        request = urllib.request.Request(
            API_URL, data=canonical_json(payload).encode(), method="POST",
            headers={"Authorization": f"Bearer {self._key}", "Content-Type": "application/json"},
        )
        last_error = "request failed"
        for attempt in range(len(RETRY_DELAYS) + 1):
            telemetry.calls += 1
            try:
                opener = self._opener or self._open
                with opener(request, timeout=180) as response:
                    envelope = json.loads(response.read().decode())
                choice = envelope["choices"][0]
                finish_reason = choice.get("finish_reason")
                if finish_reason == "insufficient_system_resource":
                    raise urllib.error.URLError("inference resources unavailable")
                if finish_reason != "stop":
                    raise OutputError(f"review response did not finish normally: {finish_reason}")
                content = choice.get("message", {}).get("content")
                if not content:
                    raise OutputError("review response content was empty")
                result = json.loads(content)
                usage = envelope.get("usage", {})
                telemetry.prompt_tokens += int(usage.get("prompt_tokens", 0))
                telemetry.completion_tokens += int(usage.get("completion_tokens", 0))
                telemetry.cache_hit_tokens += int(usage.get("prompt_cache_hit_tokens", 0))
                telemetry.cache_miss_tokens += int(usage.get("prompt_cache_miss_tokens", 0))
                telemetry.api_status = "success"
                return result
            except urllib.error.HTTPError as exc:
                last_error = f"API HTTP {exc.code}"
                if exc.code not in {408, 429, 500, 502, 503, 504}:
                    telemetry.api_status = "configuration_or_permanent_failure"
                    raise ConfigurationError(last_error) from None
            except (urllib.error.URLError, TimeoutError) as exc:
                last_error = f"transport failure: {type(exc).__name__}"
            except (KeyError, IndexError, json.JSONDecodeError, OutputError) as exc:
                last_error = f"invalid review output: {type(exc).__name__}"
            if attempt < len(RETRY_DELAYS):
                telemetry.retries += 1
                time.sleep(RETRY_DELAYS[attempt])
        telemetry.api_status = "retry_exhausted"
        raise ReviewError(last_error)


def specialist_schema_valid(value: Any, expected_pass: str | None = None) -> bool:
    if not isinstance(value, dict) or value.get("review_complete") is not True:
        return False
    if expected_pass is not None and value.get("pass") != expected_pass:
        return False
    if not isinstance(value.get("findings"), list) or not isinstance(value.get("uncertainties", []), list):
        return False
    return all(isinstance(f, dict) and f.get("severity") in SEVERITIES for f in value["findings"])


def request_validated(client: DeepSeekClient, system: str, prompt: str,
                      telemetry: Telemetry, validator: Any, label: str) -> dict[str, Any]:
    value = client.request(system, prompt, telemetry)
    if validator(value):
        return value
    telemetry.retries += 1
    repair = (
        prompt + "\nYour previous JSON did not match the required schema. Return a fresh complete JSON object only; "
        "do not discuss or quote the prior response. Follow every field and enum exactly."
    )
    value = client.request(system, repair, telemetry)
    if not validator(value):
        raise OutputError(f"schema-invalid response after repair: {label}")
    return value


def final_schema_valid(value: Any, review_type: str, snapshot_id: str) -> bool:
    if not isinstance(value, dict) or value.get("verdict") not in VERDICTS:
        return False
    if value.get("review_type") != review_type or value.get("snapshot_id") != snapshot_id:
        return False
    if value["verdict"] == "PASS":
        return value.get("review_complete") is True and value.get("blocking_findings") == []
    if value["verdict"] == "FAIL":
        findings = value.get("blocking_findings")
        return isinstance(findings, list) and bool(findings) and all(f.get("severity") in SEVERITIES for f in findings)
    return value.get("review_complete") is False


def common_prompt(review_type: str, snapshot_id: str, requirements: str, material: str) -> str:
    return (
        "Return JSON only. Review independently and neutrally. Report only high-confidence BLOCKER/HIGH defects. "
        "Review the entire assigned scope after finding defects. Before final JSON, silently re-scan for missed "
        "serious defects, false positives, and shared root causes. Never emit hidden reasoning.\n"
        f"REVIEW_TYPE={review_type}\nSNAPSHOT_ID={snapshot_id}\n"
        f"CONTROLLING_REQUIREMENTS\n{requirements}\nIMMUTABLE_REVIEW_MATERIAL\n{material}\n"
        "Required JSON shape: {\"pass\":\"<ASSIGNED_PASS>\",\"review_complete\":true,\"findings\":["
        "{\"id\":\"A-001\",\"severity\":\"HIGH\",\"category\":\"correctness\","
        "\"requirement\":\"AC-1\",\"location\":\"file:line\",\"problem\":\"defect\","
        "\"evidence\":\"proof\",\"required_outcome\":\"correction\"}],\"uncertainties\":[]} "
        "findings must contain only BLOCKER or HIGH; use an empty array when none.\n"
    )


def compact_failure(review_type: str, snapshot_id: str, verdict: str, reason: str) -> dict[str, Any]:
    return {"schema_version": 1, "review_type": review_type, "snapshot_id": snapshot_id,
            "verdict": verdict, "review_complete": False, "reason": reason}


def perform_review(client: DeepSeekClient, review_type: str, snapshot_id: str,
                   requirements: str, shards: list[str], prior: list[dict[str, Any]],
                   telemetry: Telemetry) -> dict[str, Any]:
    specialist_results: list[dict[str, Any]] = []
    specialist_failures: list[str] = []
    requirement_shards = shard_records([("controlling-requirements", requirements)])
    for requirement_index, requirement_material in enumerate(requirement_shards):
        for shard_index, material in enumerate(shards):
            common = common_prompt(review_type, snapshot_id, requirement_material, material)
            for pass_name, scope in SPECIALISTS[review_type]:
                telemetry.passes.append(pass_name)
                instruction = (
                    f"Assigned pass {pass_name}; review {scope}. Requirement shard "
                    f"{requirement_index + 1}/{len(requirement_shards)}; material shard "
                    f"{shard_index + 1}/{len(shards)}."
                )
                try:
                    value = request_validated(
                        client, "You are a strict software review gate. Return JSON only.",
                        common + instruction + f" Return pass exactly as {pass_name}.", telemetry,
                        lambda candidate, expected=pass_name: specialist_schema_valid(candidate, expected),
                        pass_name,
                    )
                    specialist_results.append(value)
                except ReviewError as exc:
                    specialist_failures.append(
                        f"{pass_name} requirement {requirement_index + 1} material "
                        f"{shard_index + 1}: {type(exc).__name__}"
                    )
    if specialist_failures:
        return {
            "schema_version": 1,
            "review_type": review_type,
            "snapshot_id": snapshot_id,
            "verdict": "INCONCLUSIVE",
            "review_complete": False,
            "missing_context": specialist_failures,
        }
    while len(canonical_json(prior).encode()) > SHARD_BYTES:
        prior_batches: list[list[dict[str, Any]]] = []
        current_prior: list[dict[str, Any]] = []
        for item in prior:
            candidate = current_prior + [item]
            if current_prior and len(canonical_json(candidate).encode()) > SHARD_BYTES:
                prior_batches.append(current_prior)
                current_prior = [item]
            else:
                current_prior = candidate
        if current_prior:
            prior_batches.append(current_prior)
        reduced_prior: list[dict[str, Any]] = []
        for index, batch in enumerate(prior_batches):
            prompt = (
                "Return JSON only. Compact this prior-finding metadata by merging duplicate IDs while preserving "
                "every unique ID, status, dispute evidence, and unresolved serious issue. Do not add findings.\n"
                + canonical_json(batch)
                + '\nReturn {"prior_findings":[]} as JSON.'
            )
            telemetry.passes.append(f"PRIOR-CONSOLIDATION-{index + 1}")
            value = request_validated(
                client, "You compact prior review metadata without losing unique records. Return JSON only.",
                prompt, telemetry,
                lambda candidate: isinstance(candidate.get("prior_findings"), list),
                f"PRIOR-CONSOLIDATION-{index + 1}",
            )
            reduced_prior.extend(value["prior_findings"])
        if len(canonical_json(reduced_prior).encode()) >= len(canonical_json(prior).encode()):
            raise OutputError("prior-finding metadata cannot be reduced within safe input bounds")
        prior = reduced_prior
    consolidation = {
        "specialists": specialist_results,
        "prior_findings": prior,
        "requirement_shard_count": len(requirement_shards),
        "material_shard_count": len(shards),
    }
    consolidation_requirements = requirements
    if len(requirements.encode()) > SHARD_BYTES:
        consolidation_requirements = canonical_json({
            "requirement_shards_reviewed": len(requirement_shards),
            "requirement_sha256": hashlib.sha256(requirements.encode()).hexdigest(),
        })
    while len(canonical_json(consolidation).encode()) > SHARD_BYTES:
        previous_size = len(canonical_json(consolidation).encode())
        batches: list[list[dict[str, Any]]] = []
        current: list[dict[str, Any]] = []
        for result in consolidation["specialists"]:
            candidate = current + [result]
            if current and len(canonical_json(candidate).encode()) > SHARD_BYTES:
                batches.append(current)
                current = [result]
            else:
                current = candidate
        if current:
            batches.append(current)
        reduced: list[dict[str, Any]] = []
        for index, batch in enumerate(batches):
            prompt = common_prompt(
                review_type, snapshot_id, consolidation_requirements,
                "Consolidate this structured finding batch without dropping any valid unique serious finding:\n"
                + canonical_json(batch),
            ) + "Return the specialist JSON shape with pass CONSOLIDATION-SHARD."
            telemetry.passes.append(f"CONSOLIDATION-SHARD-{index + 1}")
            reduced.append(request_validated(
                client, "You are a strict hierarchical review consolidator. Return JSON only.",
                prompt, telemetry,
                lambda candidate: specialist_schema_valid(candidate, "CONSOLIDATION-SHARD"),
                f"CONSOLIDATION-SHARD-{index + 1}",
            ))
        consolidation["specialists"] = reduced
        if len(canonical_json(consolidation).encode()) >= previous_size:
            raise OutputError("consolidation findings cannot be reduced within safe input bounds")
    consolidation_prompt = common_prompt(
        review_type, snapshot_id, consolidation_requirements,
        "Structured specialist findings follow. Re-check against requirements and preserve every valid unique serious finding.\n" + canonical_json(consolidation),
    ) + (
        "Adversarially consolidate. No majority voting. Reject only unsupported/stale findings; merge duplicates and root causes. "
        "Return compact final JSON with schema_version=1, review_type, snapshot_id, verdict, review_complete, "
        "blocking_findings, root_cause_groups, prior_findings. Use INCONCLUSIVE if material uncertainty remains."
    )
    telemetry.passes.append("CONSOLIDATION")
    final = request_validated(
        client, "You are the adversarial final review authority. Return JSON only.",
        consolidation_prompt, telemetry,
        lambda value: final_schema_valid(value, review_type, snapshot_id),
        "CONSOLIDATION",
    )
    if final.get("verdict") == "INCONCLUSIVE" or final.get("uncertainties"):
        telemetry.adjudication = True
        telemetry.passes.append("ADJUDICATION")
        adjudication_prompt = consolidation_prompt + "\nResolve only the material ambiguity and return a complete final result."
        final = request_validated(
            client, "You are the independent review adjudicator. Return JSON only.",
            adjudication_prompt, telemetry,
            lambda value: final_schema_valid(value, review_type, snapshot_id),
            "ADJUDICATION",
        )
    if not final_schema_valid(final, review_type, snapshot_id):
        raise OutputError("schema-invalid consolidated response")
    final["schema_version"] = 1
    return final


def write_telemetry(root: Path, telemetry: Telemetry, final: dict[str, Any]) -> None:
    directory = root / "test-artefacts" / "reviewer"
    directory.mkdir(parents=True, exist_ok=True)
    record = {"project_notice": NOTICE, "timestamp": datetime.now(timezone.utc).isoformat(),
              "review_type": telemetry.review_type, "snapshot_id": telemetry.snapshot_id,
              "api_calls": telemetry.calls, "specialist_passes": telemetry.passes,
              "adjudication": telemetry.adjudication, "retry_count": telemetry.retries,
              "api_status": telemetry.api_status, "prompt_tokens": telemetry.prompt_tokens,
              "completion_tokens": telemetry.completion_tokens,
              "prompt_cache_hit_tokens": telemetry.cache_hit_tokens,
              "prompt_cache_miss_tokens": telemetry.cache_miss_tokens,
              "serious_findings": len(final.get("blocking_findings", [])),
              "verdict": final.get("verdict"), "elapsed_seconds": round(time.monotonic() - telemetry.started, 3)}
    path = directory / f"telemetry-{int(time.time())}.json"
    path.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    files = sorted(directory.glob("telemetry-*.json"), key=lambda p: p.stat().st_mtime, reverse=True)
    for old in files[50:]:
        old.unlink()
    if final.get("verdict") == "PASS" and telemetry.review_type == "CODE":
        receipt = {"project_notice": NOTICE, "snapshot_id": telemetry.snapshot_id,
                   "verdict": "PASS", "review_complete": True}
        (directory / "code-pass.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the batched external review gate.")
    sub = parser.add_subparsers(dest="command", required=True)
    review = sub.add_parser("review")
    review.add_argument("--type", choices=sorted(SPECIALISTS), required=True)
    review.add_argument("--requirements", action="append", default=[], required=True)
    review.add_argument("--path", action="append", default=[])
    review.add_argument("--base")
    review.add_argument("--head")
    review.add_argument("--prior-findings")
    health = sub.add_parser("health-check")
    health.add_argument("--requirements", required=True)
    args = parser.parse_args()
    root = Path.cwd().resolve()
    if args.command == "health-check":
        try:
            client = DeepSeekClient()
            telemetry = Telemetry("DOCUMENTATION", "manual-health-check")
            result = client.request("Return JSON only.", "Return {\"status\":\"available\"} as JSON.", telemetry)
            print(canonical_json({"status": "available" if result.get("status") == "available" else "inconclusive"}))
            return 0 if result.get("status") == "available" else 2
        except ReviewError as exc:
            print(canonical_json({"status": "unavailable", "reason": str(exc)}))
            return 3
    try:
        requirement_text = "\n".join(resolve_inside(root, p).read_text(encoding="utf-8") for p in args.requirements)
        if args.type == "CODE":
            if not args.base or not args.head:
                raise ReviewError("CODE requires --base and --head")
            snapshot_id, records = code_packet(root, args.base, args.head)
        else:
            if not args.path:
                raise ReviewError(f"{args.type} requires at least one --path")
            snapshot_id, records = file_packet(root, args.path)
        prior = []
        if args.prior_findings:
            prior = json.loads(resolve_inside(root, args.prior_findings).read_text(encoding="utf-8"))
        telemetry = Telemetry(args.type, snapshot_id)
        client = DeepSeekClient()
        final = perform_review(client, args.type, snapshot_id, requirement_text,
                               shard_records(records), prior, telemetry)
    except ConfigurationError as exc:
        final = compact_failure(getattr(args, "type", "CODE"), locals().get("snapshot_id", "unavailable"),
                                "REVIEW_UNAVAILABLE", str(exc))
        telemetry = locals().get("telemetry")
    except (ReviewError, OutputError, json.JSONDecodeError) as exc:
        final = compact_failure(args.type, locals().get("snapshot_id", "unavailable"), "INCONCLUSIVE", str(exc))
        telemetry = locals().get("telemetry")
    if telemetry is not None:
        try:
            write_telemetry(root, telemetry, final)
        except OSError:
            final["telemetry_status"] = "unavailable"
    print(json.dumps(final, separators=(",", ":"), sort_keys=True))
    return 0 if final.get("verdict") == "PASS" else 2


if __name__ == "__main__":
    sys.exit(main())
