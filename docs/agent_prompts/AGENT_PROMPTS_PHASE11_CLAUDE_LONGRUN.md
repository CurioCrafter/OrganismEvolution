# 10 Claude Agent Prompts: Phase 11 World/Spawn/Controls Quality (Long-Run, v1)

This phase is about quality and playability. The sim now runs, but the world, spawns, water, and camera controls do not match the intended experience. Each agent must do real research in the codebase and produce a polished plan plus concrete changes.

PARALLEL RUN RULES (MUST FOLLOW)
- Ownership map: Each agent owns the files listed in their prompt. Do not edit files owned by other agents.
- Handoff policy: If you must change an unowned file, do not edit it. Write a handoff note in `docs/PHASE11_AGENTX_HANDOFF.md` with concrete API requests and file paths.
- Main integration: Agent 10 is the only agent allowed to edit `src/main.cpp`.
- Main menu: Agent 2 is the only agent allowed to edit `src/ui/MainMenu.*`.
- UI integration: Agent 5 is the only agent allowed to edit `src/ui/SimulationDashboard.*` and `src/ui/CreatureInspectionPanel.*`.
- Shaders and water: Agent 7 is the only agent allowed to edit `Runtime/Shaders/Water*` and `src/graphics/WaterRenderer.*`.
- Every agent must add instrumentation (logs, counters, or debug UI) and validate in a 5-10 minute sim run.
- Each agent doc must include: files touched, new structs/functions, tuning constants and ranges, performance notes (rough cost), validation steps (what you observed), and integration notes for the main owner (Agent 10) or UI owner (Agent 5), if relevant.

RESEARCH REQUIREMENT (NON-NEGOTIABLE)
- Before coding, use `rg` to inventory relevant systems and read at least one prior doc in `docs/phase*` or `docs/archive` related to your task.
- Summarize findings in your deliverable (what exists, what is missing, and why the change is needed).

PLANNING EXPECTATIONS (NON-NEGOTIABLE)
- Provide a pre-implementation plan with dependencies and a sequencing checklist.
- Provide an execution plan: how the code will be called in runtime (call chain + update order).
- Provide concrete parameter ranges and default values.
- Provide a rollback plan for risky changes (flags, toggles, or safe defaults).

PROJECT DIRECTION (USER CONFIRMED)
- Core fantasy: evolution sim as a spectator; optional god tools, off by default.
- Experience: isolated sandbox with multiple regions/islands and different biomes; player can nudge environments.
- Audience: simulation enthusiasts; depth and emergent outcomes over arcade goals.
- Player role: omniscient observer with close-up inspection and focus camera.
- Must-have: visible natural behaviors with procedural animation, better lighting/shaders, robust inspection/focus, procedural world variety.
- Priority: simulation depth first; target RTX 3080; success is clear multi-species competition per island.

CURRENT PRIORITIES (USER REPORTED)
- Beaches and shore transitions (no sheer dropoff to water).
- Default island/world size is 4x larger.
- More biome variety per island.
- More creature variety at generation.
- Creature list visible on the right.
- Name generator for every creature type.
- Water underwater rendering and surface transitions fixed.
- Menu spawns do not work reliably.
- Spinning/unstable creatures that die out.
- WASD and camera controls feel wrong.

---

## AGENT 1: Beaches and Shoreline Transition

```
I need beaches and shallow shorelines instead of cliff-like dropoffs. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/environment/IslandGenerator.*, src/environment/TerrainSampler.*
- Do not edit: src/environment/ProceduralWorld.*, src/ui/MainMenu.*, src/main.cpp
- If you need changes in those files, write a handoff note in docs/PHASE11_AGENT1_HANDOFF.md.

PRIMARY GOAL:
- Create a consistent, visible beach band around islands with gentle slope from land to shallow water.
- Make shore transitions deterministic and biome-aware (sand vs rock vs reef).

RESEARCH TASKS (MANDATORY):
- Read src/environment/IslandGenerator.cpp and identify how coastalTypeMap is used downstream.
- Read at least one shoreline-related doc in docs/archive (ex: WATER_FIX_AGENT1.md if relevant) and summarize what was learned.

CODE ANCHORS (CURRENT CODE)
- src/environment/IslandGenerator.cpp (beach band):
```cpp
float beachLow = waterLevel * 0.95f;
float beachHigh = waterLevel * 1.15f;
```
- src/environment/IslandGenerator.cpp (coastal erosion and smoothing):
```cpp
applyCoastalErosion(data, static_cast<int>(params.coastalErosion * 10.0f));
generateBeaches(data.heightmap, data.coastalTypeMap, size, params.waterLevel);
```
- src/environment/TerrainSampler.cpp (beach level input):
```cpp
void SetWorldParams(float worldSize, float heightScale, float waterLevel, float beachLevel) {
    BEACH_LEVEL = beachLevel;
}
```

PRE-IMPLEMENTATION PLAN
- Identify where coastalTypeMap influences rendering or biome decisions.
- Decide beach width in world units and height in normalized range.
- Add guardrails to avoid accidental cliffs near coast.

EXECUTION PLAN (CALL CHAIN)
1) IslandGenerator creates heightmap and coastalTypeMap.
2) TerrainSampler uses water/beach levels for sampling.
3) Terrain rendering uses heightmap to produce surface.

PHASE 1: COASTAL SHAPE
- Expand the beach band to a tunable width (not only 0.95-1.15 of waterLevel).
- Add a slope-based pass to clamp near-coast gradients.

PHASE 2: BEACH MATERIAL SIGNAL
- Populate coastalTypeMap with BEACH/CLIFF/REEF in a stable way.
- Add a debug overlay or log that reports % of coastline in each category.

PHASE 3: UNDERWATER TRANSITION
- Smooth the first few meters below water to avoid sudden dropoff.
- Ensure underwater terrain blending stays stable for performance.

VALIDATION
- Spawn 3 different seeds; log beach coverage ratio and sample slope histograms.
- In a 5-10 min sim run, verify coastline walkability and no sudden cliffs.

DELIVERABLES:
- Create docs/PHASE11_AGENT1_SHORELINES.md with:
  - Beach width formula + parameter ranges
  - CoastalTypeMap usage notes
  - Before/after slope distribution
  - Validation results and screenshots (described)
```

---

## AGENT 2: 4x World Size Default and World Scale Consistency

```
I need the default world size to be 4x larger and consistent across UI and worldgen. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/ui/MainMenu.*, src/environment/ProceduralWorld.*, src/environment/PlanetSeed.*
- Do not edit: src/main.cpp, src/environment/IslandGenerator.*
- If you need changes in those files, write a handoff note in docs/PHASE11_AGENT2_HANDOFF.md.

PRIMARY GOAL:
- Default world size is 4x larger than current.
- UI sliders, internal terrain scale, and seeds all agree on the new size.

RESEARCH TASKS (MANDATORY):
- Trace how MainMenu WorldGenConfig.worldSize becomes ProceduralWorld::WorldGenConfig::terrainScale.
- Read at least one worldgen doc in docs/phase* that discusses terrain scale and summarize key constraints.

CODE ANCHORS (CURRENT CODE)
- src/ui/MainMenu.h (default world size):
```cpp
float worldSize = 500.0f;
```
- src/ui/MainMenu.cpp (World Size slider):
```cpp
ImGui::SliderFloat("World Size", &m_worldGenConfig.worldSize, 200.0f, 1000.0f, "%.0f");
```
- src/environment/ProceduralWorld.h (terrain scale):
```cpp
float terrainScale = 500.0f;
```

PRE-IMPLEMENTATION PLAN
- Define the new default (ex: 2000.0f) and min/max slider range.
- Check any noise scaling assumptions tied to terrainScale.
- Verify water level and erosion passes scale reasonably with size.

EXECUTION PLAN (CALL CHAIN)
1) MainMenu edits WorldGenConfig.worldSize (UI).
2) Worldgen config maps worldSize -> terrainScale.
3) Terrain sampler and render use updated scale.

PHASE 1: DEFAULT SIZE UPDATE
- Update defaults and slider range for world size.
- Update config mapping to ProceduralWorld to avoid mismatch.

PHASE 2: SCALE-RELATED TUNING
- Adjust noiseFrequency or erosion passes if scale increases cause flatness or repetition.
- Add debug log for worldSize, terrainScale, and heightmapResolution.

VALIDATION
- Generate 3 planets; verify size change in camera bounds and horizon distance.
- Confirm no crashes or huge perf regressions.

DELIVERABLES:
- Create docs/PHASE11_AGENT2_WORLD_SCALE.md with:
  - New default range values
  - Mapping from UI config to ProceduralWorld config
  - Performance impact notes
```

---

## AGENT 3: Biome Variety per Island

```
I need islands to contain multiple distinct biomes with clean transitions and visual variety. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/environment/BiomeSystem.*, src/environment/BiomePalette.*, src/environment/PlanetTheme.*, src/environment/ClimateSystem.*
- Do not edit: src/environment/ProceduralWorld.*, src/ui/MainMenu.*
- If you need changes in those files, write a handoff note in docs/PHASE11_AGENT3_HANDOFF.md.

PRIMARY GOAL:
- Ensure each island has multiple biome patches (not mono-biome).
- Improve transition blending between adjacent biomes.

RESEARCH TASKS (MANDATORY):
- Inspect BiomeSystem::generateBiomeMap and how temperature/moisture are computed.
- Review at least one prior biome doc in docs/phase* or docs/archive (ex: PHASE8_AGENT7_WORLD_VARIETY.md).

CODE ANCHORS (CURRENT CODE)
- src/environment/BiomeSystem.cpp (biome map generation):
```cpp
void BiomeSystem::generateBiomeMap(const std::vector<float>& heightmap, int width, int height, uint32_t seed)
```
- src/environment/BiomeSystem.cpp (transition blending):
```cpp
void BiomeSystem::smoothTransitions(int iterations)
```
- src/environment/BiomePalette.cpp (biome color palettes):
```cpp
BiomePalette::applyTheme(...)
```

PRE-IMPLEMENTATION PLAN
- Measure current biome diversity: count distinct biomes per island.
- Identify biases that cause biome collapse (temperature/moisture ranges too tight).
- Propose a patch distribution model (latitudinal bands + local noise).

EXECUTION PLAN (CALL CHAIN)
1) BiomeSystem receives heightmap, climate fields.
2) Biomes assigned per cell and transitions smoothed.
3) BiomePalette colors and density read by vegetation and rendering.

PHASE 1: MULTI-BIOME DISTRIBUTION
- Add a biome patch noise layer to force mixed biomes per island.
- Add metrics: biome counts and largest patch size per island.

PHASE 2: TRANSITION QUALITY
- Tune smoothTransitions and blend widths to avoid hard borders.
- Ensure wetlands and coasts appear near water.

PHASE 3: VISUAL VARIETY
- Adjust palette variance per biome and per planet theme.
- Ensure biomes have clear color separation, not just subtle shifts.

VALIDATION
- Generate 3 planets and log biome counts and coverage ratios.
- Visually confirm at least 4 biomes per island where size permits.

DELIVERABLES:
- Create docs/PHASE11_AGENT3_BIOME_VARIETY.md with:
  - Biome distribution formula
  - Transition tuning constants
  - Coverage statistics
```

---

## AGENT 4: Creature Generation Variety (Starting Genomes)

```
I need more creature variety at generation time, not just later evolution. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/entities/Genome.*, src/entities/genetics/*
- Do not edit: src/core/CreatureManager.*, src/ui/*
- If you need changes in those files, write a handoff note in docs/PHASE11_AGENT4_HANDOFF.md.

PRIMARY GOAL:
- Expand initial creature variety by aligning genome presets to biomes and niches.
- Avoid spawning too many near-identical bodies at start.

RESEARCH TASKS (MANDATORY):
- Inventory genome randomize functions and identify which are currently used on spawn.
- Read at least one genetics-related doc in docs/phase* or docs/archive.

CODE ANCHORS (CURRENT CODE)
- src/entities/Genome.h (preset initializers):
```cpp
void initializeForPreset(EvolutionStartPreset preset, float guidanceBias, const PlanetChemistry* chemistry);
void initializeForRegion(const RegionEvolutionConfig& config, const PlanetChemistry* chemistry);
```
- src/entities/Genome.cpp (randomize variants):
```cpp
void Genome::randomizeAquatic();
void Genome::randomizeBird();
```

PRE-IMPLEMENTATION PLAN
- Identify where genome presets are chosen for initial populations.
- Define a mapping from biome/climate to genome archetypes.
- Add metrics for diversity (trait variance, morphology spread).

EXECUTION PLAN (CALL CHAIN)
1) Spawn call chooses a preset.
2) Genome initialized with preset and chemistry bias.
3) Creature created with genome and added to sim.

PHASE 1: PRESET SELECTION
- Create a deterministic chooser that maps biome and niche to preset variants.
- Ensure land/aquatic/flying distribution is stable.

PHASE 2: VARIETY METRICS
- Add logging for trait variance (size, limb count, speed, vision).
- Expose a debug readout of diversity score.

PHASE 3: BALANCE PASS
- Ensure new variety does not create instant die-offs.
- Add safety clamps for energy efficiency and locomotion viability.

VALIDATION
- Spawn initial populations and log diversity metrics.
- Run 5-10 minutes and verify at least 3 distinct body archetypes per domain.

DELIVERABLES:
- Create docs/PHASE11_AGENT4_CREATURE_VARIETY.md with:
  - Preset mapping table
  - Diversity metrics
  - Stability notes
```

---

## AGENT 5: Creature List UI (Right-Hand Side)

```
I need a reliable, visible creature list on the right side of the UI with selection and filtering. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/ui/SimulationDashboard.*, src/ui/CreatureInspectionPanel.*
- Do not edit: src/entities/SpeciesNaming.*, src/ui/MainMenu.*
- If you need changes in those files, write a handoff note in docs/PHASE11_AGENT5_HANDOFF.md.

PRIMARY GOAL:
- The creature list is always visible on the right panel, not hidden or clipped.
- Entries show name, type, and basic stats, and can be clicked to focus.

RESEARCH TASKS (MANDATORY):
- Trace renderRightPanel and renderCreatureList usage in SimulationDashboard.
- Read any UI-related doc in docs/phase* or docs/archive (ex: PHASE10_AGENT8_OBSERVER_UI.md).

CODE ANCHORS (CURRENT CODE)
- src/ui/SimulationDashboard.cpp (list rendering):
```cpp
void SimulationDashboard::renderCreatureList(std::vector<std::unique_ptr<Creature>>& creatures)
```
- src/ui/SimulationDashboard.cpp (right panel):
```cpp
renderRightPanel(creatures, camera);
```

PRE-IMPLEMENTATION PLAN
- Identify why list is not visible (layout, size, conditional flags).
- Decide list sorting and filtering (species, domain, distance).
- Ensure list selection flows to inspection and camera focus.

EXECUTION PLAN (CALL CHAIN)
1) SimulationDashboard renders right panel.
2) Creature list is populated and filtered.
3) Selection updates inspection and camera controls.

PHASE 1: LAYOUT FIX
- Ensure right panel has a stable width and scroll region.
- Make list visible by default with a compact header.

PHASE 2: DATA QUALITY
- Display name (from naming system), type, energy, and age.
- Add search or filter toggles.

PHASE 3: INTERACTION
- Click to select and focus camera.
- Add selection highlight and keep selection stable across frames.

VALIDATION
- Verify list shows at least 50 creatures in large sim.
- Confirm selection and camera focus work without UI glitches.

DELIVERABLES:
- Create docs/PHASE11_AGENT5_CREATURE_LIST_UI.md with:
  - Layout description and new UI controls
  - Selection and focus flow
  - Validation steps
```

---

## AGENT 6: Naming System for All Creature Types

```
I need a name generator that covers every creature type and always returns a name. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/entities/SpeciesNaming.*, src/entities/SpeciesNameGenerator.*, src/entities/NamePhonemeTables.*
- Do not edit: src/ui/SimulationDashboard.*, src/entities/Genome.*
- If you need changes in those files, write a handoff note in docs/PHASE11_AGENT6_HANDOFF.md.

PRIMARY GOAL:
- Every creature type gets a deterministic species name and individual name.
- Names reflect biome and planet theme, with no generic placeholders.

RESEARCH TASKS (MANDATORY):
- Identify all call sites of SpeciesNamingSystem and SpeciesNameGenerator.
- Read at least one naming-related doc in docs/phase* or docs/archive.

CODE ANCHORS (CURRENT CODE)
- src/entities/SpeciesNaming.cpp (name components):
```cpp
void SpeciesNamingSystem::initializeNameComponents()
```
- src/entities/SpeciesNameGenerator.cpp (theme prefixes):
```cpp
void SpeciesNameGenerator::initializePrefixes()
```

PRE-IMPLEMENTATION PLAN
- Inventory which creature types currently bypass naming or use defaults.
- Define naming fallback rules (no empty strings, deterministic seed).

EXECUTION PLAN (CALL CHAIN)
1) Species/creature ID requested for naming.
2) Naming system returns species name + descriptor.
3) UI uses the name for list/nametags.

PHASE 1: COVERAGE
- Guarantee naming for all CreatureType values and small creatures.
- Add a log for missing or fallback cases.

PHASE 2: DIVERSITY
- Expand phoneme tables or prefixes by biome and planet theme.
- Ensure unique names across species within one planet.

PHASE 3: API HARDENING
- Provide stable API that UI can query without null checks.
- Add unit-style test helper that generates 200 names and reports collisions.

VALIDATION
- Generate 3 seeds; verify 0 missing names and low collision rate.

DELIVERABLES:
- Create docs/PHASE11_AGENT6_NAMING_COVERAGE.md with:
  - Coverage list by creature type
  - Collision metrics
  - Example names per biome
```

---

## AGENT 7: Water Rendering and Underwater View

```
I need correct underwater rendering and a clean transition at the water surface. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/graphics/WaterRenderer.*, Runtime/Shaders/Water*
- Do not edit: src/graphics/SkyRenderer.*, src/main.cpp
- If you need changes in those files, write a handoff note in docs/PHASE11_AGENT7_HANDOFF.md.

PRIMARY GOAL:
- Water surface and underwater scene render correctly with depth and fog.
- Camera can move underwater with proper surface transition.

RESEARCH TASKS (MANDATORY):
- Audit WaterRenderer and its underwater detection path.
- Read at least one water-related doc in docs/archive (ex: WATER_FIX_AGENT1.md) and summarize.

CODE ANCHORS (CURRENT CODE)
- src/graphics/WaterRenderer.cpp (underwater detection):
```cpp
bool cameraUnderwater = underwaterDepth > 0.0;
```
- src/graphics/WaterRenderer.cpp (constant buffer update):
```cpp
UpdateConstantBuffer(view, projection, cameraPos, lightDir, lightColor, sunIntensity, time);
```
- Runtime/Shaders/Water*.hlsl (surface shading)

PRE-IMPLEMENTATION PLAN
- Identify the current rendering path for water surface vs underwater.
- Determine if depth buffer or fog is incorrect.
- Define how to blend surface color with underwater fog.

EXECUTION PLAN (CALL CHAIN)
1) WaterRenderer updates constants and renders surface pass.
2) Underwater state drives shader branch or post-process.
3) Fog and caustics applied based on depth.

PHASE 1: UNDERWATER STATE
- Fix underwater detection and add debug flag to display depth.
- Add a visual test mode to show surface boundary.

PHASE 2: SURFACE TRANSITION
- Blend refraction/reflection with underwater fog at the surface.
- Fix any depth linearization issues.

PHASE 3: UNDERWATER VISIBILITY
- Add distance-based attenuation and color absorption.
- Keep performance stable on RTX 3080.

VALIDATION
- Swim camera below water and verify stable transition and no popping.
- Confirm underwater visibility matches depth bands.

DELIVERABLES:
- Create docs/PHASE11_AGENT7_WATER_UNDERWATER.md with:
  - Shader logic summary
  - Tunable constants for fog and absorption
  - Validation notes
```

---

## AGENT 8: Menu Spawn Reliability and Spawn Tools

```
I need reliable creature spawning from menus and tools. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/core/CreatureManager.*, src/ui/SpawnTools.*, src/ui/CreatureSpawnPanel.*, src/ui/ScenarioPresets.*
- Do not edit: src/main.cpp, src/ui/MainMenu.*
- If you need changes in those files, write a handoff note in docs/PHASE11_AGENT8_HANDOFF.md.

PRIMARY GOAL:
- Menu and spawn tools reliably create creatures at valid positions.
- Spawn failures are reported clearly with logs.

RESEARCH TASKS (MANDATORY):
- Audit CreatureManager::spawn and SpawnTools::getValidSpawnPosition.
- Read at least one spawn-related doc in docs/archive.

CODE ANCHORS (CURRENT CODE)
- src/core/CreatureManager.cpp:
```cpp
CreatureHandle CreatureManager::spawn(CreatureType type, const glm::vec3& position, const Genome* parentGenome)
```
- src/ui/SpawnTools.h:
```cpp
glm::vec3 getValidSpawnPosition(const glm::vec3& position, CreatureType type);
```

PRE-IMPLEMENTATION PLAN
- Identify failure paths (terrain water checks, bounds, null terrain).
- Determine how menu spawns map to spawn tools.
- Add error counters and reporting.

EXECUTION PLAN (CALL CHAIN)
1) UI triggers spawn request.
2) SpawnTools / CreatureManager validates position.
3) Spawn result propagated to UI with success/failure.

PHASE 1: SPAWN VALIDATION
- Add robust checks for land vs water vs aerial spawns.
- Ensure a fallback position when terrain sampling fails.

PHASE 2: RELIABLE FEEDBACK
- Log spawn failures with reason codes and counts.
- Add a UI message or console line when spawn fails.

PHASE 3: MENU PRESETS
- Validate ScenarioPresets and batch spawn logic.
- Ensure spawns respect biome and island bounds.

VALIDATION
- Spawn 10x each type from UI and log success rate.
- Verify no zero-count or invalid position spawns.

DELIVERABLES:
- Create docs/PHASE11_AGENT8_SPAWN_RELIABILITY.md with:
  - Failure reason taxonomy
  - Spawn success metrics
  - Handoff notes for main menu if needed
```

---

## AGENT 9: Creature Stability (Spinning and Die-Outs)

```
I need to fix spinning creatures and early die-outs due to physics or control instability. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/entities/Creature.*, src/physics/*, src/animation/ProceduralRig.*
- Do not edit: src/main.cpp, src/ui/*
- If you need changes in those files, write a handoff note in docs/PHASE11_AGENT9_HANDOFF.md.

PRIMARY GOAL:
- Remove spinning or unstable motion that causes instant death.
- Stabilize rotation and movement across land, air, and water.

RESEARCH TASKS (MANDATORY):
- Audit Creature::updatePhysics and updateFlyingPhysics for rotation logic.
- Read at least one movement-related doc in docs/archive (ex: CAMERA_DRIFT_AGENT4.md if relevant).

CODE ANCHORS (CURRENT CODE)
- src/entities/Creature.cpp:
```cpp
void Creature::updatePhysics(float deltaTime, const Terrain& terrain)
void Creature::updateFlyingPhysics(float deltaTime, const Terrain& terrain)
```
- src/physics/Locomotion.cpp:
```cpp
js.angularVelocity += angularAccel * deltaTime;
```

PRE-IMPLEMENTATION PLAN
- Identify sources of angular instability (large deltaTime, zero damping).
- Add telemetry for angular velocity spikes.
- Propose stabilization: clamp turn rates, add damping, or align to velocity.

EXECUTION PLAN (CALL CHAIN)
1) Behavior sets desired movement.
2) Physics applies forces and updates rotation.
3) Animator consumes rotation for pose.

PHASE 1: DIAGNOSTICS
- Log max angular velocity and rotation deltas per frame.
- Capture cases where rotation flips or NaNs appear.

PHASE 2: STABILIZATION
- Add turn-rate clamps and damping.
- Smooth heading changes; avoid instantly snapping to velocity.

PHASE 3: SURVIVABILITY
- Ensure energy drains and collision responses do not instantly kill new spawns.
- Add safe spawning posture for newborns.

VALIDATION
- Spawn 50 creatures and observe for 5-10 minutes.
- Confirm no widespread spinning or immediate die-offs.

DELIVERABLES:
- Create docs/PHASE11_AGENT9_CREATURE_STABILITY.md with:
  - Root causes found
  - Fix details and tuning constants
  - Stability metrics
```

---

## AGENT 10: WASD and Camera Controls (Main Integration)

```
I need the camera and WASD controls to feel correct and predictable. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/main.cpp, src/graphics/CameraController.*, src/graphics/Camera.*
- Do not edit: src/ui/SimulationDashboard.*, src/ui/MainMenu.*
- If you need changes in those files, write a handoff note in docs/PHASE11_AGENT10_HANDOFF.md.

PRIMARY GOAL:
- Make WASD movement intuitive and consistent across orbit and free modes.
- Resolve mouse capture issues and ensure smooth camera motion.

RESEARCH TASKS (MANDATORY):
- Inventory current camera input handling in src/main.cpp.
- Read at least one camera-related doc in docs/archive (ex: CAMERA_DRIFT_AGENT4.md, MOUSE_INVERT_AGENT5.md).

CODE ANCHORS (CURRENT CODE)
- src/main.cpp (mouse capture and movement):
```cpp
if (g_app.mouseCaptured) { ... }
```
- src/main.cpp (orbit camera updates):
```cpp
g_app.cameraYaw += delta.x * g_app.mouseSensitivity * xMult;
```
- src/graphics/CameraController.cpp (alternative control flow):
```cpp
bool CameraController::processKeyboard(...)
```

PRE-IMPLEMENTATION PLAN
- Decide which camera system is authoritative (main.cpp or CameraController).
- Define control modes and key mappings (orbit, free-fly, follow).
- Add smoothing and sensitivity settings.

EXECUTION PLAN (CALL CHAIN)
1) Input reads mouse and keyboard.
2) Camera controller updates yaw/pitch/position.
3) Render uses updated camera for view/projection.

PHASE 1: CONTROL REWORK
- Unify WASD behavior with a single movement model.
- Keep orbit and free-fly consistent with one sensitivity scalar.

PHASE 2: SMOOTHING AND CLAMPS
- Add smoothing to mouse deltas and clamp pitch ranges.
- Ensure transition between follow and free modes is stable.

PHASE 3: UX POLISH
- Add on-screen debug readout for mode and sensitivity.
- Provide a safe reset hotkey.

VALIDATION
- Run 10 minutes with camera movement and ensure no jitter or drift.
- Verify WASD matches expected direction in both modes.

DELIVERABLES:
- Create docs/PHASE11_AGENT10_CAMERA_CONTROLS.md with:
  - Control scheme spec
  - Code change summary
  - Validation notes
```
