# Agent project brief — Echoes of the Broken Sun

**Author and owner:** Angelis Pseftis
**Standing:** derived entry prompt; no independent requirements, status, or permission authority.

Use this brief in the existing project task. Read [AGENTS.md](../../AGENTS.md), the shared contract for every
agent, then [Docs/README.md](../README.md) and [AgentSkillRouting.md](../AgentSkillRouting.md). Locate the
checkout, respect the DeliveryPlan pause boundary before operational inspection, and select/read only
the skills needed for the requested outcome.
Frame and verify the assigned change using [GameDevelopmentWorkflow.md](GameDevelopmentWorkflow.md).
Automatically select the supported model/effort for the bounded package using its
[routing procedure](GameDevelopmentWorkflow.md#select-model-effort-and-work-ownership), and fill its
[handoff contract](GameDevelopmentWorkflow.md#delegation-and-additional-task-handoff) before delegation or
an authorized additional task. Start at the [DeliveryPlan active state](../DeliveryPlan.md#active-execution-state) for
pause status, dependency order and package-scoped reading.

Build a professional story-driven real-time strategy game in the original world of Soryn. Preserve the
creative Bible's factions, Future Wells, characters, and consequences. The owner's campaign direction is
explicit: Missions 1–15 each have a unique story-driven map. Together they form a connected journey through
a large world; characters' motivations and backstories must matter to the battles and story. The MMO
comparison describes world scale and connection within this RTS, not authorization for MMO systems.

[Requirements.md](../Requirements.md) is the sole behavioral master. [RequirementsState.md](../RequirementsState.md)
is the sole lifecycle and owner-decision record. Read the exact affected IDs and evidence; do not infer
current implementation from this brief. The creative, architecture, art, audio, script, map, build, and
provenance documents have distinct roles listed in the authority map. Resolve known conflicts there before
changing gameplay or canon; authorized visual work can proceed within its established boundaries.

The active parent owns integration and verification. Preserve unrelated work and explicit path ownership.
Use current task coordination; never infer a lease, role, tool capability, or permission from a retired
session. Keep source changes reproducible through the registered compiler/generator and preserve simulation
authority over presentation.

Define the observable check, implement the bounded authorized change, inspect the result, and retain
attributable evidence. Continue independently through internal qualification where authorized. Ask only at
real unresolved decisions. Keep implementation, automated verification, packaged execution, physical input,
human acceptance, and release readiness separate.

Edit authoritative documents in place with authorship only Angelis Pseftis. Deliver the concrete outcome,
checks performed, evidence boundary, remaining issues, and any decision needed. Follow the shared contract
for external actions; this prompt does not authorize a push, publication, message, or release.

## Current bounded execution — standalone match recovery (2026-09-12)

Author and owner: Angelis Pseftis.

Outcome: advance D2 through a standalone Mac match using visible player controls:
launch, gather, build, train, move, fight, repair where available, save, quit, relaunch,
load and continue. Verify the first concrete failure before expanding implementation.
REL-SAV-005 controls checkpoint fidelity; DeliveryPlan defines the integrated D2 route.
Agent-operated input is internal rendered QA, not owner or unfamiliar-human acceptance.

Source root: `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project`, main at
`34906cf2610bd5d4e4608a1fb12fc1fc568fcb70` plus the uncommitted save repair. Live
origin/main matched HEAD during preflight. Unreal root `/Users/Shared/Epic Games/UE_5.8`,
version 5.8.2, target Mac arm64 Development. The deterministic EchoesSimCore remains
authoritative; presentation cannot change save, replay, fog or checksums.

Baseline: prior repair receipt is `BuildArtifacts/Evidence/reshape-checkpoint-20260912/receipt.md`.
All 15 retained candidate source hashes matched during this preflight. Prior native/editor
results remain historical evidence; no standalone package contains this uncommitted repair yet.
GUI inventory is callable through CUA; actual game capture and input delivery remain untested.

Preparation/evidence: `BuildArtifacts/Evidence/standalone-match-20260912/` contains
source-hash-check.json, baseline-status.txt, proposed-commit-paths.json and review.patch.
The 19 proposed paths cover only repair code, regressions, test compatibility and tracking.
Preserve unrelated generated art and Scripts/generate_art_assets.sh. The owner authorized scoped commit/push and prerequisite repairs on 2026-09-12. Proceed with publication;
do not weaken the clean pushed-source packaging rule or package old main as repair evidence.

After authorization, inspect the final scoped diff and stage only listed paths, commit and
push without rewriting history. Recheck remote identity and current ownership. Use
Scripts/package_from_pushed_main.sh and Scripts/package_macos.sh according to SetupAndBuild
and echoes-package-provenance, from a fresh detached worktree at exact live origin/main.
Acquire exclusive build/GPU ownership through live coordination before launch. Both disks
passed the 60 GiB preflight (73 internal, 3632 archive); refresh immediately before packaging.
Use a fresh archive outside the checkout. Do not use an old package or bypass provenance.

Verify the produced bundle with Scripts/verify_packaged_app.py, then use the normal visible
startup flow and CUA mouse/keyboard input. Resolve exact app path from the package output;
it is not yet available. Preserve production saves; establish an isolated or explicitly known
save location using the existing approved procedure before play. Record package hashes,
inputs and fresh visible state after each meaningful action. Never use console, injected saves,
Unreal MCP or direct simulation commands to substitute for the player route.

Acceptance: observed resource gathering, construction and trained-unit response; ordinary
movement/combat; successful save feedback; clean quit and cold relaunch; restored visible
state and continued orders through load. Record any unavailable step as unverified, including
audio unless a listening route exists. Fix one reproducible failure at a time, with focused
regressions and fresh package identity after code changes. Do not reopen parked AI/balance.

Stop dependent work when source publication, resource ownership, package identity or UI
capability is unresolved. Retain failed artifacts and exact recovery steps; never reset unrelated
work. Update DeliveryPlan's sole continuation table and relevant requirement evidence states,
then run python3 Scripts/build_context.py and git diff --check. No retained process exists at
this preflight handoff. Owner acceptance and release qualification remain separate gates.
