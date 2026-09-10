# Salvage inventory — five detached/branch worktrees, ~14,000 uncommitted lines

Generated: 2026-09-10T00:03:06Z  ·  Salvaged by: L2-CONTROL  ·  main at: 1122c8a

The worktrees are PRESERVED. Nothing here deletes them and `git worktree prune` must not be run.
These patches exist so each lane can cherry-pick hunks from its own domain instead of merging
worktrees that hold mutually overlapping edits to the same files.

## Per-worktree

| worktree | HEAD | tracked files | tracked +ins | untracked entries | patch files |
|---|---|---|---|---|---|
| backend-actions-20260909 | 6b55988 | 64 | 4064 | 24 | `backend-actions-20260909.patch`, `backend-actions-20260909.untracked.patch`, `backend-actions-20260909.status` |
| hud-concept-completion | 6b55988 | 85 | 7488 | 45 | `hud-concept-completion.patch`, `hud-concept-completion.untracked.patch`, `hud-concept-completion.status` |
| m01-environment-completion | 6b55988 | 10 | 342 | 2 | `m01-environment-completion.patch`, `m01-environment-completion.untracked.patch`, `m01-environment-completion.status` |
| resource-depletion-20260909 | 6b55988 | 12 | 1804 | 14 | `resource-depletion-20260909.patch`, `resource-depletion-20260909.untracked.patch`, `resource-depletion-20260909.status` |
| unit-abilities-20260909 | 6b55988 | 7 | 236 | 2 | `unit-abilities-20260909.patch`, `unit-abilities-20260909.untracked.patch`, `unit-abilities-20260909.status` |
| concept-production-pipeline | dbe4ddb | 1 | 29 | 2 | `concept-production-pipeline.patch`, `concept-production-pipeline.untracked.patch`, `concept-production-pipeline.status` |

## How to use these

`.patch` is `git diff HEAD` — modifications to tracked files.
`.untracked.patch` holds NEW files the tracked diff cannot contain (each as a /dev/null→file diff).
Apply a single file from either with: `git apply --include=<path> BuildArtifacts/Salvage/<name>.patch`
`.status` is the full porcelain listing, including anything excluded below.

## Excluded from patches, still present in their worktrees

Large untracked asset directories are inventoried but not patched, to avoid force-committing
multi-megabyte binaries into the repository. They remain on disk in the worktree:
- `Worktrees/hud-concept-completion/ArtSource/` (2372 KB)
- `Worktrees/hud-concept-completion/Content/Art/UI/` (2864 KB)

## Contended files — why these were not merged

Paths touched by more than one worktree. Merging these was rejected as costing more than re-deriving:

| file | worktrees touching it |
|---|---|
| `Config/DefaultInput.ini` | 2 (backend-actions-20260909 hud-concept-completion) |
| `Docs/Archive/AssetRegister.md` | 3 (backend-actions-20260909 hud-concept-completion m01-environment-completion) |
| `Docs/Archive/SetupAndBuild.md` | 2 (backend-actions-20260909 hud-concept-completion) |
| `Docs/CampaignAndTutorialJourneyPlan.md` | 2 (backend-actions-20260909 hud-concept-completion) |
| `Docs/DeliveryPlan.md` | 2 (backend-actions-20260909 hud-concept-completion) |
| `Docs/OpeningAndTutorialScript.md` | 2 (backend-actions-20260909 hud-concept-completion) |
| `Docs/Requirements.md` | 3 (backend-actions-20260909 hud-concept-completion resource-depletion-20260909) |
| `Docs/RequirementsState.md` | 3 (backend-actions-20260909 hud-concept-completion resource-depletion-20260909) |
| `Docs/VisualTargetSpecification.md` | 2 (backend-actions-20260909 hud-concept-completion) |
| `Docs/WorldMapWorkLog.md` | 2 (hud-concept-completion m01-environment-completion) |
| `Scripts/echoes_cliff_material.py` | 2 (hud-concept-completion m01-environment-completion) |
| `Scripts/echoes_evacuation_props.py` | 2 (hud-concept-completion m01-environment-completion) |
| `Scripts/generate_art_assets.py` | 3 (backend-actions-20260909 hud-concept-completion m01-environment-completion) |
| `Scripts/generate_art_assets.sh` | 2 (hud-concept-completion m01-environment-completion) |
| `Scripts/run_unreal_tests.sh` | 2 (backend-actions-20260909 hud-concept-completion) |
| `Scripts/test_content.sh` | 2 (hud-concept-completion m01-environment-completion) |
| `Source/EchoesOfTheBrokenSun/EchoesOfTheBrokenSun.Build.cs` | 2 (backend-actions-20260909 hud-concept-completion) |
| `Source/EchoesOfTheBrokenSun/Private/EchoesBuildPlacementPreview.cpp` | 2 (backend-actions-20260909 hud-concept-completion) |
| `Source/EchoesOfTheBrokenSun/Private/EchoesCliffMesh.cpp` | 2 (hud-concept-completion m01-environment-completion) |
| `Source/EchoesOfTheBrokenSun/Private/EchoesCommandDeckModel.cpp` | 2 (backend-actions-20260909 hud-concept-completion) |
| `Source/EchoesOfTheBrokenSun/Private/EchoesFieldHudView.cpp` | 2 (backend-actions-20260909 hud-concept-completion) |
| `Source/EchoesOfTheBrokenSun/Private/EchoesFieldHudWidget.cpp` | 2 (backend-actions-20260909 hud-concept-completion) |
| `Source/EchoesOfTheBrokenSun/Private/EchoesFogView.cpp` | 2 (backend-actions-20260909 hud-concept-completion) |
| `Source/EchoesOfTheBrokenSun/Private/EchoesNetworkSession.cpp` | 2 (backend-actions-20260909 hud-concept-completion) |
| `Source/EchoesOfTheBrokenSun/Private/EchoesPlayerController.cpp` | 3 (backend-actions-20260909 hud-concept-completion resource-depletion-20260909) |
