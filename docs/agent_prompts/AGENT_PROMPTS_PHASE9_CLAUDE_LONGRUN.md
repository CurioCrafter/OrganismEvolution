# 10 Claude Agent Prompts: Phase 9 YouTube-Ready Evolution (Long-Run, v1)

This is a high-depth prompt set. Each agent must produce a thorough, polished implementation plan and code changes, not a quick patch. Use the code anchors below to locate actual insertion points and verify your changes compile.

PARALLEL RUN RULES (MUST FOLLOW)
- Ownership map: Each agent owns the files listed in their prompt. Do not edit files owned by other agents.
- Handoff policy: If you must change an unowned file, do not edit it. Write a handoff note in `docs/PHASE9_AGENTX_HANDOFF.md` with concrete API requests and file paths.
- Main integration: Agent 1 is the only agent allowed to edit `src/main.cpp`.
- UI integration: Agent 9 is the only agent allowed to edit `src/ui/SimulationDashboard.*`, `src/ui/StatisticsPanel.*`, `src/ui/ImGuiManager.*`, `src/ui/CreatureInspectionPanel.*`, `src/ui/SelectionSystem.*`.
- Creature and animation: Agent 5 is the only agent allowed to edit `src/entities/Creature.*` and `src/animation/*`.
- Genome ownership: Agent 6 is the only agent allowed to edit `src/entities/Genome.*` and `src/entities/genetics/*`.
- Shader ownership: Agent 8 is the only agent allowed to edit `Runtime/Shaders/*`, `src/graphics/GlobalLighting.*`, `src/graphics/SkyRenderer.*`, `src/graphics/WaterRenderer.*`.
- Multi-island ownership: Agent 3 is the only agent allowed to edit `src/core/MultiIslandManager.*` and `src/entities/behaviors/InterIslandMigration.*`.
- Every agent must add instrumentation (logs, counters, or debug UI) and validate in a 5-10 minute sim run.
- Each agent doc must include: files touched, new structs/functions, tuning constants and ranges, performance notes (rough cost), validation steps (what you observed), and integration notes for Agent 1 and Agent 9.

PLANNING EXPECTATIONS (NON-NEGOTIABLE)
- Provide a pre-implementation plan with dependencies and a sequencing checklist.
- Provide an execution plan: how the code will be called in runtime (call chain + update order).
- Provide concrete parameter ranges and default values.
- Provide a rollback plan for risky changes (flags, toggles, or safe defaults).

PROJECT DIRECTION (USER CONFIRMED)
- Core fantasy: evolution sim as a spectator; optional god tools are off by default.
- Audience: simulation enthusiasts; depth and emergent outcomes over arcade goals.
- Player role: omniscient observer with close-up inspection and focus camera.
- Must-have demo: visible natural behaviors with procedural animation, better lighting/shaders, robust inspection/focus, and clear world variety.
- Worldgen: multi-region/island planets, different biomes, stars, plants/grass/trees, and adjustable generator settings.
- Evolution: start from different complexity levels with optional guidance (land/flight/aquatic); limb and sensory traits matter.
- Priority: simulation depth first; target RTX 3080; success is visible multi-species competition per island.

---

## AGENT 1: Main Menu and Planet Generator Integration (Start Pipeline)

```
I need the main menu and generator to actually drive world creation and sim settings. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/ui/MainMenu.*, src/ui/ScenarioPresets.*, src/main.cpp
- Do not edit: src/environment/ProceduralWorld.*, src/core/MultiIslandManager.*, src/entities/Genome.*, src/entities/Creature.*
- If you need to change those, write a handoff to the owning agent.

PRIMARY GOAL:
- The New Planet menu must generate a new world using its settings (seed, biomes, star type, regions) and start the sim.
- Settings should apply to runtime defaults (sim speed, max creatures, camera speed) with god tools OFF by default.

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
- `src/ui/MainMenu.h` (config structs):
```cpp
struct WorldGenConfig {
    uint32_t seed = 0;
    PlanetPreset preset = PlanetPreset::EARTH_LIKE;
    int regionCount = 1;
    float oceanCoverage = 0.3f;
    ...
};
struct EvolutionStartPreset {
    int herbivoreCount = 50;
    float mutationRate = 0.1f;
    bool enableGuidance = true;
    ...
};
```
- `src/main.cpp` (fixed seed generation):
```cpp
g_app.terrain = std::make_unique<Terrain>(256, 256, 2.0f);
g_app.terrain->generate(42);
...
g_app.vegetationManager->generate(42);
...
g_app.grassSystem->generate(42);
```

PRE-IMPLEMENTATION PLAN
- Map the current startup flow and find where to hook `MainMenu::setOnStartGame`.
- Define a translation function from ui::WorldGenConfig to environment::WorldGenConfig without editing ProceduralWorld.
- Decide how to pause/hold simulation until the menu starts a world.

EXECUTION PLAN (CALL CHAIN)
1) Main loop calls MainMenu::render when menu active.
2) Start button triggers `m_onStartGame(...)`.
3) Start handler translates config, calls ProceduralWorld::generate, applies to terrain/vegetation, and spawns initial populations.
4) SettingsConfig is applied to sim defaults (time scale, camera speed, max creatures).

PHASE 1: MENU LIFECYCLE
- Ensure menu is shown at startup and simulation updates are paused while active.
- Hook `m_onStartGame` to a new handler in main that creates/updates world state.

PHASE 2: CONFIG TRANSLATION
- Map ui::WorldGenConfig to environment::WorldGenConfig (seed, preset, ocean coverage, region count).
- Map ui::EvolutionStartPreset to `Genome::EvolutionStartPreset` and `EvolutionGuidanceBias` values.
- Keep god mode off by default and only enable god tools when toggle is set.

PHASE 3: WORLD GENERATION HOOKUP
- Replace fixed seed calls in main with generated seed from the menu.
- Use ProceduralWorld to regenerate terrain/biomes, then re-init vegetation and grass.
- Re-seed creature spawning to match the new world.

PHASE 4: SETTINGS APPLY
- Apply SettingsConfig values to sim defaults (time scale, max creatures, camera speed).
- If you need to change PerformanceManager or QualityScaler, write a handoff.

PHASE 5: DEBUG + VALIDATION
- Log seed, planet preset, region count, and generation time.
- Validate: start two different seeds, verify visual differences and correct population counts.

DELIVERABLES:
- Create docs/PHASE9_AGENT1_MENU_WORLDGEN.md with:
  - Translation mapping tables (UI config -> worldgen config)
  - Startup flow diagram and call chain
  - Validation notes and logs
```

---

## AGENT 2: Planet Variety, Biomes, and Vegetation Diversity

```
I need much stronger run-to-run world variety: terrain, biomes, vegetation, and palette. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/environment/ProceduralWorld.*, src/environment/PlanetTheme.*, src/environment/BiomePalette.*, src/environment/BiomeSystem.*, src/environment/GrassSystem.*, src/environment/VegetationManager.*, src/environment/TreeGenerator.*, src/environment/ArchipelagoGenerator.*, src/environment/Terrain.*, src/environment/TerrainErosion.*, src/environment/ClimateSystem.*, src/environment/AlienVegetation.*
- Do not edit: src/main.cpp, Runtime/Shaders/*, src/ui/*

PRIMARY GOAL:
- Every run feels like a distinct planet with different plants, grass, trees, palettes, and star/climate mood while remaining deterministic per seed.

CODE ANCHORS (CURRENT CODE)
- `src/environment/ProceduralWorld.cpp` (theme selection):
```cpp
world.planetTheme = std::make_unique<PlanetTheme>();
if (config.randomizeTheme) {
    world.planetTheme->generateRandom(world.planetSeed.paletteSeed);
} else if (config.useWeightedThemeSelection) {
    const ThemeProfile& profile = registry.selectTheme(world.planetSeed.paletteSeed);
    world.planetTheme->initializePreset(profile.basePreset, world.planetSeed.paletteSeed);
    applyThemeVariation(*world.planetTheme, world.planetSeed, profile);
}
```
- `src/environment/PlanetTheme.h` (terrain palette):
```cpp
struct TerrainPalette {
    glm::vec3 deepWaterColor;
    glm::vec3 shallowWaterColor;
    glm::vec3 sandColor;
    glm::vec3 grassColor;
    glm::vec3 forestColor;
    glm::vec3 snowColor;
};
```
- `src/environment/BiomePalette.cpp` (palette ramps):
```cpp
BiomePalette PaletteRampRegistry::generateVariedPalette(BiomeType biome, uint32_t seed) const {
    float warmth = ...;
    float saturation = ...;
    return ramps.generatePalette(warmth, saturation, seed);
}
```

PRE-IMPLEMENTATION PLAN
- Identify current variation limits (palette clamps, biome ramps, vegetation density presets).
- Decide new knobs for star/climate influence on palette and biome distribution.
- Ensure determinism for fixed seeds and high variety for random seeds.

EXECUTION PLAN (CALL CHAIN)
1) ProceduralWorld::generate derives planetSeed and climate/terrain variations.
2) PlanetTheme selects a profile and applies variation.
3) BiomeSystem uses palettes to assign biome colors and distribution.
4) VegetationManager and GrassSystem use biome palettes and density presets.

PHASE 1: PALETTE AND BIOME RAMP VARIATION
- Expand palette ramps per biome (more hue/sat ranges, temperature-based hue shifts).
- Ensure contrast between neighboring biomes (min hue distance, saturation delta).

PHASE 2: VEGETATION VARIETY
- Add seed-driven variety in tree archetypes, grass patterns, and alien plants.
- Tie vegetation density to climate and star type to create distinct ecosystems.

PHASE 3: STAR/CLIMATE INFLUENCE
- Use star type and climate to bias palette warmth, fog, and vegetation tint.
- Expose clear visual differences between star types (color temperature, sky tint).

PHASE 4: ARCHIPELAGO AND REGION MIX
- Increase biome diversity across islands and regions.
- Add per-region biome weight overrides and vegetation density modifiers.

PHASE 5: DEBUG + VALIDATION
- Add debug log showing palette variation values and vegetation densities.
- Validate three random seeds and one fixed seed (must be deterministic).

DELIVERABLES:
- Create docs/PHASE9_AGENT2_WORLD_VARIETY.md with:
  - Palette ramp changes and constraints
  - Vegetation density rules and new presets
  - Star/climate influence mapping
  - Validation notes with seed list
```

---

## AGENT 3: Multi-Island Competition and Migration

```
I need visible multi-island competition with clear per-island stats and migration. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/core/MultiIslandManager.*, src/entities/behaviors/InterIslandMigration.*, src/ui/IslandComparisonUI.*, src/ui/RegionOverviewPanel.h
- Do not edit: src/main.cpp, src/environment/ProceduralWorld.*, src/ui/SimulationDashboard.*

PRIMARY GOAL:
- Show multiple species groups competing on islands with clear stats and migration visibility.

CODE ANCHORS (CURRENT CODE)
- `src/core/MultiIslandManager.h` (island stats):
```cpp
struct IslandStats {
    int totalCreatures = 0;
    int speciesCount = 0;
    float avgFitness = 0.0f;
    ...
};
```
- `src/entities/behaviors/InterIslandMigration.h` (update loop):
```cpp
void update(float deltaTime, MultiIslandManager& islands);
```
- `src/ui/IslandComparisonUI.cpp` (overview tab):
```cpp
void IslandComparisonUI::renderOverview(const MultiIslandManager& islands) {
    auto globalStats = islands.getGlobalStats();
    ...
}
```

PRE-IMPLEMENTATION PLAN
- Verify how multi-island is currently initialized and updated.
- Decide how to bind ProceduralWorld MultiRegionConfig to islands without editing ProceduralWorld.
- Expand island stats to include competition metrics (dominant species, extinction risk).

EXECUTION PLAN (CALL CHAIN)
1) MultiIslandManager updates all islands and statistics.
2) InterIslandMigration updates and records migration events.
3) IslandComparisonUI and RegionOverviewPanel render per-island comparisons.

PHASE 1: ISLAND STATS AND COMPETITION
- Add per-island dominant species tracking and competition score (species diversity, predator-prey balance).
- Track island-level births/deaths and extinction risk for UI.

PHASE 2: MIGRATION INTEGRATION
- Improve migration triggers (population pressure, food scarcity) and expose stats.
- Add debug counters for migration attempts, successes, and failures.

PHASE 3: UI SURFACE
- Expand IslandComparisonUI to surface competition scores and dominant species.
- Update RegionOverviewPanel to show per-island risk flags and ecosystem health.

PHASE 4: VALIDATION
- Run an archipelago sim for 5-10 minutes.
- Confirm multiple islands show different population curves and migration events.

DELIVERABLES:
- Create docs/PHASE9_AGENT3_MULTI_ISLAND.md with:
  - Competition metrics formula
  - Migration tuning values and ranges
  - UI changes and validation notes
```

---

## AGENT 4: Behavior Depth and Life Events

```
I need deeper behaviors that make evolution feel alive: hunting, social groups, parental care, and variety behaviors. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/entities/behaviors/BehaviorCoordinator.*, src/entities/behaviors/ParentalCare.*, src/entities/behaviors/SocialGroups.*, src/entities/behaviors/PackHunting.*, src/entities/behaviors/TerritorialBehavior.*, src/entities/behaviors/MigrationBehavior.*, src/entities/behaviors/VarietyBehaviors.*
- Do not edit: src/entities/Creature.*, src/animation/*, src/ui/*
- If you need to change Creature or animation, write a handoff to Agent 5.

PRIMARY GOAL:
- Make behaviors visible and consistent so the player can watch real life dynamics (hunt, care, defend, socialize).

CODE ANCHORS (CURRENT CODE)
- `src/entities/behaviors/BehaviorCoordinator.h` (force blending):
```cpp
glm::vec3 calculateBehaviorForces(Creature* creature);
```
- `src/entities/behaviors/ParentalCare.h` (parent-child bond):
```cpp
void registerBirth(Creature* parent, Creature* child);
```
- `src/entities/behaviors/VarietyBehaviors.h` (variety manager):
```cpp
void update(float deltaTime, float currentTime);
glm::vec3 calculateBehaviorForce(Creature* creature, float currentTime);
```

PRE-IMPLEMENTATION PLAN
- Review how behaviors are currently triggered and blended.
- Identify gaps where behaviors never trigger or are not visible.
- Define new event hooks to ActivitySystem via handoff to Agent 5.

EXECUTION PLAN (CALL CHAIN)
1) BehaviorCoordinator::update updates each subsystem.
2) BehaviorCoordinator::calculateBehaviorForces blends forces per creature.
3) Behavior events produce signals for ActivitySystem and UI (handoff).

PHASE 1: TRIGGER QUALITY
- Add missing triggers for parental care, territorial disputes, and social play.
- Ensure hunting and fleeing have clear thresholds and cooldowns.

PHASE 2: VARIETY AND STORY BEATS
- Make curiosity, mating display, scavenging, and play behaviors reliable and visible.
- Add debug counters for each behavior type and transitions.

PHASE 3: EVENT HOOKS
- Emit explicit behavior events (hunt start, care start, display start) for ActivitySystem.
- Provide handoff details to Agent 5 for animation mapping.

PHASE 4: VALIDATION
- 5-10 minute run: observe at least three distinct behavior types per species.

DELIVERABLES:
- Create docs/PHASE9_AGENT4_BEHAVIOR_DEPTH.md with:
  - Behavior trigger table and thresholds
  - Event hooks for ActivitySystem
  - Debug counters and validation notes
```

---

## AGENT 5: Procedural Activity Animation Integration

```
I need procedural activity animations wired into the actual skeleton so behaviors look real (eat, hunt, mate, excrete, care). The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/entities/Creature.*, src/animation/*
- Do not edit: src/ui/*, src/environment/*, src/entities/Genome.*

PRIMARY GOAL:
- Activities must drive visible animations through the rig and appear in normal gameplay without manual toggles.

CODE ANCHORS (CURRENT CODE)
- `src/entities/Creature.cpp` (activity update):
```cpp
updateActivitySystem(deltaTime, foodPositions, otherCreatures);
...
m_activitySystem.update(deltaTime);
m_activityAnimDriver.update(deltaTime);
```
- `src/animation/ActivitySystem.cpp` (activity state):
```cpp
void ActivityAnimationDriver::update(float deltaTime) {
    ActivityType activity = m_stateMachine->getCurrentActivity();
    switch (activity) { ... }
}
```
- `src/animation/ActivityAnimations.h` (motion generator):
```cpp
class ActivityMotionGenerator {
public:
    void generateAllClips();
    ActivityMotionClip generateClip(ActivityType type) const;
};
```

PRE-IMPLEMENTATION PLAN
- Determine how CreatureAnimator and ActivityAnimationBlender should combine locomotion and activities.
- Decide which activities need IK targets and which can be additive.
- Ensure activity state is reflected in rendering (pose updates).

EXECUTION PLAN (CALL CHAIN)
1) Creature::updateActivitySystem updates ActivityStateMachine.
2) ActivityAnimationDriver calculates pose adjustments.
3) ActivityMotionGenerator/Blender applies activity motion to SkeletonPose.
4) CreatureAnimator uploads skinning matrices for rendering.

PHASE 1: ACTIVITY TO SKELETON
- Integrate ActivityMotionGenerator with CreatureAnimator (or AnimationStateMachine) to apply pose overrides.
- Ensure locomotion and activity blending is stable (blend weights, timing).

PHASE 2: IK TARGETS AND POSE MODIFIERS
- Use ActivityIKController for head/hand targets during eating, grooming, care.
- Add pose modifiers for excretion, sleep, mating display.

PHASE 3: VISUAL READABILITY
- Add subtle secondary motion tied to activity intensity (tail wag, breathing, head bob).
- Provide debug toggle or log for current activity and blend weight.

PHASE 4: VALIDATION
- 5-10 minute run: confirm at least 5 activity types visibly animate.

DELIVERABLES:
- Create docs/PHASE9_AGENT5_ACTIVITY_ANIMATION.md with:
  - Activity-to-pose mapping table
  - Blend weights and timing defaults
  - Validation notes and observed activities
```

---

## AGENT 6: Morphology Performance and Trait Impact

```
I need limb shape and morphology to directly affect performance and fitness. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/physics/Morphology.*, src/physics/Locomotion.*, src/physics/FitnessLandscape.*, src/physics/Metamorphosis.*, src/entities/Genome.*, src/entities/genetics/*
- Do not edit: src/entities/Creature.*, src/ui/*
- If you need Creature integration, write a handoff to Agent 5.

PRIMARY GOAL:
- Limb structure, eye placement, and body proportions must change movement efficiency and survival pressure.

CODE ANCHORS (CURRENT CODE)
- `src/physics/Morphology.h` (allometry):
```cpp
namespace Allometry {
    inline float maxSpeed(float mass) { return 10.0f * std::pow(mass, 0.17f); }
}
```
- `src/physics/Locomotion.h` (controller):
```cpp
void update(float deltaTime, const glm::vec3& desiredVelocity, float groundHeight);
```
- `src/physics/FitnessLandscape.h` (fitness factors):
```cpp
struct FitnessFactors { float movementSpeed; float energyEfficiency; ... };
```

PRE-IMPLEMENTATION PLAN
- Map current morphology genes to locomotion and fitness factors.
- Decide where to compute and cache performance metrics (per species or per creature).
- Define how morphology affects energy cost and fitness weighting.

EXECUTION PLAN (CALL CHAIN)
1) MorphologyGenes -> FitnessCalculator::calculateFactors.
2) LocomotionController uses morphology-derived speed/efficiency.
3) FitnessLandscape outputs a performance score used by evolution systems.

PHASE 1: PERFORMANCE METRICS
- Add a MorphologyPerformance struct with speed, stability, and energy efficiency derived from MorphologyGenes.
- Expose a utility function to compute this from Genome or DiploidGenome.

PHASE 2: FITNESS INTEGRATION
- Update FitnessCalculator to weight limb structure and sensory placement (eye arrangement, limb count).
- Add environmental modifiers (forest favors maneuverability, plains favors speed).

PHASE 3: MUTATION BIASES
- Adjust Genome mutation ranges so limbs and eyes can evolve without exploding cost.
- Ensure extreme morphology remains rare but possible (macro mutation guardrails).

PHASE 4: VALIDATION
- Spawn sample morphologies and log their performance metrics.
- Verify that fast vs heavy builds produce different fitness outcomes.

DELIVERABLES:
- Create docs/PHASE9_AGENT6_MORPHOLOGY_PERFORMANCE.md with:
  - Performance formulas and ranges
  - Mutation tuning rules
  - Validation logs and example creatures
```

---

## AGENT 7: Sensory Evolution and Perception Depth

```
I need sensory traits (eyes, hearing, smell) to matter and evolve in realistic ways. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/entities/SensorySystem.*, src/entities/NeuralNetwork.*, src/ai/CreatureBrainInterface.*, src/ai/BrainModules.*, src/environment/WeatherSystem.*, src/environment/ClimateSystem.*
- Do not edit: src/entities/Creature.*, src/entities/Genome.*, src/ui/*
- If you need Creature integration, write a handoff to Agent 5 or Agent 6.

PRIMARY GOAL:
- Sensory traits should strongly affect detection, behavior decisions, and energy cost in different environments.

CODE ANCHORS (CURRENT CODE)
- `src/entities/SensorySystem.h` (sensing entry point):
```cpp
void sense(..., const EnvironmentConditions& environment, ...);
```
- `src/entities/SensorySystem.h` (environment conditions):
```cpp
struct EnvironmentConditions {
    float visibility;
    float ambientLight;
    bool isUnderwater;
};
```
- `src/entities/SensorySystem.h` (energy cost):
```cpp
float calculateEnergyCost() const;
```

PRE-IMPLEMENTATION PLAN
- Identify how environment conditions are currently fed into sensing.
- Define clear detection modifiers for darkness, fog, and underwater scenes.
- Expand neural inputs to include sensory confidence and costs.

EXECUTION PLAN (CALL CHAIN)
1) Creature builds EnvironmentConditions from weather/climate.
2) SensorySystem::sense produces percepts with confidence and strength.
3) Neural inputs integrate sensory info and energy costs.

PHASE 1: ENVIRONMENTAL MODIFIERS
- Add visibility and ambient light impact for vision, and attenuation for sound/smell.
- Distinguish underwater vs land sensing (range and precision changes).

PHASE 2: SENSORY COSTS
- Scale energy cost by sensory acuity and active sensing (echolocation).
- Provide a debug summary of sensory cost per creature.

PHASE 3: EVOLUTIONARY PRESSURE
- Tie sensory performance to survival outcomes (predator detection, food finding).
- Provide handoff notes to Agent 6 for genome mutation ranges if needed.

PHASE 4: VALIDATION
- Night vs day runs should show detectable differences in behavior and success.

DELIVERABLES:
- Create docs/PHASE9_AGENT7_SENSORY_EVOLUTION.md with:
  - Sensing modifier tables
  - Energy cost formula and ranges
  - Validation observations
```

---

## AGENT 8: Shaders and Lighting Polish (Real Game Look)

```
I need modern lighting and shaders that make the sim look like a real game. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: Runtime/Shaders/*, src/graphics/GlobalLighting.*, src/graphics/SkyRenderer.*, src/graphics/WaterRenderer.*
- Do not edit: src/ui/*, src/environment/*, src/entities/*

PRIMARY GOAL:
- Improve lighting, atmosphere, terrain/creature shading, and water to reach a modern polished look.

CODE ANCHORS (CURRENT CODE)
- `Runtime/Shaders/Creature.hlsl` (frame constants):
```hlsl
cbuffer FrameConstants : register(b0)
{
    float4x4 viewProjection;
    float3 viewPos;
    float time;
    float3 lightPos;
    float3 lightColor;
    float dayTime;
    float3 skyTopColor;
    float3 skyHorizonColor;
    float sunIntensity;
    float3 ambientColor;
};
```
- `src/graphics/GlobalLighting.cpp` (theme override):
```cpp
if (theme) {
    AtmosphereSettings atm = theme->getCurrentAtmosphere();
    m_constants.lightColor = XMFLOAT3(atm.sunColor.x, atm.sunColor.y, atm.sunColor.z);
    m_constants.fogColor = XMFLOAT3(atm.fogColor.x, atm.fogColor.y, atm.fogColor.z);
}
```
- `Runtime/Shaders/Water.hlsl` or `Runtime/Shaders/Terrain.hlsl` for water/terrain shading.

PRE-IMPLEMENTATION PLAN
- Review current shader inputs and identify missing style controls.
- Decide how to use PlanetTheme data to drive shader parameters.
- Choose a small set of quality toggles for heavier effects.

EXECUTION PLAN (CALL CHAIN)
1) GlobalLighting/SkyRenderer update shader constants per frame.
2) Shaders apply stylized lighting and palette coherence.
3) WaterRenderer uses theme colors for depth and foam.

PHASE 1: CREATURE SHADING
- Add rim lighting, subsurface tint, and better specular control.
- Introduce styleStrength and ensure defaults are safe.

PHASE 2: TERRAIN AND VEGETATION
- Add slope-based blending and macro color variation.
- Keep biome contrast visible at mid distance.

PHASE 3: SKY AND ATMOSPHERE
- Add sky gradient and haze tuned by star type and theme.
- Avoid heavy post-process; keep it lightweight.

PHASE 4: WATER POLISH
- Depth-based color, shoreline foam, and gentle specular.

PHASE 5: PERFORMANCE AND VALIDATION
- Add quality toggles and estimate cost.
- Validate across at least three biome presets.

DELIVERABLES:
- Create docs/PHASE9_AGENT8_SHADERS_LIGHTING.md with:
  - Shader parameter list and defaults
  - Theme integration notes
  - Performance notes and validation screenshots
```

---

## AGENT 9: Creature Inspection, Camera Focus, and UI Readability

```
I need robust creature inspection and a readable observer UI. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/ui/CreatureInspectionPanel.*, src/ui/SelectionSystem.*, src/ui/SimulationDashboard.*, src/ui/StatisticsPanel.*, src/ui/ImGuiManager.*, src/graphics/CameraController.*, src/graphics/CinematicCamera.*
- Do not edit: src/entities/Creature.*, src/environment/*

PRIMARY GOAL:
- The player can click any creature, focus the camera, and see a deep inspection panel that updates live.

CODE ANCHORS (CURRENT CODE)
- `src/ui/SelectionSystem.h` (selection):
```cpp
bool update(const Camera& camera, Forge::CreatureManager& creatures,
            float screenWidth, float screenHeight);
```
- `src/ui/CreatureInspectionPanel.cpp` (render):
```cpp
void CreatureInspectionPanel::render() {
    if (!validateCreature()) { ... }
    renderIdentitySection(c);
    renderBiologySection(c);
    ...
}
```
- `src/graphics/CameraController.h` (inspect mode):
```cpp
void startInspect(const Creature* creature);
void exitInspect();
```

PRE-IMPLEMENTATION PLAN
- Audit selection flow and hook it to the inspection panel.
- Verify camera controller supports inspect mode and smooth focus.
- Identify missing data fields needed for inspection UI.

EXECUTION PLAN (CALL CHAIN)
1) SelectionSystem updates and emits SelectionChangedEvent.
2) SimulationDashboard or UI manager sets inspected creature.
3) CreatureInspectionPanel renders live data and controls camera focus.

PHASE 1: SELECTION AND FOCUS
- Wire SelectionSystem to inspection panel and inspect mode.
- Add focus/track/release buttons with stable camera behavior.

PHASE 2: INSPECTION UI DEPTH
- Add activity state, behavior state, and environmental context (biome, temperature).
- Show species and similarity color chips.

PHASE 3: UI READABILITY
- Improve layout spacing and reduce clutter while keeping depth.
- Ensure UI scale and readability on 1080p and 1440p.

PHASE 4: VALIDATION
- Click-to-inspect should work on land and aquatic creatures.
- Focus mode should not jitter or lose target when creature moves.

DELIVERABLES:
- Create docs/PHASE9_AGENT9_INSPECTION_UI.md with:
  - UI layout description and data bindings
  - Camera focus logic
  - Validation notes
```

---

## AGENT 10: Performance and Simulation Scaling (RTX 3080 Target)

```
I need reliable performance while keeping simulation depth high on an RTX 3080. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/core/PerformanceManager.*, src/core/QualityScaler.*, src/core/CreatureUpdateScheduler.*, src/graphics/RenderingOptimizer.*, src/graphics/LODSystem.*
- Do not edit: src/main.cpp, src/ui/*

PRIMARY GOAL:
- Maintain stable frame time with large populations by scaling updates, LOD, and render budgets.

CODE ANCHORS (CURRENT CODE)
- `src/core/QualityScaler.h` (auto adjust):
```cpp
void update(float frameTimeMs);
const QualitySettings& getCurrentSettings() const;
```
- `src/core/CreatureUpdateScheduler.h` (tiered updates):
```cpp
void scheduleUpdates(const std::vector<Creature*>& creatures,
                     const glm::vec3& cameraPosition,
                     const glm::mat4& viewProjection,
                     size_t selectedIndex = SIZE_MAX);
```
- `src/graphics/RenderingOptimizer.cpp` (culling):
```cpp
void RenderingOptimizer::cullAndSort(const std::vector<Creature*>& creatures,
                                     const Frustum& frustum, ...);
```

PRE-IMPLEMENTATION PLAN
- Identify current bottlenecks (updates vs rendering).
- Define tier budgets and quality presets for 3080.
- Decide which features scale down first when FPS drops.

EXECUTION PLAN (CALL CHAIN)
1) PerformanceManager gathers frame time and feeds QualityScaler.
2) CreatureUpdateScheduler limits update frequency by distance and importance.
3) RenderingOptimizer culls and LODs creatures based on quality settings.

PHASE 1: QUALITY PRESETS
- Tune QualityScaler defaults for 3080 (creature count, shadows, vegetation).
- Add clear thresholds for stepping down quality.

PHASE 2: UPDATE BUDGETS
- Adjust CreatureUpdateScheduler tier budgets and intervals to reduce CPU spikes.
- Ensure selected and nearby creatures remain high priority.

PHASE 3: RENDER BUDGETS
- Tie LOD distances and instance batch sizes to quality settings.
- Add debug metrics for culled counts and batch costs.

PHASE 4: VALIDATION
- 5-10 minute run at high population; record average FPS and 1 percent lows.

DELIVERABLES:
- Create docs/PHASE9_AGENT10_PERFORMANCE.md with:
  - Preset values and scaling rules
  - Scheduler tuning values
  - Validation metrics and screenshots
```
