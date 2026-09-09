#!/usr/bin/env python3
#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

from pathlib import Path
import argparse,json
from hosted_gate_authority import verify
ap=argparse.ArgumentParser(); ap.add_argument("--payload",required=True); ap.add_argument("--signature",required=True); ap.add_argument("--expected-sha",required=True); a=ap.parse_args()
body=verify(Path.cwd().resolve(),a.payload,a.signature,a.expected_sha); print(json.dumps({"status":"authorized","head_sha":body["head_sha"]},separators=(",",":")))
