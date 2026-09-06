# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

import json
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class DifferentialScenarioManifestTests(unittest.TestCase):
    def test_manifest_has_normalized_128k_scenarios(self):
        manifest = json.loads(
            (ROOT / "tests" / "differential-scenarios.json").read_text(
                encoding="utf-8"
            )
        )
        self.assertEqual(manifest["schema_version"], 2)
        self.assertEqual(
            manifest["reference_identity"],
            "94f69bd8f4acb6c0c320ae34f9b1c3ee29bc5545",
        )
        scenarios = manifest["scenarios"]
        ids = {scenario["id"] for scenario in scenarios}
        required = {
            "128k-all-bank-selection",
            "128k-raster-screen-selection",
            "128k-snapshot-state-normalization",
            "interface1-port-alias-dispatch",
            "interface1-control-latch-motor-edge",
            "microdrive-sector-header-data-wrap",
            "microdrive-write-protect-and-gap",
            "zxnet-six-state-loopback",
        }
        self.assertTrue(required.issubset(ids))
        for scenario in scenarios:
            self.assertTrue(scenario["reference_boundary"])
            self.assertEqual(scenario["classification"], "source_reviewed_match")
            self.assertTrue(scenario["compared_fields"])
            if scenario["id"].startswith("128k-"):
                self.assertEqual(scenario["profile"], "WZ_MACHINE_128K_PAL")
                self.assertTrue(scenario["setup"])
            if scenario["id"].startswith(("interface1-", "microdrive-", "zxnet-")):
                self.assertEqual(scenario["profile"], "WZ_MACHINE_48K_PAL_INTERFACE1")
                self.assertTrue(scenario["setup"])
                self.assertIn("MDRIVE.ASM", scenario["reference_boundary"])
                self.assertTrue(scenario["compared_fields"])


if __name__ == "__main__":
    unittest.main()
