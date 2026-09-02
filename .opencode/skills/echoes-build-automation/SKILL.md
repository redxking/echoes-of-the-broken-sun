---
name: echoes-build-automation
description: Build a narrow Echoes Unreal or simulation slice on the Mac while preserving lane ownership, baseline evidence, and reproducible logs; not for packaging or release claims.
metadata: { author: Angelis Pseftis }
---

# Echoes build automation

Before acting, read `CLAUDE.md`, `Docs/GameCompletionDirective.md`, the current limitations and evidence register in `Docs/Archive/ProjectLedger.md`, `../WorkstreamControl/ACTIVE_LANES.md`, and `../WorkstreamControl/HEAVY_RUN_LOCK.md`. Acquire a current detailed Heavy-Run lease before any build, then explicitly release it; never use a stale or self-invented lease. Select one unblocked directive gate and define its observable check before editing. Do not take another lane's files.

Use the smallest applicable Mac command: `Scripts/test_sim.sh`, `Scripts/test_content.sh`, `Scripts/build_editor.sh`, or the exact focused automation named by the gate. For Unreal tests set `TMPDIR` as required by `CLAUDE.md`. Bind every retained log to commit, dirty-tree status, target/configuration, command, exit status, and UTC time. A compile is not runtime evidence; a test is not packaged evidence.

Stop below the documented storage gates, on a missing Seagate volume, a lane conflict, or an untriaged baseline failure. Never relax a test, timeout, or gate. Record only results actually read this session, including warnings and claim limits, in the controlling ledger/evidence location when authorized.
