# D0 Inventory Disposition Log

**Canonical Checkout Main**: af3475eb096e2b95c264f9562cd88b88c32cf9d7
**Current Working Directory**: Preserved in `BuildArtifacts/Evidence/resume-preservation-20260910/main`. Contains uncommitted order-queue tests, document updates, and input/subsystem modifications.

## Candidate Patches and Salvage

The salvage baselines created on 2026-09-10T00:05:00Z have been verified. Patches have not changed since capture.

| Candidate / Worktree | Base Commit | Dirty Files | Status / Classification | Notes |
| :--- | :--- | :--- | :--- | :--- |
| `backend-actions-20260909` | 6b559889 | 88 | Pending Integration | Modifies UI layout, HUD, input configuration, and extensive build rules. Likely conflicts with `main` input changes. |
| `concept-production-pipeline` | dbe4ddbb | 3 | Needs verification | Isolated script changes (`diag_texture_channels_inengine.py`). Art tooling. |
| `hud-concept-completion` | 6b559889 | 132 | Needs comparison | Large HUD update. Overlaps with `backend-actions-20260909` and current `main` dirty state. Modifies `DefaultInput.ini` and materials. |
| `m01-environment-completion` | 6b559889 | 12 | Needs integration | M01 specific environment scripting and prop generation. Modifies `AssetRegister.md`. |
| `resource-depletion-20260909` | 6b559889 | 26 | Pending Integration | Modifies `EchoesSimulationSubsystem.cpp`, `Simulation.cpp`, and replay/network protocol. Deals with resource system. |
| `unit-abilities-20260909` | 6b559889 | 9 | Pending Integration | Modifies `EchoesCheckpointWorker`, network protocol, and sim core. |

## Disposition Plan
1. **Preserve Main**: Main checkout dirty state has been preserved.
2. **Sequential Integration Strategy**:
   - Instead of a blind merge, we will use a single integration branch off `main`.
   - The first candidate for integration should be `resource-depletion-20260909` and `unit-abilities-20260909`, as these affect the `SimCore` directly and have fewer dirty files.
   - We will review `backend-actions-20260909` and `hud-concept-completion` carefully, since they overlap with the HUD changes currently dirty in `main`.
