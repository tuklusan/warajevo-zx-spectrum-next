#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.
"""Non-authoritative, infrequent qualification probe for hosted structured-output capability."""
from __future__ import annotations
import argparse, json, os, ssl, urllib.error, urllib.request
from pathlib import Path
API='https://integrate.api.nvidia.com/v1/chat/completions'; MODEL='nvidia/nemotron-3-ultra-550b-a55b'; KEY='NVIDIA_API_KEY_CODING'
def main()->int:
 ap=argparse.ArgumentParser(); ap.add_argument('--record',default='test-artefacts/reviewer/provider-capability.local.json'); a=ap.parse_args()
 key=os.environ.get(KEY,'');
 if not key: raise SystemExit(f'{KEY} is required')
 schema={"type":"object","properties":{"status":{"type":"string","enum":["ok"]}},"required":["status"],"additionalProperties":False}
 # Compatible json_schema shape. Success qualifies only this exact hosted endpoint/model/profile; failure changes no gate authority.
 body={"model":MODEL,"messages":[{"role":"user","content":"Return status ok."}],"stream":False,"max_tokens":128,
       "reasoning_effort":"none","chat_template_kwargs":{"enable_thinking":False,"force_nonempty_content":True},
       "response_format":{"type":"json_schema","json_schema":{"name":"wzxn_capability_probe","strict":True,"schema":schema}}}
 req=urllib.request.Request(API,data=json.dumps(body,separators=(',',':')).encode(),method='POST',headers={'Authorization':f'Bearer {key}','Content-Type':'application/json','Accept':'application/json'})
 qualified=False; detail=''
 try:
  with urllib.request.urlopen(req,timeout=120,context=ssl.create_default_context()) as r:
   env=json.loads(r.read()); content=env['choices'][0]['message']['content']; qualified=json.loads(content)=={'status':'ok'}; detail=f'HTTP {r.status}'
 except urllib.error.HTTPError as e: detail=f'HTTP {e.code}'
 except Exception as e: detail=type(e).__name__
 out={'schema_version':1,'model':MODEL,'endpoint':API,'json_schema_qualified':qualified,'detail':detail,'authority':False}
 p=Path(a.record); p.parent.mkdir(parents=True,exist_ok=True); p.write_text(json.dumps(out,indent=2,sort_keys=True)+'\n')
 print(json.dumps(out,separators=(',',':'),sort_keys=True)); return 0 if qualified else 2
if __name__=='__main__': raise SystemExit(main())
