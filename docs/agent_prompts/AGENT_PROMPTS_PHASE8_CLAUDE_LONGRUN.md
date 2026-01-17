# 10 Claude Agent Prompts: Phase 8 Deep Overhaul (Long-Run, v3)

This is a high-depth prompt set. Each agent must produce a thorough, polished implementation plan and code changes, not a quick patch. Use the code anchors below to locate actual insertion points and verify your changes compile.

PARALLEL RUN RULES (MUST FOLLOW)
- Ownership map: Each agent owns the files listed in their prompt. Do not edit files owned by other agents.
- Handoff policy: If you must change an unowned file, do not edit it. Write a handoff note in `docs/PHASE8_AGENTX_HANDOFF.md` with concrete API requests and file paths.
- UI integration: Agent 8 is the only agent allowed to edit `src/ui/SimulationDashboard.*`, `src/ui/StatisticsPanel.*`, and `src/ui/ImGuiManager.*`.
- Genome ownership: Agent 3 is the only agent allowed to edit `src/entities/Genome.*` and `src/environment/PlanetSeed.*`.
- Shader ownership: Agent 9 is the only agent allowed to edit `Runtime/Shaders/*`, `src/graphics/GlobalLighting.*`, `src/graphics/SkyRenderer.*`, `src/graphics/WaterRenderer.*`.
- Every agent must add instrumentation (logs, counters, or debug UI) and validate in a 5-10 minute sim run.
- Each agent doc must include: files touched, new structs/functions, tuning constants and ranges, performance notes (rough cost), validation steps (what you observed), and integration notes for Agent 8.

PLANNING EXPECTATIONS (NON-NEGOTIABLE)
- Provide a pre-implementation plan with dependencies and a sequencing checklist.
- Provide an execution plan: how the code will be called in runtime (call chain + update order).
- Provide concrete parameter ranges and default values.
- Provide a rollback plan for risky changes (flags, toggles, or safe defaults).

PROJECT DIRECTION (USER CONFIRMED)
- Core fantasy: evolution sim as a spectator; optional god tools, off by default.
- Experience: isolated sandbox with multiple regions/islands and different biomes; player can nudge environments to compare outcomes.
- Audience: simulation enthusiasts; depth and emergent outcomes over arcade goals.
- Player role: omniscient observer with close-up inspection and focus camera on any creature.
- Fun definition: see how environmental changes drive evolution; run different world setups and nudge species direction.
- Must-have for next milestone: visible natural behaviors (hunt, eat, mate, excrete, care for young) with procedural animation; better lighting/shaders; robust creature inspection/focus; procedural world variety (plants/grass/trees/palettes, stars/climates).
- Worldgen UX: main menu with planet generator (realistic vs alien) and settings; pre-run scene generator controls for biomes and vegetation.
- Evolution scope: start from different complexity levels (bare to limb-bearing) with optional guidance (land/flight/aquatic); no hard shape bans; limb and sensory traits (eyes) should matter but stay simulation-cost aware.
- Priority: simulation depth first, then visual polish; target RTX 3080.
- Success metrics: multiple species groups competing across islands with clear biome differences; simulation feels realistic and organized (not sloppy).
- Visual style: realistic, procedural, rich UI.

---

## AGENT 1: Species Identity, Naming, and Nametags (No Generic Labels)

```
I need a real species naming system and better on-screen identity. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/entities/SpeciesNameGenerator.*, src/entities/SpeciesNaming.*, new src/entities/NamePhonemeTables.*, src/ui/CreatureNametags.*, src/graphics/CreatureIdentityRenderer.*
- Do not edit: src/entities/Genome.*, src/entities/genetics/Species.*, src/ui/SimulationDashboard.*, src/ui/StatisticsPanel.*, src/ui/PhylogeneticTreeVisualizer.*
- You may consume Agent 2 APIs (SpeciesSimilaritySystem) by include only.

PRIMARY GOAL:
- Species names should be distinct, deterministic, and planet-flavored.
- Remove generic labels (e.g., "apex predator"); show species name above each creature with a trait-based descriptor to the right.

CODE ANCHORS (CURRENT CODE)
- `src/ui/CreatureNametags.cpp` (name identity wiring):
```cpp
naming::CreatureIdentity getCreatureIdentity(const Creature* creature) {
    naming::CreatureIdentity identity;
    if (!creature) return identity;

    identity.creatureId = creature->getId();
    identity.generation = creature->getGeneration();

    auto speciesId = creature->getSpeciesId();
    auto& namingSystem = naming::getNamingSystem();

    naming::CreatureTraits traits;
    traits.primaryColor = creature->getGenome().color;
    traits.size = creature->getGenome().size;
    traits.speed = creature->getGenome().speed;
    // ... type flags

    const naming::SpeciesName& speciesName = namingSystem.getOrCreateSpeciesName(speciesId, traits);
    identity.speciesName = &speciesName;

    identity.individualName = namingSystem.generateIndividualName(
        speciesId, creature->getGeneration(), -1, ""
    );
    return identity;
}
```
- `src/entities/SpeciesNaming.cpp` (current name component lists):
```cpp
void SpeciesNamingSystem::initializeNameComponents() {
    m_colorPrefixes = {"Red", "Crimson", "Scarlet", "Ruby", /* ... */ };
    m_sizePrefixes = {"Giant", "Great", "Large", "Big", /* ... */ };
    m_speedPrefixes = {"Swift", "Fleet", "Quick", "Rapid", /* ... */ };
    m_morphPrefixes = {"Crested", "Horned", "Tusked", "Fanged", /* ... */ };
    m_habitatSuffixes = {"Walker", "Runner", "Crawler", "Stalker", /* ... */ };
}
```
- `src/entities/SpeciesNameGenerator.cpp` (theme and biome prefixes):
```cpp
void SpeciesNameGenerator::initializePrefixes() {
    m_mossPrefixes = {"Moss", "Fern", "Leaf", "Willow", /* ... */ };
    m_coralPrefixes = {"Coral", "Tide", "Wave", "Kelp", /* ... */ };
    m_skyPrefixes = {"Sky", "Cloud", "Storm", "Gale", /* ... */ };
}
```

PRE-IMPLEMENTATION PLAN
- Map current naming entry points and measure duplication rate (log collisions per seed).
- Decide whether to keep SpeciesNamingSystem, SpeciesNameGenerator, or consolidate. If consolidating, add a small adapter layer so existing calls remain valid.
- Design a deterministic naming pipeline keyed by (speciesId, planet seed, optional biome).

EXECUTION PLAN (CALL CHAIN)
1) CreatureIdentityRenderer::update calls CreatureNametags::updateNametags.
2) CreatureNametags::updateNametags calls getCreatureIdentity.
3) getCreatureIdentity calls SpeciesNamingSystem::getOrCreateSpeciesName.
4) This should now use your new phoneme system and descriptor generator.
5) CreatureNametags::renderNametag renders the new species name and descriptor line, plus similarity color (if provided by Agent 2).

PHASE 1: NAME PHONEME SYSTEM
- Implement NamePhonemeTables with 6+ table sets (dry, lush, oceanic, frozen, volcanic, alien).
- Weighted syllable selection: each entry has weight; selection uses seeded RNG and cumulative weights.
- Determinism: use seed = hash(planetSeed, speciesId, tableId).
- Uniqueness: maintain unordered_set per planet. If collision, apply deterministic transforms in order:
  1) swap last syllable
  2) inject connector (apostrophe or hyphen)
  3) add suffix from a rare list
  4) append roman numeral based on collision count

PHASE 2: BINOMIAL AND GENUS CONSISTENCY
- Genus should be shared across similarity clusters (Agent 2). If cluster unavailable, hash by speciesId / 8.
- Species epithet must be unique within genus.
- Add toggles: common name vs binomial, and show/hide descriptor.

PHASE 3: TRAIT-BASED DESCRIPTORS (NO GENERIC LABELS)
- Descriptor builder uses traits: diet, locomotion, habitat, size tier.
- Example outputs (format is fixed):
  - "carnivore, aquatic"
  - "herbivore, arboreal"
  - "omnivore, burrowing"
- Ban generic categories (apex, predator). Use diet + behavior only.

PHASE 4: NAMETAG LAYOUT
- Render species line first, descriptor line beneath in smaller font and lighter alpha.
- Add a small color chip or colored text using similarity cluster color.
- Add config toggles in CreatureNametags::renderSettingsPanel.

PHASE 5: DEBUG + VALIDATION
- Add logging: number of unique names, collisions, average name length.
- Validate by generating 200+ names across 3 seeds and report collision rate.

DELIVERABLES:
- Create docs/PHASE8_AGENT1_SPECIES_IDENTITY.md with:
  - Phoneme tables + weights
  - Deterministic hash formula and collision resolution steps
  - Descriptor mapping table (traits -> labels)
  - UI layout notes and debug output sample
  - Validation results
```

---

## AGENT 2: Species Similarity, Taxonomy, and Lineage Viewer

```
I need species similarity coloring and a real lineage view. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/entities/genetics/Species.*, new src/entities/genetics/SpeciesSimilarity.*, src/ui/PhylogeneticTreeVisualizer.*
- Do not edit: src/entities/Genome.*, src/ui/CreatureNametags.*, src/ui/SimulationDashboard.*, src/ui/StatisticsPanel.*
- Provide read-only APIs that Agent 1 and Agent 8 can consume.

PRIMARY GOAL:
- Color-code species by similarity clusters and provide a lineage viewer that is readable and interactive.

CODE ANCHORS (CURRENT CODE)
- `src/entities/genetics/Species.cpp` (species stats + distance + color):
```cpp
void Species::updateStatistics(const std::vector<Creature*>& memberList) {
    // ... stats and niche update
    niche.dietSpecialization = dietSum / stats.size;
    niche.habitatPreference = habitatSum / stats.size;
    niche.activityTime = activitySum / stats.size;
    updateAlleleFrequencies(members);
}

float Species::distanceTo(const Species& other) const {
    if (members.empty() || other.members.empty()) return 1.0f;
    DiploidGenome rep1 = getRepresentativeGenome();
    DiploidGenome rep2 = other.getRepresentativeGenome();
    return rep1.distanceTo(rep2);
}

glm::vec3 Species::getColor() const {
    float hue = fmod(id * 137.508f, 360.0f) / 360.0f;
    // HSV to RGB conversion
}
```
- `src/ui/PhylogeneticTreeVisualizer.cpp` (node build + color):
```cpp
node->speciesId = sp->getId();
node->name = sp->getName();
node->population = sp->getStats().size;
node->generation = sp->getFoundingGeneration();
node->isExtinct = sp->isExtinct();
node->color = sp->getColor();
```

PRE-IMPLEMENTATION PLAN
- Define a species feature vector (size, speed, diet, niche, genome distance proxy).
- Choose a clustering strategy and threshold that yields 6-15 clusters for typical runs.
- Decide update triggers (speciation, extinction, or periodic interval).

EXECUTION PLAN (CALL CHAIN)
1) SpeciationTracker updates species lists.
2) SpeciesSimilaritySystem runs clustering on update triggers.
3) PhylogeneticTreeVisualizer pulls colors from SpeciesSimilaritySystem instead of Species::getColor.
4) Agent 1 requests cluster color for name coloring.

PHASE 1: SIMILARITY METRIC DESIGN
- Build a vector per species:
  - normalized size (stats.size or genome size)
  - average speed
  - niche (dietSpecialization, habitatPreference, activityTime)
  - representative genome distance to a global centroid (optional)
- Compute weighted L2 distance with explicit weights (start with 0.2 size, 0.2 speed, 0.4 niche, 0.2 genome).
- Normalize each feature to [0, 1] using observed min/max each update.

PHASE 2: CLUSTERING
- Implement hierarchical clustering (UPGMA) or k-medoids with a fixed threshold.
- Choose threshold to keep clusters stable (recommend 0.25-0.35 for normalized distance).
- Cache results and only recompute when the species list changes.

PHASE 3: COLOR PALETTE
- Build a deterministic palette using HCL or HSV with min delta hue of 35 degrees.
- Provide light and dark variants for UI legibility.
- Map cluster id -> color using planet seed as shuffle salt.

PHASE 4: LINEAGE VIEWER UPGRADE
- Add filter controls: by cluster id, by name substring, by extant/extinct.
- Add tooltip: name, age, divergence, cluster.
- Add zoom/pan and ensure large trees remain usable.

PHASE 5: DEBUG + VALIDATION
- Log: cluster count, average distance, recompute cost (ms).
- Validate 5-10 minute run with >50 species.

DELIVERABLES:
- Create docs/PHASE8_AGENT2_TAXONOMY_SIMILARITY.md with:
  - Similarity metric formula and weights
  - Clustering method and threshold
  - Color palette examples
  - UI interaction details and performance notes
```

---

## AGENT 3: Planet Chemistry and Biochemistry Constraints

```
I need planet chemistry and realistic biochemical variation that changes life each run. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/environment/PlanetSeed.*, new src/environment/PlanetChemistry.*, src/entities/Genome.*, new src/core/BiochemistrySystem.*
- Do not edit: src/entities/Creature.*, src/environment/PlanetTheme.*, src/ui/*
- Integration for fitness penalties must be requested via handoff to Agent 10 (owns Creature.cpp/h).

PRIMARY GOAL:
- Add planet-specific chemistry that drives realistic biochemical differences in species.

CODE ANCHORS (CURRENT CODE)
- `src/environment/PlanetSeed.h` (seed derivation):
```cpp
void deriveAllSeeds() {
    paletteSeed     = deriveSubSeed(PlanetSeedConstants::PALETTE_SEED_OFFSET);
    climateSeed     = deriveSubSeed(PlanetSeedConstants::CLIMATE_SEED_OFFSET);
    terrainSeed     = deriveSubSeed(PlanetSeedConstants::TERRAIN_SEED_OFFSET);
    lifeSeed        = deriveSubSeed(PlanetSeedConstants::LIFE_SEED_OFFSET);
    oceanSeed       = deriveSubSeed(PlanetSeedConstants::OCEAN_SEED_OFFSET);
    biomeSeed       = deriveSubSeed(PlanetSeedConstants::BIOME_SEED_OFFSET);
    vegetationSeed  = deriveSubSeed(PlanetSeedConstants::VEGETATION_SEED_OFFSET);
    creatureSeed    = deriveSubSeed(PlanetSeedConstants::CREATURE_SEED_OFFSET);
    weatherSeed     = deriveSubSeed(PlanetSeedConstants::WEATHER_SEED_OFFSET);
    archipelagoSeed = deriveSubSeed(PlanetSeedConstants::ARCHIPELAGO_SEED_OFFSET);
    fingerprint     = generateFingerprint();
}
```
- `src/entities/Genome.h` (existing trait block):
```cpp
class Genome {
public:
    float size;
    float speed;
    float visionRange;
    float efficiency;
    glm::vec3 color;
    // ... large set of aquatic/flight traits below
};
```

PRE-IMPLEMENTATION PLAN
- Add a CHEMISTRY sub-seed in PlanetSeedConstants and derive it in PlanetSeed.
- Define PlanetChemistry struct and make it accessible via ProceduralWorld or a global registry.
- Add new biochemistry genes to Genome with strict ranges and mutation rules.
- Define evolution start presets and guidance biases so worlds can start from different complexity levels.

EXECUTION PLAN (CALL CHAIN)
1) PlanetSeed derives chemistry seed.
2) PlanetChemistry is generated at world creation and stored in world state.
3) BiochemistrySystem computes compatibility for each species/genome.
4) Agent 10 integrates compatibility into Creature update (fitness/energy penalty).
5) Agent 8 consumes compatibility for inspection UI.
6) Initial population creation applies EvolutionStartPreset and optional guidance bias (handoff required).

PHASE 1: PLANET CHEMISTRY MODEL
- PlanetChemistry fields:
  - solventType (water/ammonia/methane)
  - atmosphereMix (O2, CO2, N2, trace)
  - mineralProfile (Fe, Si, Ca, S)
  - radiationLevel, acidity, salinity
- Add PlanetSeedConstants::CHEMISTRY_SEED_OFFSET and derive a chemistrySeed.

PHASE 2: GENOME BIOCHEMISTRY TRAITS
- Add genes:
  - biopigmentFamily (0-5)
  - membraneFluidity (0.0-1.0)
  - oxygenTolerance (0.0-1.0)
  - mineralizationBias (0.0-1.0)
- Constrain mutation based on chemistry (e.g., high acidity -> higher mineralization bias).

PHASE 2B: EVOLUTION START + GUIDANCE CONTROLS
- Add EvolutionStartPreset (e.g., PROTO, EARLY_LIMB, COMPLEX) and EvolutionGuidanceBias (NONE, LAND, FLIGHT, AQUATIC).
- Provide helper: initializeGenomeForPreset(genome, preset, bias, chemistry) with explicit ranges.
- Support per-region overrides (if multi-region world gen provides RegionEvolutionConfig).
- Provide safe defaults (NONE guidance, EARLY_LIMB preset).

PHASE 3: BIOCHEMISTRY SYSTEM
- Implement BiochemistrySystem with:
  - computeCompatibility(genome, chemistry)
  - computePigmentHint(genome, chemistry)
  - getSpeciesAffinity(speciesId)
- Cache per-species affinity to avoid per-frame cost.

PHASE 4: INTEGRATION HANDOFF
- Write handoff note to Agent 10 to apply compatibility penalty in Creature update.
- Write handoff note to Agent 8 to display chemistry affinity in inspection UI.
- Write handoff note to Agent 8 for main menu controls (preset + guidance), and to the spawner owner for applying preset at initial spawn.

PHASE 5: DEBUG + VALIDATION
- Log average and min compatibility per species every 60 seconds.
- Validate that incompatible chemistry shifts population over 5 minutes.

DELIVERABLES:
- Create docs/PHASE8_AGENT3_PLANET_CHEMISTRY.md with:
  - Chemistry field ranges
  - Genome gene ranges and mutation rules
  - Compatibility formula and penalties
  - Handoff notes to Agent 8 and Agent 10
```

---

## AGENT 4: Morphology Diversity and Family Differentiation

```
I need creatures to look wildly different across families, not samey. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/graphics/procedural/* (MorphologyBuilder, CreatureBuilder, CreatureTextureGenerator)
- Do not edit: src/entities/Genome.*, src/entities/genetics/Species.*, src/ui/*
- Request new genome traits via handoff to Agent 3 if needed.

PRIMARY GOAL:
- Increase morphological variety and build clear family archetypes with visible differences.

CODE ANCHORS (CURRENT CODE)
- `src/graphics/procedural/MorphologyBuilder.cpp` (feature construction):
```cpp
void MorphologyBuilder::buildFromMorphology(MetaballField& metaballs, const MorphologyGenes& genes,
                                            CreatureType type, const VisualState* visualState) {
    buildTorso(metaballs, genes, center);
    buildHead(metaballs, genes, neckEnd, visualState);
    if (genes.tailLength > 0.01f) buildTail(metaballs, genes, backEnd);
    if (genes.finCount > 0 || genes.hasDorsalFin || genes.hasPectoralFins || genes.hasCaudalFin) {
        buildFins(metaballs, genes, center);
    }
    if (genes.hornCount > 0 && genes.hornLength > 0.05f) {
        buildHorns(metaballs, genes, headPos, headRadius);
    }
    if (genes.antennaeCount > 0) {
        buildAntennae(metaballs, genes, headPos, headRadius);
    }
}
```

PRE-IMPLEMENTATION PLAN
- Define archetype taxonomy with explicit geometry cues (spines, plates, fins, segments).
- Map archetypes to existing MorphologyGenes features.
- Identify gaps that require new genes (handoff to Agent 3).

EXECUTION PLAN (CALL CHAIN)
1) Genome -> MorphologyBuilder::genomeToMorphology
2) MorphologyBuilder::buildFromMorphology or buildFromMorphologyWithLOD
3) Mesh output via MarchingCubes
4) CreatureTextureGenerator applies patterns and colors

PHASE 1: FAMILY ARCHETYPES
- Create 8 archetypes: segmented, plated, finned, long-limbed, radial, burrowing, gliding, spined.
- Deterministic selection per speciesId + planet seed.
- Each archetype must set different gene ranges (segmentCount, finCount, hornCount, bodyAspect).

PHASE 2: GEOMETRY MODULES
- Add modular features with per-archetype toggles:
  - dorsal ridges, crest fins, shell plates, tendrils
- Add constraints (min/max ratios) to avoid mesh artifacts.
- Emit morphology metrics (limb leverage, fin aspect, body aspect) for locomotion efficiency; document handoff for movement systems.

PHASE 3: TEXTURE AND PATTERN DIVERSITY
- Add 5+ pattern families tied to archetypes.
- Map pigment hints from Agent 3 into palette selection.

PHASE 4: LOD AND VERTEX BUDGET
- Define explicit vertex budgets: FULL <= 18k, REDUCED <= 8k, SIMPLIFIED <= 2k.
- Add a debug report that logs archetype + vertex counts for 20 random creatures.

PHASE 5: VALIDATION
- Spawn 30 creatures across 6 archetypes and confirm clear silhouette diversity.

DELIVERABLES:
- Create docs/PHASE8_AGENT4_MORPHOLOGY_VARIETY.md with:
  - Archetype definitions and selection logic
  - Geometry module list and parameters
  - Pattern families and palette rules
  - Perf and LOD notes
```

---

## AGENT 5: Insect Scale and Micro-Fauna Fixes

```
Insects are huge and stuck on trees. Fix their scale and ecology. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/graphics/SmallCreatureRenderer.*, src/entities/small/SmallCreatures.*, src/entities/small/SmallCreatureType.*, src/entities/small/SmallCreaturePhysics.*, src/entities/small/TreeDwellers.*
- Do not edit: Runtime/Shaders/*, src/ui/*

PRIMARY GOAL:
- Make insects properly small, varied, and distributed (ground + air + trees).

CODE ANCHORS (CURRENT CODE)
- `src/graphics/SmallCreatureRenderer.cpp` (size scaling):
```cpp
float scale = props.minSize * creature.genome->size;
XMMATRIX scaleM = XMMatrixScaling(scale, scale, scale);

if (lod == LODLevel::POINT) {
    sprite.size = props.minSize * creature.genome->size * 10.0f;  // Scaled for visibility
}
```
- `src/entities/small/SmallCreatureType.h` (size categories):
```cpp
enum class SizeCategory : uint8_t {
    MICROSCOPIC,  // < 1mm
    TINY,         // 1mm - 1cm
    SMALL,        // 1cm - 10cm
    MEDIUM        // 10cm - 30cm
};
```
- `src/entities/small/TreeDwellers.cpp` (tree usage rules):
```cpp
bool TreeDwellerSystem::canUseTree(SmallCreatureType type) {
    auto props = getProperties(type);
    return props.canClimb || type == SmallCreatureType::SQUIRREL_TREE ||
           type == SmallCreatureType::TREE_FROG || isSpider(type);
}
```

PRE-IMPLEMENTATION PLAN
- Audit size values for all insect types.
- Decide a consistent scale conversion for render vs physics.
- Define habitat distribution rules and adjust tree-only behavior.

EXECUTION PLAN (CALL CHAIN)
1) SmallCreatureManager spawns types with properties in SmallCreatureType.
2) SmallCreatureRenderer applies scale from properties and genome.
3) TreeDwellerSystem restricts tree usage (adjust here).

PHASE 1: SIZE CALIBRATION
- Set explicit min/max size (meters) for each insect type.
- Update scaling formula to respect size category and avoid *10 hack.
- Adjust speed, jump, and drag in SmallCreaturePhysics based on size.

PHASE 2: HABITAT DISTRIBUTION
- Change tree usage to be opt-in per type, not default for all climbers.
- Add spawn rules in SmallCreatureEcosystem for ground vs air vs canopy.
- Add biome and season modifiers for insect density.

PHASE 3: VISUAL READABILITY
- Add a visibility bias for micro fauna at mid distance.
- Add debug overlay: insect density by habitat.

PHASE 4: VALIDATION
- 5-minute run: verify insects appear small and distributed.
- Confirm no physics instability due to scaling.

DELIVERABLES:
- Create docs/PHASE8_AGENT5_INSECT_SCALE.md with:
  - Size ranges and scaling rules
  - Spawn/habitat changes
  - Physics tuning changes
  - Validation notes
```

---

## AGENT 6: Ocean Ecosystem Simulation (Plants + Fish + Aquatic Life)

```
I want real underwater life: plants, fish, and aquatic creatures visible in the ocean. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/environment/AquaticPlants.*, src/environment/EcosystemManager.*, src/environment/ProducerSystem.*, src/entities/SwimBehavior.*, src/entities/aquatic/* (if present)
- Do not edit: src/graphics/WaterRenderer.*, Runtime/Shaders/*

PRIMARY GOAL:
- Populate the ocean with visible life and simulate aquatic resource loops.

CODE ANCHORS (CURRENT CODE)
- `src/environment/EcosystemManager.cpp` (update loop):
```cpp
void EcosystemManager::update(float deltaTime,
                              const std::vector<std::unique_ptr<Creature>>& creatures) {
    seasons->update(deltaTime);
    producers->update(deltaTime, seasons.get());
    decomposers->update(deltaTime, seasons.get());
    updatePopulationCounts(creatures);
    metrics->update(deltaTime, currentPopulations, producers.get(), decomposers.get(), seasons.get());
}
```
- `src/environment/AquaticPlants.cpp` (plant config):
```cpp
case AquaticPlantType::KELP_GIANT:
    config.minHeight = 15.0f;
    config.maxHeight = 45.0f;
    config.preferredZone = WaterZone::MEDIUM;
    config.minDepth = 5.0f;
    config.maxDepth = 40.0f;
    config.minLight = 0.3f;
    config.attractsFish = true;
    config.fishAttractionRadius = 10.0f;
    break;
```
- `src/entities/SwimBehavior.cpp` (water depth + schooling):
```cpp
bool SwimBehavior::isInWater(const glm::vec3& pos, const Terrain& terrain) {
    float waterSurface = getWaterSurfaceHeight(pos.x, pos.z, terrain);
    float seaFloor = getSeaFloorHeight(pos.x, pos.z, terrain);
    return pos.y < waterSurface && pos.y > seaFloor;
}

SchoolingForces SwimBehavior::calculateSchoolingForces(...)
```

PRE-IMPLEMENTATION PLAN
- Identify why aquatic creatures are not spawning or visible (spawn counts, placement, culling).
- Define aquatic spawn zones and base resource loops.
- Decide whether to use small creatures or full creatures for fish schools.
- Ensure aquatic populations can be tracked per region/island when multi-region worlds are enabled.

EXECUTION PLAN (CALL CHAIN)
1) EcosystemManager update tick.
2) ProducerSystem updates resources (plankton/algae).
3) AquaticPlants update spawns/updates plant instances.
4) Creature spawner uses aquatic zones to spawn fish and predators.
5) SwimBehavior uses depth and schooling to keep fish in water.

PHASE 1: SPAWN ZONES + DEPTH BANDS
- Implement depth bands: shallow (0-5m), mid (5-25m), deep (25m+).
- Add spawn weights by biome and temperature.
- Add debug histogram: counts per depth band.
- Add per-region population stats so island competition is visible in UI.

PHASE 2: UNDERWATER FLORA
- Expand plant types: kelp forests, algae mats, coral clusters.
- Tie growth to light and nutrients (season + water clarity).

PHASE 3: FOOD WEB INTEGRATION
- Add plankton or detritus as a base resource.
- Hook fish and herbivores to algae/plankton. Predators follow schools.

PHASE 4: BEHAVIOR POLISH
- Improve schooling cohesion and predator chase patterns.
- Prevent surface pinning or terrain clipping.

PHASE 5: VALIDATION
- 5-10 minute run: confirm underwater life appears consistently.
- Verify population stability (no mass extinction or explosion).

DELIVERABLES:
- Create docs/PHASE8_AGENT6_OCEAN_ECOSYSTEM.md with:
  - Spawn zones and probabilities
  - Aquatic plant types and growth rules
  - Food web integration details
  - Validation observations
```

---

## AGENT 7: Run-to-Run World Variety (Terrain, Grass, Trees, Colors)

```
Each exe run should generate new terrain, grass, trees, and colors. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/environment/ProceduralWorld.*, src/environment/PlanetTheme.*, src/environment/BiomePalette.*, src/environment/GrassSystem.*, src/environment/VegetationManager.*, src/environment/TreeGenerator.*, src/environment/ArchipelagoGenerator.*, src/environment/ClimateSystem.*, src/environment/Terrain.*, src/environment/TerrainErosion.*, src/environment/BiomeSystem.*
- Do not edit: src/environment/PlanetSeed.* (owned by Agent 3), src/ui/*
- Provide getters for Agent 9 to use in shaders.

PRIMARY GOAL:
- Every run feels like a new planet with clear palette and terrain variation, while remaining deterministic per seed.
- Support multi-region/island simulations with distinct biome mixes and competing species.
- Expose star/climate variation (sun color, intensity, temperature curve) for planet identity.

CODE ANCHORS (CURRENT CODE)
- `src/environment/ProceduralWorld.cpp` (seed + theme + biome flow):
```cpp
uint32_t seed = ensureSeed(config.seed);
world.planetSeed.setMasterSeed(seed);
// terrain/ocean/arch variations from seed
world.planetTheme = std::make_unique<PlanetTheme>();
world.planetTheme->generateRandom(world.planetSeed.paletteSeed);
world.biomeSystem = std::make_unique<BiomeSystem>();
world.biomeSystem->initializeWithTheme(*world.planetTheme);
world.biomeSystem->generateFromIslandData(world.islandData);
```
- `src/environment/PlanetTheme.h` (palette data):
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

PRE-IMPLEMENTATION PLAN
- Identify fixed constants limiting variation (noise layers, erosion passes, palette clamps).
- Add a session seed override when no seed is specified.
- Expand biome palette ramps per biome.
- Add RegionConfig for multi-island competition (per-region biome weights + evolution bias hooks).
- Add star type parameterization (spectral class or sun color bias) to PlanetTheme.

EXECUTION PLAN (CALL CHAIN)
1) ProceduralWorld::generate uses PlanetSeed.
2) PlanetTheme generates palette and atmosphere.
3) BiomeSystem uses theme to assign biome colors.
4) GrassSystem/VegetationManager use biome colors for vegetation.
5) RegionConfig (if enabled) overrides biome weights and spawn presets per island.

PHASE 1: SESSION SEED OVERRIDE
- If config.seed == 0, generate a time-based seed and store fingerprint.
- Maintain determinism when a seed is provided.

PHASE 2: PALETTE EXPANSION
- Add 3-5 palette ramps per biome with controlled hue/sat ranges.
- Ensure adjacent biome contrast (min hue distance 20 degrees, sat delta 0.15).

PHASE 2B: REGION + STAR VARIATION
- Add RegionConfig array (region count, island ids, biome weight overrides, evolution bias hooks).
- Add star type controls (sun color/intensity, sky gradient bias, temperature curve offsets).
- Ensure RegionConfig can be surfaced in the main menu generator UI (Agent 8).

PHASE 3: TERRAIN + VEGETATION MIX
- Vary erosion passes and noise octaves per theme.
- Add vegetation density presets per theme (sparse, lush, alien).

PHASE 4: VALIDATION
- Launch 3 consecutive runs without explicit seed and confirm visible differences.
- Fixed seed run must match exactly.

DELIVERABLES:
- Create docs/PHASE8_AGENT7_WORLD_VARIETY.md with:
  - Seed rules and fingerprint format
  - Palette ramps and constraints
  - Terrain/vegetation variability ranges
  - Validation notes
```

---

## AGENT 8: Creature Inspection and Up-Close View (UI Integration Owner)

```
I need to click any creature and see it up close with details. You are the UI integration owner. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/ui/SelectionSystem.*, src/ui/SimulationDashboard.*, src/ui/StatisticsPanel.*, src/ui/ImGuiManager.*, new src/ui/CreatureInspectionPanel.*, src/graphics/CameraController.*, src/graphics/CinematicCamera.*
- Do not edit: src/ui/CreatureNametags.* (Agent 1), src/ui/PhylogeneticTreeVisualizer.* (Agent 2)
- Integrate other agents' APIs via getters only.

PRIMARY GOAL:
- Add a robust inspect mode: click a creature, focus the camera, and show a full detail panel.
- Add a main menu with planet generator and settings; god tools are opt-in and off by default.

CODE ANCHORS (CURRENT CODE)
- `src/ui/SelectionSystem.h` (selection entry point):
```cpp
bool update(const Camera& camera, Forge::CreatureManager& creatures,
            float screenWidth, float screenHeight);
void setOnSelectionChanged(SelectionChangedCallback cb) { m_onSelectionChanged = cb; }
```
- `src/ui/SimulationDashboard.cpp` (tab bar hook):
```cpp
if (ImGui::BeginTabBar("DashboardTabs")) {
    if (ImGui::BeginTabItem("Overview")) { /* ... */ }
    if (ImGui::BeginTabItem("Genetics")) { /* ... */ }
    if (ImGui::BeginTabItem("Species")) { m_speciesPanel.render(); }
    if (ImGui::BeginTabItem("World")) { renderWorldTab(dayNight, camera); }
    ImGui::EndTabBar();
}
```
- `src/graphics/CameraController.cpp` (focus/target modes):
```cpp
void CameraController::setMode(CameraMode mode, bool smooth);
void CameraController::setFollowTarget(const Creature* creature);
```

PRE-IMPLEMENTATION PLAN
- Define inspection UI layout and data bindings.
- Decide how selection will trigger inspect mode and camera focus.
- Add a safe fallback when the creature despawns.
- Design the main menu flow: New Planet (generator), Settings, Continue, Quit.

EXECUTION PLAN (CALL CHAIN)
1) SelectionSystem detects click and emits SelectionChangedEvent.
2) SimulationDashboard listens and sets current inspected creature.
3) CreatureInspectionPanel reads live data and renders UI.
4) CameraController switches to a focus mode (smooth follow or orbit).
5) Main menu writes WorldGenConfig + EvolutionStartPreset + guidance bias before world generation.

PHASE 1: INSPECT PANEL
- Implement CreatureInspectionPanel with sections:
  - Identity: species name + descriptor, similarity color chip
  - Biology: size, age, sex, chemistry affinity
  - Morphology: archetype and key features
  - Status: health, energy, hunger, activity state
  - Environment: biome, depth, temperature
- Include a compact palette strip from creature color/pigment.

PHASE 2: CAMERA FOCUS MODE
- Implement a focus mode using damped smoothing or cinematic follow.
- Provide buttons: Focus, Track, Release.

PHASE 2B: MAIN MENU + GENERATOR UI
- Implement a main menu UI with:
  - Planet type presets (realistic vs alien)
  - Seed entry + randomize
  - Region count + biome mix sliders
  - Star type / climate bias controls
  - Evolution start preset + guidance bias toggles
  - God tools toggle (default OFF)
- Add a Settings screen for graphics/perf toggles and sim speed defaults.

PHASE 3: UI INTEGRATION
- Add an "Inspect" tab in SimulationDashboard.
- Add a small on-screen indicator for the inspected creature.
- Ensure observer mode is default and god tools require explicit toggle.
- Add a Region Overview panel showing per-island populations and biome mix (if RegionConfig exists).

PHASE 4: VALIDATION
- Verify click-to-focus works on land and aquatic creatures.
- Confirm panel updates live as the creature moves.

DELIVERABLES:
- Create docs/PHASE8_AGENT8_CREATURE_INSPECTION.md with:
  - UI layout description and data bindings
  - Camera focus logic
  - Integration notes for other agents
  - Validation notes
```

---

## AGENT 9: Shaders and Visual Identity (No Man's Sky Style)

```
I want the game to look like a real game, closer to No Man's Sky. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: Runtime/Shaders/*, src/graphics/GlobalLighting.*, src/graphics/SkyRenderer.*, src/graphics/WaterRenderer.*
- Do not edit: src/ui/*, src/environment/*, src/entities/*
- Consume PlanetTheme data via getters only (Agent 7).

PRIMARY GOAL:
- Add stylized shading, palette cohesion, and atmosphere so the game looks polished.

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
- `src/graphics/WaterRenderer.cpp` (water constants):
```cpp
cbuffer WaterConstants : register(b0) {
    float4 waterColor;
    float4 shallowColor;
    float waveScale;
    float waveSpeed;
    float waveHeight;
    float foamIntensity;
    float specularPower;
    float waterHeight;
    float underwaterDepth;
};
```

PRE-IMPLEMENTATION PLAN
- Identify existing shader inputs and decide where to add new style params.
- Map PlanetTheme palette values into shader constants.
- Decide a quality toggle strategy to avoid heavy passes.

EXECUTION PLAN (CALL CHAIN)
1) C++ sets GlobalLighting and SkyRenderer constants per frame.
2) Shaders read constants and apply stylized shading.
3) WaterRenderer sets water palette and under-surface colors.
4) Agent 8 can expose styleStrength controls in UI.

PHASE 1: CREATURE SHADING
- Add rim lighting and soft gradient shading.
- Add styleStrength parameter with a safe default.
- Use pigment hints from Agent 3 if available.

PHASE 2: TERRAIN + VEGETATION
- Add slope-based palette blending and subtle specular for vegetation.
- Ensure biome contrast is visible at mid distance.

PHASE 3: SKY + ATMOSPHERE
- Add sky gradient and atmospheric haze based on theme.
- Avoid heavy post-process; do it in sky/lighting stage.

PHASE 4: WATER POLISH
- Match water tint to theme palette.
- Add shallow subsurface tint and foam color variation.

PHASE 5: PERFORMANCE + TOGGLES
- Quality modes: off/low/med/high with cost estimates.
- Ensure compile and no extra render targets.

PHASE 6: VALIDATION
- Capture screenshots across 3 biomes and confirm distinct styles.

DELIVERABLES:
- Create docs/PHASE8_AGENT9_SHADERS_STYLE.md with:
  - Shader parameter list and defaults
  - Palette integration notes
  - Performance tradeoffs and toggles
  - Validation notes
```

---

## AGENT 10: Procedural Animation, Rigging, and Creature Activities

```
I need procedural rigs and animations that show real creature activities (eating, mating, peeing, pooping, etc). The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

PARALLEL SAFETY:
- Owned files: src/animation/*, new src/entities/ActivitySystem.*, src/entities/Creature.cpp, src/entities/Creature.h
- Do not edit: src/entities/Genome.*, src/entities/behaviors/*, src/ui/*
- Expose activity state for Agent 8 (inspection UI).

PRIMARY GOAL:
- Add a procedural rig and activity system that visibly animates creature behaviors.
- Activities should cover daily-life behaviors (eat, hunt, mate, sleep, excrete, care for young) with readable motion.

CODE ANCHORS (CURRENT CODE)
- `src/animation/AnimationStateMachine.cpp` (state transitions):
```cpp
void AnimationStateMachine::setState(const std::string& stateName, bool immediate) {
    if (!hasState(stateName) || stateName == m_currentState) return;
    if (immediate) {
        forceState(stateName);
    } else if (!m_isTransitioning) {
        m_previousState = m_currentState;
        m_nextState = stateName;
        m_isTransitioning = true;
        m_transitionProgress = 0.0f;
    }
}
```
- `src/animation/ProceduralLocomotion.cpp` (gait update):
```cpp
void ProceduralLocomotion::update(float deltaTime) {
    m_time += deltaTime;
    updateGaitPhase(deltaTime);
    updateFootPlacements();
    updateBodyMotion();
}
```

PRE-IMPLEMENTATION PLAN
- Define new ActivitySystem states and how they map to AnimationStateMachine.
- Define rig generation rules based on morphology data.
- Decide event hooks for UI.

EXECUTION PLAN (CALL CHAIN)
1) Creature update computes activity state based on hunger/reproduction/etc.
2) ActivitySystem updates state and emits events.
3) AnimationStateMachine sets activity states and blends.
4) ProceduralLocomotion adds motion overlays for activity.

PHASE 1: RIG GENERATION
- Build a rig generator that adapts to morphology:
  - spine chain length from segments
  - limb anchors from limb count and spacing
  - tail and neck bones from body proportions
- Provide fallback rigs for fish-like bodies.

PHASE 2: ACTIVITY STATE MACHINE
- Implement ActivitySystem states: Idle, Eat, Hunt, Mate, Sleep, Excrete, Groom, ThreatDisplay, ParentalCare, Nesting.
- Add triggers from Creature state (hunger, reproduction readiness, stress).
- Add cooldowns and minimum duration for readability.

PHASE 3: ACTIVITY ANIMATION CLIPS
- Build procedural motion clips for each activity.
- Add secondary motion (breathing, tail sway, head bob).

PHASE 4: EVENT HOOKS + UI
- Emit activity start events and expose current activity to Agent 8.
- Add debug panel logging activity transitions.

PHASE 5: VALIDATION
- 5-10 minute run: confirm visible activities and no crashes on unusual morphologies.

DELIVERABLES:
- Create docs/PHASE8_AGENT10_PROCEDURAL_ANIMATION.md with:
  - Rig generation details and bone counts
  - Activity state machine and triggers
  - Motion parameter ranges
  - Integration notes for Agent 8
```
