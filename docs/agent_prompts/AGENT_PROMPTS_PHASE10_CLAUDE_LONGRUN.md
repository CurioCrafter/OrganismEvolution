# 10 Claude Agent Prompts: Phase 10 Integration and Wiring (Long-Run, v1)

This phase is about making the existing systems actually run in the live sim. The codebase has many complete subsystems that are not wired into the runtime loop. These prompts focus on integration, not net-new features.

PARALLEL RUN RULES (MUST FOLLOW)
- Ownership map: Each agent owns the files listed in their prompt. Do not edit files owned by other agents.
- Handoff policy: If you must change an unowned file, do not edit it. Write a handoff note in `docs/PHASE10_AGENTX_HANDOFF.md` with concrete API requests and file paths.
- Main integration: Agent 1 is the only agent allowed to edit `src/main.cpp`.
- Creature and animation: Agent 5 is the only agent allowed to edit `src/entities/Creature.*` and `src/animation/*`.
- Genome and genetics: Agent 6 is the only agent allowed to edit `src/entities/Genome.*` and `src/entities/genetics/*`.
- Worldgen: Agent 2 owns the environment pipeline files listed in their prompt (including `src/ui/MainMenu.*`).
- UI integration: Agent 8 is the only agent allowed to edit UI files except `src/ui/MainMenu.*`, `src/ui/IslandComparisonUI.*`, and `src/ui/RegionOverviewPanel.*`.
- Multi-island: Agent 3 owns multi-island core and island UI listed in their prompt.
- Shaders and postprocess: Agent 10 owns `Runtime/Shaders/*`, `src/graphics/PostProcess_DX12.*`, `src/graphics/GlobalLighting.*`, `src/graphics/SkyRenderer.*`, `src/graphics/WaterRenderer.*`, and `src/graphics/CinematicCamera.*`.
- Every agent must add instrumentation (logs, counters, or debug UI) and validate in a 5-10 minute sim run.
- Each agent doc must include: files touched, new structs/functions, tuning constants and ranges, performance notes (rough cost), validation steps (what you observed), and integration notes for Agent 1 and Agent 8.

PLANNING EXPECTATIONS (NON-NEGOTIABLE)
- Provide a pre-implementation plan with dependencies and a sequencing checklist.
- Provide an execution plan: how the code will be called in runtime (call chain + update order).
- Provide concrete parameter ranges and default values.
- Provide a rollback plan for risky changes (flags, toggles, or safe defaults).

PROJECT DIRECTION (USER CONFIRMED)
- Core fantasy: evolution sim as a spectator; optional god tools, off by default.
- Audience: simulation enthusiasts; depth and emergent outcomes over arcade goals.
- Player role: omniscient observer with close-up inspection and focus camera.
- Must-have demo: visible natural behaviors with procedural animation, better lighting/shaders, robust inspection/focus, and clear world variety.
- Worldgen: multi-region/island planets, different biomes, stars, plants/grass/trees, adjustable generator settings.
- Evolution: start from different complexity levels with optional guidance (land/flight/aquatic); limb and sensory traits matter.
- Priority: simulation depth first; target RTX 3080; success is visible multi-species competition per island.

CURRENT CODE STATUS (DISCOVERED)
- Runtime uses `SimulationWorld` and `SimCreature` in `src/main.cpp`, bypassing `Creature`, `CreatureManager`, behavior systems, activity animation, and genetics depth.
- `MainMenu` start callback is not wired to worldgen; `main.cpp` uses fixed seed 42 for terrain/vegetation/grass.
- `MultiIslandManager`, `InterIslandMigration`, and island UI exist but are not wired into the runtime loop.
- `BehaviorCoordinator` exists but is not invoked anywhere.
- `ActivityAnimationDriver::applyToPose` is a no-op, so activity animation is not visible.
- `BiochemistrySystem` and `PlanetChemistry` exist but are not applied to runtime fitness/energy.
- `PerformanceManager`, `QualityScaler`, and scheduler systems exist but are not connected.
- `PostProcess_DX12` PSO creation is placeholder, and cinematic camera mode is not implemented.

---

## AGENT 1: Simulation Stack Unification (SimCreature -> CreatureManager)

```
I need the runtime loop to use the actual Creature systems instead of the SimCreature stub. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/main.cpp, src/core/CreatureManager.*, src/core/GameplayManager.*
- Do not edit: src/entities/Creature.*, src/animation/*, src/ui/*, src/environment/*
- If you need changes in those files, write a handoff note to the owning agent.

PRIMARY GOAL:
- Replace or adapt the SimCreature/SimulationWorld loop so the live sim uses CreatureManager and Creature::update.
- Keep save/load and replay functional after the migration.

CODE ANCHORS (CURRENT CODE)
- `src/main.cpp` (SimCreature and SimulationWorld):
```cpp
struct SimCreature {
    glm::vec3 position;
    glm::vec3 velocity;
    float energy;
    float fear;
    CreatureType type;
    Genome genome;
    bool alive;
    u32 id;
    ...
};

class SimulationWorld {
public:
    std::vector<std::unique_ptr<SimCreature>> creatures;
    ...
};
```
- `src/main.cpp` (integration mismatch):
```cpp
// Note: GameplayManager needs a CreatureManager, but we use SimulationWorld
```

PRE-IMPLEMENTATION PLAN
- Decide migration strategy: full replacement (preferred) or adapter layer that exposes CreatureManager to existing render/compute code.
- Inventory all SimCreature-only fields used in main and map to Creature equivalents.
- Identify any systems that must stay temporarily (e.g., replay input structures) and isolate them behind adapters.

EXECUTION PLAN (CALL CHAIN)
1) App init creates CreatureManager and initializes it with Terrain/Ecosystem pointers.
2) Spawn initial populations via CreatureManager, not SimulationWorld.
3) Per-frame update calls CreatureManager::update and Creature::update for alive creatures.
4) Render and GPU compute input uses Creature data (position/velocity/energy/type).
5) Remove or gate SimulationWorld update path behind a compatibility flag.

PHASE 1: CREATURE MANAGER BOOTSTRAP
- Add CreatureManager to app state and initialize on world creation.
- Replace SimulationWorld spawn with CreatureManager::spawn.

PHASE 2: UPDATE LOOP MIGRATION
- Replace SimCreature update loops with Creature::update on managed creatures.
- Ensure spatial grid rebuilds and stats update each frame.
- Validate no crashes during 5-10 minute run.

PHASE 3: RENDER/COMPUTE ADAPTER
- Build CreatureInput arrays from CreatureManager for GPU compute.
- Update any render code that assumes SimCreature fields.

PHASE 4: CLEANUP AND COMPAT
- Remove unused SimCreature data paths or keep behind a debug toggle.
- Add logs: creature count, alive count, update time.

DELIVERABLES:
- Create docs/PHASE10_AGENT1_SIM_STACK.md with:
  - Mapping table (SimCreature -> Creature fields)
  - Updated call chain diagram
  - Compatibility flags and rollback plan
  - Validation notes (what you saw)
```

---

## AGENT 2: Main Menu and Worldgen Wiring (Menu -> ProceduralWorld)

```
I need the MainMenu settings to actually drive world generation. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/ui/MainMenu.*, src/environment/ProceduralWorld.*, src/environment/PlanetTheme.*, src/environment/BiomePalette.*, src/environment/BiomeSystem.*, src/environment/ClimateSystem.*, src/environment/VegetationManager.*, src/environment/GrassSystem.*, src/environment/TreeGenerator.*, src/environment/ArchipelagoGenerator.*, src/environment/Terrain.*, src/environment/TerrainErosion.*
- Do not edit: src/main.cpp (Agent 1 owns it), src/entities/*, src/ui/* (other than MainMenu)
- Provide a handoff to Agent 1 for runtime hookups.

PRIMARY GOAL:
- MainMenu settings (seed, planet preset, region count, climate, biome mix) must flow into ProceduralWorld and then into terrain/vegetation/grass.
- Keep determinism for fixed seeds and high variety for random seeds.

CODE ANCHORS (CURRENT CODE)
- `src/ui/MainMenu.cpp` (start button):
```cpp
if (ImGui::Button("Generate Planet & Start", ImVec2(startButtonWidth, startButtonHeight))) {
    if (m_onStartGame) {
        m_onStartGame(m_worldGenConfig, m_evolutionPreset, m_godModeEnabled);
    }
    m_active = false;
    m_canContinue = true;
}
```
- `src/environment/ProceduralWorld.cpp` (seed + theme + biome flow):
```cpp
uint32_t seed = ensureSeed(config.seed);
world.planetSeed.setMasterSeed(seed);
world.planetTheme = std::make_unique<PlanetTheme>();
world.planetTheme->generateRandom(world.planetSeed.paletteSeed);
world.biomeSystem = std::make_unique<BiomeSystem>();
world.biomeSystem->initializeWithTheme(*world.planetTheme);
world.biomeSystem->generateFromIslandData(world.islandData);
```

PRE-IMPLEMENTATION PLAN
- Map MainMenu::WorldGenConfig fields to ProceduralWorld::WorldGenConfig.
- Identify missing fields in ProceduralWorld config and add them if needed (star type, region count).
- Define a handoff function signature for Agent 1 to call from main.

EXECUTION PLAN (CALL CHAIN)
1) MainMenu emits WorldGenConfig and EvolutionStartPreset via m_onStartGame.
2) Agent 1 calls ProceduralWorld::generate with translated config.
3) Terrain/Vegetation/Grass rebuild from the generated world outputs.
4) Store seed, theme name, and biome mix for UI display.

PHASE 1: CONFIG TRANSLATION
- Add a translation helper (MainMenu config -> ProceduralWorld config).
- Ensure seed handling uses ProceduralWorld::ensureSeed.

PHASE 2: WORLD OUTPUT HOOKS
- Expose getters for terrain scale, water level, biome palette for other systems.
- Add logs: seed, theme name, generation time, biome distribution.

PHASE 3: HANDOFF TO MAIN
- Provide a clear handoff doc for Agent 1: function name, required calls, and return data.

PHASE 4: VALIDATION
- Generate 3 worlds with different seeds and confirm visible differences.
- Fixed seed must be deterministic.

DELIVERABLES:
- Create docs/PHASE10_AGENT2_WORLDGEN_WIRING.md with:
  - Config mapping table
  - New/updated ProceduralWorld fields
  - Handoff notes for Agent 1
  - Validation notes
```

---

## AGENT 3: Multi-Island Runtime Integration and Migration

```
I need multi-island competition and migration to run in the live sim. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/core/MultiIslandManager.*, src/entities/behaviors/InterIslandMigration.*, src/scenarios/DarwinMode.*, src/graphics/IslandCamera.*, src/ui/IslandComparisonUI.*, src/ui/RegionOverviewPanel.*
- Do not edit: src/main.cpp, src/environment/ProceduralWorld.*, src/ui/* (other than the two island UI files)
- Provide a handoff to Agent 1 for runtime hookups.

PRIMARY GOAL:
- Multi-island archipelagos should be created, updated, and visible with per-island stats and migration events.

CODE ANCHORS (CURRENT CODE)
- `src/core/MultiIslandManager.h` (island stats and systems):
```cpp
struct Island {
    std::unique_ptr<Terrain> terrain;
    std::unique_ptr<CreatureManager> creatures;
    std::unique_ptr<VegetationManager> vegetation;
    IslandStats stats;
};
```
- `src/entities/behaviors/InterIslandMigration.h` (update entry point):
```cpp
void update(float deltaTime, MultiIslandManager& islands);
```
- `src/ui/IslandComparisonUI.cpp` (overview view):
```cpp
void IslandComparisonUI::renderOverview(const MultiIslandManager& islands) {
    auto globalStats = islands.getGlobalStats();
    ...
}
```

PRE-IMPLEMENTATION PLAN
- Verify MultiIslandManager::generateAll and per-island creature spawn logic.
- Define which island is active for camera and UI at startup.
- Decide how to tie migration events to UI and camera cues.

EXECUTION PLAN (CALL CHAIN)
1) MultiIslandManager::init and generateAll called after worldgen (via Agent 1).
2) Per-frame: MultiIslandManager::update and updateStatistics.
3) InterIslandMigration::update runs and emits events.
4) Island UI renders per-island stats and migration events.

PHASE 1: ISLAND POPULATION INIT
- Ensure each island has its own CreatureManager and vegetation.
- Add deterministic per-island seeding using archipelago seed.

PHASE 2: MIGRATION EVENTS
- Improve migration triggers and track success/failure counts.
- Add event callbacks for UI and camera cues.

PHASE 3: ISLAND UI READABILITY
- Add a per-island summary row: population, species count, avg fitness, diversity.
- Add migration event feed and island comparison charts.

PHASE 4: VALIDATION
- 5-10 minute run with 3-6 islands, confirm distinct population curves.

DELIVERABLES:
- Create docs/PHASE10_AGENT3_MULTI_ISLAND.md with:
  - Island initialization rules
  - Migration triggers and tuning
  - UI additions and screenshots/notes
  - Handoff notes for Agent 1
```

---

## AGENT 4: BehaviorCoordinator Wiring and Life Events

```
I need the behavior systems to actually run and influence movement and activity. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/entities/behaviors/BehaviorCoordinator.*, src/entities/behaviors/ParentalCare.*, src/entities/behaviors/SocialGroups.*, src/entities/behaviors/PackHunting.*, src/entities/behaviors/TerritorialBehavior.*, src/entities/behaviors/MigrationBehavior.*, src/entities/behaviors/VarietyBehaviors.*
- Do not edit: src/entities/Creature.*, src/animation/*, src/ui/*
- Provide handoff notes to Agent 5 for ActivitySystem mapping if needed.

PRIMARY GOAL:
- BehaviorCoordinator should drive meaningful steering forces and emit life events for activities (hunt, care, territorial, social).

CODE ANCHORS (CURRENT CODE)
- `src/entities/behaviors/BehaviorCoordinator.h`:
```cpp
void init(CreatureManager* creatureManager, SpatialGrid* spatialGrid, ...);
void update(float deltaTime);
glm::vec3 calculateBehaviorForces(Creature* creature);
```

PRE-IMPLEMENTATION PLAN
- Identify where behavior forces should be applied (likely in Creature update or steering).
- Define event hooks for activity state changes (hunt start, care start, display).
- Add debug stats to verify behaviors trigger.

EXECUTION PLAN (CALL CHAIN)
1) BehaviorCoordinator::init called once with CreatureManager and SpatialGrid.
2) BehaviorCoordinator::update runs every frame.
3) Creature update queries BehaviorCoordinator for force blends.
4) Behavior events are emitted and consumed by ActivitySystem (handoff to Agent 5).

PHASE 1: FORCE INTEGRATION
- Add a clear path for calculateBehaviorForces to influence movement.
- Document how this blends with existing steering.

PHASE 2: LIFE EVENT SIGNALS
- Emit signals for parenting, hunting, display, and play.
- Add counters and cooldowns to avoid spam.

PHASE 3: DEBUG INSTRUMENTATION
- Add debug counters for behavior occurrences and active states.
- Provide a periodic log or on-screen debug readout.

PHASE 4: VALIDATION
- 5-10 minute run: observe at least three distinct behavior types.

DELIVERABLES:
- Create docs/PHASE10_AGENT4_BEHAVIOR_WIRING.md with:
  - Behavior force blending and weights
  - Event hook definitions for ActivitySystem
  - Debug output sample
```

---

## AGENT 5: Activity Animation Pose Blending (Visible Actions)

```
I need activity animation to actually move bones and show eating, sleeping, mating, excreting, and care behaviors. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/entities/Creature.*, src/animation/*
- Do not edit: src/ui/*, src/environment/*, src/entities/Genome.*

PRIMARY GOAL:
- Implement ActivityAnimationDriver::applyToPose and blend it into the existing locomotion pose so activity is visible.

CODE ANCHORS (CURRENT CODE)
- `src/animation/ActivitySystem.cpp` (no-op):
```cpp
void ActivityAnimationDriver::applyToPose(...) {
    if (!m_stateMachine || m_activityWeight < 0.01f) return;
    // simplified implementation - full version would modify bones directly
}
```
- `src/animation/Animation.h` (locomotion only):
```cpp
m_locomotion.applyToPose(m_skeleton, m_pose, m_ikSystem);
m_pose.updateMatrices(m_skeleton);
```

PRE-IMPLEMENTATION PLAN
- Decide blend order: locomotion first, then activity overrides, then IK.
- Identify which activities need IK targets (head/mouth/limbs).
- Add a toggle to disable activity blending for debugging.

EXECUTION PLAN (CALL CHAIN)
1) Creature::updateActivitySystem sets current activity and weight.
2) ActivityAnimationDriver computes offsets and targets.
3) ActivityAnimationDriver::applyToPose modifies SkeletonPose.
4) CreatureAnimator uploads final skinning matrices.

PHASE 1: POSE BLENDING
- Implement bone-level pose offsets for key activities.
- Blend activity weight with locomotion pose safely.

PHASE 2: IK TARGETS
- Add head/limb targets for eating, grooming, and care.
- Ensure targets are stable at varying body sizes.

PHASE 3: READABILITY POLISH
- Add subtle breathing and head bob during idle.
- Add body offsets for sleep and excretion.

PHASE 4: VALIDATION
- 5-10 minute run and record at least 5 visible activity types.

DELIVERABLES:
- Create docs/PHASE10_AGENT5_ACTIVITY_POSE.md with:
  - Activity-to-bone mapping table
  - Blend weights and timing
  - Validation notes
```

---

## AGENT 6: Biochemistry and Evolution Preset Integration

```
I need planet chemistry to affect survival and evolution presets to affect starting complexity. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/environment/PlanetChemistry.*, src/core/BiochemistrySystem.*, src/entities/Genome.*, src/entities/genetics/*
- Do not edit: src/entities/Creature.*, src/main.cpp, src/ui/*
- Provide a handoff to Agent 1 for runtime application of penalties.

PRIMARY GOAL:
- PlanetChemistry should be generated per world and BiochemistrySystem compatibility should affect energy/fitness.
- EvolutionStartPreset and EvolutionGuidanceBias should be applied at spawn time.

CODE ANCHORS (CURRENT CODE)
- `src/environment/PlanetChemistry.h`:
```cpp
struct PlanetChemistry {
    SolventType solventType;
    AtmosphereMix atmosphere;
    MineralProfile minerals;
    float radiationLevel;
    float acidity;
    float salinity;
    ...
};
```
- `src/core/BiochemistrySystem.h`:
```cpp
BiochemistryCompatibility computeCompatibility(const Genome& genome,
                                               const PlanetChemistry& chemistry) const;
```
- `src/entities/Genome.h`:
```cpp
enum class EvolutionStartPreset : uint8_t { PROTO, EARLY_LIMB, COMPLEX, ADVANCED };
enum class EvolutionGuidanceBias : uint8_t { NONE, LAND, AQUATIC, FLIGHT, UNDERGROUND };
```

PRE-IMPLEMENTATION PLAN
- Determine where PlanetChemistry should be stored in world state for access by runtime.
- Decide how evolution presets map to genome initialization ranges.
- Define compatibility penalty application points (energy drain, reproduction).

EXECUTION PLAN (CALL CHAIN)
1) Worldgen creates PlanetChemistry for the world seed.
2) Spawner initializes genomes using EvolutionStartPreset and guidance bias.
3) BiochemistrySystem computes compatibility for each creature/species.
4) Agent 1 applies penalties in runtime update (handoff).

PHASE 1: PRESET RANGES
- Define explicit ranges per preset for size, speed, vision, locomotion traits.
- Apply guidance bias as soft multipliers (not hard locks).

PHASE 2: CHEMISTRY GENERATION
- Add helper to ProceduralWorld output for chemistry profile (handoff to Agent 2 if needed).
- Add logs: solvent type, atmosphere, acidity, temperature range.

PHASE 3: COMPATIBILITY CACHE
- Ensure BiochemistrySystem cache is cleared on world reset.
- Provide a low-cost per-species cache update strategy.

PHASE 4: VALIDATION
- 5-10 minute run: log average compatibility and show penalties affecting survival.

DELIVERABLES:
- Create docs/PHASE10_AGENT6_BIOCHEM_INTEGRATION.md with:
  - Preset range tables and bias rules
  - Compatibility formulas and penalty thresholds
  - Handoff notes for Agent 1
```

---

## AGENT 7: Save/Load and Replay Integrity (Full Creature Data)

```
I need saves and replay to use real creature data, not placeholders. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/core/SaveManager.*, src/core/Serializer.*, src/core/ReplaySystem.*, src/core/SaveSystemIntegration.*
- Do not edit: src/main.cpp, src/entities/Creature.*, src/ui/*
- Provide a handoff to Agent 1 for runtime wiring.

PRIMARY GOAL:
- Save/load should capture the full creature state (age, generation, behaviors, genome, neural weights).
- Replay should use the same data fields as the runtime uses after Agent 1 migration.

CODE ANCHORS (CURRENT CODE)
- `src/core/SaveSystemIntegration.h` (SimulationWorld helpers):
```cpp
// Helper to build CreatureSaveData from a Creature struct
template<typename CreatureType, typename NeuralNetType>
CreatureSaveData buildCreatureSaveData(const CreatureType& creature, ...);
```
- `src/main.cpp` (placeholder save values):
```cpp
data.health = 100.0f;  // SimCreature doesn't track health separately
data.age = 0.0f;       // SimCreature doesn't track age
data.generation = 1;   // SimCreature doesn't track generation per-creature
```

PRE-IMPLEMENTATION PLAN
- Define the full data model needed for CreatureManager-backed saves.
- Align SaveData structs with Creature fields to avoid lossy writes.
- Verify replay frames still load without crashes.

EXECUTION PLAN (CALL CHAIN)
1) SaveManager writes SaveFileHeader and WorldSaveData.
2) CreatureSaveData is built from real Creature objects.
3) Replay frames use the same runtime creature snapshot fields.
4) Agent 1 hooks save/load calls to the new runtime data.

PHASE 1: DATA MODEL ALIGNMENT
- Add missing fields to SaveData structs if required (activity state, behavior state).
- Provide default values for older saves (backward compatibility).

PHASE 2: REPLAY COMPAT
- Ensure ReplayFrame uses the same creature id and transform data.
- Add versioning if necessary.

PHASE 3: VALIDATION
- Save, reload, and confirm creature counts and genome traits persist.
- Start replay and confirm creature motion matches baseline.

DELIVERABLES:
- Create docs/PHASE10_AGENT7_SAVE_REPLAY.md with:
  - SaveData field list and versioning notes
  - Backward compatibility strategy
  - Handoff notes for Agent 1
```

---

## AGENT 8: Observer UI, Selection, and Inspection (Creature Focus)

```
I need reliable click-to-inspect and observer UI that works with the new runtime. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/ui/SelectionSystem.*, src/ui/CreatureInspectionPanel.*, src/ui/SimulationDashboard.*, src/ui/StatisticsPanel.*, src/ui/ImGuiManager.*, src/ui/CreatureNametags.*, src/ui/DiscoveryPanel.*
- Do not edit: src/ui/MainMenu.*, src/ui/IslandComparisonUI.*, src/ui/RegionOverviewPanel.* (owned by other agents)
- Do not edit: src/main.cpp, src/entities/Creature.*

PRIMARY GOAL:
- Selection and inspection must work on live creatures with a stable focus camera and clear readouts.
- Observer UI should expose behavior state, activity state, and chemistry compatibility.

CODE ANCHORS (CURRENT CODE)
- `src/ui/SelectionSystem.h`:
```cpp
bool update(const Camera& camera, Forge::CreatureManager& creatures,
            float screenWidth, float screenHeight);
```
- `src/ui/SimulationDashboard.cpp` (tab bar):
```cpp
if (ImGui::BeginTabItem("Overview")) { ... }
if (ImGui::BeginTabItem("World")) { renderWorldTab(dayNight, camera); }
```

PRE-IMPLEMENTATION PLAN
- Ensure SelectionSystem is called in the main UI loop and connected to inspection panel.
- Identify the data fields to display (activity, behavior, chemistry, morphology).
- Define camera follow or inspect behavior through the camera controller API (handoff if needed).

EXECUTION PLAN (CALL CHAIN)
1) SelectionSystem updates on mouse input and emits SelectionChangedEvent.
2) SimulationDashboard stores inspected creature.
3) CreatureInspectionPanel renders live data and provides focus controls.
4) Camera controller switches to inspect or follow mode.

PHASE 1: INSPECTION DATA
- Show identity, activity state, behavior state, chemistry compatibility, and energy.
- Add a small activity badge and behavior badge with color coding.

PHASE 2: CAMERA FOCUS
- Add focus and release controls.
- Ensure smooth follow does not jitter or lose target.

PHASE 3: UI READABILITY
- Reduce clutter and use section grouping.
- Add search or filter for species focus.

PHASE 4: VALIDATION
- Click-to-inspect works on land and aquatic creatures.
- Inspection panel updates live.

DELIVERABLES:
- Create docs/PHASE10_AGENT8_OBSERVER_UI.md with:
  - UI layout and data bindings
  - Camera focus behavior
  - Validation notes
```

---

## AGENT 9: Performance Integration (QualityScaler + Scheduler + Rendering)

```
I need the performance systems to actually run and scale the sim for RTX 3080. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/core/PerformanceManager.*, src/core/QualityScaler.*, src/core/CreatureUpdateScheduler.*, src/core/PerformanceIntegration.*, src/graphics/RenderingOptimizer.*, src/graphics/LODSystem.*
- Do not edit: src/main.cpp, src/ui/*, src/entities/Creature.*
- Provide a handoff to Agent 1 for runtime wiring.

PRIMARY GOAL:
- Connect the performance managers to the runtime loop and provide stable FPS with large populations.

CODE ANCHORS (CURRENT CODE)
- `src/core/PerformanceManager.h`:
```cpp
void init(CreatureManager* creatures, Camera* camera);
void classifyCreatures(const glm::vec3& cameraPos, const glm::mat4& viewProjection);
```
- `src/core/CreatureUpdateScheduler.h`:
```cpp
void scheduleUpdates(const std::vector<Creature*>& creatures,
                     const glm::vec3& cameraPosition,
                     const glm::mat4& viewProjection,
                     size_t selectedIndex = SIZE_MAX);
```

PRE-IMPLEMENTATION PLAN
- Verify how creature lists are passed to scheduler and renderer after Agent 1 changes.
- Define target FPS ranges and scaling rules for 3080.
- Decide which systems scale first (AI update rate, LOD distances, grass density).

EXECUTION PLAN (CALL CHAIN)
1) PerformanceManager::beginFrame/endFrame around each frame.
2) Classify creatures for LOD and update buckets.
3) CreatureUpdateScheduler uses performance data for update throttling.
4) RenderingOptimizer uses LOD and culling to reduce draw costs.

PHASE 1: CONFIG DEFAULTS
- Define 3080 defaults for LOD thresholds and update distances.
- Add a debug overlay for current quality scale and FPS.

PHASE 2: UPDATE THROTTLING
- Use UpdateBucket to reduce far creature updates.
- Ensure selected and nearby creatures are always full update.

PHASE 3: RENDER CULLING
- Tie LOD and culling to quality scale.
- Report draw calls and instance counts in debug stats.

PHASE 4: VALIDATION
- 5-10 minute run at high population; record average FPS and 1 percent low.

DELIVERABLES:
- Create docs/PHASE10_AGENT9_PERFORMANCE_INTEGRATION.md with:
  - Default thresholds and scaling rules
  - Debug metrics and validation stats
  - Handoff notes for Agent 1
```

---

## AGENT 10: Visual Pipeline Completion (PostProcess + Lighting + Camera)

```
I need the visual pipeline to feel modern and complete: postprocess, lighting polish, and cinematic camera. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: Runtime/Shaders/*, src/graphics/PostProcess_DX12.*, src/graphics/GlobalLighting.*, src/graphics/SkyRenderer.*, src/graphics/WaterRenderer.*, src/graphics/CinematicCamera.*
- Do not edit: src/main.cpp, src/ui/*, src/entities/*
- Provide handoff notes to Agent 1 for any runtime hooks.

PRIMARY GOAL:
- Replace placeholder postprocess PSOs with a real path, improve lighting consistency, and implement cinematic camera mode.

CODE ANCHORS (CURRENT CODE)
- `src/graphics/PostProcess_DX12.cpp` (placeholder):
```cpp
// For now, we'll leave them as placeholders since the project uses DXC
return true;
```
- `src/graphics/IslandCamera.cpp` (cinematic not implemented):
```cpp
case IslandCameraMode::CINEMATIC:
    // Cinematic mode not implemented - fall back to island view
    updateIslandView(deltaTime);
```

PRE-IMPLEMENTATION PLAN
- Decide how to compile compute and graphics PSOs using the active shader path.
- Implement a simple cinematic camera path system (keyframes or orbit).
- Ensure lighting constants use PlanetTheme palette and day/night values.

EXECUTION PLAN (CALL CHAIN)
1) PostProcessManagerDX12 creates PSOs and runs passes (SSAO, bloom, tone map).
2) Lighting systems feed shader constants every frame.
3) CinematicCamera updates and can be toggled from UI (handoff).

PHASE 1: POSTPROCESS PSO CREATION
- Hook DXC shader compilation for compute and graphics passes.
- Add toggles to disable postprocess on low quality.

PHASE 2: CINEMATIC CAMERA
- Add keyframe or orbital path with smooth easing.
- Expose start/stop interface for UI (handoff to Agent 8).

PHASE 3: LIGHTING POLISH
- Ensure sky, fog, and water colors are consistent with PlanetTheme and day/night.
- Add minimal rim lighting or atmosphere tweaks in shaders.

PHASE 4: VALIDATION
- Capture 3 shots with postprocess on/off and compare.
- Verify no crashes when cinematic mode toggles.

DELIVERABLES:
- Create docs/PHASE10_AGENT10_VISUAL_PIPELINE.md with:
  - Postprocess pass list and toggles
  - Cinematic camera behavior
  - Validation notes and screenshots
```
