---
name: echoes-package-sign-notarize
description: Route macOS Echoes packaging, provenance, Developer ID/notarization, installer, and clean-machine work to separately authorized skills without executing credentialed or release-state actions.
metadata: { author: Angelis Pseftis }
---

# Echoes package release router

Read `CLAUDE.md`, `Docs/GameCompletionDirective.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/SetupAndBuild.md`, `../WorkstreamControl/handoffs/build-distribution.md`, `../WorkstreamControl/ACTIVE_LANES.md`, and `../WorkstreamControl/HEAVY_RUN_LOCK.md`.

This skill routes; it does not package, sign, notarize, create an installer, upload, publish, or claim release status. Packaging belongs to `echoes-package-provenance`; Developer ID/keychain/notary/installer work belongs to `echoes-developer-id-notarization-installer`; a clean-machine run belongs to `echoes-clean-machine-install-qualification`.

Any package attempt requires a current detailed Heavy-Run lease acquired through the live control process and explicitly released afterward; never reuse, assume, or invent a lease. The package skill must require a clean detached dedicated worktree at the exact pushed live `origin/main`, equality among `HEAD`, `origin/main`, and live remote, hydrated LFS, cleared `GIT_*` overrides, a new archive, and at least 60 GiB free on both archive and internal filesystems. The authorized configuration comes from the live gate: `package_macos.sh` is currently a Development procedure, so never silently substitute Shipping.

Developer ID credentials, keychain use, notarization submission, installer creation, upload, and publishing are external-state actions requiring separate explicit Angelis authority. A submission receipt, ad-hoc signature, local launch, or Development package is not notarization or release qualification.
