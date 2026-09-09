# Echoes interface art

Author and owner: Angelis Pseftis

The original visual targets remain `site/assets/concepts/echoes-hud-traditional-console.jpg` and
`echoes-main-menu-bridge.jpg`. SPEC-UI-007 and the newest owner direction in
`Docs/VisualTargetSpecification.md` govern the bottom tactical console. Incidental concept labels,
resources and unsupported menu modes do not define gameplay.

`command-bridge-background.png` is a derived, text-free menu plate, with provenance in
`provenance.json`. It preserves the Command Bridge composition while allowing real interactive
controls to be rendered over it. It does not constitute the specified real-time 3D bridge scene.
Run `Scripts/import_hud_art.py` through the Unreal Python editor route to import the registered source
into `/Game/Art/UI/T_EBS_CommandBridge`; do not overwrite the original concept. Start a fresh editor
process after import so the native widget constructor resolves its cooked texture reference.

The interface implementation uses C++ UMG widgets: ShellWidget supplies the shared menu presentation;
FieldHudWidget supplies tactical panels, campaign selection, technology, multiplayer, reconnect and
tutorial surfaces. Command glyphs are original resolution-independent line geometry in
EchoesHudGlyph; action labels remain visible and authoritative. Health telemetry reads the same
player-scoped selection data as the text panel. No portrait or faction capability is invented.

The first presentation batch is integrated into the main checkout under an exclusive UI integration
window. The editor build passed and the registered Texture2D imported and saved successfully. The
first focused suite exposed a detached Slate fixture that needed a second layout pass for automatic
text wrapping. After that fixture correction, all eleven focused checks passed, including pointer/focus,
HUD authority, camera and checkpoint recovery tests. The first rendered Development-game inspection
confirmed the title backdrop, keyboard title/settings/tutorial entry, UI scale at 80% and 150%, and
high-contrast settings. Pointer click/drag did not establish activation; tutorial selection and the
remaining route matrix are unverified. The inspection found excess menu height and undersized console
text; the corresponding polish is staged in the isolated HUD worktree and is not yet built.

Import and automation used protected ephemeral save storage. The rendered game exited normally (0)
and the wrapper cleaned its scoped storage, but the wrapper returned 8 because ordinary settings and
tutorial use had created scoped save files before cleanup. This is not an empty-scope automation pass.
The protected-path and synthetic-denial checks remained true.
Evidence is retained at `BuildArtifacts/Evidence/hud-concept-completion-20260909` relative to the
workspace root. No source-only, image-generation or headless result is a rendered HUD pass.

The second revision was authored in the isolated `ui/concept-console-completion` worktree and its
source is now present in this checkout: `EchoesPlayerController.h` consumes `EchoesInputBindingModel`
and `EchoesFeedbackHistoryModel`, and `EchoesFieldHudWidget.cpp` consumes `EchoesHudGlyph`. It has not
been compiled, rendered or test-run here. It adds compact title/briefing geometry, grouped Options,
larger console text, a visible compact resource strip, active binding prompts, Controls capture and
validation, filterable command history, a real battlefield Menu, canonical roster names/roles, maintenance
command buttons, and a scoped resource monitor. New source tests cover those routes and their failure cases;
their results remain NOT_RUN. Main integration and heavy engine work remain with the current backend
and environment owners until explicit handoff. No source-only result establishes rendered acceptance.

## Verification scope

The coordinated engine pass must cover title and campaign entry, skirmish setup/briefing, pause,
results/charts, settings and display rollback, save/load and destructive confirmations, errors,
credits, help, replay browser/transport, campaign map/inspector, multiplayer entry/lobby/local menu,
reconnect, technology and tutorial dialogs. Shared styling alone does not establish each route passed.

Retain keyboard arrival, forward/reverse focus, activation, pointer hit bounds, modal swallowing,
Escape recovery, disabled reasons, and controls at HUD scale 0.8/1.0/1.5 and high contrast. Exercise
live resize without losing slider capture or focus. Health fills must reflect player-scoped health
and keep their corresponding details with each selected entry. Controls must reject collisions before
mutation, preserve bindings after cancellation, resolve current prompts, and survive an actual restart.
The input persistence test must refuse all writes unless its Input config is beneath the launcher's
isolated UserDir; it restores both config bytes/cache and active mappings.

Command history consumes the player-scoped backend event contract. Queued, applied and no-effect
results remain distinct, retained-history loss stays visible, and live history never substitutes for
replay history. Filter controls precede the event list and incoming updates preserve the reading widget.
The resource monitor separates already invested costs from waiting uninvested requests and preserves
network projection limits. Preserve timers identify scheduled opportunities, conditional on uncontested
control. Resource income still needs exact backend 30-second and 60-second credit telemetry; changing liquid
balances cannot substitute for income. Canonical names and roles now come from the same immutable catalog used to build simulation rules.
Detailed purpose/use/limitation/counterplay text is being traced to the normative roster clauses.
Unexposed resource and route telemetry remains unavailable, not filled with estimates.
