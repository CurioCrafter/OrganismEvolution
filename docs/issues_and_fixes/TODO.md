# TODO

## Rendering
- [x] Implement procedural creature mesh generation (replace oval spheres with generated bodies).
- [x] Align grass/tree projection FOV with terrain/camera.
- [x] Use shared view-projection for terrain/grass/trees/creatures/ground to keep depth consistent.
- [x] Ground creatures with terrain height sampling + mesh bounds offset + clearance.
- [x] Keep ground creature orientation horizontal in rendering (no tilt into terrain).
- [x] Improve vegetation height sampling (bilinear interpolation over terrain height map).
- [x] Expand creature mesh variety (body/head/tail categories) and add per-creature scale variation.
- [x] Add simple locomotion animation (bobbing/flap/swim) driven by speed.
- [x] Add nametag overlay toggle and distance control.
- [ ] Verify creature base scale vs. terrain height so bodies feel grounded and not oversized.
- [ ] Verify depth buffer consistency after shared view-projection change (no phasing while moving camera).
- [ ] Validate grass/tree visibility after scale changes; confirm no floating or horizon drift.
- [ ] Validate vegetation stays fixed in world space when camera moves (no LOD pop that feels like drift).
- [x] Align vegetation placement with TerrainRendererDX12 height sampling (single source of truth for terrain height).
- [x] Rebalance grass density + LOD thresholds for world-scale generation (avoid thin patches or sudden dropouts).
- [x] Review frustum culling bounds for trees (mesh bounds + scale produce correct sphere radius).
- [x] Constrain grass/vegetation generation to actual terrain bounds (avoid sky/void instances).
- [ ] Investigate any remaining texture glitches by tracing depth state + projection + terrain shader output.
- [ ] Add an optional debug draw mode for terrain/trees/creatures (bounding spheres + axes).
- [ ] Add proper sky rendering (skybox/atmosphere) beyond clear-color day/night.
- [ ] Evaluate remaining creature silhouette similarity; consider extra archetypes or feature toggles.

## Simulation / World
- [x] Add spawn/reset controls for creatures and food in debug panel.
- [x] Add spawn controls for flying/aquatic archetypes.
- [x] Respawn food on terrain height (not y=0).
- [ ] Keep baseline population alive (auto-respawn or minimum population floor).
- [x] Reconcile world bounds (tie creature bounds to terrain size to avoid off-terrain spawns).
- [x] Spawn initial flying/aquatic populations with varied subtypes.
- [x] Clamp spawn radius to world bounds in spawn tools.
- [ ] Add per-type energy presets for spawns.
- [x] Verify creature Y placement against terrain height after movement + steering updates.
- [ ] Add logging for first N creature positions when rendering shows zero on screen (diagnostic toggle).
- [x] Scale vegetation/grass generation to full terrain size.
- [ ] Check replay mode camera behavior to ensure it matches live camera transforms.
- [ ] Add basic aquatic food sources so fish don't starve immediately.

## UI / Input
- [x] Add FPS-style mouse look (click to capture/release mouse) so the window feels like a game.
- [x] Focus window on click, release cursor lock on focus loss, and capture mouse while locked.
- [ ] Ensure ImGui capture state is reflected in UI (small overlay: "UI has focus").
- [ ] Add explicit "Click to focus" hint when window doesn't have focus.
- [ ] Consider a key to toggle mouse capture for camera rotation vs. UI interaction.
- [ ] Ensure mouse capture can be toggled even when ImGui has focus (game-view click).

## Build / Tooling
- [ ] Clean up `nul` at repo root (breaks tools like `rg` on Windows).
- [ ] Decide whether `CMakeLists_DX12.txt` stays or gets merged with the main build.
- [ ] Fix warnings: `C4267` size_t -> u32 and `C4996` getenv usage.
- [ ] Add CI or local test target so tests under `tests/` run.

## Save / Replay
- [ ] Capture and restore RNG state for deterministic saves/replays.
- [ ] Update save/load to restore creature IDs and pool counters safely.
- [ ] Capture real genome/brain data in replays or remove unused fields.
