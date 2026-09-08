# Rift Niagara Authoring

Author and creator: Angelis Pseftis

This is an editor-only plugin for the isolated art sandbox. It is not installed in
the game project and does not attach any asset to gameplay, sockets, Blueprints, or
runtime data.

After a successful editor-plugin build and sandbox installation, call it from the
Unreal Python console with a new package path and an existing Amber material path:

```python
receipt = unreal.RiftNiagaraAuthoringLibrary.author_shard(
    "/Game/ArtSandbox/NS_EBS_RiftShard",
    "/Game/ArtSandbox/Materials/M_EBS_KHA_RiftShard_Amber.M_EBS_KHA_RiftShard_Amber",
)
print(receipt)
```

The returned JSON is a receipt. A `success: false` result is intentional for a
missing template, module, topology input, renderer, material, compilation result,
or save. The bridge does not report an empty or partially known template as authored.

The authored facts are: one `SpawnBurst_Instantaneous` shard, `User.ShardVelocity` with +X 1200 cm/s default,
0.5-second lifetime, supplied Amber sprite material, renderer/system shadows off,
and fixed local bounds from `(-620,-620,-620)` to `(620,620,620)`. This contains
the 600 cm maximum flight in every aim direction plus 20 cm clearance. Runtime
must normalize the supplied direction to 1200 cm/s. A fresh factory emitter is
used because the engine Minimal template did not contain the presumed modules. This is authoring evidence;
it does not establish VFX performance, game binding, gameplay timing, or visual
acceptance.
