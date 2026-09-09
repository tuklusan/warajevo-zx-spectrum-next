#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.
from __future__ import annotations
import argparse, hashlib, hmac, json
from pathlib import Path
from hosted_gate_authority import cj, key
from evidence_schema_v2 import deterministic_tree_index

PROJECT_ID="github.com/tuklusan/warajevo-zx-spectrum-next"

def sha(data:bytes)->str:return hashlib.sha256(data).hexdigest()

def main()->int:
    ap=argparse.ArgumentParser()
    ap.add_argument("--run-id",required=True); ap.add_argument("--head-sha",required=True); ap.add_argument("--workflow-blob-sha",required=True)
    ap.add_argument("--jobs-json",type=Path,required=True); ap.add_argument("--artifacts-json",type=Path,required=True)
    ap.add_argument("--expected-lanes",required=True); ap.add_argument("--evidence-root",type=Path,required=True); ap.add_argument("--out-dir",type=Path,required=True)
    a=ap.parse_args(); lanes=a.expected_lanes.split(); jobs=json.loads(a.jobs_json.read_text()); artifacts=json.loads(a.artifacts_json.read_text())
    if len(lanes)!=20 or len(set(lanes))!=20: raise SystemExit("publication manifest requires exactly 20 lanes")
    if not isinstance(jobs,list) or len(jobs)!=20: raise SystemExit("publication manifest requires exactly 20 jobs")
    if sorted(str(x.get("name","")) for x in jobs)!=sorted(lanes): raise SystemExit("publication job names do not match exact lane set")
    if not all(x.get("head_sha")==a.head_sha and x.get("status")=="completed" and x.get("conclusion")=="success" for x in jobs):
        raise SystemExit("publication jobs are not exact-commit terminal successes")
    if not isinstance(artifacts,list): raise SystemExit("publication artifact metadata is not a list")
    artifact_by_name={str(x.get("name","")):x for x in artifacts if isinstance(x,dict) and x.get("name")}
    if len(artifact_by_name)!=len([x for x in artifacts if isinstance(x,dict) and x.get("name")]): raise SystemExit("publication artifact names are not unique")

    lane_evidence=[]; seen=set()
    manifests=sorted(a.evidence_root.rglob("result-manifest.json"))
    if len(manifests)!=20: raise SystemExit(f"publication evidence requires 20 result manifests, found {len(manifests)}")
    for manifest in manifests:
        result=json.loads(manifest.read_text(encoding="utf-8")); lane=result.get("lane_id")
        if lane not in lanes or lane in seen: raise SystemExit("publication evidence has duplicate/unknown lane")
        seen.add(lane); artifact_dir=manifest.parent; artifact_name=artifact_dir.name
        if artifact_name not in artifact_by_name: raise SystemExit(f"publication API metadata lacks artifact {artifact_name}")
        tree,tree_hash=deterministic_tree_index(artifact_dir)
        api=artifact_by_name[artifact_name]
        lane_evidence.append({"lane_id":lane,"artifact_name":artifact_name,"result_manifest_sha256":sha(manifest.read_bytes()),
                              "tree_sha256":tree_hash,"tree_file_count":len(tree),"tree_bytes":sum(int(x["bytes"]) for x in tree),
                              "artifact_id":api.get("id"),"artifact_digest":api.get("digest"),"artifact_size_in_bytes":api.get("size_in_bytes")})
    if seen!=set(lanes): raise SystemExit("publication evidence lane set is incomplete")
    body={"schema_version":2,"project_id":PROJECT_ID,"run_id":str(a.run_id),"build_id":a.head_sha,"publication_id":str(a.run_id),
          "head_sha":a.head_sha,"workflow_blob_sha":a.workflow_blob_sha,"matrix_lane_count":20,"expected_lanes":lanes,
          "jobs_sha256":sha(cj(jobs).encode()),"artifacts_sha256":sha(cj(artifacts).encode()),"jobs":jobs,"artifacts":artifacts,
          "lane_evidence":sorted(lane_evidence,key=lambda x:x["lane_id"])}
    a.out_dir.mkdir(parents=True,exist_ok=True); p=a.out_dir/"publication-manifest.json"; raw=(json.dumps(body,indent=2,sort_keys=True)+"\n").encode(); p.write_bytes(raw)
    (a.out_dir/"publication-manifest.sig").write_text(hmac.new(key(),raw,hashlib.sha256).hexdigest()+"\n",encoding="ascii")
    return 0
if __name__=="__main__": raise SystemExit(main())
