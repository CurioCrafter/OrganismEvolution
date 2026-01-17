# 10 Agent Prompts: Phase 7 Fun + Uniqueness (Parallel Safe)

These prompts are designed to be copied into separate Claude sessions and run in parallel. Read the rules before executing any prompt.

PARALLEL RUN RULES (MUST FOLLOW)
- Ownership map: Each agent owns the files listed in their prompt. Do not edit files owned by other agents.
- Shared file policy: If you must change a shared file, do not edit it. Instead write a handoff note in docs/PHASE7_AGENTX_HANDOFF.md and let Agent 10 integrate.
- UI integration: Agent 10 is the only agent allowed to edit `src/ui/SimulationDashboard.*` and `src/ui/StatisticsPanel.*`.
- Shader boundaries: Agent 2 owns underwater and water pipeline changes. Agent 8 owns stylized shading for creatures/terrain/vegetation/sky. Do not overlap.
- Genetics/traits boundaries: Agent 3 owns amphibious transition thresholds and state. Agent 5 owns mutation ranges and morphology traits. Avoid overlapping edits.
- Every agent doc must include: files touched, new structs/functions, tuning constants and ranges, perf notes, and integration notes.

---

## AGENT 1: Run-to-Run Uniqueness (Planet Themes and Biome Variety)

```
I need you to make the simulation feel unique every run, like No Man's Sky planet variety. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/environment/PlanetTheme.*, src/environment/BiomePalette.*, src/environment/ProceduralWorld.*, src/environment/ArchipelagoGenerator.*, src/environment/ClimateSystem.*, src/environment/BiomeSystem.*, src/environment/TerrainErosion.*
- Do not edit: Runtime/Shaders/*, src/ui/*, src/graphics/*, src/entities/*, src/core/CreatureManager.cpp
- If you need UI exposure, add getters only and document for Agent 10.

PRIMARY GOAL:
- Each new run produces a clearly distinct world theme, biome mix, palette, and terrain feel, while remaining deterministic per seed.

PHASE 0: RECON (audit and map)
- Trace seed usage in ProceduralWorld and WorldGenConfig.
- List hard-coded constants that reduce variation (noise frequency, erosion passes, water thresholds, island counts).
- Inventory PlanetTheme and BiomePalette parameters and where they are consumed.

PHASE 1: PLANET SEED SYSTEM (data model)
- Add a PlanetSeed struct with stable sub-seeds (paletteSeed, climateSeed, terrainSeed, lifeSeed, oceanSeed).
- Use a deterministic hash or PRNG (PCG or splitmix) to derive sub-seeds from base seed.
- Add a seed fingerprint string (short code) for logging.
- Update WorldGenConfig to carry PlanetSeed and ensure generate() is deterministic.

PHASE 2: THEME PRESETS AND PALETTE VARIATION
- Create 8-12 theme presets with explicit parameter ranges: sky tint, fog tint, water tint, sun color, biome saturation bias.
- Add a ThemeProfile selection function based on planet seed with weighted rarity.
- Expand BiomePalette to use theme-specific ramp curves (desert/ice/lush/alien distinct).
- Add a "contrast budget" rule so biomes cannot collapse into similar colors.

PHASE 3: CLIMATE AND BIOME MIX
- Use climateSeed to drive temperature/humidity gradients (latitudinal + altitude + noise).
- Add biome mixing rules that differ per theme (arid worlds favor desert/rock; lush favors forest/wetland).
- Expose final biome weights in BiomeSystem for debugging.

PHASE 4: TERRAIN + ARCHIPELAGO VARIATION
- Use terrainSeed to vary noise layers (frequency, octave mix, ridge/valley bias).
- Use oceanSeed to vary ocean coverage and shoreline complexity (keep changes in terrain gen, not shaders).
- Expand ArchipelagoGenerator parameters (island count, size dispersion, coast irregularity, lagoon probability).
- Tune erosion parameters per theme (sharp volcanic vs soft dunes).

PHASE 5: DEBUG OUTPUT AND VALIDATION
- Add a lightweight log print of seed, theme name, ocean coverage, biome mix.
- Ensure same seed reproduces identical world; different seeds show visible differences.

DELIVERABLES:
- Create docs/PHASE7_AGENT1_UNIQUENESS.md with:
  - Seed system and sub-seed derivation
  - Theme profiles table with ranges
  - Biome palette examples and contrast rules
  - Terrain/archipelago parameter ranges
  - Integration notes for Agent 8 (shader consumption) and Agent 10 (UI display)
```

---

## AGENT 2: Underwater Visibility and Water Atmosphere

```
I need underwater visibility to be dramatically better and more readable. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/graphics/WaterRenderer.*, src/graphics/PostProcess_DX12.*, src/graphics/weather/FogSystem.*
- Do not edit: Runtime/Shaders/Terrain.hlsl, Runtime/Shaders/Weather.hlsl, src/graphics/GlobalLighting.*, src/ui/*
- If you need UI exposure, add public setters and document for Agent 10.

PRIMARY GOAL:
- Underwater is clear, readable, and beautiful. Creatures and terrain remain visible at mid-distance.

PHASE 0: RECON (pipeline)
- Identify how "camera underwater" is detected (camera position vs water height).
- Trace water rendering path and post-process chain.
- Identify where fog is applied and how depth is available.

PHASE 1: UNDERWATER STATE AND PARAMETERS
- Add an UnderwaterParams struct (fogColor, absorptionRGB, fogDensity, clarityScalar, causticIntensity).
- Add a single source of truth for camera depth below surface (float underwaterDepth).
- Expose WaterRenderer/PostProcess setters for UnderwaterParams and underwaterDepth.

PHASE 2: VISIBILITY IMPROVEMENTS (post-process)
- Add depth-based color absorption using depth buffer and underwaterDepth.
- Add fog falloff that keeps silhouettes readable (fogStart > 0, clamp to preserve mid-distance).
- Add a subtle blue/green tint that varies with depth.

PHASE 3: LIGHT SHAFTS AND CAUSTICS (cheap, stable)
- Implement light shafts using a screen-space radial gradient + noise (no heavy sampling).
- Add a simple animated caustic overlay (sine noise or low-frequency texture).
- Ensure effects are disabled above water.

PHASE 4: WATER SURFACE READABILITY
- Improve surface clarity when camera is underwater: adjust Fresnel or specular on the underside.
- Reduce surface aliasing or harsh reflection artifacts.

PHASE 5: PERFORMANCE AND TUNING
- Add quality toggles (off/low/med) with a single integer flag.
- Ensure underwater pass cost is minimal (avoid new render targets if possible).

DELIVERABLES:
- Create docs/PHASE7_AGENT2_UNDERWATER_VISIBILITY.md with:
  - UnderwaterParams definitions and suggested ranges
  - Where underwaterDepth is computed
  - Performance notes and toggle behavior
  - Integration notes for Agent 10 UI controls
```

---

## AGENT 3: Aquatic-to-Land Evolution Pipeline

```
I need a clear evolution path from underwater creatures to amphibious and land creatures. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/physics/Metamorphosis.cpp, src/entities/CreatureTraits.* (amphibious section only), src/entities/CreatureType.h, src/entities/SwimBehavior.*, src/animation/Animation.h, src/core/CreatureManager.cpp (transition logic only)
- Do not edit: src/entities/Genome.*, src/graphics/procedural/*, src/ui/*
- If you need genome changes, write a handoff note for Agent 5.

PRIMARY GOAL:
- Create meaningful transitions from aquatic -> amphibious -> land locomotion and traits without instant type flips.

PHASE 0: RECON (current behavior)
- Audit Metamorphosis.cpp for existing amphibian logic.
- Identify current aquatic trait signals and how locomotion is chosen.
- Confirm CreatureType::AMPHIBIAN usage and spawn rules.

PHASE 1: TRANSITION STATE MODEL
- Add AmphibiousStage enum (Aquatic, Transitioning, Amphibious, Land).
- Add a per-creature transition progress value and timers.
- Ensure transition state is saved/restored if there is save data.

PHASE 2: TRAIT THRESHOLDS AND ENVIRONMENTAL TRIGGERS
- Define amphibious thresholds using existing traits (lungCapacity, limbStrength, skinMoisture, aquaticAffinity).
- Use environmental exposure (time near shore, oxygen availability, time on land) to drive progress.
- Add penalties for long exposure to the wrong environment (energy drain, speed reduction).

PHASE 3: LOCOMOTION AND ANIMATION BLENDING
- Blend swim and walk velocity profiles based on transition progress.
- Adjust Animation.h to blend between aquatic and land rigs if available.
- Add a safe fallback if amphibious blend data is missing.

PHASE 4: POPULATION DYNAMICS
- In CreatureManager, allow aquatic species to transition in-place rather than hard respawn.
- Add safeguards to prevent sudden mass conversion (cooldowns and population caps).

PHASE 5: DEBUG + VALIDATION
- Add a debug flag or log line that prints transition state changes.
- Validate with a forced test: spawn aquatic group near shore and confirm gradual transition.

DELIVERABLES:
- Create docs/PHASE7_AGENT3_AQUATIC_TO_LAND.md with:
  - Diagram of state transitions and thresholds
  - Exact trait variables and formulas
  - Integration notes for Agent 10 UI or info panels
```

---

## AGENT 4: Behavior Variety Expansion

```
I need more behavior variety so creatures feel unpredictable and alive. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/entities/behaviors/*, src/ai/BrainModules.cpp, src/entities/SensorySystem.*, src/entities/SteeringBehaviors.*, src/entities/aquatic/FishSchooling.*
- Do not edit: src/environment/* (Agent 6), src/entities/Genome.* (Agent 5), src/ui/*
- If you need new ecosystem signals, request them via a handoff note to Agent 6.

PRIMARY GOAL:
- Add new behaviors and decision variety for both land and aquatic creatures with clear triggers and smooth transitions.

PHASE 0: RECON (behavior scheduler)
- Review BehaviorCoordinator and how behaviors are selected.
- Map available sensory inputs and steering utilities.

PHASE 1: NEW BEHAVIORS (modules)
- Implement at least 3 new behaviors with clear triggers and exit conditions:
  - Curiosity/Exploration (approach novel stimuli, wander).
  - Mating Ritual/Display (short cycles, energy cost, social target).
  - Scavenging (seek carcass/decay zones when hungry).
- Add a lightweight memory system (lastSeenFood, lastThreatTime).

PHASE 2: PERSONALITY + WEIGHTING
- Add per-creature behavior weights derived from existing traits (aggression, curiosity, sociability).
- Ensure weights vary by species and are stable across frames.

PHASE 3: AQUATIC GROUP DYNAMICS
- Expand FishSchooling to support group split/rejoin, panic waves, and leader following.
- Add schooling intensity variation based on threat or food.

PHASE 4: TRANSITIONS AND COOLDOWNS
- Add cooldown timers to avoid rapid behavior flipping.
- Add conflict resolution priorities (survival > mating > curiosity).

PHASE 5: DEBUG + VALIDATION
- Add debug hooks (log or counter) for behavior entry/exit events.
- Validate with a small population: confirm variety over 2-3 minutes.

DELIVERABLES:
- Create docs/PHASE7_AGENT4_BEHAVIOR_VARIETY.md with:
  - Behavior descriptions, triggers, and priorities
  - Mapping table: sensory input -> behavior weight
  - Integration notes for Agent 10 (optional UI debug)
```

---

## AGENT 5: Evolution Variety and Morphology Diversity

```
I need more evolution variety so creatures look and feel unique. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/entities/Genome.*, src/entities/CreatureTraits.* (morphology section only), src/graphics/procedural/*, src/graphics/SmallCreatureRenderer.*
- Do not edit: src/physics/Metamorphosis.cpp (Agent 3), src/ui/*, Runtime/Shaders/* (Agent 8)
- If you need amphibious traits, write a handoff note to Agent 3.

PRIMARY GOAL:
- Expand genetic and morphological diversity across all creature types, including aquatic and small creatures.

PHASE 0: RECON (trait mapping)
- Map how Genome -> CreatureTraits -> MorphologyBuilder -> Mesh/Texture generation.
- Identify current mutation ranges and where diversity collapses.

PHASE 1: GENOME EXPANSION
- Add new morphology genes (segmentCount, finCount, crestHeight, tailType, hornCount, bodyAspect).
- Use a heavy-tailed distribution for rare extreme traits (1-3 percent).
- Update mutation config to support rare macro-mutations.

PHASE 2: MORPHOLOGY BUILDER
- Implement new features: fins/frills, dorsal ridges, horns/antennae, segmented bodies.
- Ensure features are species-aware (aquatic vs land vs flying).
- Add constraints to avoid broken silhouettes (ratio clamps).

PHASE 3: TEXTURE AND COLOR VARIATION
- Expand CreatureTextureGenerator with 3-5 new pattern families (stripes, bands, speckles, gradients).
- Tie pattern selection and palette to genome traits for repeatability.
- Increase contrast variation while avoiding fully neon colors unless explicitly rare.

PHASE 4: PERFORMANCE + LOD
- Ensure new mesh features are LOD-aware and cache-friendly.
- Add a validation path to generate 20 random creatures and measure vertex counts.

DELIVERABLES:
- Create docs/PHASE7_AGENT5_MORPHOLOGY_VARIETY.md with:
  - New genes and mutation ranges
  - Mapping from genes to features
  - Example silhouettes (text list)
  - Perf notes and LOD adjustments
```

---

## AGENT 6: Ecosystem Depth and Food Web Complexity

```
I need the ecosystem to feel deeper with clearer predator/prey roles and resource cycles. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/environment/EcosystemManager.*, src/environment/ProducerSystem.*, src/environment/DecomposerSystem.*, src/environment/PlantCreatureInteraction.*, src/environment/AquaticPlants.*, src/environment/GrassSystem.*, src/environment/FungiSystem.*, src/ui/statistics/FoodWebViz.*
- Do not edit: src/entities/behaviors/* (Agent 4), src/ui/SimulationDashboard.* (Agent 10)
- If you need behavioral changes, write a handoff note to Agent 4.

PRIMARY GOAL:
- Add ecosystem depth so population dynamics create interesting stories and feedback loops.

PHASE 0: RECON (current food web)
- Map producers, consumers, decomposers, and resource flows.
- Identify missing loops (scavengers, detritus, plankton).

PHASE 1: NEW RESOURCES AND LOOPS
- Add at least one new primary resource (plankton or detritus) and one scavenger loop.
- Make producers vary by season and biome (seasonal blooms, fungal bursts).
- Add nutrient recycling where decomposers feed back to producer growth.

PHASE 2: SCARCITY SIGNALS
- Add an EcosystemSignals struct in EcosystemManager with scarcity indicators (foodPressure, carrionDensity).
- Expose read-only access for behaviors without editing behavior code.

PHASE 3: VISUALIZATION
- Update FoodWebViz to include new nodes and links.
- Add a simple count or density metric for new resources.

PHASE 4: VALIDATION
- Run a short sim and verify new resources appear and influence populations.

DELIVERABLES:
- Create docs/PHASE7_AGENT6_ECOSYSTEM_DEPTH.md with:
  - New resource types and flows
  - EcosystemSignals fields and meaning
  - Food web updates and metrics
```

---

## AGENT 7: Discovery, Scanning, and Species Catalog (No Man's Sky Feel)

```
I need a discovery loop like No Man's Sky: scan, name, and catalog species and biomes. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/entities/SpeciesNameGenerator.*, src/entities/SpeciesNaming.*, src/graphics/CreatureIdentityRenderer.*, new src/core/SpeciesCatalog.*, new src/ui/DiscoveryPanel.*
- Do not edit: src/ui/SimulationDashboard.* (Agent 10), src/ui/StatisticsPanel.* (Agent 10)
- If you need dashboard integration, provide notes for Agent 10.

PRIMARY GOAL:
- Create a fun discovery loop that makes each run feel like exploration.

PHASE 0: RECON (data sources)
- Review SpeciesNameGenerator and SpeciesNaming usage.
- Identify existing creature identity rendering and labels.

PHASE 1: SPECIES CATALOG DATA MODEL
- Implement a SpeciesCatalog system (ID, name, rarity, firstSeenTime, biome, sampleCount).
- Add methods to record discovery and query the catalog.
- Ensure data can be saved/loaded if save pipeline exists.

PHASE 2: SCANNING MECHANIC
- Add a ScanProgress struct per creature or per species (time observed, proximity).
- Require a minimum observation time to unlock details.
- Add rarity tiers based on genome complexity or morphology variance.

PHASE 3: DISCOVERY UI (NEW PANEL)
- Create a DiscoveryPanel that shows discovered species, rarity, and last seen.
- Add a simple "scan progress" widget for the current target.
- Do not wire into SimulationDashboard; leave integration to Agent 10.

PHASE 4: NAMING FLAVOR
- Tie name generation to planet theme or biome (if accessible via PlanetTheme getters).
- Provide fallback naming if theme data is unavailable.

DELIVERABLES:
- Create docs/PHASE7_AGENT7_DISCOVERY_LOOP.md with:
  - Data fields and persistence notes
  - Scan thresholds and rarity formula
  - Example discovery record
  - Integration notes for Agent 10
```

---

## AGENT 8: Shaders and Visual Identity (No Man's Sky Style)

```
I need the visuals to feel more stylized and unique per biome, closer to No Man's Sky. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: Runtime/Shaders/Creature.hlsl, Runtime/Shaders/Terrain.hlsl, Runtime/Shaders/Vegetation.hlsl, Runtime/Shaders/Weather.hlsl, Runtime/Shaders/ProceduralPattern.hlsl, src/graphics/SkyRenderer.*, src/graphics/GlobalLighting.*
- Do not edit: src/graphics/WaterRenderer.*, src/graphics/PostProcess_DX12.*, src/ui/*, src/environment/PlanetTheme.* (Agent 1)
- Consume PlanetTheme data via getters only; if missing, use safe defaults.

PRIMARY GOAL:
- Improve shaders and lighting so biomes and creatures feel visually distinct and exciting without breaking performance.

PHASE 0: RECON (shader map)
- Identify current shader constants and where they are bound.
- Map how PlanetTheme and BiomePalette data flows to shaders.

PHASE 1: CREATURE SHADING
- Add stylized rim lighting and subtle gradient shading for creatures.
- Add a simple "style strength" parameter to tune the look.
- Preserve existing lighting and add style as a blend.

PHASE 2: TERRAIN AND VEGETATION MATERIALS
- Add biome-specific tint or palette remap (desert vs ice vs lush vs alien).
- Improve material variation with slope and height blending.
- Add a small stylized specular or sheen for vegetation.

PHASE 3: SKY AND ATMOSPHERE
- Adjust sky/atmosphere tints per theme or time of day.
- Add a gentle color grading pass in GlobalLighting (no post-process edits).

PHASE 4: PARAMETERIZATION
- Define a VisualStyleParams struct in GlobalLighting or SkyRenderer with defaults.
- Ensure all new shader params have safe defaults if data is missing.

PHASE 5: VALIDATION
- Verify multiple biomes look distinct in one run.
- Verify stylized changes do not impact water/underwater (Agent 2).

DELIVERABLES:
- Create docs/PHASE7_AGENT8_SHADERS_STYLE.md with:
  - Shader parameters and defaults
  - Biome style mapping rules
  - Performance notes and toggle strategy
```

---

## AGENT 9: Cinematic Camera and Presentation

```
I need better presentation so the simulation feels fun to watch and explore. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/graphics/Camera.*, src/graphics/CameraController.*, src/graphics/IslandCamera.*
- Do not edit: src/ui/SimulationDashboard.* (Agent 10), src/graphics/PostProcess_DX12.* (Agent 2)
- Expose camera mode setters for Agent 10 UI integration.

PRIMARY GOAL:
- Add cinematic camera modes and presentation polish without touching post-process code.

PHASE 0: RECON (camera stack)
- Review CameraController and IslandCamera behavior and state.
- Identify how camera is updated and how targets are selected.

PHASE 1: CINEMATIC MODES
- Add at least 3 modes: SlowOrbit, Glide, FollowTarget.
- Implement a CinematicCameraConfig struct with speed, distance, height, smoothing.
- Add camera smoothing (critically damped) to reduce jitter.

PHASE 2: TARGET SELECTION
- Implement a target selection heuristic (largest creature, nearest action, random focus).
- Provide an API so Agent 10 can override or lock the target.

PHASE 3: COLLISION AND CLIPPING SAFETY
- Add simple collision/avoidance to keep camera out of terrain and water surface.
- Add auto height correction when underwater is not desired.

PHASE 4: PRESENTATION TOUCHES
- Add slow FOV changes or subtle roll to simulate cinematic movement (no post-process).
- Add a "photo mode" flag that freezes camera movement but preserves manual control.

DELIVERABLES:
- Create docs/PHASE7_AGENT9_CINEMATIC_CAMERA.md with:
  - Camera modes and parameters
  - Target selection rules
  - Integration notes for Agent 10 UI
```

---

## AGENT 10: Fun Loop and Progression Hooks (Integration Owner)

```
I need the sim to be more fun with short-term goals and surprises. You are the integration owner for UI wiring. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/ui/SimulationDashboard.*, src/ui/StatisticsPanel.*, src/scenarios/*, new src/core/GoalSystem.*, new src/core/AnomalySystem.*
- Do not edit: files owned by other agents. Integrate their exposed APIs only.

PRIMARY GOAL:
- Add progression hooks and integrate UI panels so each run feels guided and surprising.

PHASE 0: RECON (existing hooks)
- Review current UI panels and any scenario hooks.
- Identify available stats and events you can listen to.

PHASE 1: GOAL SYSTEM
- Implement GoalSystem with a small template library (discover X species, reach population Y, survive Z minutes).
- Pick goals from seed so each run is different.
- Track completion and provide rewards (UI highlight, minor parameter boosts).

PHASE 2: ANOMALY SYSTEM
- Implement AnomalySystem with timed events (storm surge, plankton bloom, predator invasion).
- Ensure anomalies are temporary and reversible.
- Provide event data for UI log.

PHASE 3: EVENT LOG AND UI
- Add a small on-screen log of milestones, goals, and anomalies.
- Add a Phase 7 panel with tabs: Goals, Anomalies, Discovery, Camera.
- Wire Agent 2 clarity slider, Agent 7 DiscoveryPanel, Agent 9 camera mode toggles, Agent 3 transition info, Agent 1 theme display, Agent 6 ecosystem metrics, Agent 8 style toggles, Agent 4 behavior debug (if provided).

PHASE 4: VALIDATION
- Verify goals complete, anomalies trigger, and UI updates without crashes.
- Confirm no UI edits were required in other agents' files.

DELIVERABLES:
- Create docs/PHASE7_AGENT10_FUN_LOOP.md with:
  - Goal templates and parameters
  - Anomaly list and effects
  - UI integration checklist for all agents
  - Final integration notes and known limitations
```
