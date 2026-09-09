#!/usr/bin/env python3
#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

from pathlib import Path
import argparse,json
from hosted_gate_authority import make
ap=argparse.ArgumentParser(); ap.add_argument("--ttl-seconds",type=int,default=14400); a=ap.parse_args()
payload,sig,body=make(Path.cwd().resolve(),a.ttl_seconds)
print(json.dumps({"authorization_payload":payload,"authorization_signature":sig,"head_sha":body["head_sha"],"expires_at":body["expires_at"]},separators=(",",":"),sort_keys=True))
