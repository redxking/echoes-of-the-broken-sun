---
name: echoes-package-sign-notarize
description: Route macOS Echoes packaging, provenance, Developer ID/notarization, installer, and clean-machine work to separately authorized skills without executing credentialed or release-state actions.
metadata:
  author: Angelis Pseftis
---

# Echoes package release router

Inherits [AGENTS.md](../../../AGENTS.md), [Docs/README.md](../../../Docs/README.md) and [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md). Fetch the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md) records with `python3 Scripts/req.py both <ID>...`, never the masters whole.

Use this compatibility router only to choose the narrow work skill.

Package identity and reproducibility: [echoes-package-provenance](../echoes-package-provenance/SKILL.md). Developer ID signing, notarization, installer, and clean-machine qualification: [echoes-developer-id-notarization-installer](../echoes-developer-id-notarization-installer/SKILL.md) and [echoes-clean-machine-install-qualification](../echoes-clean-machine-install-qualification/SKILL.md).

This router does not authorize cross-domain changes or replace the selected skill's required evidence.
