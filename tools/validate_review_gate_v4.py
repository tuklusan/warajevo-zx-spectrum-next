#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.
"""Static fail-closed drift checks for external-review protocol v4."""
from __future__ import annotations
import argparse, ast, json, re, subprocess
from pathlib import Path

PROTECTED={
 "tools/reviewer/review_gate.py","tools/reviewer/legacy_bootstrap_gate.py","tools/reviewer/review-profile-v4.json",
 "tools/reviewer/probe_provider_capabilities.py","design/review-gate.md","design/FRESH-PROJECT-EXTERNAL-REVIEW-GATE.md","WORKFLOW.md",
 "tools/harness/invoke_remote_harness.py","tools/harness/review_authority_v4.py","tools/harness/evidence_schema_v2.py",
 "tools/harness/verify_platform_evidence.py","tools/harness/hosted_gate_authority.py","tools/harness/make_hosted_authorization.py",
 "tools/harness/verify_hosted_authorization.py","tools/harness/make_publication_manifest.py","tools/harness/cleanup-hosted-runner-state.sh",
 "tools/validate_project_gates.py","tools/validate_review_gate_v4.py","tools/harness/harness-lock.json",
 ".githooks/pre-commit",".githooks/pre-push",".github/workflows/platform-smoke.yml",".github/workflows/repository-gates.yml"
}
EXPECTED=[
 ("linux-x64","ubuntu-22.04"),("linux-x64-latest","ubuntu-latest"),("linux-x64-24","ubuntu-24.04"),("linux-x64-26","ubuntu-26.04"),
 ("linux-arm64-22","ubuntu-22.04-arm"),("linux-arm64","ubuntu-24.04-arm"),("linux-arm64-26","ubuntu-26.04-arm"),
 ("windows-latest","windows-latest"),("windows-server-2022","windows-2022"),("windows-server-2025","windows-2025"),
 ("windows-server-2025-vs2026","windows-2025-vs2026"),("windows-arm-11","windows-11-arm"),("windows-arm-11-vs2026","windows-11-vs2026-arm"),
 ("macos-arm64-latest","macos-latest"),("macos-arm64","macos-15"),("macos-arm64-14","macos-14"),("macos-intel-x64","macos-15-intel"),
 ("macos-intel-26","macos-26-intel"),("macos-arm64-26","macos-26"),("macos-xcode-27","xcode-27")]

def run(root:Path,*args:str)->str:
 return subprocess.run(["git",*args],cwd=root,check=True,capture_output=True,text=True).stdout

def authorized_preflight_count(root:Path)->int:
 tracker=json.loads((root/"issues/change-requests.json").read_text()); active=[x for x in tracker.get("change_requests",[]) if x.get("status")=="in_progress"]
 count=0
 for item in active:
  cr=item.get("cr_number"); p=root/"design"/"cr-preflight"/f"{cr}.md"
  if not p.is_file(): continue
  text=p.read_text(encoding="utf-8")
  if ("Status: APPROVED_FOR_IMPLEMENTATION" in text and "## Zero-Gap Exit Scan" in text and
      re.search(r"(?im)^Review-Base:\s*\S+\s*$",text) and re.search(r"(?im)^Operator-Authorized-Gate-Change:\s*YES\s*$",text)):
   count+=1
 return count

def changed(root:Path,mode:str)->set[str]:
 try:
  out=run(root,"diff","--cached","--name-only") if mode=="staged" else run(root,"diff-tree","--no-commit-id","--name-only","-r","HEAD") if mode=="head" else ""
 except Exception:return set()
 return {x.strip() for x in out.splitlines() if x.strip()}

def main()->int:
 ap=argparse.ArgumentParser(); ap.add_argument("--mode",choices=("none","staged","head"),default="none"); a=ap.parse_args(); root=Path.cwd().resolve(); errors=[]
 gate=(root/"tools/reviewer/review_gate.py").read_text(encoding="utf-8"); bootstrap=(root/"tools/reviewer/legacy_bootstrap_gate.py").read_text(encoding="utf-8")
 workflow=(root/".github/workflows/platform-smoke.yml").read_text(encoding="utf-8"); evidence=(root/"tools/harness/evidence_schema_v2.py").read_text(encoding="utf-8")
 ast.parse(gate); ast.parse(bootstrap); ast.parse(evidence)
 profile=json.loads((root/"tools/reviewer/review-profile-v4.json").read_text())
 if profile.get("protocol")!=4 or profile.get("model")!="nvidia/nemotron-3-ultra-550b-a55b": errors.append("review profile identity drift")
 if profile.get("force_nonempty_content") is not True or profile.get("response_format")!="json_object": errors.append("provider content/response profile drift")
 if profile.get("context_bytes")!=96000 or profile.get("max_context_tokens")!=1000000: errors.append("review context profile drift")
 for phase,p in profile.get("phase_profile",{}).items():
  effort=p.get("reasoning_effort")
  if effort not in {"none","medium","high"}: errors.append(f"unsupported reasoning effort: {phase}")
  budget=int(p.get("reasoning_budget",0)); maximum=int(p.get("max_tokens",0))
  if not 1<=maximum<=32768: errors.append(f"output token range invalid: {phase}")
  if effort=="none" and budget!=0: errors.append(f"non-thinking phase has reasoning budget: {phase}")
  if effort!="none" and not 0<budget<maximum: errors.append(f"reasoning answer reserve invalid: {phase}")
 for phase in ("CODE-DISCOVERY","DOCUMENTATION-DISCOVERY","TEST-DISCOVERY"):
  if profile.get("phase_profile",{}).get(phase,{}).get("reasoning_effort")!="medium": errors.append(f"discovery reasoning profile drift: {phase}")
 for phase in ("CODE-INTEGRATION","FALSIFICATION","ADJUDICATION"):
  if profile.get("phase_profile",{}).get(phase,{}).get("reasoning_effort")!="high": errors.append(f"high reasoning profile drift: {phase}")
 if re.search(r"(?:from\s+review_gate\s+import|import\s+review_gate\b)",bootstrap): errors.append("bootstrap imports normal gate")
 if 'force_nonempty_content": True' not in gate or 'force_nonempty_content":True' not in bootstrap.replace(" ",""): errors.append("NVIDIA non-empty final-content guard missing")
 if "RETRYABLE_HTTP_STATUS = {408, 429, 500, 502, 503, 504}" not in gate or "404" in re.search(r"RETRYABLE_HTTP_STATUS\s*=\s*\{[^}]*\}",gate).group(0): errors.append("provider retry-status contract drift")
 if 'STATUS_URL = "https://integrate.api.nvidia.com/v1/status/{request_id}"' not in gate: errors.append("NVIDIA pending-status polling endpoint missing")
 if 'decision == "NON_BLOCKING"' not in gate or 'conclusion != "VIOLATION"' not in gate: errors.append("decision/evidence algebra drift")
 if 'MAX_DISCOVERY_EXPANSIONS = 2' not in gate or '_run_discovery_with_expansion' not in gate: errors.append("bounded pre-candidate evidence expansion missing")
 if 'PROTECTED_GATE_PATHS' not in gate or 'tools/harness/evidence_schema_v2.py' not in gate: errors.append("normal self-certification protected set incomplete")
 if 'cancel-in-progress: false' not in workflow or 'cancel-in-progress: true' in workflow: errors.append("hosted workflow can cancel live runs")
 if re.search(r"(?m)^\s{2}(?:push|pull_request):",workflow): errors.append("protected product workflow has automatic trigger")
 pairs=re.findall(r"^\s+- id:\s*([A-Za-z0-9][A-Za-z0-9-]*)\s*\n\s+label:\s*([^\s]+)\s*$",workflow,re.M)
 if pairs!=EXPECTED: errors.append("hosted exact 20-lane ID/label/order changed")
 for required in ("verify_hosted_authorization.py","make_publication_manifest.py","--evidence-root .wzsn-published-artifacts","sokol-host","verify_platform_evidence.py --verify-tree"):
  if required not in workflow: errors.append(f"hosted protected-evidence stage missing: {required}")
 for required in ("EXPECTED_FUSE_MANIFESTS","validate_lane_result","deterministic_tree_index","PINNED_FUSE_COMMIT"):
  if required not in evidence: errors.append(f"canonical evidence verifier drift: {required}")
 changes=changed(root,a.mode)
 if changes & PROTECTED and authorized_preflight_count(root)!=1: errors.append("protected gate/harness change lacks exactly one active operator-authorized preflight")
 if errors:
  for e in errors: print("REVIEW-GATE-V4 FAIL:",e,file=__import__('sys').stderr)
  return 1
 print("REVIEW-GATE-V4 PASS"); return 0
if __name__=="__main__": raise SystemExit(main())
