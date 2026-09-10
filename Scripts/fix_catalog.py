import re

with open('Docs/ComponentDesignCatalog.md', 'r') as f:
    text = f.read()

replacements = {
    r'Anchor \[Command Headquarters\].*?\n- \*\*Footprint & Scale:\*\* .*?\n': 
        'Anchor [Command Headquarters] (`SPEC-BLD-015.MC.ANCHOR`)\n- **Footprint & Scale:** 5x5 build grid (1000 cm x 1000 cm), octagonal reinforced perimeter hull.\n',
    r'Power Link \[Conduit Pylon\].*?\n- \*\*Footprint & Scale:\*\* .*?\n': 
        'Power Link [Conduit Pylon] (`SPEC-BLD-015.MC.LINK`)\n- **Footprint & Scale:** 2x2 build grid (400 cm x 400 cm), slender 18-meter hexagonal pylon.\n',
    r'Array Foundry \[Mech & Vehicle Assembly\].*?\n- \*\*Footprint & Scale:\*\* .*?\n': 
        'Array Foundry [Mech & Vehicle Assembly] (`SPEC-BLD-015.MC.FOUNDRY`)\n- **Footprint & Scale:** 4x4 build grid (800 cm x 800 cm), industrial fabrication facility.\n',
    r'Aegis Post \[Defensive Turret\].*?\n- \*\*Footprint & Scale:\*\* .*?\n': 
        'Aegis Post [Defensive Turret] (`SPEC-BLD-015.MC.AEGIS`)\n- **Footprint & Scale:** 2x2 build grid (400 cm x 400 cm), low-profile fortified bunker.\n',
    r'Memory Hearth \[Assembly Headquarters\].*?\n- \*\*Footprint & Scale:\*\* .*?\n': 
        'Memory Hearth [Assembly Headquarters] (`SPEC-BLD-016.KA.HEARTH`)\n- **Footprint & Scale:** 5x5 build grid (1000 cm x 1000 cm), 145-meter living geological dome.\n',
    r'Waystone \[Mobile Supply & Root Node\].*?\n- \*\*Footprint & Scale:\*\* .*?\n': 
        'Waystone [Mobile Supply & Root Node] (`SPEC-BLD-016.KA.WAYSTONE`)\n- **Footprint & Scale:** 2x2 build grid (400 cm x 400 cm) rooted; 80-meter monolithic pillar.\n',
    r'Growth Basin \[Warform Gestation Facility\].*?\n- \*\*Footprint & Scale:\*\* .*?\n': 
        'Growth Basin [Warform Gestation Facility] (`SPEC-BLD-016.KA.BASIN`)\n- **Footprint & Scale:** 4x4 build grid (800 cm x 800 cm), circular stepped terrace.\n',
    r'Listening Spine \[Seismic Detection Array\].*?\n- \*\*Footprint & Scale:\*\* .*?\n': 
        'Listening Spine [Seismic Detection Array] (`SPEC-BLD-016.KA.SPINE`)\n- **Footprint & Scale:** 2x2 build grid (400 cm x 400 cm), slender 110-meter geological needle.\n',
    r'Concordance \[Choir Core Sanctuary\].*?\n- \*\*Footprint & Scale:\*\* .*?\n': 
        'Concordance [Choir Core Sanctuary] (`SPEC-BLD-017.HC.CONCORDANCE`)\n- **Footprint & Scale:** 5x5 build grid (1000 cm x 1000 cm), levitating geometric hyper-structure.\n',
    r'Interval Loom \[Possibility Resonator / Pylon\].*?\n- \*\*Footprint & Scale:\*\* .*?\n': 
        'Interval Loom [Possibility Resonator / Pylon] (`SPEC-BLD-017.HC.INTERVAL`)\n- **Footprint & Scale:** 2x2 build grid (400 cm x 400 cm), 22-meter floating tuning fork silhouette.\n',
    r'Chorus Loom \[Entity Synthesis Matrix\].*?\n- \*\*Footprint & Scale:\*\* .*?\n': 
        'Chorus Loom [Entity Synthesis Matrix] (`SPEC-BLD-017.HC.CHORUS`)\n- **Footprint & Scale:** 4x4 build grid (800 cm x 800 cm), open-air spatial distortion basin.\n',
    r'Phase Anchor \[Dimensional Interceptor Turret\].*?\n- \*\*Footprint & Scale:\*\* .*?\n': 
        'Phase Anchor [Dimensional Interceptor Turret] (`SPEC-BLD-017.HC.PHASE_ANCHOR`)\n- **Footprint & Scale:** 2x2 build grid (400 cm x 400 cm), tri-fold levitating prism.\n'
}

for pattern, repl in replacements.items():
    text = re.sub(pattern, repl, text, flags=re.MULTILINE)

with open('Docs/ComponentDesignCatalog.md', 'w') as f:
    f.write(text)

print("Catalog footprints updated.")
