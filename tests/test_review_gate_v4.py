# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

from __future__ import annotations

import hashlib,hmac,importlib.util,json,os,subprocess,tempfile,unittest,sys
from pathlib import Path

PACKAGE=Path(__file__).resolve().parents[1]
HARNESS=PACKAGE/'tools/harness'; sys.path.insert(0,str(HARNESS))
GATE=PACKAGE/'tools/reviewer/review_gate.py'
spec=importlib.util.spec_from_file_location('wz_gate_v4',GATE); gate=importlib.util.module_from_spec(spec); sys.modules[spec.name]=gate; spec.loader.exec_module(gate)
from evidence_schema_v2 import EXPECTED_FUSE_MANIFESTS, deterministic_tree_index, semantic_result_signature, validate_lane_result
AUTH_PATH=HARNESS/'review_authority_v4.py'
auth_spec=importlib.util.spec_from_file_location('wz_auth_v4',AUTH_PATH); auth=importlib.util.module_from_spec(auth_spec); sys.modules[auth_spec.name]=auth; auth_spec.loader.exec_module(auth)
LANES=['linux-x64','linux-x64-latest','linux-x64-24','linux-x64-26','linux-arm64-22','linux-arm64','linux-arm64-26','windows-latest','windows-server-2022','windows-server-2025','windows-server-2025-vs2026','windows-arm-11','windows-arm-11-vs2026','macos-arm64-latest','macos-arm64','macos-arm64-14','macos-intel-x64','macos-intel-26','macos-arm64-26','macos-xcode-27']
LABELS=['ubuntu-22.04','ubuntu-latest','ubuntu-24.04','ubuntu-26.04','ubuntu-22.04-arm','ubuntu-24.04-arm','ubuntu-26.04-arm','windows-latest','windows-2022','windows-2025','windows-2025-vs2026','windows-11-arm','windows-11-vs2026-arm','macos-latest','macos-15','macos-14','macos-15-intel','macos-26-intel','macos-26','xcode-27']

def run(root,*a): return subprocess.run(['git',*a],cwd=root,check=True,capture_output=True,text=True).stdout.strip()
def shab(b): return hashlib.sha256(b).hexdigest()
def cj(v): return json.dumps(v,ensure_ascii=True,separators=(',',':'),sort_keys=True)

class Response:
 def __init__(self,status,body,headers=None): self.status=status; self.body=body; self.headers=headers or {}
 def getcode(self): return self.status
 def read(self): return self.body
 def __enter__(self): return self
 def __exit__(self,*a): return False

class GateV4Tests(unittest.TestCase):
 def repo(self,two_hunks=False):
  td=tempfile.TemporaryDirectory(); root=Path(td.name); run(root,'init'); run(root,'config','user.email','test@example.com'); run(root,'config','user.name','Test')
  (root/'design/cr-preflight').mkdir(parents=True); (root/'src').mkdir(); (root/'docs').mkdir(); (root/'tools/reviewer').mkdir(parents=True); (root/'test-artefacts/reviewer').mkdir(parents=True)
  req='Requirement one: changed function must return 2.\nRequirement two: same source second excerpt.\n'
  (root/'design/review-gate.md').write_text(req)
  tracker={'change_requests':[{'cr_number':'CR-0001','status':'in_progress','title':'test','notes':'scope','source_authority':['design/review-gate.md']}]}
  (root/'issues').mkdir(); (root/'issues/change-requests.json').write_text(json.dumps(tracker))
  (root/'design/cr-preflight/CR-0001.md').write_text('Status: APPROVED_FOR_IMPLEMENTATION\nReview-Base: BASE\nOperator-Authorized-Gate-Change: YES\n## Zero-Gap Exit Scan\nzero\n')
  extra=('\n'*20)+'int g(void) { return 3; }\n' if two_hunks else ''
  base_text='int f(void) { return 1; }\n'+extra
  (root/'src/a.c').write_text(base_text); (root/'src/unchanged.c').write_text('int helper(void) { return 9; }\n'); (root/'docs/a.md').write_text('Documented behavior: f returns 2.\n'); (root/'tools/reviewer/review_gate.py').write_text('# gate baseline\n')
  (root/'.gitignore').write_text('review-map.json\ntest-artefacts/\n')
  run(root,'add','.'); run(root,'commit','-m','base'); run(root,'tag','BASE'); base=run(root,'rev-parse','HEAD')
  extra=('\n'*20)+'int g(void) { return 4; }\n' if two_hunks else ''
  head_text='int f(void) { return 2; }\n'+extra
  (root/'src/a.c').write_text(head_text); run(root,'add','src/a.c'); run(root,'commit','-m','head'); head=run(root,'rev-parse','HEAD')
  data=(root/'src/a.c').read_bytes(); rdata=(root/'design/review-gate.md').read_bytes()
  review_map={'links':[
   {'id':'a1','requirement':{'source':'design/review-gate.md','sha256':shab(rdata),'start':1,'end':1},'related':{'path':'src/a.c','sha256':shab(data),'start':1,'end':1}},
   {'id':'a2','requirement':{'source':'design/review-gate.md','sha256':shab(rdata),'start':2,'end':2},'related':{'path':'src/a.c','sha256':shab(data),'start':1,'end':1}},]}
  (root/'review-map.json').write_text(json.dumps(review_map)); return td,root,base,head

 def packet(self):
  td,root,base,head=self.repo(); scope=gate.load_cr_scope(root,'CR-0001',None); packet,reqs=gate.linked_packet(root,'CODE',gate.load_review_map(root,'review-map.json'),['design/review-gate.md'],scope,base,head); return td,root,base,head,scope,packet,reqs

 def test_linked_code_original_locations_same_source_requirements_and_canonical_snapshot(self):
  td,root,base,head,scope,packet,_=self.packet(); self.addCleanup(td.cleanup)
  self.assertTrue(packet.snapshot_id.startswith(f'git:{base}..{head}:sha256:')); self.assertTrue(gate.candidate_location_valid(packet,'src/a.c:1'))
  self.assertEqual(len(packet.requirement_index),2); self.assertEqual(len(gate._requirements_source_bindings(packet)),1); self.assertFalse(packet.insufficient_evidence)

 def test_hunk_level_coverage_catches_second_unmapped_change(self):
  td,root,base,head=self.repo(two_hunks=True); self.addCleanup(td.cleanup); scope=gate.load_cr_scope(root,'CR-0001',None)
  packet,_=gate.linked_packet(root,'CODE',gate.load_review_map(root,'review-map.json'),['design/review-gate.md'],scope,base,head)
  self.assertTrue(any('omits changed hunk' in x for x in packet.insufficient_evidence))

 def test_deleted_file_can_be_mapped_from_base(self):
  td,root,base,head=self.repo(); self.addCleanup(td.cleanup)
  # Create a fresh delete commit from current head and move Review-Base to that head.
  (root/'design/cr-preflight/CR-0001.md').write_text('Status: APPROVED_FOR_IMPLEMENTATION\nReview-Base: DELETEBASE\nOperator-Authorized-Gate-Change: YES\n## Zero-Gap Exit Scan\nzero\n')
  run(root,'add','design/cr-preflight/CR-0001.md'); run(root,'commit','-m','pre-delete'); base2=run(root,'rev-parse','HEAD'); run(root,'tag','DELETEBASE')
  old=(root/'src/a.c').read_bytes(); (root/'src/a.c').unlink(); run(root,'add','-u'); run(root,'commit','-m','delete'); head2=run(root,'rev-parse','HEAD')
  r=(root/'design/review-gate.md').read_bytes(); m={'links':[{'id':'del','requirement':{'source':'design/review-gate.md','sha256':shab(r),'start':1,'end':1},'related':{'path':'src/a.c','snapshot':'base','sha256':shab(old),'start':1,'end':1}},{'id':'auth2','requirement':{'source':'design/review-gate.md','sha256':shab(r),'start':2,'end':2},'related':{'path':'src/a.c','snapshot':'base','sha256':shab(old),'start':1,'end':1}}]}
  (root/'review-map.json').write_text(json.dumps(m)); scope=gate.load_cr_scope(root,'CR-0001',None); packet,_=gate.linked_packet(root,'CODE',m['links'],['design/review-gate.md'],scope,base2,head2)
  self.assertFalse(packet.insufficient_evidence); self.assertIn('src/a.c',packet.deleted_paths); self.assertTrue(gate.candidate_location_valid(packet,'src/a.c:1'))

 def test_lazy_unchanged_path_materializes_for_path_and_symbol(self):
  td,root,base,head,scope,packet,_=self.packet(); self.addCleanup(td.cleanup)
  self.assertTrue(packet.source_index['src/unchanged.c']['lazy'])
  r=gate.resolve_context_request(root,packet,{'type':'PATH','path':'src/unchanged.c'}); self.assertEqual(r['status'],'RESOLVED'); self.assertIn('helper',r['content'])
  r=gate.resolve_context_request(root,packet,{'type':'SYMBOL','symbol':'helper','path':'src/unchanged.c'}); self.assertEqual(r['status'],'RESOLVED')

 def test_malformed_candidate_is_protocol_gap(self):
  td,root,base,head,scope,packet,_=self.packet(); self.addCleanup(td.cleanup); tm=gate.Telemetry('CODE',packet.snapshot_id)
  accepted,harmless,gaps=gate.deterministic_filter(root,[{'candidate_id':'x'}],packet,tm,'CODE',set()); self.assertFalse(accepted); self.assertFalse(harmless); self.assertTrue(gaps)

 def test_discovery_requires_explicit_completion_and_phase(self):
  self.assertTrue(gate.discovery_schema_errors({'candidates':[],'uncertainties':[],'evidence_requests':[]},'CODE-DISCOVERY'))

 def test_decision_algebra_strict(self):
  base={'review_complete':True,'new_candidates':[]}
  rejected={**base,'decisions':[{'candidate_id':'C','decision':'REJECTED','evidence_conclusion':'INCONCLUSIVE','reason':'x','proof_refs':['EV-1'],'confirmed_severity':None,'authority_conflict':False,'negative_check':'x'}]}
  nonblock={**base,'decisions':[{'candidate_id':'C','decision':'NON_BLOCKING','evidence_conclusion':'COMPLIANCE','reason':'x','proof_refs':['EV-1'],'confirmed_severity':None,'authority_conflict':False,'negative_check':'x'}]}
  self.assertTrue(gate.decision_schema_errors(rejected,{'C'},{'EV-1'})); self.assertTrue(gate.decision_schema_errors(nonblock,{'C'},{'EV-1'}))

 def test_discovery_evidence_request_reruns_same_phase(self):
  td,root,base,head,scope,packet,_=self.packet(); self.addCleanup(td.cleanup)
  rid=next(iter(packet.requirement_index)); seq=[{'pass':'CODE-DISCOVERY','review_complete':True,'candidates':[],'uncertainties':['need helper'],'evidence_requests':[{'type':'PATH','path':'src/unchanged.c'}]}, {'pass':'CODE-DISCOVERY','review_complete':True,'candidates':[],'uncertainties':[],'evidence_requests':[]}]
  class C:
   def request(self,*a,**k): return seq.pop(0)
  tm=gate.Telemetry('CODE',packet.snapshot_id); out=gate._run_discovery_with_expansion(C(),root,packet,tm,gate.ReviewDeadline(60),'s','p','CODE-DISCOVERY'); self.assertEqual(out['candidates'],[]); self.assertEqual(len(seq),0); self.assertEqual(tm.context_request_resolved_count,1)

 def test_global_lock_second_acquire_fails_and_live_lock_not_time_stale(self):
  td,root,base,head,scope,packet,_=self.packet(); self.addCleanup(td.cleanup); lock=gate.acquire_review_lock(root,packet,'CODE','CR-0001',3600); self.addCleanup(lambda:gate.release_review_lock(lock))
  with self.assertRaises(gate.ReviewError): gate.acquire_review_lock(root,packet,'CODE','CR-0001',3600)
  rec=json.loads(lock.read_text()); rec['expires_epoch']=0; self.assertFalse(gate._lock_stale(lock,rec))

 def test_reasoning_profile_answer_reserve_and_force_nonempty(self):
  captured=[]
  def opener(req,timeout):
   captured.append(json.loads(req.data)); content={'review_complete':True,'decisions':[],'new_candidates':[]}; env={'choices':[{'finish_reason':'stop','message':{'content':json.dumps(content)}}],'usage':{}}; return Response(200,json.dumps(env).encode())
  c=gate.CodeReviewerClient('x',opener); tm=gate.Telemetry('CODE','x'); c.request('s','u',tm,phase='FALSIFICATION')
  body=captured[0]; self.assertEqual(body['reasoning_effort'],'high'); self.assertNotIn('reasoning_budget',body); self.assertTrue(body['chat_template_kwargs']['enable_thinking']); self.assertTrue(body['chat_template_kwargs']['force_nonempty_content']); self.assertNotIn('temperature',body)

 def test_202_polls_without_duplicate_post(self):
  calls=[]
  def opener(req,timeout):
   calls.append(req.full_url)
   if req.full_url==gate.API_URL:return Response(202,b'{"requestId":"abc-1"}')
   env={'choices':[{'finish_reason':'stop','message':{'content':'{"status":"available"}'}}],'usage':{}}; return Response(200,json.dumps(env).encode())
  c=gate.CodeReviewerClient('x',opener); c._sleep=lambda *a,**k:None; tm=gate.Telemetry('DOCUMENTATION','x'); result=c.request('s','u',tm,phase='HEALTH-CHECK',deadline=gate.ReviewDeadline(60)); self.assertEqual(result['status'],'available'); self.assertEqual(tm.http_posts,1); self.assertEqual(tm.http_polls,1)

 def test_nonretryable_404_is_one_post(self):
  def opener(req,timeout): return Response(404,b'{}')
  c=gate.CodeReviewerClient('x',opener); tm=gate.Telemetry('CODE','x')
  with self.assertRaises(gate.ConfigurationError): c.request('s','u',tm,phase='CODE-DISCOVERY',deadline=gate.ReviewDeadline(60))
  self.assertEqual(tm.http_posts,1)

 def test_prior_snapshot_mismatch_fails(self):
  td,root,base,head,scope,packet,_=self.packet(); self.addCleanup(td.cleanup); data=(root/'src/a.c').read_bytes(); p=root/'prior.json'; p.write_text(json.dumps([{'id':'X','status':'DISPUTED','evidence':[{'source':'src/a.c','sha256':shab(data),'snapshot_id':'wrong','location':'src/a.c:1','claim':'x','evidence_type':'source'}]}]));
  with self.assertRaises(gate.ReviewError): gate.load_prior_findings(root,packet,'prior.json')

 def test_secret_shaped_content_is_blocked(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d); token='gh'+'p_'+'ABCDEFGHIJKLMNOPQRSTUVWXYZ123456'; (root/'x.c').write_text(f'const char*x="{token}";')
   with self.assertRaises(gate.ReviewError): gate.enforce_external_review_content_policy(root,'x.c',(root/'x.c').read_bytes())

 def test_source_line_zero_never_valid(self):
  packet=gate.ReviewPacket('x','y',[],[],source_index={'empty.txt':{'path':'empty.txt','line_count':0}}); self.assertFalse(gate.candidate_location_valid(packet,'empty.txt:1'))

 def test_profile_contains_only_supported_efforts(self):
  p=gate.review_profile(); self.assertTrue(p['force_nonempty_content']); self.assertFalse(p['json_schema_qualified'])
  for v in p['phase_profile'].values(): self.assertIn(v['reasoning_effort'],{'none','medium','high'}); self.assertLess(v['reasoning_budget'],v['max_tokens']) if v['reasoning_effort']!='none' else self.assertEqual(v['reasoning_budget'],0)

 def test_documentation_document_context_binding(self):
  td,root,base,head,scope,code_packet,_=self.packet(); self.addCleanup(td.cleanup)
  code_packet.revalidation['review_map_source']='review-map.json'; code_packet.revalidation['review_map_sha256']=shab((root/'review-map.json').read_bytes())
  (root/'test-artefacts/reviewer/code-pass.json').write_text(json.dumps(gate._receipt(root,code_packet,scope)))
  req=(root/'design/review-gate.md').read_bytes(); doc=gate.git_object(root,head,'docs/a.md'); ctx=gate.git_object(root,head,'src/unchanged.c')
  links=[
   {'id':'doc','requirement':{'source':'design/review-gate.md','sha256':shab(req),'start':1,'end':1},'related':{'path':'docs/a.md','role':'document','sha256':shab(doc),'start':1,'end':1}},
   {'id':'ctx','requirement':{'source':'design/review-gate.md','sha256':shab(req),'start':2,'end':2},'related':{'path':'src/unchanged.c','role':'context','sha256':shab(ctx),'start':1,'end':1}},]
  packet,_=gate.linked_packet(root,'DOCUMENTATION',links,['design/review-gate.md'],scope,base,head,document_paths=['docs/a.md'])
  self.assertFalse(packet.insufficient_evidence); self.assertEqual(packet.source_index['docs/a.md']['semantic_role'],'document'); self.assertEqual(packet.source_index['src/unchanged.c']['semantic_role'],'context')

 def test_normal_gate_refuses_protected_self_certification(self):
  td,root,base,head=self.repo(); self.addCleanup(td.cleanup)
  (root/'design/cr-preflight/CR-0001.md').write_text('Status: APPROVED_FOR_IMPLEMENTATION\nReview-Base: GATEBASE\nOperator-Authorized-Gate-Change: YES\n## Zero-Gap Exit Scan\nzero\n')
  run(root,'add','design/cr-preflight/CR-0001.md'); run(root,'commit','-m','gate-preflight'); base2=run(root,'rev-parse','HEAD'); run(root,'tag','GATEBASE')
  (root/'tools/reviewer/review_gate.py').write_text('# modified gate\n'); run(root,'add','tools/reviewer/review_gate.py'); run(root,'commit','-m','gate-change'); head2=run(root,'rev-parse','HEAD')
  req=(root/'design/review-gate.md').read_bytes(); src=(root/'tools/reviewer/review_gate.py').read_bytes(); links=[{'id':'gate','requirement':{'source':'design/review-gate.md','sha256':shab(req),'start':1,'end':1},'related':{'path':'tools/reviewer/review_gate.py','sha256':shab(src),'start':1,'end':1}}]
  scope=gate.load_cr_scope(root,'CR-0001',None)
  with self.assertRaisesRegex(gate.ReviewError,'BOOTSTRAP_REQUIRED'): gate.linked_packet(root,'CODE',links,['design/review-gate.md'],scope,base2,head2)

 def test_schema_failure_does_not_trigger_second_semantic_inference(self):
  class C:
   def __init__(self): self.calls=0
   def request(self,*a,**k): self.calls+=1; return {'candidates':[]}
  c=C(); tm=gate.Telemetry('CODE','x')
  with self.assertRaises(gate.OutputError): gate.request_validated(c,'s','p',tm,lambda v:gate.discovery_schema_errors(v,'CODE-DISCOVERY'),'CODE-DISCOVERY')
  self.assertEqual(c.calls,1)

 def test_proof_refs_must_be_supplied_evidence_ids(self):
  value={'review_complete':True,'new_candidates':[],'decisions':[{'candidate_id':'C','decision':'CONFIRMED','evidence_conclusion':'VIOLATION','reason':'x','proof_refs':['EV-OTHER'],'confirmed_severity':'HIGH','authority_conflict':False,'negative_check':'x'}]}
  self.assertTrue(gate.decision_schema_errors(value,{'C'},{'EV-1'})); value['decisions'][0]['proof_refs']=['EV-1']; self.assertFalse(gate.decision_schema_errors(value,{'C'},{'EV-1'}))

 def test_v4_code_receipt_authorizes_only_exact_bound_state(self):
  td,root,base,head,scope,packet,_=self.packet(); self.addCleanup(td.cleanup)
  packet.revalidation['review_map_source']='review-map.json'; packet.revalidation['review_map_sha256']=shab((root/'review-map.json').read_bytes())
  (root/'tools/reviewer/review-profile-v4.json').write_text((PACKAGE/'tools/reviewer/review-profile-v4.json').read_text())
  receipt=gate._receipt(root,packet,scope); original=auth.run_git
  def fake_run(r,*args):
   if args and args[0]=='ls-remote': return f'{head}\trefs/heads/main'
   return original(r,*args)
  auth.run_git=fake_run
  try: self.assertEqual(auth.validate_code_receipt(root,receipt),head)
  finally: auth.run_git=original
  bad=dict(receipt); bad['review_profile_hash']='0'*64
  with self.assertRaises(SystemExit): auth.validate_code_receipt(root,bad)

 def test_bootstrap_is_executable_independent_of_normal_gate(self):
  import ast
  source=(PACKAGE/'tools/reviewer/legacy_bootstrap_gate.py').read_text(); tree=ast.parse(source)
  imported=[]
  for node in ast.walk(tree):
   if isinstance(node,ast.Import): imported.extend(a.name for a in node.names)
   elif isinstance(node,ast.ImportFrom): imported.append(node.module or '')
  self.assertFalse(any('review_gate' in name for name in imported))

class EvidenceV4Tests(unittest.TestCase):
 def _fuse(self,name,selection,total):
  n=0 if total is None else total; cases=[{'name':f'{i:04x}','status':'passed'} for i in range(n)]
  return {'schema_version':1,'corpus':'Fuse Z80','commit':'e7fe9a0f3625b0aef649b114fc98323c7d241840','suite_path':'z80/tests','selection':selection,'total':n,'passed':n,'failed':0,'silent_skips':0,'known_unresolved':[],'unexpected_failures':[],'cases':cases}
 def evidence_repo(self):
  td=tempfile.TemporaryDirectory(); root=Path(td.name); run(root,'init'); run(root,'config','user.email','x@y'); run(root,'config','user.name','T'); (root/'.github/workflows').mkdir(parents=True); (root/'tools/harness').mkdir(parents=True); (root/'evidence').mkdir()
  wf='name: platform-smoke\njobs:\n  smoke:\n    strategy:\n      matrix:\n        include:\n'+''.join(f'          - id: {i}\n            label: {l}\n' for i,l in zip(LANES,LABELS)); (root/'.github/workflows/platform-smoke.yml').write_text(wf)
  baseline={'schema_version':1,'commit':'e7fe9a0f3625b0aef649b114fc98323c7d241840','case_names':[]}; (root/'tools/harness/fuse-unresolved-baseline.json').write_text(json.dumps(baseline))
  run(root,'add','.'); run(root,'commit','-m','base'); head=run(root,'rev-parse','HEAD'); workflow_blob=run(root,'rev-parse',f'{head}:.github/workflows/platform-smoke.yml')
  jobs=[]; artifacts=[]; lane_evidence=[]
  for idx,lane in enumerate(LANES):
   name=f'platform-smoke-{lane}-retry-2' if idx==0 else f'platform-smoke-{lane}'; d=root/'evidence'/name; d.mkdir(); summary={'status':'passed','missing_tools':[]}; inventory={'platform':{'system':'test','lane':lane}}; (d/'summary.json').write_text(json.dumps(summary)); (d/'inventory.json').write_text(json.dumps(inventory))
   source_files={'summary':{'path':'summary.json','bytes':(d/'summary.json').stat().st_size,'sha256':shab((d/'summary.json').read_bytes())},'inventory':{'path':'inventory.json','bytes':(d/'inventory.json').stat().st_size,'sha256':shab((d/'inventory.json').read_bytes())},'unresolved_baseline':{'path':'tools/harness/fuse-unresolved-baseline.json','sha256':shab((root/'tools/harness/fuse-unresolved-baseline.json').read_bytes()),'case_count':0}}
   fitems=[]
   for fname,(sel,total) in EXPECTED_FUSE_MANIFESTS.items():
    payload=self._fuse(fname,sel,total); fp=d/fname; fp.write_text(json.dumps(payload)); digest={'path':fname,'bytes':fp.stat().st_size,'sha256':shab(fp.read_bytes())}; source_files['fuse:'+fname]=digest; fitems.append({'name':fname,'digest':digest,'payload':payload})
   sokol={'required':lane in {'windows-latest','macos-arm64-14'},'status':'not_applicable'}
   if sokol['required']:
    sd=d/'sokol-host'; sd.mkdir(); ss={'status':'passed','missing_tools':[]}; (sd/'summary.json').write_text(json.dumps(ss)); dig={'path':'sokol-host/summary.json','bytes':(sd/'summary.json').stat().st_size,'sha256':shab((sd/'summary.json').read_bytes())}; source_files['sokol_summary']=dig; sokol={'required':True,'status':'passed','summary':ss,'summary_digest':dig}
   inspection={'lane_id':lane,'summary_status':'passed','platform':inventory['platform'],'visual_supported':False,'screenshots':[],'traces':[],'screenshot_status':'not_produced','trace_status':'not_produced'}
   result={'schema_version':2,'lane_id':lane,'summary':summary,'inventory':inventory,'fuse_manifest':fitems[0]['payload'],'fuse_manifests':fitems,'unresolved_baseline':source_files['unresolved_baseline'],'source_files':source_files,'inspection':inspection,'sokol':sokol}; validate_lane_result(result,lane,set()); (d/'result-manifest.json').write_text(json.dumps(result,sort_keys=True))
   tree,tree_hash=deterministic_tree_index(d); art={'name':name,'id':idx+1,'digest':'sha256:'+('a'*64),'size_in_bytes':sum(x['bytes'] for x in tree)}; artifacts.append(art); lane_evidence.append({'lane_id':lane,'artifact_name':name,'result_manifest_sha256':shab((d/'result-manifest.json').read_bytes()),'tree_sha256':tree_hash,'tree_file_count':len(tree),'tree_bytes':sum(x['bytes'] for x in tree),'artifact_id':art['id'],'artifact_digest':art['digest'],'artifact_size_in_bytes':art['size_in_bytes']}); jobs.append({'name':lane,'head_sha':head,'status':'completed','conclusion':'success'})
  pub={'schema_version':2,'project_id':gate.PROJECT_ID,'run_id':'7','build_id':head,'publication_id':'7','head_sha':head,'workflow_blob_sha':workflow_blob,'matrix_lane_count':20,'expected_lanes':LANES,'jobs_sha256':shab(cj(jobs).encode()),'artifacts_sha256':shab(cj(artifacts).encode()),'jobs':jobs,'artifacts':artifacts,'lane_evidence':sorted(lane_evidence,key=lambda x:x['lane_id'])}; raw=(json.dumps(pub,indent=2,sort_keys=True)+'\n').encode(); (root/'evidence/publication-manifest.json').write_bytes(raw); os.environ['WZXN_HOSTED_GATE_HMAC_KEY']='z'*40; (root/'evidence/publication-manifest.sig').write_text(hmac.new(b'z'*40,raw,hashlib.sha256).hexdigest()); return td,root,head
 def test_exact_20_retry_dir_and_signed_tree(self):
  td,root,head=self.evidence_repo(); self.addCleanup(td.cleanup); packet=gate.test_artifact_packet(root,'evidence','7',head,'7'); self.assertFalse(packet.insufficient_evidence); self.assertEqual(len(packet.manifest[0]['lane_outcomes']),20); self.assertIn('platform-smoke-linux-x64-retry-2/result-manifest.json',packet.source_index); self.assertEqual(len(packet.manifest[0]['result_groups']),2)
 def test_tree_tamper_is_fail_closed(self):
  td,root,head=self.evidence_repo(); self.addCleanup(td.cleanup); target=root/'evidence/platform-smoke-linux-x64-retry-2/summary.json'; target.write_text('{"status":"passed","missing_tools":[],"tamper":true}')
  packet=gate.test_artifact_packet(root,'evidence','7',head,'7'); self.assertTrue(packet.insufficient_evidence)
 def test_fuse_semantic_invariant_rejects_unexpected_failure(self):
  fname,(sel,total)=next(iter(EXPECTED_FUSE_MANIFESTS.items())); payload=self._fuse(fname,sel,total); payload['unexpected_failures']=['x'];
  with self.assertRaises(ValueError): validate_lane_result({'schema_version':2,'lane_id':'x','summary':{'status':'passed','missing_tools':[]},'fuse_manifests':[{'name':n,'payload':self._fuse(n,s,t)} for n,(s,t) in EXPECTED_FUSE_MANIFESTS.items() if n!=fname]+[{'name':fname,'payload':payload}],'sokol':{}},'x',set())

 def test_signed_artifact_metadata_tamper_is_fail_closed_even_when_resigned(self):
  td,root,head=self.evidence_repo(); self.addCleanup(td.cleanup); pub_path=root/'evidence/publication-manifest.json'; pub=json.loads(pub_path.read_text()); pub['artifacts'][0]['digest']='sha256:'+('b'*64); pub['artifacts_sha256']=shab(cj(pub['artifacts']).encode()); raw=(json.dumps(pub,indent=2,sort_keys=True)+'\n').encode(); pub_path.write_bytes(raw); (root/'evidence/publication-manifest.sig').write_text(hmac.new(b'z'*40,raw,hashlib.sha256).hexdigest())
  with self.assertRaises(gate.ReviewError): gate.test_artifact_packet(root,'evidence','7',head,'7')

 def test_missing_and_duplicate_transport_bundle_fail_closed(self):
  import shutil
  td,root,head=self.evidence_repo(); self.addCleanup(td.cleanup); first=root/'evidence/platform-smoke-linux-x64-retry-2'; shutil.rmtree(first); packet=gate.test_artifact_packet(root,'evidence','7',head,'7'); self.assertTrue(packet.insufficient_evidence)
  # Fresh tree with an unlisted duplicate bundle must also fail closed.
  td2,root2,head2=self.evidence_repo(); self.addCleanup(td2.cleanup); shutil.copytree(root2/'evidence/platform-smoke-linux-x64-retry-2',root2/'evidence/platform-smoke-linux-x64-duplicate'); packet2=gate.test_artifact_packet(root2,'evidence','7',head2,'7'); self.assertTrue(packet2.insufficient_evidence)

if __name__=='__main__': unittest.main()
