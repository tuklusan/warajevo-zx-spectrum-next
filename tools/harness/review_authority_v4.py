#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

"""Deterministic receipt authority validation shared by protected harness entry points."""
from __future__ import annotations
import hashlib, json, re, subprocess
from pathlib import Path

PROJECT_ID="github.com/tuklusan/warajevo-zx-spectrum-next"
PROTOCOL_VERSION=4


def cj(v): return json.dumps(v,ensure_ascii=True,separators=(",",":"),sort_keys=True)
def sha(b): return hashlib.sha256(b).hexdigest()

def run_git(root:Path,*args:str)->str:
    try:
        return subprocess.run(["git",*args],cwd=root,check=True,capture_output=True,text=True,timeout=30).stdout.strip()
    except Exception as exc:
        raise SystemExit("remote smoke blocked: git identity check failed") from exc

def bound(root:Path,value:str)->Path:
    p=(root/value).resolve()
    try:p.relative_to(root.resolve())
    except ValueError as exc: raise SystemExit("remote smoke blocked: authority path escapes project") from exc
    if not p.is_file(): raise SystemExit("remote smoke blocked: authority source is missing")
    return p

def profile_hash(root:Path)->str:
    p=bound(root,"tools/reviewer/review-profile-v4.json")
    try: data=json.loads(p.read_text(encoding="utf-8"))
    except Exception as exc: raise SystemExit("remote smoke blocked: review profile is malformed") from exc
    if data.get("protocol")!=PROTOCOL_VERSION: raise SystemExit("remote smoke blocked: review profile protocol mismatch")
    return sha(cj(data).encode())

def active_scope(root:Path,cr:str,receipt:dict)->dict:
    tracker_path=bound(root,"issues/change-requests.json"); tracker_data=tracker_path.read_bytes(); tracker=json.loads(tracker_data)
    items=tracker.get("change_requests",[]) if isinstance(tracker,dict) else []
    matches=[x for x in items if isinstance(x,dict) and x.get("cr_number")==cr]
    if len(matches)!=1 or matches[0].get("status")!="in_progress": raise SystemExit("remote smoke blocked: reviewed CR is not uniquely active")
    item=matches[0]
    pf=bound(root,receipt.get("preflight_source",f"design/cr-preflight/{cr}.md")); pf_data=pf.read_bytes(); text=pf_data.decode("utf-8")
    if "Status: APPROVED_FOR_IMPLEMENTATION" not in text or "## Zero-Gap Exit Scan" not in text:
        raise SystemExit("remote smoke blocked: CR preflight is not active/approved")
    if sha(pf_data)!=receipt.get("preflight_sha256"): raise SystemExit("remote smoke blocked: preflight changed after review")
    m=re.search(r"(?im)^Review-Base:\s*(\S+)\s*$",text)
    if not m or m.group(1)!=receipt.get("review_base"): raise SystemExit("remote smoke blocked: Review-Base authority mismatch")
    scope={"cr_number":cr,"title":item.get("title"),"status":item.get("status"),"source_authority":item.get("source_authority",[]),"notes":item.get("notes",""),
           "tracker_source":"issues/change-requests.json","tracker_sha256":sha(tracker_data),"record_sha256":sha(cj(item).encode()),
           "preflight_source":pf.relative_to(root).as_posix(),"preflight_sha256":sha(pf_data),"review_base":m.group(1),
           "operator_authorized_gate_change":bool(re.search(r"(?im)^Operator-Authorized-Gate-Change:\s*YES\s*$",text))}
    private=receipt.get("scope_private_source")
    if private:
        if not isinstance(private,str): raise SystemExit("remote smoke blocked: malformed private scope source")
        data=bound(root,private).read_bytes()
        if sha(data)!=receipt.get("scope_private_sha256"): raise SystemExit("remote smoke blocked: private scope changed after review")
        scope["private_scope"]={"source":private,"sha256":sha(data),"content":data.decode("utf-8")}
    if sha(cj(scope).encode())!=receipt.get("scope_manifest_hash"): raise SystemExit("remote smoke blocked: CR/preflight/scope identity changed")
    return scope

def validate_requirement_authority(root:Path,receipt:dict)->None:
    sources=receipt.get("requirement_sources")
    excerpts=receipt.get("requirement_excerpt_bindings")
    if not isinstance(sources,list) or not sources or not isinstance(excerpts,list) or not excerpts:
        raise SystemExit("remote smoke blocked: v4 requirement authority binding is incomplete")
    seen={}; canonical=[]
    for rec in sources:
        if not isinstance(rec,dict) or not isinstance(rec.get("source"),str) or not re.fullmatch(r"[0-9a-f]{64}",str(rec.get("sha256",""))):
            raise SystemExit("remote smoke blocked: malformed requirement source binding")
        src=rec["source"]
        if src in seen: raise SystemExit("remote smoke blocked: duplicate requirement source binding")
        data=bound(root,src).read_bytes(); digest=sha(data)
        if digest!=rec["sha256"]: raise SystemExit("remote smoke blocked: requirement source changed after review")
        seen[src]=data; canonical.append({"source":src,"sha256":digest})
    ecanon=[]
    ids=set()
    for rec in excerpts:
        if not isinstance(rec,dict): raise SystemExit("remote smoke blocked: malformed requirement excerpt binding")
        rid,src=rec.get("requirement_id"),rec.get("source")
        if not isinstance(rid,str) or rid in ids or src not in seen: raise SystemExit("remote smoke blocked: invalid requirement excerpt identity")
        ids.add(rid)
        try:start,end=int(rec["start"]),int(rec["end"])
        except Exception as exc: raise SystemExit("remote smoke blocked: malformed requirement range") from exc
        lines=seen[src].decode("utf-8").splitlines()
        if start<1 or end<start or end>len(lines): raise SystemExit("remote smoke blocked: requirement range no longer valid")
        excerpt="\n".join(lines[start-1:end])
        if sha(excerpt.encode())!=rec.get("excerpt_sha256"): raise SystemExit("remote smoke blocked: requirement excerpt identity mismatch")
        ecanon.append({"requirement_id":rid,"source":src,"start":start,"end":end,"excerpt_sha256":rec["excerpt_sha256"]})
    computed=sha(cj({"sources":canonical,"excerpts":sorted(ecanon,key=lambda x:x["requirement_id"])}).encode())
    if computed!=receipt.get("requirements_manifest_hash"): raise SystemExit("remote smoke blocked: requirement manifest identity mismatch")

def validate_code_receipt(root:Path,receipt:dict)->str:
    if receipt.get("project_id")!=PROJECT_ID or receipt.get("review_protocol_version")!=PROTOCOL_VERSION or receipt.get("schema_version")!=PROTOCOL_VERSION:
        raise SystemExit("remote smoke blocked: legacy or foreign CODE PASS receipt")
    if receipt.get("verdict")!="PASS" or receipt.get("review_complete") is not True:
        raise SystemExit("remote smoke blocked: reviewer verdict is not PASS")
    if receipt.get("review_profile_hash")!=profile_hash(root): raise SystemExit("remote smoke blocked: unaccepted review profile")
    cr=receipt.get("cr_number")
    if not isinstance(cr,str) or not cr: raise SystemExit("remote smoke blocked: CODE receipt lacks CR")
    validate_requirement_authority(root,receipt); active_scope(root,cr,receipt)
    review_map = receipt.get("review_map_source")
    if not isinstance(review_map, str) or not review_map:
        raise SystemExit("remote smoke blocked: CODE receipt lacks review-map authority")
    if sha(bound(root, review_map).read_bytes()) != receipt.get("review_map_sha256"):
        raise SystemExit("remote smoke blocked: review map changed after review")
    snapshot=str(receipt.get("snapshot_id","")); m=re.fullmatch(r"git:([0-9a-f]{40})\.\.([0-9a-f]{40}):sha256:([0-9a-f]{64})",snapshot)
    if not m: raise SystemExit("remote smoke blocked: CODE PASS snapshot is malformed")
    base,head,digest=m.groups(); current=run_git(root,"rev-parse","HEAD")
    if current!=head: raise SystemExit("remote smoke blocked: CODE PASS does not match current commit")
    diff=subprocess.run(["git","diff","--no-ext-diff","--unified=80",base,head],cwd=root,check=True,capture_output=True).stdout
    if sha(diff)!=digest: raise SystemExit("remote smoke blocked: CODE PASS diff identity mismatch")
    published=run_git(root,"ls-remote","--heads","origin","main").split()
    if len(published)!=2 or published[0]!=head: raise SystemExit("remote smoke blocked: reviewed commit is not published exactly on origin/main")
    return head
def _bootstrap_profile_hash() -> str:
    profile={"schema":2,"model":"nvidia/nemotron-3-ultra-550b-a55b","reasoning_effort":"high",
             "reasoning_budget":10240,"max_tokens":16384,"force_nonempty_content":True,"response_format":"json_object"}
    return sha(cj(profile).encode())

def _parse_changes(payload:bytes)->list[dict[str,str]]:
    fields=payload.split(b"\0")
    if fields and not fields[-1]: fields.pop()
    out=[]; i=0
    while i<len(fields):
        status=fields[i].decode("ascii"); i+=1
        if status.startswith(("R","C")):
            old=fields[i].decode("utf-8","surrogateescape"); new=fields[i+1].decode("utf-8","surrogateescape"); i+=2
        else:
            path=fields[i].decode("utf-8","surrogateescape"); i+=1; old=path; new=path
            if status.startswith("A"): old=""
            if status.startswith("D"): new=""
        out.append({"status":status,"base_path":old,"head_path":new})
    return out

def _bootstrap_manifest(root:Path,base:str,head:str)->list[dict]:
    raw=subprocess.run(["git","diff","--name-status","-z","--find-renames","--find-copies",base,head],cwd=root,check=True,capture_output=True).stdout
    manifest=[]
    for ch in _parse_changes(raw):
        path=ch["head_path"] or ch["base_path"]; commit=head if ch["head_path"] else base
        data=subprocess.run(["git","show",f"{commit}:{path}"],cwd=root,check=True,capture_output=True).stdout
        try: line_count=len(data.decode("utf-8","strict").splitlines())
        except UnicodeDecodeError: line_count=-1
        manifest.append({**ch,"path":path,"commit":commit,"sha256":sha(data),"bytes":len(data),"line_count":line_count})
    return manifest

def validate_bootstrap_receipt(root:Path,receipt:dict,cr:str,published_ref:str)->str:
    if receipt.get("project_id")!=PROJECT_ID or receipt.get("review_protocol_version")!=PROTOCOL_VERSION or receipt.get("bootstrap_schema")!=2:
        raise SystemExit("remote smoke blocked: legacy/foreign bootstrap receipt")
    if receipt.get("verdict")!="PASS" or receipt.get("review_complete") is not True or receipt.get("cr_number")!=cr:
        raise SystemExit("remote smoke blocked: bootstrap PASS is not authorized for this CR")
    if receipt.get("bootstrap_profile_hash") != _bootstrap_profile_hash():
        raise SystemExit("remote smoke blocked: bootstrap review profile mismatch")
    bootstrap=bound(root,"tools/reviewer/legacy_bootstrap_gate.py")
    if sha(bootstrap.read_bytes()) != receipt.get("bootstrap_implementation_sha256"):
        raise SystemExit("remote smoke blocked: bootstrap implementation changed after review")

    tracker=bound(root,"issues/change-requests.json"); tracker_data=tracker.read_bytes(); tracker_payload=json.loads(tracker_data)
    matches=[x for x in tracker_payload.get("change_requests",[]) if isinstance(x,dict) and x.get("cr_number")==cr]
    if len(matches)!=1 or matches[0].get("status")!="in_progress":
        raise SystemExit("remote smoke blocked: bootstrap CR is not uniquely active")
    if sha(tracker_data)!=receipt.get("tracker_sha256"):
        raise SystemExit("remote smoke blocked: bootstrap CR tracker changed after review")
    pf=bound(root,f"design/cr-preflight/{cr}.md"); pf_data=pf.read_bytes(); text=pf_data.decode("utf-8")
    if sha(pf_data)!=receipt.get("preflight_sha256") or not re.search(r"(?im)^Operator-Authorized-Gate-Change:\s*YES\s*$",text):
        raise SystemExit("remote smoke blocked: bootstrap preflight/operator authorization mismatch")
    mbase=re.search(r"(?im)^Review-Base:\s*(\S+)\s*$",text)
    if not mbase or mbase.group(1)!=receipt.get("review_base"):
        raise SystemExit("remote smoke blocked: bootstrap Review-Base mismatch")
    scope={"record":matches[0],"tracker_sha256":sha(tracker_data),"preflight_sha256":sha(pf_data),"review_base":mbase.group(1)}
    if sha(cj(scope).encode())!=receipt.get("bootstrap_scope_hash"):
        raise SystemExit("remote smoke blocked: bootstrap scope identity changed")

    sources=receipt.get("requirement_sources")
    if not isinstance(sources,list) or not sources:
        raise SystemExit("remote smoke blocked: bootstrap authority missing")
    canonical=[]; seen=set()
    for rec in sources:
        if not isinstance(rec,dict) or not isinstance(rec.get("source"),str) or rec["source"] in seen:
            raise SystemExit("remote smoke blocked: malformed/duplicate bootstrap authority")
        seen.add(rec["source"]); data=bound(root,rec["source"]).read_bytes(); digest=sha(data)
        if digest!=rec.get("sha256"):
            raise SystemExit("remote smoke blocked: bootstrap authority changed")
        canonical.append({"source":rec["source"],"sha256":digest})
    if sha(cj(canonical).encode())!=receipt.get("requirements_manifest_hash"):
        raise SystemExit("remote smoke blocked: bootstrap authority manifest mismatch")
    if "design/review-gate.md" not in seen:
        raise SystemExit("remote smoke blocked: bootstrap lacks review-gate authority")

    head=run_git(root,"rev-parse","HEAD"); published=run_git(root,"ls-remote","--heads","origin",published_ref).split()
    if len(published)!=2 or published[0]!=head:
        raise SystemExit("remote smoke blocked: maintenance candidate is not published exactly")
    snapshot=str(receipt.get("snapshot_id","")); m=re.fullmatch(r"git:([0-9a-f]{40})\.\.([0-9a-f]{40}):sha256:([0-9a-f]{64})",snapshot)
    if not m or m.group(2)!=head:
        raise SystemExit("remote smoke blocked: bootstrap snapshot does not match current commit")
    base=m.group(1)
    if run_git(root,"rev-parse",f"{receipt.get('review_base')}^{{commit}}")!=base:
        raise SystemExit("remote smoke blocked: bootstrap base no longer matches CR Review-Base")
    diff=subprocess.run(["git","diff","--no-ext-diff","--unified=80",base,head],cwd=root,check=True,capture_output=True).stdout
    if sha(diff)!=m.group(3):
        raise SystemExit("remote smoke blocked: bootstrap diff identity mismatch")
    manifest=_bootstrap_manifest(root,base,head)
    if sha(cj(manifest).encode())!=receipt.get("packet_manifest_hash"):
        raise SystemExit("remote smoke blocked: bootstrap packet manifest identity mismatch")

    touches_bootstrap=any(x.get("path")=="tools/reviewer/legacy_bootstrap_gate.py" for x in manifest)
    rot=receipt.get("root_of_trust")
    if touches_bootstrap:
        if not isinstance(rot,dict) or rot.get("required") is not True or not isinstance(rot.get("source"),str):
            raise SystemExit("remote smoke blocked: bootstrap replacement lacks root-of-trust binding")
        rot_path=bound(root,rot["source"]); rot_data=rot_path.read_bytes()
        if sha(rot_data)!=rot.get("sha256"):
            raise SystemExit("remote smoke blocked: bootstrap root-of-trust record changed")
        record=json.loads(rot_data)
        if (record.get("project_id")!=PROJECT_ID or record.get("cr_number")!=cr or record.get("baseline_commit")!=base
                or record.get("new_bootstrap_sha256")!=sha(bootstrap.read_bytes()) or record.get("operator_approved") is not True):
            raise SystemExit("remote smoke blocked: bootstrap root-of-trust identity mismatch")
        for field in ("ledger_sha256","package_manifest_sha256","package_tree_sha256","certification_sha256"):
            if not re.fullmatch(r"[0-9a-f]{64}",str(record.get(field,""))):
                raise SystemExit("remote smoke blocked: bootstrap root-of-trust package identity malformed")
    elif isinstance(rot,dict) and rot.get("required") is True:
        raise SystemExit("remote smoke blocked: unnecessary bootstrap root-of-trust claim")
    return head
