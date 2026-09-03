---
name: echoes-third-party-agent-skill-review
description: Review a third-party game-development agent skill or plugin at a pinned revision before any Echoes project-local installation, with scripts, hooks, dependencies, permissions, network, license, and overlap examined.
metadata:
  author: Angelis Pseftis
---

# Echoes third-party agent-skill review

Read `CLAUDE.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Requirements.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. This skill reviews; it does not install, execute, enable, update, or trust a package.

Resolve the primary repository, exact commit/tag, authorship, license, release date, supported engine/tool version, and every file that would enter the project. Inspect every `SKILL.md`, script, hook, installer, package manifest, lockfile, MCP definition, executable, generated file, dependency, credential request, network endpoint, and permission. Identify duplicate IDs, conflicts with Echoes directives, automatic invocation, source/generated mutations, editor/Python reach, destructive capability, telemetry, update behavior, and transitive downloads.

Prefer a minimal project-local selection over global or bulk installation. Copy no content until the owner authorizes the exact pinned set and provenance record; never run `npx`, install scripts, hooks, or MCP servers merely to list them when repository inspection can answer the question. A catalog badge, popularity count, vendor assertion, automated scanner, or open-source license is not a security guarantee.

Output a keep/reject/adapt matrix with exact evidence, least-privilege configuration, overlap with existing `echoes-*` skills, and remaining unknowns. Route security findings to `echoes-security-privacy`; any owner-approved project-local tooling import routes through `echoes-workstream-integration`, not the game asset register. Stop for an unpinned revision, unreadable dependency, ambiguous license, credential exposure, or owner authorization gap.
