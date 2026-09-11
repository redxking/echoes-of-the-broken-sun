---
name: echoes-portability-security
description: "Assess or implement Echoes macOS-first release hardening, data integrity, privacy, and future-platform guardrails without overstating shipping or security evidence."
metadata:
  author: Angelis Pseftis
---

# Echoes portability and security

Inherits [AGENTS.md](../../../AGENTS.md), [Docs/README.md](../../../Docs/README.md) and [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md). Fetch the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md) records with `python3 Scripts/req.py both <ID>...`, never the masters whole.

Use this compatibility router only to choose the narrow work skill.

Platform and device portability: [echoes-platform-portability](../echoes-platform-portability/SKILL.md). Security, privacy, and data boundaries: [echoes-security-privacy](../echoes-security-privacy/SKILL.md). Package identity and reproducibility: [echoes-package-provenance](../echoes-package-provenance/SKILL.md). Packaging, signing, notarization, installer, and clean-machine routing: [echoes-package-sign-notarize](../echoes-package-sign-notarize/SKILL.md).

This router does not authorize cross-domain changes or replace the selected skill's required evidence.
