---
name: echoes-platform-portability
description: "Protect Echoes macOS Apple Silicon release behavior and future Linux/SteamOS and Windows portability constraints without making unsupported shipping claims."
metadata:
  author: Angelis Pseftis
---

# Echoes platform portability

Read `CLAUDE.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/Archive/ProjectLedger.md`, `Docs/GameCompletionDirective.md`, `Docs/InitialReleaseRequirements.md`, and `../WorkstreamControl/ACTIVE_LANES.md`; verify exact lane lease/worktree/branch/SHA before mutation.

macOS Apple Silicon is the release target. Linux/SteamOS and Windows are future compatibility constraints, not evidence of builds, support, signing, or release. Identify platform-bound code/assets/input/render/audio/file paths, target hardware/device matrices, and thermal/sustained-load checks, then establish a named compile, package, runtime, or clean-machine check before changing them. Generated platform artifacts follow official source/build pipelines; never hand-edit package output.

Route macOS build/package work to `echoes-workstation-toolchain-readiness` and `echoes-heavy-run-coordination`, runtime observation to `echoes-gui-control-readiness`, security/signing to `echoes-security-privacy`, evidence to `echoes-evidence-gate-review`, and owner release decision to `echoes-human-acceptance-session`. Stop on missing platform host, signing authority, unverifiable compatibility claim, dependency license uncertainty, or release-scope change.
