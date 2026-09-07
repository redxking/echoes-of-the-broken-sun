---
title: Provisional asset cards — reconciliation bundle for Docs/Requirements.md
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: PROPOSED. Nothing here is applied to Docs/Requirements.md; the owner's ruling of 2026-09-07 makes this reconciliation a precondition of final technical qualification.
---

# Provisional asset cards — reconciliation bundle

Every card below was authored in the isolated production worktree AFTER its candidate concept was traced, and each package was built and imported against it. They are rendered here in the Requirements card grammar (`.MESH_PROP` / `.TEX_MAPS` / `.MAT_RULE` / `.ANIM_RIG` / `.VFX_POLY`) so they can be reconciled with §18.2 in one pass. Card IDs keep the `.ASSET` suffix used in the worktree; the owner may renumber. The Riftstalker's provisional card is SUPERSEDED by the authoritative `REL-ART-005.KA.RIFTSTALKER` and is not proposed; its rig amendment is appended instead.

#### [Asset Card: REL-BLD-016.KA.HEARTH.ASSET — Visual Asset Manifest] (PROPOSED; source kharuun-asset-cards.json; package EBS-KHA-BLD-001)
* .MESH_PROP: LOD0 polycount ceiling ≤8000 tris; LOD1 ceiling ≤3500 tris. Scope: the complete assembly including any articulated parts and sub-objects. Footprint [5, 5] tiles; Nanite False; pivot dome ground-contact centre. Components: A wide grown dome of banded strata, breadth greater than shell height; Several arched worker hollows at the base with walkable thresholds (five in the current build); A matter-intake cleft whose silhouette is distinct from a worker arch: lower and wider, with a receiving lip and a mineral-lined recess; A crown of rooted adaptation spires emerging from the shell strata, not mounted on it.
* .TEX_MAPS: 2048x2048 packed PBR per REL-ART-029; micro-noise detail normal on all mineral surfaces; worn-path wear masked into the thresholds and the cleft apron.
* .MAT_RULE: construction: Faceted mineral only. The shell is banded strata sampled as straight-sided prisms; a subdivided or smoothed dome breaches REL-ART-029.; emissive: Localised amber, <= 15% of surface area as a CEILING not a target. Amber appears only in the worker hollows, the growth seams between bands, and the spire veins.; forbidden: No machined panel, bolt, conduit or plate seam; no cyan; nothing volcanic — no lava, fire, smoke or molten cracks.
* .ANIM_RIG: rig: Static primary structure. Canon's motion is a slow interior glow breath and a cleft settle on delivery, neither of them geometric.; articulated_requirement: PENDING, not waived: a small articulated assembly for the cleft settle on delivery. Do not mark this asset compliant until it is implemented or this card is amended.; root_motion: None. The Hearth never moves and cannot be rebuilt in standard play.. Sockets: Target_Anchor_Center, Unit_Emergence, Rally_Default, Matter_Dropoff, Adaptation_Crown.
* .VFX_POLY: States — working: interior glow breathes slowly; damaged: a spire dark, strata cracked; destroyed: ceramic collapse inward. Readability — LOD1 shall preserve the crown, the worker hollows, the intake cleft and the headquarters silhouette at gameplay distance. A LOD that drops the spires, closes the hollows, or loses the cleft's distinct silhouette fails this card regardless of triangle count. The intake cleft must never read as another worker arch: that distinction is the asset's whole navigational job. Damage must be legible in the tactical camera, not only in a close view.
* Provenance: traced from —; canon SPEC-BLD-016.KA.HEARTH, Bible line 555.

#### [Asset Card: REL-BLD-016.KA.WAYSTONE.ASSET — Visual Asset Manifest] (PROPOSED; source kharuun-asset-cards.json; package EBS-KHA-BLD-002)
* .MESH_PROP: LOD0 polycount ceiling ≤5000 tris; LOD1 ceiling ≤2200 tris. Scope: the complete assembly including the root ring and the grown carriage. Footprint [2, 2] tiles; Nanite False; pivot ground-contact centre under the monolith. Components: A tall faceted monolith of stacked dark strata courses with amber seams; A stepped root plinth at the base; A ring of root-strata that splays flat into the ground when rooted and retracts into braided bundles when mobile; A grown carriage plate on four stubby legs that lifts the monolith when uprooted.
* .TEX_MAPS: 2048x2048 packed PBR per REL-ART-029; ground-contact wear masked into the root ring.
* .MAT_RULE: construction: Faceted dark strata monolith. The carriage is grown mineral, not a machined chassis and not a tread.; emissive: Amber seams up the monolith, <= 15% surface area as a ceiling. Seam brightness is the rooted/uprooted tell only in combination with the roots; it is never the sole tell.; forbidden: No wheels, tracks, treads, pistons or machined carriage. No cyan.
* .ANIM_RIG: rig: Own rig, not shared: root plus a root-ring assembly and a carriage assembly. Provisional plan is 7 bones — root, ring_sink, carriage_lift and FOUR carriage feet. It is NOT the Hearth's static plan and NOT any unit's limb plan. (Corrected 2026-09-07: this card first said three feet, written before the candidate was traced. The candidate's MIGRATING view shows four legs under the carriage plate, and the concept governs the form, so the card was corrected rather than the model bent to fit it.); clips: ['rooted_idle', 'uproot (40 ticks)', 'mobile_move (120 cm/s)', 'root (60 ticks: preparation, contact, settling, release)', 'restore']; root_motion: None. The runtime owns translation while mobile.. Sockets: Target_Anchor_Center, Matter_Dropoff, Root_Ring_Center, Carriage_Front.
* .VFX_POLY: States — rooted: the root ring is sunk into the ground and the amber seams hold a steady low glow; uprooted_mobile: the ring is clear of the ground, the carriage carries the monolith, and the seams read unsettled; damaged: seams broken and strata chipped; destroyed: the monolith falls and the ring is left in the ground. Readability — Rooted or moving shall be readable from the roots alone at gameplay distance, per canon. A LOD that removes the root ring fails this card. The vulnerable uproot and root windows are the opponent's counterplay, so both transitions must be visibly distinct from the settled states.
* Provenance: traced from —; canon SPEC-BLD-016 Waystone row.

#### [Asset Card: REL-BLD-016.KA.BASIN.ASSET — Visual Asset Manifest] (PROPOSED; source kharuun-asset-cards.json; package EBS-KHA-BLD-003)
* .MESH_PROP: LOD0 polycount ceiling ≤8000 tris; LOD1 ceiling ≤3500 tris. Scope: the complete assembly including every molt niche. Footprint [4, 4] tiles; Nanite False; pivot basin ground-contact centre. Components: A shallow bowl of grown strata, circular in plan; An amber-lit matrix pool at the centre, 0.75 of the bowl diameter as traced; SIX open curved molt alcoves cut into a continuous outer ring, each large enough to hold a warform and each individually readable. Six is this asset's VISUAL COMPONENT COUNT, confirmed by the owner on 2026-09-07 following the concept. It is NOT a concurrent-molt capacity and implies no gameplay slots.; A per-niche amber floor and back-wall lip that light only while that alcove is in use — the occupancy tell.
* .TEX_MAPS: 2048x2048 packed PBR per REL-ART-029; the pool surface carries its own emissive mask channel so brightness can be driven without a second material.
* .MAT_RULE: construction: A shallow bowl of grown strata. Faceted throughout.; emissive: Amber concentrated in the matrix pool at the centre, with a lesser read in the molt niches. <= 15% surface area as a ceiling; the pool is allowed to dominate that budget because canon makes it the asset's identity.; forbidden: No machinery around the rim, no vats, no pipework. No cyan.
* .ANIM_RIG: rig: Own rig, not shared: a static bowl with per-alcove articulation. Root plus one bone per alcove, each carrying canon's crack-and-settle at molt completion. It is NOT the Hearth's plan and NOT the Waystone's carriage plan. The alcoves are OPEN — no hood or lid is required (owner ruling 2026-09-07: the open-alcove, crack-and-settle treatment is the selected direction).; clips: ['idle', 'growing (the pool brightens)', 'molt_start', 'molt_complete (a crack and settle)', 'restore']; root_motion: None.. Sockets: Target_Anchor_Center, Rally_Default, Pool_Center, Molt_Niche_01..N.
* .VFX_POLY: States — idle: the pool holds a low steady glow and every niche floor is dark; growing: the pool brightens; this is the pool's emissive, a material parameter, not geometry; molting: the alcoves in use have their floor and back-wall lip lit; the rest stay dark. Which alcoves are lit follows authoritative adaptation activity.; damaged: the bowl cracked and the pool dimmed; destroyed: the bowl broken and the pool dark. Readability — The pool must say 'grows warforms' and the alcoves must say 'adaptation choices' at gameplay distance; LOD1 shall preserve both. SIX open alcoves, following the concept. This is a visual component count only. Occupancy lighting shall reflect AUTHORITATIVE ADAPTATION ACTIVITY. It must not imply six independently available gameplay slots, and no six-unit limit, reservation, queue or other adaptation rule follows from this card. The number of lit alcoves is a presentation of what the runtime reports, never a promise about capacity. Maximum-zoom readability of the occupancy tell is OPEN until verified. The lit floor and back-wall lip are legible in the tactical camera but subtle at full zoom-out; this is not yet closed.
* Provenance: traced from —; canon SPEC-BLD-016 Growth Basin row.

#### [Asset Card: REL-BLD-016.KA.SPINE.ASSET — Visual Asset Manifest] (PROPOSED; source kharuun-asset-cards.json; package EBS-KHA-BLD-004)
* .MESH_PROP: LOD0 polycount ceiling ≤3000 tris; LOD1 ceiling ≤1200 tris. Scope: the complete assembly including the rooted socket. Footprint [2, 2] tiles; Nanite False; pivot socket ground-contact centre. Components: One tall rib of strata; Amber sensor nodules climbing it, individually addressable; A rooted socket at the base.
* .TEX_MAPS: 2048x2048 packed PBR per REL-ART-029.
* .MAT_RULE: construction: A single tall faceted rib of strata set into a rooted socket. Faceted, tapering, asymmetric.; emissive: Amber sensor nodules climbing the rib, <= 15% surface area as a ceiling. The nodules are individually addressable so they can light in sequence.; forbidden: No dish, antenna, radar or mast language — canon says 'a spine, not a weapon'. No cyan.
* .ANIM_RIG: rig: Own rig, not shared: root, socket, and a two-bone rib that can lean toward a detected direction. Provisional plan is 4 bones. It is NOT a turret rig — the rib leans, it does not aim.; clips: ['idle_pulse (a slow pulse that quickens with signatures)', 'detect_sweep (nodules light in sequence toward the source direction)', 'offline', 'restore']; root_motion: None.. Sockets: Target_Anchor_Center, Nodule_Base, Nodule_Tip, Socket_Root.
* .VFX_POLY: States — listening: the nodules pulse slowly; contact: the nodules light in sequence toward the source direction; damaged: part of the nodule run dark and the rib chipped; destroyed: the rib snapped at the socket. Readability — The sequence direction is the asset's only information channel and shall survive LOD1: the nodules must remain individually resolvable at gameplay distance. It must never read as a weapon or a comms mast.
* Provenance: traced from —; canon SPEC-BLD-016 Listening Spine row.

#### [Asset Card: REL-FAC-025.KA.TENDER.ASSET — Visual Asset Manifest] (PROPOSED; source kharuun-asset-cards.json; package EBS-KHA-UNT-001)
* .MESH_PROP: LOD0 polycount ceiling ≤4500 tris; LOD1 ceiling ≤1800 tris. Scope: the complete assembly. Footprint — tiles; Nanite False; pivot ground-contact centre. Components: Stocky cultivator body with forearms thickened by working strata; A resonance staff; A woven mineral-fibre sling of carried matter across the back; Amber nodules at the wrists.
* .TEX_MAPS: 2048x2048 packed PBR per REL-ART-029.
* .MAT_RULE: construction: Faceted Kharuun mineral anatomy. Two material slots maximum.; emissive: Amber <= 5% of surface area, concentrated at the working wrist nodules — tighter than the faction's 15% ceiling, by the owner's Tender selection.; forbidden: Surveyor machinery, tread language and the cyan treatment do not transfer.
* .ANIM_RIG: rig: The existing 19-bone humanoid cultivator rig, retained by owner selection, subject to deformation and animation checks.; clips: ['idle', 'move', 'turn', 'stop', 'gather (a kneeling press of the staff into strata)', 'grow (a slow circling walk leaving the first ring)', 'death']; root_motion: None.. Sockets: Staff_Grip, Sling_Anchor, Wrist_Node_L, Wrist_Node_R, Target_Anchor_Center.
* .VFX_POLY: States — loaded: the sling carries visible matter; unloaded: the sling hangs empty. Readability — Carried matter and the staff must say cultivator, not soldier, at gameplay distance.
* Provenance: traced from —; canon SPEC-UNIT-005, Bible line 551.

#### [Asset Card: REL-FAC-025.KA.CAIRNBACK.ASSET — Visual Asset Manifest] (PROPOSED; source kharuun-asset-cards.json; package EBS-KHA-UNT-003)
* .MESH_PROP: LOD0 polycount ceiling ≤8000 tris; LOD1 ceiling ≤3500 tris. Scope: the complete assembly. The grown mineral cover it creates is a SEPARATE asset with its own budget and is not counted here.. Footprint — tiles; Nanite False; pivot ground-contact centre. Components: A broad low body with the head carried low and protected; A layered strata back slab; Thick forelimbs capable of the heave.
* .TEX_MAPS: 2048x2048 packed PBR per REL-ART-029; the strata back carries a heat-hold gradient in its own mask.
* .MAT_RULE: construction: Broad low body; the back is a slab of layered heat-holding strata. Thick forelimbs, head low and protected.; emissive: Amber restricted to the seams between back strata, <= 15% surface area as a ceiling. The back is armour, not a lamp.; forbidden: No Meridian shield-panel language; the Bulwark's barrier system does not transfer.
* .ANIM_RIG: rig: Own rig, not shared: a heavy low-slung plan with four thick limb chains, a short protected neck, and a back-slab chain that flexes on the heave. Provisional plan is 22 bones, weighted toward the limbs and the back rather than the extremities.; clips: ['idle', 'move (heavy stone footfalls)', 'attack', 'heave (leaves a grown barrier behind it)', 'damage_chip', 'death (a ceramic slump)']; root_motion: None.. Sockets: Cover_Cast_Origin, Back_Slab_Center, Target_Anchor_Center, Foot_Contact_FL, Foot_Contact_FR.
* .VFX_POLY: States — intact: the back slab whole; chipped: strata visibly chipped by damage. Readability — BEFORE any detailing, establish a CONTRASTING SILHOUETTE against the Riftstalker: broader and heavier mass, and a different stance and gait, wherever its own concept supports them. Silhouette separation is the first task of that build, not a later correction (owner ruling 2026-09-07). It must remain distinguishable from the lean Riftstalker in MONOCHROME at tactical distance. Colour, emissive and texture do not count toward that separation. The strata back must say 'absorbs fire, becomes cover' at gameplay distance and shall survive LOD1. The heave must read as the source of the barrier that appears, so the cast origin and the barrier's placement must agree. Damage chips the strata visibly; that is the unit's health tell.
* Provenance: traced from —; canon SPEC-UNIT-007.

#### [Asset Card: REL-FAC-025.KA.RESONANT.ASSET — Visual Asset Manifest] (PROPOSED; source kharuun-asset-cards.json; package EBS-KHA-UNT-004)
* .MESH_PROP: LOD0 polycount ceiling ≤4500 tris; LOD1 ceiling ≤1800 tris. Scope: the complete assembly including every sensor fin. Footprint — tiles; Nanite False; pivot ground-contact centre. Components: A tall, extremely slender QUADRUPED on four long stilt legs — taller than it is long, which no other Kharuun unit is; An arched spine rising from the shoulders to a small head carried high; A dorsal array of translucent amber sensor fins along that spine, largest at mid-back and tapering fore and aft, each its own component so the runtime can brighten them in sequence; A delicate frame throughout: the silhouette must promise no fight.
* .TEX_MAPS: 2048x2048 packed PBR per REL-ART-029; the fins carry a translucency mask.
* .MAT_RULE: construction: Tall, thin, delicate frame. Faceted but slender — the delicacy is the read.; emissive: Translucent amber sensor fins along the spine and head, individually addressable so they can brighten in sequence. <= 15% surface area as a ceiling.; forbidden: No weapon read. Its 8 damage is incidental; the silhouette must not promise a fight.
* .ANIM_RIG: rig: Own rig, not shared: a slender QUADRUPED plan with a segmented spine carrying the addressable fin array. Provisional plan is 24 bones, weighted toward the spine and neck rather than the limbs — the inverse of the Cairnback's weighting, and unlike the Riftstalker's long-limbed one.; clips: ['idle', 'move (almost silent)', 'detect (fins brighten in sequence)', 'attack', 'death']; root_motion: None.; anatomy_correction: {'date': '2026-09-07', 'superseded': 'An earlier draft of THIS card described a slender BIPED with 26 bones.', 'status': 'SUPERSEDED AND NOT GUIDANCE. Retained only as history so the change is traceable. The quadruped plan above is the sole anatomy for this asset.', 'source_references': ['BuildArtifacts/Evidence/concept-discovery-20260906/resonant-review/resonant-candidate.png, LISTENING view: four stilt legs', 'the same file, DETECTING view: four legs, spine lowered, fins lit in sequence', 'the same file, SENSOR FINS detail: the dorsal array along the spine', 'BuildArtifacts/Evidence/asset-production-20260906T221157Z/concept-crops/all/EBS-PKG-KA-RESONANT/EBS-CON-KHA-UNT-004.png'], 'confirmed_by_owner': 'not yet reviewed; corrected against the concept under the standing rule that the concepts define every asset'}. Sockets: Fin_Array_Base, Fin_Array_Tip, Target_Anchor_Center, Emitter_Muzzle.
* .VFX_POLY: States — passive: fins dark and still; detecting: fins brightening in sequence. Readability — Fins and delicacy must say 'listens, does not fight' at gameplay distance. The detection sequence is the unit's information channel and shall survive LOD1: fins must stay individually resolvable. It must remain distinguishable from BOTH other Kharuun quadrupeds in MONOCHROME at tactical distance. Its own separation is that it is taller than it is long, which neither of the others is; colour and emissive do not count. Triangle counts under the ceiling are headroom, not sufficiency.
* Provenance: traced from —; canon SPEC-UNIT-008.

#### [Asset Card: REL-BLD-017.HC.CONCORDANCE.ASSET — Visual Asset Manifest] (PROPOSED; source hollow-choir-asset-cards.json; package EBS-HOL-BLD-001)
* .MESH_PROP: LOD0 polycount ceiling ≤8000 tris; LOD1 ceiling ≤3500 tris. Scope: the complete assembly including every slab pair and its offset duplicate. Footprint [5, 5] tiles; Nanite False; pivot disc ground-contact centre. Components: A low circular disc plinth; A ring of fourteen slab PAIRS: each a main vitrified slab with an offset duplicate 3 cm behind it, the pair reading as one object that has not decided where it is; A ground intake apron at the front of the ring where matter is delivered; Magenta fracture edging on the slab borders.
* .TEX_MAPS: 2048x2048 packed PBR (albedo, normal, packed roughness/metallic, superposition mask), following the HC unit cards' stack. The slabs carry a vitrified glass core value under a stone face.
* .MAT_RULE: construction: Flat vitrified slabs standing on a low disc. No mineral growth, no plating, no machinery: the Choir builds with placed geometry, not grown or engineered mass.; emissive: Magenta Fracture edges on the slabs, <= 12% of visible surface footprint as a CEILING not a target.; forbidden: No Kharuun strata banding, no Meridian plate seams or conduits, no amber and no cyan.
* .ANIM_RIG: rig: Static primary structure. Provisional plan is root plus one bone per slab pair so pairs can drift independently; that drift is the faction's reality-bleed device and is NOT a canon motion clause for this building.; articulated_requirement: PENDING, not waived: the pipeline expects a static primary structure with role-required articulated components.; root_motion: None. The Concordance never moves and is the faction's Command Core.. Sockets: Target_Anchor_Center, Unit_Emergence, Rally_Default, Matter_Dropoff, Choir_Thread_Center.
* .VFX_POLY: States — working: every pair whole, the fracture edging lit, the intake apron clear; damaged: one pair cracked, as the candidate's DAMAGED PAIR panel draws it; destroyed: the ring broken: slabs down, the disc scarred. Readability — The ring must read as a ring at gameplay distance, and the intake apron must read as the place matter goes. LOD1 shall preserve the ring, the pair count and the intake apron. The offset duplicate must remain visible as a duplicate; if it merges into its parent slab the reality-bleed read is lost. Triangle counts under the ceiling are headroom, not sufficiency.
* Provenance: traced from BuildArtifacts/Evidence/concept-discovery-20260906/concordance-review/concordance-candidate.png (WORKING, RING / INTAKE, DAMAGED PAIR), traced 2026-09-07 before this card was written; canon SPEC-BLD-017.HC.CONCORDANCE.

#### [Asset Card: REL-BLD-017.HC.INTERVAL.ASSET — Visual Asset Manifest] (PROPOSED; source hollow-choir-asset-cards.json; package EBS-HOL-BLD-002)
* .MESH_PROP: LOD0 polycount ceiling ≤3000 tris; LOD1 ceiling ≤1200 tris. Scope: the complete assembly including both arches, all four feet and the drop-off pad. Footprint [2, 2] tiles; Nanite False; pivot ground-contact centre between the four feet. Components: Two ribbon arches crossing diagonally over the footprint; Four flat foot plates, one at each arch end; A flat drop-off pad on the ground with delivered matter on it; Magenta fracture edge lines along both borders of each arch.
* .TEX_MAPS: 2048x2048 packed PBR following the HC unit cards' stack; the arch ribbons carry a vitrified glass core value.
* .MAT_RULE: construction: Two flat ribbon arches crossing over a bare footprint. No walls, no roof, no machinery: the Loom is a frame, not a building.; emissive: Magenta Fracture edge lines along both borders of each arch, <= 12% of surface area as a CEILING not a target.; forbidden: No Kharuun strata, no Meridian plates or conduits, no amber, no cyan.
* .ANIM_RIG: rig: Static primary structure. Provisional plan is root plus one bone per arch so the two can drift independently — the faction's reality-bleed device, NOT a canon motion clause for this building.; articulated_requirement: PENDING, not waived.; root_motion: None.. Sockets: Target_Anchor_Center, Matter_Dropoff, Arch_Crossing_Center, Upkeep_Signal.
* .VFX_POLY: States — supplied: the edge lines hold a steady glow; upkeep_tick: a surge band runs the arch crowns as the 600-tick charge falls due; insolvent: every edge line dark; the frame reads as unpowered; destroyed: the arches down, the feet and pad left. Readability — Supplied, ticking and insolvent must be distinguishable at gameplay distance; the player's Dawn solvency is the read. The drop-off pad must read as a place workers deliver to. LOD1 shall preserve both arches, the four feet, the pad and the edge lines. Triangle counts under the ceiling are headroom, not sufficiency.
* Provenance: traced from BuildArtifacts/Evidence/concept-discovery-20260906/interval-loom-review/interval-loom-candidate.png (SUPPLIED, UPKEEP TICK, INSOLVENT), traced 2026-09-07 before this card was written; canon SPEC-BLD-017.HC.INTERVAL.

#### [Asset Card: REL-BLD-017.HC.CHORUS.ASSET — Visual Asset Manifest] (PROPOSED; source hollow-choir-asset-cards.json; package EBS-HOL-BLD-003)
* .MESH_PROP: LOD0 polycount ceiling ≤6000 tris; LOD1 ceiling ≤2400 tris. Scope: the complete assembly including both posts, the warp and the floating beam. Footprint [4, 4] tiles; Nanite False; pivot platform ground-contact centre. Components: A low rectangular platform filling the footprint; Two upright posts, one at each end of the platform, with magenta edge strips on their inner borders; A warp of fine horizontal threads strung between the posts; A beam hovering above the warp, touching nothing.
* .TEX_MAPS: 2048x2048 packed PBR following the HC unit cards' stack.
* .MAT_RULE: construction: A low platform with two upright posts and a beam that hovers unsupported above them. Nothing braces the beam, and nothing should be added to.; emissive: Magenta Fracture on the post edge strips and the beam's underside, <= 12% of surface area as a CEILING not a target.; forbidden: No Kharuun strata, no Meridian plates or conduits, no amber, no cyan.
* .ANIM_RIG: rig: Static primary structure. Provisional plan is root, one bone per post and one for the floating beam, so the beam can hang out of step with the posts — the faction's reality-bleed device, NOT a canon motion clause for this building.; articulated_requirement: PENDING, not waived.; root_motion: None.. Sockets: Target_Anchor_Center, Rally_Default, Weave_Center, Research_Beam, Unit_Emergence.
* .VFX_POLY: States — producing: the warp is strung and the weave centre is occupied; a unit is being woven; researching: the warp doubles and the hovering beam lights; the weave centre is empty; insolvent: warp and beam dark; the frame reads as unpowered; destroyed: the posts down and the beam fallen onto the platform. Readability — Producing and researching must be distinguishable at gameplay distance, and both from insolvent. The woven form at the weave centre is a LIGHT EFFECT, not geometry; the mesh provides a socket only. The hovering beam must read as unsupported; adding a strut destroys the faction read. LOD1 shall preserve both posts, the warp and the beam. Triangle counts under the ceiling are headroom, not sufficiency.
* Provenance: traced from BuildArtifacts/Evidence/concept-discovery-20260906/chorus-loom-review/chorus-loom-candidate.png (PRODUCING, RESEARCHING, WEAVING DETAIL), traced 2026-09-07 before this card was written; canon SPEC-BLD-017.HC.CHORUS.

#### [Asset Card: REL-BLD-017.HC.ANCHOR.ASSET — Visual Asset Manifest] (PROPOSED; source hollow-choir-asset-cards.json; package EBS-HOL-BLD-004)
* .MESH_PROP: LOD0 polycount ceiling ≤4000 tris; LOD1 ceiling ≤1600 tris. Scope: the complete assembly including the stepped plinth, the tapering spire and its apex cap. Footprint [2, 2] tiles; Nanite False; pivot plinth ground-contact centre. Components: A low stepped hexagonal plinth, inside the 2x2 footprint; A tapering hexagonal spire rising from the plinth to a pointed apex cap; Magenta arris lines up the spire's edges; A diamond register lattice on the spire's faces, the read the concept's REGISTER DETAIL panel calls out.
* .TEX_MAPS: 2048x2048 packed PBR following the HC unit cards' stack.
* .MAT_RULE: construction: A tapering hexagonal spire standing on a low stepped hexagonal plinth. No arms, no dish, no aperture: the Anchor is a marker, and its whole silhouette is the taper.; emissive: Magenta Fracture on the shaft's arris lines and on the diamond register up its face, <= 12% of surface area as a CEILING not a target.; forbidden: No Kharuun strata, no Meridian plates or conduits, no amber, no cyan.
* .ANIM_RIG: rig: Static primary structure. Provisional plan is root, one bone for the spire and one for the apex, so the spire can drift out of phase with its own tip — the faction's reality-bleed device, NOT a canon motion clause for this building.; articulated_requirement: PENDING, not waived.; root_motion: None.. Sockets: Target_Anchor_Center, Field_Ring_Origin, Apex_Beacon, Register_Center.
* .VFX_POLY: States — field_active: arrises and register lit; the 700 cm aura ring is projected from Field_Ring_Origin; field_lost: arrises and register dark; the spire reads as inert stone; destroyed: PROPOSED DESTRUCTION STUDY, not an approved treatment: the candidate draws no destroyed panel. Judge it on navigation, selection and gameplay readability, not on concept fidelity, and do not read the built segmentation as mechanically mandatory.. Readability — Field active and field lost must be distinguishable at gameplay distance. The 700 cm aura ring is a LIGHT EFFECT, not geometry; the mesh provides Field_Ring_Origin only. Its radius belongs to gameplay data, and the mesh must not imply any other radius. The Anchor must stay separable from the Chorus Loom. Evaluate that in representative terrain, lighting, team colour treatment and tactical camera views. A monochrome IoU figure is comparison evidence only and carries no acceptance meaning: no criterion is defined (owner ruling 2026-09-07). The taper must read at tactical distance: this is the faction's tallest, thinnest silhouette and must not be confused with the Chorus Loom's posts. LOD1 shall preserve the taper, the plinth steps and the arris lines. Triangle counts under the ceiling are headroom, not sufficiency.
* Provenance: traced from BuildArtifacts/Evidence/concept-discovery-20260906/phase-anchor-review/phase-anchor-candidate.png (FIELD ACTIVE, FIELD LOST, REGISTER DETAIL), traced 2026-09-07 before this card was written; canon SPEC-BLD-017.HC.ANCHOR.

---

## Appendix — Riftstalker rig amendment (PROPOSED, unchanged from the package document)

---
title: Targeted amendment proposal — REL-ART-005.KA.RIFTSTALKER .ANIM_RIG
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: PROPOSAL. The requirements document has NOT been amended and this asset is NOT technically accepted.
generated_from: build_riftstalker.py BONES (do not edit this file by hand)
---

# Targeted amendment proposal — `REL-ART-005.KA.RIFTSTALKER` `.ANIM_RIG`

## What this is

The owner selected the existing 22-bone quadruped rig as the production direction on 2026-09-07,
and directed that a limb segment and the caster's independent aim must NOT be removed merely to
reach 14 bones. This proposes the smallest change to the card that records that selection.

**It is a proposal.** `Docs/Requirements.md` is unchanged, amending it is outside this worktree's
authority, and nothing here constitutes technical acceptance.

## The discrepancy, stated plainly

| | Value |
|---|---|
| Card `.ANIM_RIG` as written | 14-bone kinematic rigging layout |
| Asset as built | 22-bone layout |
| Difference | 8 bones |

The card's figure and the concept's anatomy are inconsistent. **I do not know why the card says 14.**
An earlier note of mine suggested the figure implied the card intended a biped; that was an
inference about provenance I cannot support, and it is withdrawn. The only established facts are
the number in the card, the four-legged animal in the selected concept, and the owner's selection
of the quadruped.

Until an authoritative amendment is applied, the asset is **non-compliant with the card's
`.ANIM_RIG` clause as written**, and that is recorded in its manifest and README rather than
treated as settled.

## Proposed replacement text for `.ANIM_RIG`

> `.ANIM_RIG`: 22-bone kinematic rigging layout for a quadruped chassis: a root, a body, a
> two-bone forward prow, a two-bone caster mount that aims independently of the gait, and four
> limb chains of four bones each. Skeleton layout carries fixed named sockets:
> `VFX_Muzzle_Shard_01`, `VFX_Molt_Origin_Base`, and `Target_Hitbox_Center`. Locomotion requires
> direction-independent movement speeds and turn rates with instant snaps under Reduced Motion.

Only the bone count and the chassis description change. The socket names, the locomotion clause
and every other clause of the card are untouched.

## Bone hierarchy and purpose

| # | Bone | Parent | Head (cm) | Purpose |
|---|---|---|---|---|
| 1 | `root` | `—` | (0, 0, 0) | ground-contact centre between the feet; the runtime owns translation |
| 2 | `body` | `root` | (0, 0, 156) | the carapace body: the low forward posture lives in its rest pose |
| 3 | `prow_base` | `body` | (58, 0, 148) | where the carapace begins tapering forward |
| 4 | `prow_tip` | `prow_base` | (112, 0, 122) | the prow's forward point |
| 5 | `caster_yaw` | `body` | (46, 0, 176) | shard-caster mount 1: aims independently of the gait |
| 6 | `caster_pitch` | `caster_yaw` | (66, 0, 176) | shard-caster mount 2 |
| 7 | `fl_hip` | `body` | (34, -46, 144) | fl leg: attachment at the body |
| 8 | `fl_upper` | `fl_hip` | (34, -46, 144) | fl leg: upper segment, pivoting at the hip toward the high outboard knee |
| 9 | `fl_lower` | `fl_upper` | (65, -96, 152) | fl leg: lower segment, pivoting at the knee |
| 10 | `fl_foot` | `fl_lower` | (96, -74, 26) | fl leg: the pointed foot, pivoting at the ankle |
| 11 | `fr_hip` | `body` | (34, 46, 144) | fr leg: attachment at the body |
| 12 | `fr_upper` | `fr_hip` | (34, 46, 144) | fr leg: upper segment, pivoting at the hip toward the high outboard knee |
| 13 | `fr_lower` | `fr_upper` | (65, 96, 152) | fr leg: lower segment, pivoting at the knee |
| 14 | `fr_foot` | `fr_lower` | (96, 74, 26) | fr leg: the pointed foot, pivoting at the ankle |
| 15 | `rl_hip` | `body` | (-70, -46, 144) | rl leg: attachment at the body |
| 16 | `rl_upper` | `rl_hip` | (-70, -46, 144) | rl leg: upper segment, pivoting at the hip toward the high outboard knee |
| 17 | `rl_lower` | `rl_upper` | (-105, -96, 152) | rl leg: lower segment, pivoting at the knee |
| 18 | `rl_foot` | `rl_lower` | (-140, -74, 26) | rl leg: the pointed foot, pivoting at the ankle |
| 19 | `rr_hip` | `body` | (-70, 46, 144) | rr leg: attachment at the body |
| 20 | `rr_upper` | `rr_hip` | (-70, 46, 144) | rr leg: upper segment, pivoting at the hip toward the high outboard knee |
| 21 | `rr_lower` | `rr_upper` | (-105, 96, 152) | rr leg: lower segment, pivoting at the knee |
| 22 | `rr_foot` | `rr_lower` | (-140, 74, 26) | rr leg: the pointed foot, pivoting at the ankle |

## Why the count cannot fall to 14 without losing something the owner kept

- The four limb chains account for **16 bones** on their own, before any body, prow or caster.
- Dropping one segment per limb reaches 18, still above 14, and costs the knee articulation the
  gait depends on.
- Dropping the two-bone caster mount reaches 20 and costs the independent aim that canon's
  "fires while it moves" requires.
- Reaching 14 needs both, which the owner's ruling of 2026-09-07 excludes.

## What this proposal does not do

- It does not amend `Docs/Requirements.md`.
- It does not waive the card's three vertex ID channels for public molting phase transitions,
  which remain required and unimplemented.
- It does not assert technical acceptance of the asset.


