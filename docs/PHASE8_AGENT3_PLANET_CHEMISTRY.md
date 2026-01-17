# Phase 8: Planet Chemistry and Biochemistry Constraints

## Overview

This document describes the Planet Chemistry system implemented by Agent 3, which introduces planet-specific chemical constraints that drive realistic biochemical differences in species across simulation runs.

## System Architecture

### Files Created/Modified

| File | Status | Description |
|------|--------|-------------|
| `src/environment/PlanetSeed.h` | Modified | Added `CHEMISTRY_SEED_OFFSET` constant and `chemistrySeed` field |
| `src/environment/PlanetChemistry.h` | Created | Chemistry structures and generation interface |
| `src/environment/PlanetChemistry.cpp` | Created | Chemistry generation from seed |
| `src/entities/Genome.h` | Modified | Added biochemistry genes |
| `src/entities/Genome.cpp` | Modified | Biochemistry gene initialization, crossover, mutation |
| `src/core/BiochemistrySystem.h` | Created | Compatibility computation interface |
| `src/core/BiochemistrySystem.cpp` | Created | Compatibility logic and caching |

---

## Chemistry Field Ranges

### PlanetChemistry Fields

| Field | Range | Description |
|-------|-------|-------------|
| `solventType` | 0-4 (enum) | WATER, AMMONIA, METHANE, SULFURIC_ACID, ETHANOL |
| `atmosphere.oxygen` | 0.0-0.4 | Oxygen fraction (Earth ~0.21) |
| `atmosphere.nitrogen` | 0.0-0.98 | Nitrogen fraction (Earth ~0.78) |
| `atmosphere.carbonDioxide` | 0.0-0.96 | CO2 fraction |
| `atmosphere.methane` | 0.0-0.2 | Methane fraction |
| `atmosphere.pressure` | 0.01-100.0 | Atmospheric pressure in atm |
| `minerals.iron` | 0.0-1.0 | Iron abundance |
| `minerals.silicon` | 0.0-1.0 | Silicon abundance |
| `minerals.calcium` | 0.0-1.0 | Calcium abundance |
| `minerals.sulfur` | 0.0-1.0 | Sulfur abundance |
| `minerals.phosphorus` | 0.0-1.0 | Phosphorus abundance |
| `minerals.copper` | 0.0-1.0 | Copper abundance |
| `minerals.magnesium` | 0.0-1.0 | Magnesium abundance |
| `minerals.zinc` | 0.0-1.0 | Zinc abundance |
| `radiationLevel` | 0.0-2.0 | UV/cosmic ray exposure (1.0 = Earth) |
| `acidity` | 0.0-14.0 | pH scale (7.0 = neutral) |
| `salinity` | 0.0-1.0 | Salt concentration |
| `temperatureBase` | -200 to 500°C | Average surface temperature |
| `temperatureRange` | 0-200°C | Day/night/seasonal variation |

### World Type Distribution

| World Type | Probability | Solvent | Key Characteristics |
|------------|-------------|---------|---------------------|
| Earth-like | 40% | Water | Moderate O2, neutral pH, temperate |
| Ocean | 15% | Water | High calcium/magnesium, high organic complexity |
| Desert | 15% | Water | High silicon/iron, high radiation, low pressure |
| Frozen | 10% | Ammonia/Methane | Very cold, low minerals, high N2 |
| Volcanic | 10% | Sulfuric Acid | High sulfur/iron, acidic, very hot |
| Toxic | 10% | Ethanol/H2SO4 | High methane/sulfur, acidic, high radiation |

---

## Genome Gene Ranges and Mutation Rules

### Biochemistry Genes Added to Genome

| Gene | Range | Default | Description |
|------|-------|---------|-------------|
| `biopigmentFamily` | 0-5 | 0-2 | Pigment type (chlorophyll, carotenoid, etc.) |
| `membraneFluidity` | 0.0-1.0 | 0.4-0.7 | Low=cold adapted, high=warm adapted |
| `oxygenTolerance` | 0.0-1.0 | 0.5-0.85 | Low=anaerobic, high=aerobic |
| `mineralizationBias` | 0.0-1.0 | 0.2-0.6 | Low=soft-bodied, high=armored |
| `solventAffinity` | 0.0-1.0 | 0.4-0.6 | 0.5=water, 0=ammonia, 1=acid |
| `temperatureTolerance` | 5.0-50.0°C | 15-35 | Degrees from optimal temperature |
| `radiationResistance` | 0.0-1.0 | 0.1-0.4 | Radiation damage resistance |
| `pHPreference` | 0.0-1.0 | 0.4-0.6 | Low=acidophile, high=alkaliphile |
| `metabolicPathway` | 0-3 | 0 | 0=aerobic, 1=anaerobic, 2=chemosynthesis, 3=photosynthesis |

### Biopigment Family Types

| ID | Name | Color | Optimal Environment |
|----|------|-------|---------------------|
| 0 | Chlorophyll | Green | Oxygen-rich, moderate light |
| 1 | Carotenoid | Orange/Red | High radiation, warm |
| 2 | Phycocyanin | Blue | Low light, requires copper |
| 3 | Bacteriorhodopsin | Purple | Extreme conditions |
| 4 | Melanin | Brown/Black | UV protection |
| 5 | Flavin | Yellow | Sulfur-rich worlds |

### Mutation Rules

- **Biopigment family**: Rare mutation (10% of base rate), ±1 step, macro-mutation can jump to any type
- **Membrane fluidity**: Normal mutation rate, heavy-tailed distribution for extreme adaptation
- **Oxygen tolerance**: Normal mutation rate, heavy-tailed for anaerobic extremes
- **Solvent affinity**: Very rare (30% of base rate), represents fundamental biochemistry change
- **Metabolic pathway**: Very rare (5% of base rate), major evolutionary event
- All continuous traits use heavy-tailed mutation with 2% macro-mutation and 1.5% extreme trait chances

### Crossover Rules

Traits are inherited as linked groups:
1. **Core biochemistry group**: biopigmentFamily, membraneFluidity, solventAffinity, metabolicPathway
2. **Environmental adaptation group**: oxygenTolerance, radiationResistance
3. **Structural biochemistry group**: mineralizationBias, pHPreference
4. **Temperature tolerance**: Blended from both parents with ±2°C variation

---

## Evolution Start Presets and Guidance Biases

### EvolutionStartPreset Enum

Controls initial complexity level of creatures at world creation.

| Preset | Description | Size Range | Sensory | Morphology |
|--------|-------------|------------|---------|------------|
| `PROTO` | Primordial soup organisms | 0.3-0.6 | Minimal | 1 segment, no appendages |
| `EARLY_LIMB` | Early multicellular (default) | Standard | Moderate | 1-3 segments |
| `COMPLEX` | Developed organisms | 0.6-1.5 | Enhanced | 2-5 segments, fins/horns |
| `ADVANCED` | Sophisticated life | 0.8-2.0 | High | 3-7 segments, complex features |

### EvolutionGuidanceBias Enum

Provides soft pressure toward certain evolutionary directions.

| Bias | Effect |
|------|--------|
| `NONE` | Pure natural selection (default) |
| `LAND` | Enhanced vision, air-breathing, reduced aquatic traits |
| `AQUATIC` | Full aquatic initialization (fins, gills, schooling) |
| `FLIGHT` | Small size, low density, wing potential |
| `UNDERGROUND` | Elongated body, enhanced vibration sense, claws |

### RegionEvolutionConfig Struct

For multi-region world generation with per-island customization:

```cpp
struct RegionEvolutionConfig {
    EvolutionStartPreset preset = EvolutionStartPreset::EARLY_LIMB;
    EvolutionGuidanceBias bias = EvolutionGuidanceBias::NONE;
    float mutationRateModifier = 1.0f;    // 1.5 = 50% faster evolution
    float selectionPressure = 1.0f;       // Higher = more deaths for unfit
    bool allowExoticBiochemistry = true;  // Restrict non-standard biochemistry
};
```

### Initialization API

```cpp
// Initialize with preset and guidance
Genome g;
g.initializeForPreset(EvolutionStartPreset::COMPLEX,
                      EvolutionGuidanceBias::AQUATIC,
                      world.chemistry);

// Initialize with full region config
RegionEvolutionConfig config;
config.preset = EvolutionStartPreset::ADVANCED;
config.bias = EvolutionGuidanceBias::FLIGHT;
g.initializeForRegion(config, world.chemistry);

// Adapt existing genome to chemistry
g.adaptToChemistry(world.chemistry);

// Chemistry-aware mutation (applies selective pressure)
g.mutateWithChemistry(0.1f, 1.0f, world.chemistry);
```

---

## Compatibility Formula

### Component Weights

| Component | Weight | Description |
|-----------|--------|-------------|
| Solvent Compatibility | 25% | How well genome works with planet's solvent |
| Oxygen Compatibility | 20% | Oxygen level matching |
| Temperature Compatibility | 20% | Temperature range overlap |
| Acidity Compatibility | 15% | pH tolerance vs environment |
| Radiation Compatibility | 10% | Radiation resistance vs exposure |
| Mineral Compatibility | 10% | Mineral availability vs needs |

### Overall Score Calculation

```
overall = solvent * 0.25 + oxygen * 0.20 + temperature * 0.20 +
          acidity * 0.15 + radiation * 0.10 + mineral * 0.10
```

### Penalty Thresholds

| Threshold | Compatibility | Effect |
|-----------|--------------|--------|
| Excellent | ≥ 0.8 | No penalties |
| Good | 0.6 - 0.8 | No penalties |
| Moderate | 0.4 - 0.6 | Up to 20% energy penalty, 10% reproduction penalty |
| Poor | 0.2 - 0.4 | 20-50% energy penalty, health loss, 40% reproduction penalty |
| Lethal | < 0.2 | Rapid health loss, cannot reproduce |

### Penalty Formulas

For compatibility between thresholds, penalties scale linearly:

```cpp
// Moderate zone (0.4 - 0.6)
energyPenalty = 1.0 + severity * 0.2;      // 1.0 - 1.2x
reproPenalty = 1.0 - severity * 0.1;       // 0.9 - 1.0

// Poor zone (0.2 - 0.4)
energyPenalty = 1.2 + severity * 0.3;      // 1.2 - 1.5x
healthLoss = severity * 0.5;               // 0 - 0.5/sec
reproPenalty = 0.9 - severity * 0.3;       // 0.6 - 0.9

// Very Poor zone (below 0.2)
energyPenalty = 1.5 + severity * 0.5;      // 1.5 - 2.0x
healthLoss = 0.5 + severity * 1.5;         // 0.5 - 2.0/sec
reproPenalty = 0.6 - severity * 0.4;       // 0.2 - 0.6
```

---

## API Usage

### Computing Compatibility

```cpp
#include "core/BiochemistrySystem.h"
#include "environment/PlanetChemistry.h"
#include "entities/Genome.h"

// Direct computation
BiochemistryCompatibility compat = BiochemistrySystem::getInstance()
    .computeCompatibility(creature.genome, world.chemistry);

float energyMultiplier = compat.energyPenaltyMultiplier;
float healthLoss = compat.healthPenaltyRate;

// Using convenience macros
float energy = BIOCHEM_ENERGY_PENALTY(creature.genome, world.chemistry);
float health = BIOCHEM_HEALTH_PENALTY(creature.genome, world.chemistry);
float repro = BIOCHEM_REPRO_PENALTY(creature.genome, world.chemistry);
```

### Species Caching (Performance Optimization)

```cpp
// Get cached compatibility (computes if not cached)
const SpeciesAffinity& affinity = BiochemistrySystem::getInstance()
    .getSpeciesAffinity(species.id, species.representativeGenome, world.chemistry);

// Invalidate when species genome changes significantly
BiochemistrySystem::getInstance().invalidateSpeciesCache(species.id);

// Clear all on world reset
BiochemistrySystem::getInstance().clearAllCaches();
```

### Pigment Color Hints

```cpp
PigmentHint hint = BiochemistrySystem::getInstance()
    .computePigmentHint(genome, chemistry);

// Blend with existing genome color
glm::vec3 suggestedColor = hint.blendWithGenome(genome.color, 0.3f);
```

### Diagnostic Logging

```cpp
// Log every 60 seconds (3600 frames at 60 FPS)
if (frameCount % 3600 == 0) {
    BiochemistrySystem::getInstance().logStatistics();
}
```

---

## Handoff Notes

### For Agent 10: Creature Update Integration

**Owner**: Agent 10 (Creature.cpp/h)

**Required Integration**:
1. Include `"core/BiochemistrySystem.h"` in Creature.cpp
2. In `Creature::update()`, apply energy and health penalties:

```cpp
// In Creature::update(float deltaTime, const PlanetChemistry& chemistry)
BiochemistryCompatibility compat = BiochemistrySystem::getInstance()
    .computeCompatibility(m_genome, chemistry);

// Apply energy penalty (increase consumption)
float energyUsed = baseEnergyConsumption * compat.energyPenaltyMultiplier;
m_energy -= energyUsed * deltaTime;

// Apply health penalty
if (compat.healthPenaltyRate > 0.0f) {
    m_health -= compat.healthPenaltyRate * deltaTime;
}

// Store for reproduction check
m_reproductionPenalty = compat.reproductionPenalty;
```

3. In reproduction logic, apply reproduction penalty:
```cpp
float effectiveReproductionChance = baseChance * m_reproductionPenalty;
```

**Note**: Consider caching per-species for performance if many creatures share genomes.

---

### For Agent 8: Inspection UI Integration

**Owner**: Agent 8 (UI systems)

**Required Integration**:
1. Display chemistry affinity in creature inspection panel
2. Show compatibility breakdown (overall, each component)
3. Display active penalties

**Suggested UI Layout**:
```
╔═══════════════════════════════════════╗
║ Biochemistry Compatibility            ║
╠═══════════════════════════════════════╣
║ Overall: ████████░░ 78%               ║
║                                       ║
║ Solvent:     ██████████ 100%          ║
║ Oxygen:      █████████░  90%          ║
║ Temperature: ██████░░░░  60%          ║
║ Radiation:   ██████████ 100%          ║
║ Acidity:     ███████░░░  70%          ║
║ Minerals:    ████████░░  80%          ║
║                                       ║
║ Active Penalties:                     ║
║   Energy: +8%                         ║
║   Health: None                        ║
║   Reproduction: -5%                   ║
╚═══════════════════════════════════════╝
```

**API for UI**:
```cpp
BiochemistryCompatibility compat = BiochemistrySystem::getInstance()
    .computeCompatibility(selectedCreature.genome, world.chemistry);

// Display each field
displayBar("Overall", compat.overall);
displayBar("Solvent", compat.solventCompatibility);
// ... etc

// Format penalties
if (compat.energyPenaltyMultiplier > 1.01f) {
    displayText("Energy: +" + formatPercent(compat.energyPenaltyMultiplier - 1.0f));
}
```

---

### For Agent 8: Main Menu Evolution Controls

**Owner**: Agent 8 (UI systems)

**Required Integration**:
Add world generation options for evolution presets:

1. **Evolution Start Preset dropdown**:
   - PROTO (Primordial - simple organisms)
   - EARLY_LIMB (Early Multicellular - default)
   - COMPLEX (Developed Life)
   - ADVANCED (Sophisticated Life)

2. **Evolution Guidance dropdown**:
   - NONE (Pure Natural Selection - default)
   - LAND (Land Creatures)
   - AQUATIC (Aquatic Creatures)
   - FLIGHT (Flying Creatures)
   - UNDERGROUND (Burrowing Creatures)

3. Pass options to world generator:
```cpp
// In world creation UI handler
RegionEvolutionConfig config;
config.preset = static_cast<EvolutionStartPreset>(presetDropdown.selectedIndex);
config.bias = static_cast<EvolutionGuidanceBias>(guidanceDropdown.selectedIndex);

// Store in world settings for creature spawning
worldSettings.evolutionConfig = config;
```

---

### For Spawner Owner: Initial Population Creation

**Required Integration**:
Apply evolution presets when spawning initial creatures.

```cpp
void spawnInitialCreatures(const PlanetChemistry& chemistry,
                           const RegionEvolutionConfig& config,
                           int count) {
    for (int i = 0; i < count; ++i) {
        Creature c;
        c.genome.initializeForRegion(config, chemistry);
        addCreature(c);
    }
}

// For multi-island worlds with per-region variation:
void spawnForRegion(int regionId, const PlanetChemistry& chemistry,
                    const std::vector<RegionEvolutionConfig>& regionConfigs,
                    int count) {
    const auto& config = regionConfigs[regionId];
    for (int i = 0; i < count; ++i) {
        Creature c;
        c.genome.initializeForRegion(config, chemistry);
        addCreatureToRegion(regionId, c);
    }
}
```

---

## Validation Checklist

- [ ] Chemistry seed generates deterministically from master seed
- [ ] Different world types produce distinct chemistry profiles
- [ ] Biochemistry genes initialize within valid ranges
- [ ] Crossover preserves linked gene groups
- [ ] Mutation rates appropriate (rare for fundamental changes)
- [ ] Compatibility formula produces values 0-1
- [ ] Penalties scale appropriately with compatibility
- [ ] Species cache invalidates correctly
- [ ] Diagnostic logging works at 60-second intervals
- [ ] Population shifts toward compatibility over 5 minutes
- [ ] Evolution presets produce visibly different creature complexity
- [ ] Guidance biases affect initial trait distributions
- [ ] Chemistry-aware mutation drifts toward compatibility
- [ ] adaptToChemistry() correctly initializes for each solvent type

## Performance Notes

- BiochemistrySystem uses singleton pattern for global access
- Species-level caching reduces per-frame computation
- Cache lifetime configurable (default 3600 frames = 60 seconds @ 60 FPS)
- Direct Genome+Chemistry computation is O(1), suitable for occasional per-creature use
- For bulk operations, use species caching

## Future Extensions

- Biome-local chemistry variations
- Chemistry evolution over geological time
- Chemical interactions between species
- Environmental chemistry events (volcanic eruptions, acid rain)
