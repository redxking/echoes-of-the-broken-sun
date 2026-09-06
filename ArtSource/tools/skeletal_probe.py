#!/usr/bin/env python3
"""Skeletal encoding probe for the installed Interchange glTF importer.

Author: Angelis Pseftis.

  skeletal_probe.py <out_dir>                    -> writes SK_EBS_SkeletalProbe.glb + probe-job.json for ue_import_inspect_skeletal.py
  skeletal_probe.py check <out_dir> [label]      -> evaluates probe-report[-label].json (and the editor log it names) against the
                                                    expectations, writes probe-checks[-label].json; exit 1 on any failure
  skeletal_probe.py run <out_dir> <label>        -> launches UnrealEditor-Cmd headlessly on a per-run copy of the job
                                                    (probe-job-<label>.json -> probe-report-<label>.json, UnrealEditor-Cmd-<label>.log,
                                                    stdout-<label>.txt), refuses to start while another UnrealEditor-Cmd runs, then
                                                    performs the check; exit 1 unless editor exit 0, no report errors and all checks pass

Probe: root (0,0,0) > hip (0,0,100) > knee (0,0,50) > foot (0,0,0); a 20 cm box bound to
each bone just above its head along +X; sockets Toe (foot, (20,0,0), yaw 0) and Side
(hip, (0,30,100), yaw 90); clips PitchLift (knee pitch 0->90), YawTurn (hip yaw 0->90),
RollTest (hip roll 0->90), Translate (hip (0,0,0)->(0,0,-20) cm), each 1 s.

The inspector imports into a fresh <destination>/Run_<stamp> folder every run (see its
docstring); the check therefore also requires report['destination_cleared'] not False, a base
folder that is empty on disk and in the registry, an empty pre-import run folder, the expected
imported-object count, no inspector errors, and the 'Interchange import: Using stack
[OverridePipeline]' line in the editor log (a re-import never logs it).
"""
import datetime
import json
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ebs_meshkit as kit  # noqa: E402
import ebs_skelkit as skel  # noqa: E402

AUTHOR = "Angelis Pseftis"
NAME = "SK_EBS_SkeletalProbe"
DESTINATION = "/Game/Echoes/SkeletalProbe"
ENGINE_CMD = "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd"
SANDBOX_PROJECT = ("/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z/"
                   "IsolatedPreview/EBSPreview/EBSPreview.uproject")
OVERRIDE_STACK_LINE = "Interchange import: Using stack [OverridePipeline] to import."
DELETE_WARNING = "DeleteDirectory: Could not remove the directory"
HEADS = {"root": (0.0, 0.0, 0.0), "hip": (0.0, 0.0, 100.0), "knee": (0.0, 0.0, 50.0), "foot": (0.0, 0.0, 0.0)}
PARENTS = {"root": None, "hip": "root", "knee": "hip", "foot": "knee"}
ORDER = ["root", "hip", "knee", "foot"]
CLIPS = {"PitchLift": ("knee", (90.0, 0.0, 0.0), (0.0, 0.0, 0.0)),
         "YawTurn": ("hip", (0.0, 90.0, 0.0), (0.0, 0.0, 0.0)),
         "RollTest": ("hip", (0.0, 0.0, 90.0), (0.0, 0.0, 0.0)),
         "Translate": ("hip", (0.0, 0.0, 0.0), (0.0, 0.0, -20.0))}
SOCKETS = {"Toe": ("foot", (20.0, 0.0, 0.0), 0.0), "Side": ("hip", (0.0, 30.0, 100.0), 90.0)}


def build():
    mesh = kit.Mesh(NAME)
    mesh.slot("Probe")
    skeleton = skel.Skeleton("root")
    for name in ORDER:
        skeleton.add(name, PARENTS[name], HEADS[name], purpose=f"probe {name}")
    for name in ORDER:
        hx, hy, hz = HEADS[name]
        dx = -25.0 if name == "root" else 25.0  # root box sits behind so it does not overlap the foot box
        mesh.box((hx + dx, hy, hz + 10.0), (20.0, 20.0, 20.0), 0, f"box_{name}")
    counts = skel.bind_polygons(mesh, "root", {f"box_{n}": n for n in ORDER})
    for sname, (bone, pos, yaw) in SOCKETS.items():
        mesh.sockets.append(kit.Socket(sname, pos, yaw, f"probe socket on {bone}"))
    clips = []
    for cname, (bone, rot, tr) in CLIPS.items():
        clip = skel.AnimationClip(cname, 1.0, purpose="encoding probe")
        clip.key(bone, 0.0)
        clip.key(bone, 1.0, rotation_deg=rot, translation_cm=tr)
        clips.append(clip)
    sockets_on_bones = {s: v[0] for s, v in SOCKETS.items()}
    return mesh, skeleton, clips, sockets_on_bones, counts


def expectations():
    return {
        "bones": ORDER,
        "parents": {n: PARENTS[n] for n in ORDER},
        "heads_component_cm": {n: list(HEADS[n]) for n in ORDER},
        "clips": {c: {"bone": v[0], "rotator_end": list(v[1]), "translation_end_cm": list(v[2])} for c, v in CLIPS.items()},
        "sockets": {s: {"bone": v[0], "relative_location_cm": list(kit.v_sub(v[1], HEADS[v[0]])), "rotation_deg": [0.0, v[2], 0.0], "scale": [1.0, 1.0, 1.0]}
                    for s, v in SOCKETS.items()},
        "triangles": 4 * 12,
    }


def write_probe(out):
    os.makedirs(out, exist_ok=True)
    mesh, skeleton, clips, sockets_on_bones, counts = build()
    path = os.path.join(out, f"{NAME}.glb")
    digest = skel.write_skinned_glb(mesh, skeleton, path, animations=clips, include_collision=False, sockets_on_bones=sockets_on_bones,
                                    extras={"probe": "skeletal encoding probe", "kit": skel.SKEL_REVISION})
    job = {"author": AUTHOR, "destination": DESTINATION, "report": os.path.join(out, "probe-report.json"),
           "assets": [{"name": NAME, "file": path, "clips": [c.name for c in clips]}], "expected": {NAME: expectations()}}
    with open(os.path.join(out, "probe-job.json"), "w", encoding="utf-8") as handle:
        json.dump(job, handle, indent=1, sort_keys=True)
    print(json.dumps({"glb": path, "sha256": digest, "bound_triangles": counts, "job": os.path.join(out, "probe-job.json")}))


def _close(a, b, tol):
    return all(abs(float(x) - float(y)) <= tol for x, y in zip(a, b)) and len(a) == len(b)


def _ang_close(a, b, tol=0.5):
    return all(abs(((float(x) - float(y) + 180.0) % 360.0) - 180.0) <= tol for x, y in zip(a, b)) and len(a) == len(b)


def report_name(label=None):
    return f"probe-report-{label}.json" if label else "probe-report.json"


def checks_name(label=None):
    return f"probe-checks-{label}.json" if label else "probe-checks.json"


def harness_checks(report, log_text):
    """Idempotency / provenance checks: the run must have been a fresh import through the override
    pipeline stack, not a silent re-import of a surviving package."""
    out = []

    def add(name, passed, evidence):
        out.append({"check": name, "passed": bool(passed), "evidence": evidence})

    add("inspector reported no errors", not report.get("errors"), f"errors={report.get('errors')}")
    add("destination cleared (delete_directory not False)", report.get("destination_cleared") is not False,
        f"destination_cleared={report.get('destination_cleared')} destination={report.get('destination')}")
    add("unique per-run destination, empty before import", bool(report.get("destination_unique_per_run")) and not report.get("destination_preexisting_assets"),
        f"destination_used={report.get('destination_used')} preexisting={report.get('destination_preexisting_assets')}")
    imports = report.get("imports", [])
    counts_ok = bool(imports) and all("imported_object_paths" in i and len(i["imported_object_paths"]) == i.get("expected_imported_object_count") for i in imports)
    add("imported object count == skeleton + mesh + clips", counts_ok,
        f"{[(i.get('name'), len(i.get('imported_object_paths', [])), i.get('expected_imported_object_count')) for i in imports]}")
    add("base destination clean on disk and in the registry before import",
        report.get("destination_disk_files_after_clear") == [] and report.get("destination_registry_assets_after_clear") == [],
        f"disk={report.get('destination_disk_files_after_clear')} registry={report.get('destination_registry_assets_after_clear')} "
        f"engine_delete={report.get('destination_engine_delete')} removed_by_hand={report.get('destination_removed_files')}")
    if log_text is None:
        add("editor log names the override pipeline stack", False, f"log not readable: log_path={report.get('log_path')}")
    else:
        add("editor log names the override pipeline stack", OVERRIDE_STACK_LINE in log_text,
            f"'{OVERRIDE_STACK_LINE}' in {report.get('log_path')}; engine DeleteDirectory warning present={DELETE_WARNING in log_text}")
    return out


def read_log(report, log_override=None):
    path = log_override or report.get("log_path")
    if not path or not os.path.exists(path):
        return None
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        return handle.read()


def check(out, label=None, log_override=None):
    with open(os.path.join(out, report_name(label)), "r", encoding="utf-8") as handle:
        report = json.load(handle)
    exp = expectations()
    checks = harness_checks(report, read_log(report, log_override))

    def add(name, passed, evidence):
        checks.append({"check": name, "passed": bool(passed), "evidence": evidence})

    meshes = report.get("skeletal_meshes", [])
    add("skeletal mesh imported", len(meshes) == 1, f"{len(meshes)} skeletal mesh asset(s): {[m.get('path') for m in meshes]}")
    if not meshes:
        return finish(out, checks, label)
    sm = meshes[0]
    bones = sm.get("bones", [])
    names = [b["name"] for b in bones]
    add("bones root/hip/knee/foot in order", names == exp["bones"], f"names={names}")
    parents_ok = all(b.get("parent") == exp["parents"].get(b["name"]) for b in bones) and len(bones) == 4
    add("bone parents", parents_ok, f"{[(b['name'], b.get('parent')) for b in bones]}")
    heads_ok = len(bones) == 4 and all(_close(b.get("ref_component_location_cm", [None] * 3), exp["heads_component_cm"][b["name"]], 0.05) for b in bones if b["name"] in exp["heads_component_cm"])
    add("reference pose heads in component space", heads_ok, f"{[(b['name'], b.get('ref_component_location_cm')) for b in bones]}")
    rest_rot_ok = len(bones) == 4 and all(_ang_close(b.get("ref_component_rotation_deg", [1e9] * 3), [0, 0, 0]) for b in bones)
    add("reference pose rotations identity", rest_rot_ok, f"{[(b['name'], b.get('ref_component_rotation_deg')) for b in bones]}")
    lods = sm.get("lods", [])
    add("one LOD", len(lods) == 1, f"lods={lods}")
    add("material slots == [Probe]", [m.get("imported_name") for m in sm.get("materials", [])] == ["Probe"], f"{sm.get('materials')}")
    socks = {s["name"]: s for s in sm.get("sockets", [])}
    for sname, want in exp["sockets"].items():
        got = socks.get(sname)
        ok = got is not None and got.get("bone") == want["bone"] and _close(got.get("location_cm", [1e9] * 3), want["relative_location_cm"], 0.05) \
            and _ang_close(got.get("rotation_deg", [1e9] * 3), want["rotation_deg"]) and _close(got.get("scale", [0] * 3), want["scale"], 0.001)
        add(f"socket {sname} bone/location/rotation/scale", ok, f"got={got} want={want}")
    # Interchange names each AnimSequence <destination_name><clip name>; match clips by suffix.
    anims = {}
    for a in report.get("animations", []):
        for cname in exp["clips"]:
            if a["name"] == cname or a["name"].endswith(cname):
                anims[cname] = a
    add("four AnimSequences (one per clip, named <mesh><clip>)", sorted(anims) == sorted(exp["clips"]),
        f"{sorted(a['name'] for a in report.get('animations', []))}")
    for cname, want in exp["clips"].items():
        a = anims.get(cname)
        if a is None:
            add(f"{cname} evaluated", False, "missing")
            continue
        end = a.get("end", {}).get(want["bone"], {}) or {}
        start = a.get("start", {}).get(want["bone"], {}) or {}
        rot_ok = _ang_close(end.get("local_rotation_deg", [1e9] * 3), want["rotator_end"]) and _ang_close(start.get("local_rotation_deg", [1e9] * 3), [0, 0, 0])
        add(f"{cname} local rotator at t=end == {want['rotator_end']} (t=0 identity)", rot_ok, f"t0={start.get('local_rotation_deg')} end={end.get('local_rotation_deg')}")
        delta = end.get("local_translation_delta_cm")
        tr_ok = delta is not None and _close(delta, want["translation_end_cm"], 0.05)
        add(f"{cname} local translation delta at t=end == {want['translation_end_cm']} cm", tr_ok, f"delta={delta} local={end.get('local_translation_cm')}")
        add(f"{cname} length 1 s", abs(float(a.get("length_s", 0)) - 1.0) < 1e-3, f"length_s={a.get('length_s')} frames={a.get('num_frames')} keys={a.get('num_keys')}")
    return finish(out, checks, label)


def finish(out, checks, label=None):
    passed = [c for c in checks if c["passed"]]
    failed = [c for c in checks if not c["passed"]]
    doc = {"author": AUTHOR, "report": report_name(label), "passed": len(passed), "failed": len(failed), "checks": checks}
    with open(os.path.join(out, checks_name(label)), "w", encoding="utf-8") as handle:
        json.dump(doc, handle, indent=1)
    print(json.dumps(doc, indent=1))
    return len(failed) == 0


def other_editor_cmd_running():
    """Names of running UnrealEditor-Cmd processes (the UnrealEditorServices helper is ignored)."""
    result = subprocess.run(["pgrep", "-fl", "UnrealEditor-Cmd"], capture_output=True, text=True, check=False)
    return [line for line in result.stdout.splitlines() if "UnrealEditor-Cmd" in line]


def run(out, label, timeout_s=600):
    """One reproducible headless probe import: per-run job, log, stdout and report, then the check."""
    with open(os.path.join(out, "probe-job.json"), "r", encoding="utf-8") as handle:
        job = json.load(handle)
    job["report"] = os.path.join(out, report_name(label))
    job_path = os.path.join(out, f"probe-job-{label}.json")
    with open(job_path, "w", encoding="utf-8") as handle:
        json.dump(job, handle, indent=1, sort_keys=True)
    busy = other_editor_cmd_running()
    if busy:
        print(json.dumps({"label": label, "refused": "another UnrealEditor-Cmd is running", "processes": busy}))
        return False
    log_path = os.path.join(out, f"UnrealEditor-Cmd-{label}.log")
    inspector = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ue_import_inspect_skeletal.py")
    command = [ENGINE_CMD, SANDBOX_PROJECT, "-unattended", "-nop4", "-nosplash", "-nullrhi", "-NoSound", "-SCCProvider=None",
               "-ScriptErrorsAreFatal", f"-ExecutePythonScript={inspector}", f"-abslog={log_path}"]
    started = datetime.datetime.now(datetime.timezone.utc)
    with open(os.path.join(out, f"{label}-started.txt"), "w", encoding="utf-8") as handle:
        handle.write(started.isoformat(timespec="seconds") + "\n")
    env = dict(os.environ, EBS_IMPORT_JOB=job_path)
    clock = time.time()
    with open(os.path.join(out, f"stdout-{label}.txt"), "w", encoding="utf-8") as handle:
        try:
            proc = subprocess.run(command, stdout=handle, stderr=subprocess.STDOUT, env=env, timeout=timeout_s, check=False)
            exit_code = proc.returncode
        except subprocess.TimeoutExpired:
            exit_code = None
        handle.write(f"exit={exit_code}\n")
    finished = datetime.datetime.now(datetime.timezone.utc)
    summary = {"label": label, "job": job_path, "log": log_path, "report": job["report"], "exit_code": exit_code,
               "started_utc": started.isoformat(timespec="seconds"), "finished_utc": finished.isoformat(timespec="seconds"),
               "elapsed_s": round(time.time() - clock)}
    ok = exit_code == 0 and os.path.exists(job["report"]) and check(out, label)
    summary["checks"] = checks_name(label)
    summary["passed"] = ok
    print(json.dumps(summary))
    return ok


if __name__ == "__main__":
    if sys.argv[1] == "check":
        sys.exit(0 if check(sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else None) else 1)
    if sys.argv[1] == "run":
        sys.exit(0 if run(sys.argv[2], sys.argv[3]) else 1)
    write_probe(sys.argv[1])
