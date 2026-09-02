---
name: echoes-package-provenance
description: Create or verify an evidence-bound Echoes macOS package from a clean detached live-origin worktree, without signing, notarizing, uploading, or release claims.
metadata: { author: Angelis Pseftis }
---

# Echoes package provenance

Read `CLAUDE.md`, `Docs/GameCompletionDirective.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/Archive/ProjectLedger.md`, `../WorkstreamControl/handoffs/build-distribution.md`, `../WorkstreamControl/ACTIVE_LANES.md`, and `../WorkstreamControl/HEAVY_RUN_LOCK.md`. Acquire a current detailed Heavy-Run lease through the live control process before packaging and explicitly release it after cleanup; stop if it is absent, stale, held for another operation, or self-invented.

Package only in a clean detached dedicated worktree at the exact pushed live `origin/main`. Prove equality of `HEAD`, local `origin/main`, and the live remote reference; hydrate LFS; clear inherited `GIT_*` overrides; verify the Seagate archive and internal filesystem each have at least 60 GiB free. Create a new archive and follow the current authorized procedure in `Docs/Archive/SetupAndBuild.md` and `../WorkstreamControl/handoffs/build-distribution.md`; configuration is the live gate's choice, not an assumed Shipping build.

Retain command, UTC time, worktree path, commit, remote equality, LFS state, free-space readings, manifest/provenance/package hashes, signature result, smoke output, warnings/errors, archive path, and claim limit. Reject dirty, unpushed, ambiguous, reused, or unverifiable candidates. Development/ad-hoc packaging is not Developer ID, notarization, installer, clean-machine, performance, soak, or release evidence; route each separately.

Before any third-party skill, script, hook, dependency, plugin, or networked build input is used, identify its source, revision/hash, license, permissions, network behavior, and project relevance; review it before use and record the decision. A catalog or scanner result is not a security guarantee.
