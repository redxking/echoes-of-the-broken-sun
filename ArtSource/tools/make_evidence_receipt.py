#!/usr/bin/env python3
"""Write an evidence receipt for one ArtSource package stage.

Author: Angelis Pseftis.

Collects the source identity (worktree HEAD, dirty paths), environment (Python,
platform, installed Unreal Build.version), the package build manifest hashes,
and a SHA-256 inventory of everything under the package evidence directory.
Standard library only.

  python3 make_evidence_receipt.py --evidence-dir <dir> --package EBS-MER-BLD-002 \
      --manifest <ArtSource/.../build-manifest.json> --stage BLOCKOUT \
      --command "..." [--command "..."] [--note "..."]
"""
from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import os
import platform
import subprocess
import sys

AUTHOR = "Angelis Pseftis"
ENGINE_VERSION_FILE = "/Users/Shared/Epic Games/UE_5.8/Engine/Build/Build.version"


def sha256_file(path: str) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git(repo: str, *args: str) -> str:
    return subprocess.run(["git", "-C", repo, *args], capture_output=True, text=True, check=False).stdout.strip()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--evidence-dir", required=True)
    parser.add_argument("--package", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--stage", required=True)
    parser.add_argument("--repo", default=os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
    parser.add_argument("--command", action="append", default=[])
    parser.add_argument("--note", action="append", default=[])
    parser.add_argument("--output", default="receipt.json")
    args = parser.parse_args()

    evidence_dir = os.path.abspath(args.evidence_dir)
    inventory = []
    for root, _dirs, files in os.walk(evidence_dir):
        for name in sorted(files):
            if name == args.output:
                continue
            path = os.path.join(root, name)
            inventory.append({"path": os.path.relpath(path, evidence_dir), "bytes": os.path.getsize(path), "sha256": sha256_file(path)})
    inventory.sort(key=lambda item: item["path"])

    with open(args.manifest, "r", encoding="utf-8") as handle:
        manifest = json.load(handle)
    engine = None
    if os.path.exists(ENGINE_VERSION_FILE):
        with open(ENGINE_VERSION_FILE, "r", encoding="utf-8") as handle:
            engine = json.load(handle)

    dirty = [line for line in git(args.repo, "status", "--porcelain").splitlines() if line.strip()]
    receipt = {
        "author": AUTHOR,
        "creator": AUTHOR,
        "package": args.package,
        "stage": args.stage,
        "created_utc": dt.datetime.now(dt.timezone.utc).replace(microsecond=0).isoformat(),
        "evidence_dir": evidence_dir,
        "source_identity": {
            "repo": os.path.abspath(args.repo),
            "branch": git(args.repo, "branch", "--show-current"),
            "head": git(args.repo, "rev-parse", "HEAD"),
            "dirty_paths": dirty,
        },
        "environment": {
            "python": platform.python_version(),
            "platform": platform.platform(),
            "machine": platform.machine(),
            "unreal_installed": engine,
        },
        "manifest": {"path": os.path.abspath(args.manifest), "sha256": sha256_file(args.manifest), "revision": manifest.get("revision"),
                     "budgets": manifest.get("budgets"), "outputs": manifest.get("outputs")},
        "commands": args.command,
        "notes": args.note,
        "inventory": inventory,
        "boundary": "Retained generation, review-render and structural-check evidence for the named stage. It is not a gate pass, integration, packaged evidence or owner acceptance.",
    }
    out_path = os.path.join(evidence_dir, args.output)
    with open(out_path, "w", encoding="utf-8", newline="\n") as handle:
        json.dump(receipt, handle, indent=1, sort_keys=True)
        handle.write("\n")
    print(json.dumps({"receipt": out_path, "files": len(inventory), "head": receipt["source_identity"]["head"], "dirty": len(dirty)}))
    return 0


if __name__ == "__main__":
    sys.exit(main())
