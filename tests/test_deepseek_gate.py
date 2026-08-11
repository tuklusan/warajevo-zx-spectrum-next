#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools" / "reviewer"))
import deepseek_gate as gate


class FakeClient:
    def __init__(self, responses):
        self.responses = iter(responses)
        self.calls = []

    def request(self, system, user, telemetry):
        self.calls.append((system, user))
        telemetry.calls += 1
        return next(self.responses)


class FailingThenPassingClient(FakeClient):
    def request(self, system, user, telemetry):
        self.calls.append((system, user))
        telemetry.calls += 1
        value = next(self.responses)
        if isinstance(value, Exception):
            raise value
        return value


def specialist(name, findings=None, uncertainties=None):
    return {"pass": name, "review_complete": True, "findings": findings or [],
            "uncertainties": uncertainties or []}


def final(verdict="PASS", findings=None):
    return {"schema_version": 1, "review_type": "CODE", "snapshot_id": "snap",
            "verdict": verdict, "review_complete": True,
            "blocking_findings": findings or [], "root_cause_groups": [], "prior_findings": []}


class GateTests(unittest.TestCase):
    def test_payload_configuration_and_no_liveness(self):
        captured = {}

        class Response:
            def __enter__(self): return self
            def __exit__(self, *_): return False
            def read(self):
                return json.dumps({"choices": [{"finish_reason": "stop", "message": {"content": "{}"}}]}).encode()

        def opener(request, timeout):
            captured.update(json.loads(request.data))
            return Response()

        client = gate.DeepSeekClient("secret", opener)
        client.request("system", "real review", gate.Telemetry("CODE", "x"))
        self.assertEqual(captured["model"], gate.MODEL)
        self.assertEqual(captured["thinking"], {"type": "enabled"})
        self.assertEqual(captured["reasoning_effort"], "high")
        self.assertEqual(captured["response_format"], {"type": "json_object"})
        self.assertNotIn("temperature", captured)
        self.assertNotIn("top_p", captured)

    def test_missing_key_is_unavailable_configuration(self):
        with self.assertRaises(gate.ConfigurationError):
            gate.DeepSeekClient("")

    def test_all_specialists_and_consolidation_run(self):
        responses = [specialist("CODE-A"), specialist("CODE-B"), specialist("CODE-C"), final()]
        client = FakeClient(responses)
        telemetry = gate.Telemetry("CODE", "snap")
        result = gate.perform_review(client, "CODE", "snap", "requirements", ["material"], [], telemetry)
        self.assertEqual(result["verdict"], "PASS")
        self.assertEqual(len(client.calls), 4)
        self.assertEqual(telemetry.passes, ["CODE-A", "CODE-B", "CODE-C", "CONSOLIDATION"])

    def test_all_review_types_execute_three_specialists(self):
        for review_type, passes in gate.SPECIALISTS.items():
            responses = [specialist(name) for name, _ in passes]
            result = {"schema_version": 1, "review_type": review_type, "snapshot_id": "snap",
                      "verdict": "PASS", "review_complete": True, "blocking_findings": [],
                      "root_cause_groups": [], "prior_findings": []}
            client = FakeClient(responses + [result])
            gate.perform_review(client, review_type, "snap", "requirements", ["material"], [],
                                gate.Telemetry(review_type, "snap"))
            self.assertEqual(len(client.calls), 4)

    def test_schema_repair_is_bounded_and_succeeds(self):
        client = FakeClient([{"wrong": True}, {"still_wrong": True}, specialist("CODE-A")])
        telemetry = gate.Telemetry("CODE", "snap")
        value = gate.request_validated(
            client, "system", "prompt", telemetry,
            lambda candidate: gate.specialist_schema_valid(candidate, "CODE-A"), "CODE-A",
        )
        self.assertEqual(value["pass"], "CODE-A")
        self.assertEqual(len(client.calls), 3)
        self.assertEqual(telemetry.retries, 2)

    def test_schema_repair_exhaustion_fails_closed(self):
        client = FakeClient([{"wrong": True}, {"still_wrong": True}, {"wrong_again": True}])
        telemetry = gate.Telemetry("CODE", "snap")
        with self.assertRaises(gate.OutputError):
            gate.request_validated(
                client, "system", "prompt", telemetry,
                lambda candidate: gate.specialist_schema_valid(candidate, "CODE-A"), "CODE-A",
            )
        self.assertEqual(len(client.calls), 3)
        self.assertEqual(telemetry.retries, 2)

    def test_failed_specialist_does_not_skip_remaining_passes(self):
        client = FailingThenPassingClient([
            gate.ReviewError("failed"), specialist("CODE-B"), specialist("CODE-C")
        ])
        result = gate.perform_review(client, "CODE", "snap", "requirements", ["material"], [],
                                     gate.Telemetry("CODE", "snap"))
        self.assertEqual(result["verdict"], "INCONCLUSIVE")
        self.assertEqual(len(client.calls), 3)

    def test_serious_finding_is_not_capped(self):
        findings = [
            {"severity": "HIGH", "id": str(i), "category": "correctness",
             "requirement": "AC-1", "location": "file:1", "problem": "defect",
             "evidence": "proof", "required_outcome": "correction"}
            for i in range(100)
        ]
        self.assertTrue(gate.specialist_schema_valid(specialist("CODE-A", findings)))

    def test_invalid_and_incomplete_outputs_cannot_pass(self):
        self.assertFalse(gate.specialist_schema_valid({"review_complete": False, "findings": []}))
        self.assertFalse(gate.final_schema_valid({"review_type": "CODE", "snapshot_id": "snap",
                                                  "verdict": "PASS", "review_complete": False,
                                                  "blocking_findings": []}, "CODE", "snap"))
        self.assertFalse(gate.final_schema_valid({"review_type": "CODE", "snapshot_id": "snap",
                                                  "verdict": "MAYBE"}, "CODE", "snap"))
        self.assertFalse(gate.specialist_schema_valid(
            {"pass": "CODE-A", "review_complete": True,
             "findings": [{"severity": "HIGH"}], "uncertainties": []}, "CODE-A"
        ))

    def test_adjudication_only_for_ambiguity(self):
        inconclusive = {"review_type": "CODE", "snapshot_id": "snap", "verdict": "INCONCLUSIVE",
                        "review_complete": False, "reason": "ambiguity"}
        responses = [specialist("A"), specialist("B"), specialist("C"), inconclusive, final()]
        client = FakeClient(responses)
        telemetry = gate.Telemetry("CODE", "snap")
        gate.perform_review(client, "CODE", "snap", "requirements", ["material"], [], telemetry)
        self.assertTrue(telemetry.adjudication)
        self.assertEqual(len(client.calls), 5)

    def test_sharding_never_drops_material(self):
        content = "x" * (gate.SHARD_BYTES * 2 + 17)
        shards = gate.shard_records([("large.log", content)])
        self.assertGreaterEqual(len(shards), 3)
        self.assertGreaterEqual(sum(len(s) for s in shards), len(content))

    def test_sharding_preserves_unicode_and_part_identity(self):
        content = "\u20ac" * gate.SHARD_BYTES
        shards = gate.shard_records([("unicode.txt", content)])
        self.assertGreater(len(shards), 1)
        self.assertNotIn("\ufffd", "".join(shards))
        self.assertTrue(all("unicode.txt (part " in shard for shard in shards))

    def test_api_key_not_present_in_payload_content(self):
        key = "do-not-leak-this-value"
        captured = {}

        class Response:
            def __enter__(self): return self
            def __exit__(self, *_): return False
            def read(self):
                return b'{"choices":[{"finish_reason":"stop","message":{"content":"{}"}}]}'

        def opener(request, timeout):
            captured["data"] = request.data.decode()
            return Response()

        gate.DeepSeekClient(key, opener).request("s", "u", gate.Telemetry("CODE", "x"))
        self.assertNotIn(key, captured["data"])


if __name__ == "__main__":
    unittest.main()
