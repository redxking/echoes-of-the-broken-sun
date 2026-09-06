#!/usr/bin/env python3
"""Read-only audit of the map concept collection. Author: Angelis Pseftis."""
from pathlib import Path
from html.parser import HTMLParser
import hashlib
import json
import re
import sys

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[2]

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def audit():
    errors = []
    def check(ok, message):
        if not ok:
            errors.append(message)
    def load(name):
        path = ROOT / name
        check(path.is_file(), f"Missing {name}")
        return json.loads(path.read_text()) if path.is_file() else {}
    maps = load("map-concepts.json").get("maps", [])
    register = load("concept-object-register.json")
    objects = register.get("concept_objects", [])
    images = load("source-images.json").get("source_images", [])
    inspection = load("visual-inspection.json")
    coverage = load("coverage.json").get("counts", {})
    expected = {f"M{i:02d}" for i in range(1,16)} | {"SK01","SK02","SK03"}
    check({m["map_id"] for m in maps} == expected, "Map brief coverage")
    check(len(maps)==18 and len(images)==18, "Exactly 18 selected sheets")
    check({s["map_id"] for s in images} == expected, "Selected image map coverage")
    check(sum(len(m["details"]) for m in maps)==54, "Exactly 54 named detail panels")
    check(set(inspection.get("maps",{}))==expected, "Visual inspection coverage")
    ids = [o.get("concept_id") for o in objects]
    check(len(ids)==len(set(ids)), "Duplicate object IDs")
    old = json.loads((ROOT.parent/"concept-register.json").read_text())["concept_objects"]
    check(not set(ids)&{o["concept_id"] for o in old}, "Collision with existing register")
    required = {"concept_id","production_asset_id","display_name","source_refs","domain","object_type",
        "canon_status","production_maturity","gameplay_mapping","visual_description","silhouette_requirements",
        "dimensions","material_language","team_readability","animation_rigging","construction_behavior",
        "state_changes","damage_destruction","vfx_requirements","audio_interfaces","collision_navigation",
        "nanite_lod","texture_material_budget","rts_readability_status","performance_validation_status",
        "future_unreal_destination","provenance_rights","dependencies","open_decisions","notes"}
    for o in objects:
        key=o.get("concept_id") or "UNKNOWN"
        check(required <= set(o), f"{key}: missing fields {sorted(required-set(o))}")
        check(bool(re.fullmatch(r"EBS-CON-WRL-(ENV|PRP|BLD|SYS)-[0-9]{3}",str(key))), f"{key}: ID pattern")
        check(o.get("canon_status")=="CANDIDATE" and o.get("production_maturity")=="NOT_STARTED",f"{key}: unintended promotion")
        check(o.get("canon_approval_evidence") is None, f"{key}: invented owner approval")
        check(o.get("production_asset_id") is None, f"{key}: premature production allocation")
        check(o.get("rts_readability_status")=="NOT_EVALUATED_IN_GAME",f"{key}: unsupported RTS acceptance")
        if o.get("parent_concept_id"):
            check(o["parent_concept_id"] in ids, f"{key}: missing parent")
        for ref in o.get("source_refs",[]):
            needed={"source_repository_path","source_sha256","locator"}
            check(needed <= set(ref),f"{key}: incomplete source reference")
            if not needed <= set(ref):
                continue
            path=REPO/ref["source_repository_path"]
            check(path.is_file(),f"{key}: missing source pixels")
            if path.is_file():
                check(digest(path)==ref["source_sha256"],f"{key}: source hash mismatch")
            check(bool(ref.get("locator")), f"{key}: missing object locator")
        check(bool(o.get("source_refs")),f"{key}: missing provenance")
    for s in images:
        path=ROOT/s["path"]
        check(path.is_file(),f'{s["map_id"]}: missing selected image')
        if path.is_file():
            check(digest(path)==s["sha256"],f'{s["map_id"]}: image hash mismatch')
        r=load(s["receipt"])
        check(r.get("status")=="RETAINED_CANDIDATE",f'{s["map_id"]}: receipt status')
        check(r.get("sha256")==s["sha256"],f'{s["map_id"]}: receipt hash')
        check(hashlib.sha256(r.get("prompt","").encode()).hexdigest()==r.get("prompt_sha256"),f'{s["map_id"]}: prompt binding')
        check(inspection["maps"][s["map_id"]]["selected_revision"]==s["selected_revision"],f'{s["map_id"]}: wrong revision')
        for ref in r.get("retained_inputs",[]):
            p=ROOT/ref["retained_path"]
            check(p.is_file(),f'{s["map_id"]}: missing retained reference')
            if p.is_file():
                check(digest(p)==ref["sha256"],f'{s["map_id"]}: reference hash mismatch')
    check(coverage.get("concept_object_records")==len(objects),"Coverage object count")
    check(coverage.get("detail_panels")==54,"Coverage detail count")
    check(coverage.get("selected_source_images")==18,"Coverage source count")
    check(coverage.get("canon_approved_records")==0,"Coverage canon boundary")
    check(coverage.get("component_design_records")==sum(o.get("classification")=="assembly_component_design" for o in objects),"Component count")
    for p in ROOT.glob("*.json"):
        d=json.loads(p.read_text())
        check(d.get("author")=="Angelis Pseftis" and d.get("creator")=="Angelis Pseftis",f"{p.name}: authorship")
    doc=(ROOT/"MapEnvironmentConcepts.md").read_text()
    check("**Author and owner:** Angelis Pseftis" in doc,"Document authorship")
    for target in re.findall(r"\]\(([^)]+)\)",doc):
        if not target.startswith(("http:","https:","#")):
            check((ROOT/target.split("#")[0]).exists(),f"Broken document link: {target}")
    class Links(HTMLParser):
        def __init__(self):
            super().__init__();self.links=[];self.articles=[];self.author=None
        def handle_starttag(self,tag,attrs):
            d=dict(attrs)
            if tag in ("a","img"):
                self.links.append(d.get("href") or d.get("src"))
            if tag=="article":
                self.articles.append(d.get("id"))
            if tag=="meta" and d.get("name")=="author":
                self.author=d.get("content")
    parser=Links();gallery_path=ROOT/"review.html"
    check(gallery_path.is_file(),"Missing gallery")
    if gallery_path.is_file():
        parser.feed(gallery_path.read_text())
    check(set(parser.articles)==expected and len(parser.articles)==18,"Gallery card coverage")
    check(parser.author=="Angelis Pseftis","Gallery authorship")
    for target in parser.links:
        if target and not target.startswith(("http:","https:","#")):
            check((ROOT/target).is_file(),f"Broken gallery link: {target}")
    result={"author":"Angelis Pseftis","creator":"Angelis Pseftis",
        "status":"PASS" if not errors else "FAIL","errors":errors,
        "counts":{"maps":len(maps),"source_images":len(images),"detail_panels":54,
                  "object_records":len(objects),"component_design_records":coverage.get("component_design_records")},
        "method":"Read-only structural, local-link, metadata and SHA-256 checks",
        "boundary":"Does not certify raster geometry, canon, branch completeness, Unreal integration, RTS readability or performance."}
    print(json.dumps(result,indent=2))
    return 1 if errors else 0

if __name__=="__main__":
    sys.exit(audit())
