#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

from __future__ import annotations
import base64, hashlib, hmac, json, os, time
from pathlib import Path
from review_authority_v4 import PROJECT_ID, PROTOCOL_VERSION, profile_hash, validate_code_receipt

ENV="WZXN_HOSTED_GATE_HMAC_KEY"
def cj(v): return json.dumps(v,ensure_ascii=True,separators=(",",":"),sort_keys=True)
def b64e(b): return base64.urlsafe_b64encode(b).decode().rstrip("=")
def b64d(s): return base64.urlsafe_b64decode(s+"="*((-len(s))%4))
def key():
    k=os.environ.get(ENV,"").encode()
    if len(k)<32: raise SystemExit(f"{ENV} must contain at least 32 bytes of secret material")
    return k
def sign(payload:str)->str: return hmac.new(key(),payload.encode(),hashlib.sha256).hexdigest()
def make(root:Path,ttl:int=14400)->tuple[str,str,dict]:
    receipt_path=root/"test-artefacts"/"reviewer"/"code-pass.json"; receipt=json.loads(receipt_path.read_text(encoding="utf-8"))
    head=validate_code_receipt(root,receipt); now=int(time.time())
    body={"schema_version":1,"project_id":PROJECT_ID,"protocol":PROTOCOL_VERSION,"head_sha":head,"snapshot_id":receipt["snapshot_id"],
          "packet_manifest_hash":receipt["packet_manifest_hash"],"review_profile_hash":profile_hash(root),"receipt_sha256":hashlib.sha256(receipt_path.read_bytes()).hexdigest(),
          "issued_at":now,"expires_at":now+ttl}
    payload=b64e(cj(body).encode()); return payload,sign(payload),body
def verify(root:Path,payload:str,signature:str,expected_sha:str)->dict:
    if not hmac.compare_digest(sign(payload),signature): raise SystemExit("hosted authorization signature mismatch")
    try: body=json.loads(b64d(payload))
    except Exception as exc: raise SystemExit("hosted authorization payload malformed") from exc
    now=int(time.time())
    if body.get("schema_version")!=1 or body.get("project_id")!=PROJECT_ID or body.get("protocol")!=PROTOCOL_VERSION: raise SystemExit("hosted authorization identity mismatch")
    if body.get("head_sha")!=expected_sha: raise SystemExit("hosted authorization is for a different commit")
    if body.get("review_profile_hash")!=profile_hash(root): raise SystemExit("hosted authorization review profile mismatch")
    if not isinstance(body.get("issued_at"),int) or not isinstance(body.get("expires_at"),int) or not (body["issued_at"]<=now<=body["expires_at"]): raise SystemExit("hosted authorization expired/not-yet-valid")
    return body
