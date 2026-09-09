# Historical replay writer fixtures

Author and owner: Angelis Pseftis

The original schema24/schema25 baselines and checksums were emitted by the historical simulation writers,
compiled from the exact source commits in each receipt on 2026-09-06. Each writer verified
its own replay before producing the fixture. The driver is retained in this directory.
The source archives and build/run evidence are retained under
`BuildArtifacts/Evidence/p0-p3-readiness-20260906/legacy-writer-oracles`.

Both replays use version 24, final tick 100, and the driver's two commands: player 0 sequence 1
produces a Worker at Core 1; sequence 2 moves Worker 3 to tile (7,7), both at tick 0. Other command
fields keep the historical default values. Each receipt retains the writer's final checksum,
source and baseline SHA-256 identities. These cases prove only the exercised old-writer
production and movement histories; they do not certify every legacy gameplay feature.

The final snapshot fixtures retain the state after those same two commands. Their separate
final receipts bind the exact snapshot and driver hashes for compatibility diagnosis.
The overlap receipts use a third command: player 0 sequence 3 produces another Worker at
Core 1 at tick 1. The historical busy producer resolves that command without queuing a unit.
`historical_overlap_writer.cpp` retains this driver; `LegacyReplayOracles.h` embeds the
baseline bytes and overlap checksums for detached Unreal replay transport tests.

The schema29-checkpoint fixture uses the same hash-verified archived schema29 writer with the default 64x64 Glass Scar checkpoint context. Its dedicated driver and receipt retain the original replay checksum and fixture/source hashes. It exercises legacy continuation through current game checkpoint encoding and reload; it is not a current snapshot relabeled as historical.

The schema30 Bulwark fixture was captured from the unchanged candidate28 simulation before
the deployment timing repair. It is an actual schema30/replay27 writer output, not a newer
snapshot projected backwards. The receipt binds the dirty source hashes, archived source,
driver and baseline. At tick1 the old writer deploys instantly and applies cover; at tick2 it
packs instantly. Those recorded checksums protect existing replay execution. This provenance
does not claim candidate28 was a released build or that its instant timing met requirements.

The schema30 Bulwark checkpoint fixture is emitted by that same hash-verified unchanged historical writer using the admitted 64x64 Glass Scar terrain and seed. Its separate driver and receipt retain the historical oracle for adapter save/reload tests. The earlier 24x24 combat baseline remains unchanged and continues to cover its original replay.

The schema31 construction-assist fixture was emitted on 2026-09-09 by the unchanged
candidate36 schema31/replay28 writer. `historical_schema31_assist_writer.cpp` and
`schema31-assist-receipt.json` bind the archived source and driver hashes to the real baseline
bytes and checksums. The retained source/build/output evidence is in
`BuildArtifacts/Evidence/backend-actions-20260909`. The original writer reproduced both a
cancelled/refunded site being revived by a later same-tick assist and a missing assist target
falling through to paid construction. Its receipt also records a snapshot reload bound to
the replay prefix, followed by another cancel/assist continuation. These are historical
compatibility oracles, not endorsements of those behaviors: current replay29 targets an
existing construction site only and rejects dead or missing sites without placing or paying
for another building. Historical replay28 remains historical; an explicitly new recording
rebases to the current semantics.
