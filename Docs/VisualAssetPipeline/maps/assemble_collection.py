#!/usr/bin/env python3
"""Assemble this documentation-only collection from explicit inspected selections.
Author: Angelis Pseftis
Never reads or writes Unreal production assets. Existing source images remain intact.
"""
from pathlib import Path
import copy
from collections import Counter
import datetime
import hashlib
import html
import json
import shutil
import struct
import subprocess

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[2]
EVIDENCE = Path("/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/map-concepts-20260906")
AUTHOR = "Angelis Pseftis"

def read(name):
    return json.loads((ROOT / name).read_text())

def save(name, obj):
    (ROOT / name).write_text(json.dumps(obj, ensure_ascii=False, indent=2) + "\n")

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def retain(src, dest):
    if dest.exists():
        if sha(src) != sha(dest):
            raise ValueError(f"Refusing to overwrite different retained pixels: {dest}")
    else:
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dest)

def assemble():
    data = read("map-concepts.json")
    choices = read("visual-inspection.json")
    maps = data["maps"]
    assert set(choices["maps"]) == {m["map_id"] for m in maps}, "Inspect all requested maps first"
    assert all(v["status"] == "RETAINED_CANDIDATE" for v in choices["maps"].values())
    snapshot = read("source-snapshot.json")
    now = datetime.datetime.now(datetime.timezone.utc).isoformat()
    receipts = {}
    source_images = []
    for file in sorted((ROOT / "receipts").glob("*.json")):
        r = json.loads(file.read_text())
        original = Path(r["generated_original"])
        assert original.is_file(), original
        image_hash = sha(original)
        evidence_copy = EVIDENCE / "generated-originals" / f'{r["map_id"].lower()}-r{r["revision"]}.png'
        retain(original, evidence_copy)
        inputs = []
        for ref in r["reference_images"]:
            src = Path(ref)
            digest = sha(src)
            local = ROOT / "reference-inputs" / (digest[:16] + src.suffix.lower())
            retain(src, local)
            inputs.append({"original_path":ref, "retained_path":str(local.relative_to(ROOT)),
                           "sha256":digest, "role":"Edit target" if str(src).startswith("/Users/") else "Supplementary visual reference"})
        r.update({"sha256":image_hash, "prompt_sha256":hashlib.sha256(r["prompt"].encode()).hexdigest(),
                  "retained_inputs":inputs, "evidence_copy":str(evidence_copy),
                  "evidence_copy_sha256":sha(evidence_copy)})
        key = (r["map_id"], r["revision"])
        if choices["maps"][r["map_id"]]["selected_revision"] == r["revision"]:
            final = ROOT / "images" / (r["map_id"].lower() + "-environment-candidate.png")
            retain(original, final)
            r["selected_image"] = str(final.relative_to(ROOT))
            r["status"] = "RETAINED_CANDIDATE"
            r["visual_inspection"] = choices["maps"][r["map_id"]]
        else:
            r["status"] = "SUPERSEDED_BY_TARGETED_CORRECTION"
        receipts[key] = r
        file.write_text(json.dumps(r, ensure_ascii=False, indent=2)+"\n")

    objects = []
    base_fields = {
        "production_asset_id":None, "canonical_name":None, "canon_status":"CANDIDATE",
        "canon_approval_evidence":None, "production_maturity":"NOT_STARTED",
        "dimensions":{"production_dimensions_cm":"TBD","gameplay_footprint":"TBD — authoritative map binding required","evidence":[]},
        "team_readability":"Neutral environment. Ownership, team and selection cues must remain distinct from decorative state.",
        "animation_rigging":"TBD per functional need; see MapEnvironmentConcepts.md shared kits.",
        "construction_behavior":"TBD; no decorative construction behavior is a gameplay fact.",
        "state_changes":"TBD — consume established state only; branch_boundary remains controlling.",
        "damage_destruction":"TBD — no inferred destructibility; MAP-DEC-004 applies to Wells.",
        "vfx_requirements":"Restrained; preserve ground/selection readability and fog knowledge.",
        "audio_interfaces":"TBD — proposed kit ambience and interaction cues require existing state binding.",
        "collision_navigation":"TBD — cosmetic geometry must not alter authoritative passability, traces or navigation.",
        "nanite_lod":"TBD against current ArtDirection and measured target-platform constraints.",
        "texture_material_budget":"TBD — no numeric production allocation made in this concept pass.",
        "rts_readability_status":"NOT_EVALUATED_IN_GAME",
        "performance_validation_status":"NOT_EVALUATED",
        "future_unreal_destination":"TBD — later authorized integration task",
        "provenance_rights":{"status":"PRODUCTION_REVIEW_PENDING","recorded_direction_author":AUTHOR,
                             "generator":"Built-in image generation tool; backend model unspecified",
                             "authority":"Docs/Archive/AssetRegister.md",
                             "restriction":"Derived reference; no independent legal rights determination or owner canon acceptance."},
        "open_decisions":["MAP-DEC-001","MAP-DEC-006","MAP-DEC-007"],
        "visual_use_status":"CANDIDATE_FOR_OWNER_REVIEW",
        "replacement_assessment":"SUPPLEMENT_EXISTING_REFERENCES"
    }
    old = read("../concept-register.json")["concept_objects"]
    old_ids = {o["concept_id"] for o in old}
    source_refs = {}
    art_commit = subprocess.check_output(["git","-C",str(REPO),"log","-1","--format=%H","--","Docs/VisualAssetPipeline/maps/images"],text=True).strip() or None
    for m in maps:
        chosen = choices["maps"][m["map_id"]]
        r = receipts[(m["map_id"], chosen["selected_revision"])]
        final = ROOT / r["selected_image"]
        width,height = struct.unpack(">II",final.read_bytes()[16:24])
        src_id = "EBS-SRC-" + r["sha256"][:16]
        source_refs[m["map_id"]] = {
            "source_id":src_id, "source_repository_path":str(final.relative_to(REPO)),
            "source_filename":final.name, "source_url":None,
            "source_commit":art_commit, "source_revision":"sha256:"+r["sha256"],
            "source_sha256":r["sha256"], "receipt_path":f'receipts/{m["map_id"].lower()}-r{r["revision"]}.json'
        }
        source_images.append({"source_id":src_id,"map_id":m["map_id"],"path":r["selected_image"],
                              "sha256":r["sha256"],"width":width,"height":height,
                              "selected_revision":r["revision"],"source_commit":art_commit,"receipt":source_refs[m["map_id"]]["receipt_path"]})
        req = f'SPEC-MSN-{int(m["map_id"][1:]):03d}' if m["map_id"].startswith("M") else {
            "SK01":"SPEC-SKM-011","SK02":"SPEC-SKM-012","SK03":"SPEC-SKM-013"}[m["map_id"]]
        mapping = {"status":"SUPPORTED_LOCATION_CONTEXT","entity_id":None,"map_id":m["map_id"],
                   "display_name":m["place"], "requirement_context":req,
                   "evidence":[{"path":"Docs/MapConcepts.md","locator":m["map_id"]+" story-to-place table",
                                "sha256":next(x["sha256"] for x in snapshot["authority_files"] if x["path"]=="Docs/MapConcepts.md")},
                               {"path":"Docs/Requirements.md","locator":req,
                                "sha256":next(x["sha256"] for x in snapshot["authority_files"] if x["path"]=="Docs/Requirements.md")}],
                   "boundary":"Location/contract association only; no actor, placement or executable binding asserted."}
        binding = next((x for x in snapshot.get("registered_campaign_maps",[]) if x["mission_id"]==m["map_id"]), None)
        if binding:
            mapping["registered_map_id"] = binding["map_id"]
            mapping["evidence"].append({"path":binding["path"],"locator":"map_id="+binding["map_id"],"sha256":binding["sha256"]})
        region = ("Lume Reach civic" if m["map_id"] in ["M03","M06","M09","M10","M11","M12"] else
                  "Kharuun mineral/ecology" if m["map_id"] in ["M02","M04","M07"] else
                  "Crownfall / Confluence" if m["map_id"] in ["M08","M13","M14","M15","SK02","SK03"] else
                  "Glass Scar" if m["map_id"] in ["M01","SK01"] else "Neutral Line of Parity")
        entries = [{"concept_id":m["concept_id"],"display_name":m["place"],"brief":m["scene"],
                    "object_locator":"Upper environment composition", "object_type":"ENV","classification":"environment_composition"}]
        entries += [dict(d, object_type="PRP",classification="detail_design_or_assembly") for d in m["details"]]
        for e in entries:
            obj = copy.deepcopy(base_fields)
            ref = dict(source_refs[m["map_id"]],locator=e["object_locator"],bbox_normalized=None,
                       locator_precision="Textual original-image region; no exact cutout or scale claim")
            obj.update({"concept_id":e["concept_id"],"display_name":e["display_name"],"source_refs":[ref],
                        "domain":"WRL","regional_grammar":region,"object_type":e["object_type"],
                        "classification":e["classification"],"map_id":m["map_id"],"gameplay_mapping":copy.deepcopy(mapping),
                        "visual_description":e["brief"],
                        "silhouette_requirements":"Preserve reviewed source design; quiet route massing and distinct usable-space edges.",
                        "material_language":"Candidate regional grammar; see retained prompt and source image.",
                        "dependencies":[m["concept_id"]] if e["object_type"]!="ENV" else [],
                        "branch_boundary":m["branch_boundary"],"notes":"Original concepts remain retained. This new visual interpretation is not owner approved.",
                        "visual_review":{"method":"Direct full-sheet visual inspection","date":choices["date"],
                                         "boundary":"Candidate identification only; no rendered Unreal evidence.",
                                         "observation":chosen["observation"]}})
            if e["object_type"]!="ENV":
                obj["gameplay_mapping"]["status"] = "CONTEXT_ONLY_NOT_ENTITY_BOUND"
            objects.append(obj)
    # Child IDs are deliberately specified in the reviewed manifest, not allocated on rebuild.
    by_id = {o["concept_id"]:o for o in objects}
    for c in choices.get("subobjects",[]):
        parent = by_id[c["parent_concept_id"]]
        obj = copy.deepcopy(parent)
        obj.update({"concept_id":c["concept_id"],"display_name":c["display_name"],
                    "classification":c.get("classification","assembly_component_design"),"object_type":c.get("object_type","PRP"),"visual_description":c["observation"],
                    "parent_concept_id":c["parent_concept_id"],"dependencies":[c["parent_concept_id"]],
                    "notes":"Individually tracked reusable component, not an additional rendered panel or a repeated-instance count."})
        obj["source_refs"][0]["locator"] += " — " + c["locator"]
        obj["gameplay_mapping"]["status"] = "CONTEXT_ONLY_NOT_ENTITY_BOUND"
        objects.append(obj)
    ids = [o["concept_id"] for o in objects]
    assert len(ids)==len(set(ids)), "Duplicate new IDs"
    assert not old_ids.intersection(ids), "Collision with existing register"
    for o in objects:
        if "Matter" in o["display_name"]:
            o["dependencies"].append("EBS-CON-WRL-PRP-012")
        if "Well" in o["display_name"]:
            o["dependencies"].append("EBS-CON-FWL-SYS-007")
    register = {"author":AUTHOR,"creator":AUTHOR,"schema_version":1,"created":"2026-09-06",
                "authority":"Scoped candidate object supplement; existing register and game authorities remain unchanged.",
                "concept_objects":objects}
    save("concept-object-register.json",register)
    save("source-images.json",{"author":AUTHOR,"creator":AUTHOR,"source_images":source_images})
    counts = {"campaign_sheets":15,"skirmish_sheets":3,"selected_source_images":len(source_images),
              "detail_panels":sum(len(m["details"]) for m in maps),
              "component_design_records":sum(o["classification"]=="assembly_component_design" for o in objects),"scene_landmark_records":sum(o["classification"]=="scene_landmark_design" for o in objects),"concept_object_records":len(objects),
              "candidate_records":len(objects),"canon_approved_records":0,
              "not_started_records":len(objects),"supported_location_records":18,
              "entity_bound_detail_records":0,"context_only_detail_and_component_records":len(objects)-18}
    save("coverage.json",{"author":AUTHOR,"creator":AUTHOR,"assembled_at":now,"counts":counts,
         "by_domain":dict(Counter(o["domain"] for o in objects)),"by_type":dict(Counter(o["object_type"] for o in objects)),"by_regional_grammar":dict(Counter(o["regional_grammar"] for o in objects)),
         "evidence_root":str(EVIDENCE),"boundary":"All counts are scoped to this collection, not a replacement for repository inventory.",
         "not_covered":["Individual campaign branch and ending state sheets","Conquest sector concepts",
                        "Team/FFA map-format variants","Orthographic/scale/footprint production packages",
                        "RTS camera validation","Unreal implementation","Owner acceptance"]})
    cards = []
    for m,s in zip(maps,source_images):
        esc = html.escape
        group = "campaign" if m["map_id"].startswith("M") else "skirmish"
        labels = ", ".join(d["display_name"] for d in m["details"])
        cards.append(f'<article id="{m["map_id"]}" data-group="{group}" data-search="{esc((m["map_id"]+" "+m["place"]+" "+m["mission_title"]).lower())}">'
          f'<a class="art" href="{s["path"]}" aria-label="Open {esc(m["place"])} at full resolution">'
          f'<img loading="lazy" src="{s["path"]}" alt="{esc(m["place"])} environment concept with {esc(labels)} detail studies"></a>'
          f'<div class="copy"><div class="eyebrow">{m["map_id"]} · {group} · candidate</div>'
          f'<h2>{esc(m["place"])}</h2><p class="mission">{esc(m["mission_title"])}</p>'
          f'<p>{esc(m["scene"])}</p><p><strong>Detail studies:</strong> {esc(labels)}.</p>'
          f'<details><summary>Context and review limits</summary><p>{esc(m["branch_boundary"])}</p>'
          f'<p>{esc(choices["maps"][m["map_id"]]["observation"])}</p>'
          f'<p>No Unreal layout, scale, collision, camera or performance acceptance is inferred.</p></details>'
          f'<a class="file" href="{s["path"]}" download>Save full-resolution sheet</a></div></article>')
    gallery = """<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><meta name="author" content="Angelis Pseftis">
<title>Echoes — campaign and skirmish environments</title>
<style>
:root{color-scheme:dark;--bg:#101517;--card:#192124;--text:#edf0ea;--muted:#b9c6c8;--line:#39464b;--accent:#eac896}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:16px/1.6 system-ui,sans-serif}
header,main,footer{max-width:1600px;margin:auto;padding:32px}header{padding-top:56px}h1{font:500 clamp(34px,5vw,62px)/1.08 Georgia,serif;max-width:950px;margin:16px 0}header p{max-width:850px;color:var(--muted)}
.eyebrow{color:var(--accent);letter-spacing:.1em;text-transform:uppercase;font-size:12px;font-weight:650}
.tools{display:flex;flex-wrap:wrap;gap:10px;align-items:center;margin-top:24px}button,input{font:inherit;background:#202b2e;color:var(--text);border:1px solid #637378;border-radius:7px;padding:9px 15px}
button{cursor:pointer}button[aria-pressed=true]{background:var(--accent);color:#172024}input{min-width:230px;flex:1;max-width:380px}
a{color:#bce7eb}a:focus-visible,button:focus-visible,input:focus-visible,summary:focus-visible{outline:3px solid #ffd492;outline-offset:4px}
#count{color:var(--muted)}main{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:28px;padding-top:0}
article{background:var(--card);border:1px solid var(--line);border-radius:10px;overflow:hidden}article[hidden]{display:none}
.art{display:block;background:#080c0e}.art img{display:block;width:100%;height:auto}.copy{padding:24px}
h2{font:500 29px/1.2 Georgia,serif;margin:8px 0}.mission{color:var(--muted);margin:4px 0 18px}.copy p{font-size:15px}
details{border-top:1px solid var(--line);padding-top:13px;margin-top:18px}summary{cursor:pointer;color:var(--accent)}
.file{display:inline-block;margin-top:18px;font-size:14px}footer{color:var(--muted);border-top:1px solid var(--line)}
@media(max-width:850px){main{grid-template-columns:1fr}header,main,footer{padding-left:18px;padding-right:18px}}
@media print{body{background:white;color:black}header .tools{display:none}main{display:block}article{break-inside:avoid;margin-bottom:24px;background:white;color:black}a{color:black}}
</style></head><body><header><div class="eyebrow">Echoes of the Broken Sun · Environment studies</div>
<h1>Soryn, beyond the bases</h1><p>Fifteen campaign locations and three skirmish battlefields, with terrain, civic infrastructure, geological features and neutral environmental details. Select a sheet to inspect the original pixels.</p>
<p><strong>18 sheets · 54 detail panels</strong> · Candidate artwork for review. These compositions preserve story context; they are not executable navigation plans or approved production models.</p>
<div class="tools" role="group" aria-label="Filter map collection"><button data-filter="all" aria-pressed="true">All maps</button><button data-filter="campaign" aria-pressed="false">Campaign</button><button data-filter="skirmish" aria-pressed="false">Skirmish</button>
<input id="search" type="search" aria-label="Search map names" placeholder="Find a map or place"><span id="count" aria-live="polite">18 maps</span></div></header><main>"""
    gallery += "\n".join(cards)
    gallery += """</main><footer><p>Author and owner: Angelis Pseftis. Original concepts remain preserved. <a href="MapEnvironmentConcepts.md">Production brief and remaining variants</a> · <a href="concept-object-register.json">Object register</a> · <a href="coverage.json">Coverage</a></p><p>This gallery uses local images and needs no server or network connection.</p></footer>
<script>
let filter='all';
const cards=[...document.querySelectorAll('article')], search=document.getElementById('search');
function update(){let n=0;for(const c of cards){const show=(filter==='all'||c.dataset.group===filter)&&c.dataset.search.includes(search.value.toLowerCase().trim());c.hidden=!show;if(show)n++;}document.getElementById('count').textContent=n+' map'+(n===1?'':'s');}
document.querySelectorAll('[data-filter]').forEach(b=>b.addEventListener('click',()=>{filter=b.dataset.filter;document.querySelectorAll('[data-filter]').forEach(x=>x.setAttribute('aria-pressed',String(x===b)));update();}));search.addEventListener('input',update);
</script></body></html>"""
    (ROOT / "review.html").write_text(gallery)
    print(json.dumps(counts))

if __name__ == "__main__":
    assemble()
