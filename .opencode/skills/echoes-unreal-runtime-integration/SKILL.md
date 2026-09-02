---
name: echoes-unreal-runtime-integration
description: "Integrate authoritative Echoes state into Unreal runtime systems while preserving adapter boundaries, asset safety, and Mac build evidence."
metadata:
  author: Angelis Pseftis
---

# Echoes Unreal runtime integration

Use for Unreal adapters, subsystems, actor lifecycle, input bridges, asset binding, or runtime presentation. Not for changing simulation authority.

1. Read live `CLAUDE.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/Archive/ProjectLedger.md`, `Docs/DemoReadinessRequirements.md`, `Docs/InitialReleaseRequirements.md`, and `../WorkstreamControl/ACTIVE_LANES.md`; verify lease, worktree, branch, dirty paths, and Mac build conditions before mutation.
2. Treat `EchoesSimCore` as authority. Unreal consumes authoritative state and emits validated commands; it must not duplicate rules, predict authoritative outcomes, or place presentation state in saves/replays/checksums.
3. Verify every Unreal API, class hierarchy, module dependency, macro, and engine-version assumption against the installed UE 5.8.2 source/headers or current Epic primary documentation. A public skill, code sample, model response, or older UE release is not API evidence.
4. Presentation assets must have collision, overlaps, navigation influence, and shadows disabled where the contract requires. Bind registered assets from source-compiled data; do not use unregistered placeholders or direct generated-output edits.
5. Run the narrowest applicable native/content check, then the documented Mac editor/automation path when the claim is runtime behavior. Preserve the `TMPDIR` rule for Unreal automation and record host/commit.
6. Stop when an Editor session, heavy lock, shared hotspot, volume issue, missing asset provenance, or build failure makes the planned evidence invalid.

## Acceptance checks

Provide adapter contract evidence, focused tests, build/automation result, observed runtime boundary, and explicitly separate rendered interaction or packaged-play proof from code inspection. Route heavy work to `echoes-heavy-run-coordination`, physical GUI/play to `echoes-gui-control-readiness`, evidence to `echoes-evidence-gate-review`, and owner acceptance to `echoes-human-acceptance-session`.
