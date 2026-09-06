#!/usr/bin/env python3
"""Socket orientation probes for the installed Interchange glTF importer.
Author: Angelis Pseftis.
  socket_probe.py sweep <out_dir>   -> 40 sockets: 10 raw glTF rotations x 4 scale signs, all at identity Unreal intent
  socket_probe.py verify <out_dir>  -> sockets with the kit's compensated encoding at yaw 0/90/-90/180
"""
import os, sys, json, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ebs_meshkit as kit

def q_axis(axis, deg):
    h = math.radians(deg) / 2.0
    s = math.sin(h)
    return tuple(_r for _r in ((s if axis == 0 else 0.0), (s if axis == 1 else 0.0), (s if axis == 2 else 0.0), math.cos(h)))

mode, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)
m = kit.Mesh("SM_EBS_SocketProbe")
m.slot("Probe")
m.box((0, 0, 50), (100, 60, 100), 0, "body")
m.box((60, 0, 50), (20, 20, 20), 0, "nose")
m.collision.append(kit.CollisionBox("body", (0, 0, 50), (100, 60, 100)))
expected = {}
if mode == "sweep":
    rotations = {"I": (0.0, 0.0, 0.0, 1.0), "X90": q_axis(0, 90), "Xm90": q_axis(0, -90), "X180": q_axis(0, 180),
                 "Y90": q_axis(1, 90), "Ym90": q_axis(1, -90), "Y180": q_axis(1, 180),
                 "Z90": q_axis(2, 90), "Zm90": q_axis(2, -90), "Z180": q_axis(2, 180)}
    scales = {"S111": (1.0, 1.0, 1.0), "Sm11": (-1.0, 1.0, 1.0), "S1m1": (1.0, -1.0, 1.0), "S11m": (1.0, 1.0, -1.0)}
    for rn, rq in rotations.items():
        for sn, sv in scales.items():
            name = f"{rn}_{sn}"
            m.sockets.append(kit.Socket(name, (0.0, 0.0, 100.0), 0.0, "probe", raw_gltf_rotation=rq, raw_gltf_scale=sv))
            expected[name] = [0, 0, 0]
else:
    for name, yaw in (("Yaw000", 0.0), ("Yaw090", 90.0), ("YawM090", -90.0), ("Yaw180", 180.0), ("Yaw045", 45.0)):
        m.sockets.append(kit.Socket(name, (0.0, 0.0, 100.0), yaw, "probe"))
        expected[name] = [0, yaw, 0]
path = os.path.join(out, "SM_EBS_SocketProbe.glb")
digest = m.write_glb(path)
job = {"author": "Angelis Pseftis", "destination": "/Game/Echoes/Probe", "report": os.path.join(out, "probe-report.json"),
       "assets": [{"name": "SM_EBS_SocketProbe", "lod0": path}],
       "expected": {"SM_EBS_SocketProbe": {"lod0_triangles": 24, "height_cm": 100.0, "collision_boxes": 1, "sockets": sorted(expected), "socket_rotations": expected}}}
json.dump(job, open(os.path.join(out, "probe-job.json"), "w"), indent=1)
print(json.dumps({"mode": mode, "glb": path, "sha256": digest, "sockets": len(expected)}))
