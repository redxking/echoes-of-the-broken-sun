# Historical replay writer fixtures

Author and owner: Angelis Pseftis

These baselines and checksums were emitted by the actual historical simulation writers,
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
