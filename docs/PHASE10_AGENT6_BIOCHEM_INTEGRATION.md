# Phase 10 - Agent 6: Biochemistry and Evolution Preset Integration

## Overview
This document details the integration of planet chemistry with survival mechanics and evolution presets with starting complexity. The system creates a feedback loop where planet chemistry affects creature fitness, and evolution presets determine initial genome complexity at spawn time.

---

## 1. System Architecture

### 1.1 Component Relationships
```
WorldGen (Agent 2)
    └─> Generates PlanetChemistry per world seed
         └─> Stored in GeneratedWorld
              └─> Accessed by Spawner
                   └─> Applied to Genome initialization
                        └─> BiochemistrySystem evaluates compatibility
                             └─> Runtime (Agent 1) applies penalties
```

### 1.2 Data Flow
1. **World Generation**: ProceduralWorld creates PlanetChemistry from seed
2. **Spawn Time**: Spawner uses EvolutionStartPreset + PlanetChemistry to initialize genomes
3. **Runtime Evaluation**: BiochemistrySystem computes compatibility scores
4. **Penalty Application**: Agent 1 applies energy/health/reproduction penalties

---

## 2. Evolution Presets

### 2.1 Preset Definitions
Four evolutionary stages determine starting complexity:

| Preset | Description | Complexity Level |
|--------|-------------|------------------|
| **PROTO** | Primordial soup - minimal traits | 0-25% |
| **EARLY_LIMB** | Early multi-cellular | 25-50% |
| **COMPLEX** | Developed sensory systems | 50-75% |
| **ADVANCED** | Sophisticated behaviors | 75-100% |

### 2.2 Trait Range Tables

#### Physical Traits
| Trait | PROTO | EARLY_LIMB | COMPLEX | ADVANCED |
|-------|-------|------------|---------|----------|
| **size** | 0.5-0.8 | 0.7-1.2 | 1.0-1.5 | 1.2-2.0 |
| **speed** | 5-10 | 8-14 | 12-18 | 15-20 |
| **visionRange** | 10-20 | 15-30 | 25-45 | 35-50 |
| **efficiency** | 1.2-1.5 | 1.0-1.3 | 0.7-1.1 | 0.5-0.9 |

#### Sensory Traits
| Trait | PROTO | EARLY_LIMB | COMPLEX | ADVANCED |
|-------|-------|------------|---------|----------|
| **visionFOV** | 1.5-3.0 | 2.0-4.0 | 3.0-5.0 | 4.0-6.0 |
| **visionAcuity** | 0.1-0.3 | 0.2-0.5 | 0.4-0.7 | 0.6-1.0 |
| **colorPerception** | 0.0-0.2 | 0.1-0.4 | 0.3-0.7 | 0.6-1.0 |
| **hearingRange** | 10-30 | 20-50 | 40-80 | 60-100 |
| **smellRange** | 10-40 | 30-70 | 50-100 | 80-150 |
| **memoryCapacity** | 0.0-0.2 | 0.1-0.4 | 0.3-0.7 | 0.5-1.0 |

#### Morphology Traits
| Trait | PROTO | EARLY_LIMB | COMPLEX | ADVANCED |
|-------|-------|------------|---------|----------|
| **segmentCount** | 1-2 | 2-4 | 3-6 | 4-8 |
| **limbSegments** | 2 | 2-3 | 3-4 | 4-5 |
| **hornCount** | 0 | 0-1 | 0-3 | 0-6 |
| **finCount** | 3-4 | 4-5 | 5-7 | 6-8 |

---

## 3. Evolution Guidance Bias

### 3.1 Bias Types
Soft multipliers that nudge evolution without hard constraints:

| Bias | Effect | Affected Traits |
|------|--------|-----------------|
| **NONE** | No guidance | - |
| **LAND** | +Land locomotion | limbSegments, footSpread, clawLength |
| **AQUATIC** | +Aquatic traits | finSize, swimFrequency, gillEfficiency |
| **FLIGHT** | +Aerial traits | wingSpan, flapFrequency, bodyDensity |
| **UNDERGROUND** | +Burrowing | clawLength, touchRange, vibrationSensitivity |

### 3.2 Bias Application Rules
```cpp
// Soft multipliers (1.0 = neutral)
LAND bias:
  - limbSegments: +20% range extension
  - footSpread: +30% increase
  - clawLength: +40% increase
  - visionRange: +10% increase

AQUATIC bias:
  - finSize: +40% increase
  - swimFrequency: +20% increase
  - gillEfficiency: +30% increase
  - lateralLineSensitivity: +40% increase

FLIGHT bias:
  - wingSpan: +50% increase
  - flapFrequency: +20% increase
  - bodyDensity: -20% (lighter)
  - visionAcuity: +30% increase

UNDERGROUND bias:
  - clawLength: +50% increase
  - touchRange: +60% increase
  - vibrationSensitivity: +70% increase
  - visionRange: -40% (less important)
```

**Important**: Biases are applied as multipliers on top of preset ranges, not as hard locks. Evolution can still produce variants outside these suggestions.

---

## 4. Planet Chemistry Integration

### 4.1 Storage Location
`PlanetChemistry` is now stored in `GeneratedWorld` struct:
```cpp
struct GeneratedWorld {
    // ... existing fields ...
    PlanetChemistry planetChemistry;  // <-- NEW FIELD
};
```

### 4.2 Chemistry Generation
Called during world generation in `ProceduralWorld::generate()`:
```cpp
GeneratedWorld world;
world.planetChemistry = PlanetChemistry::fromSeed(config.seed);
```

### 4.3 Chemistry Logs (Agent 2 Integration)
World generation logs now include:
- Solvent type (Water, Ammonia, Methane, Sulfuric Acid, Ethanol)
- Atmosphere composition (O2, N2, CO2, etc.)
- Acidity (pH 0-14)
- Temperature range (base ± variation)
- Radiation level (multiplier)
- Mineral availability

Example log output:
```
=== Planet Chemistry Profile ===
  World Type: Oxygen-Rich Temperate World
  Solvent: Water
  Atmosphere: 21% O2, 78% N2, 0.04% CO2 (1.02 atm)
  Temperature: 18°C ± 45°C
  pH: 7.2 (neutral)
  Radiation: 1.1x Earth
  Dominant Mineral: Silicon
================================
```

---

## 5. Biochemistry Compatibility System

### 5.1 Compatibility Formula
```cpp
overall_compatibility =
    solvent_compat      * 0.25 +
    oxygen_compat       * 0.20 +
    temperature_compat  * 0.20 +
    radiation_compat    * 0.10 +
    acidity_compat      * 0.15 +
    mineral_compat      * 0.10
```

### 5.2 Component Calculation

#### Solvent Compatibility
Compares `genome.solventAffinity` (0-1) to expected affinity for planet's solvent:
- Water: expected = 0.5
- Ammonia/Methane: expected = 0.15
- Sulfuric Acid/Ethanol: expected = 0.85

Tolerance: `0.2 + genome.membraneFluidity * 0.2`

#### Oxygen Compatibility
Compares atmospheric oxygen to preferred level based on `genome.oxygenTolerance`.
- Aerobic metabolism (pathway 0): low tolerance for low oxygen
- Anaerobic metabolism (pathway 1): high tolerance across range
- Chemo/photosynthesis (pathway 2/3): moderate tolerance

#### Temperature Compatibility
Optimal temperature derived from `genome.membraneFluidity`:
```cpp
optimal_temp = -50°C + (membraneFluidity * 200°C)
```
Tolerance: `genome.temperatureTolerance` (5-50 degrees)

#### Radiation Compatibility
Required protection = `radiationLevel - 1.0`
Available protection = `genome.radiationResistance`

#### Acidity Compatibility
pH preference mapping:
- pHPreference < 0.33: Acidophile (pH 2-4)
- 0.33-0.67: Neutrophile (pH 6-8)
- > 0.67: Alkaliphile (pH 10-12)

Tolerance: ±2 pH units

#### Mineral Compatibility
Mineral demand = `genome.mineralizationBias`
Mineral supply = average of (iron, calcium, silicon, phosphorus)
Special pigment modifiers (chlorophyll needs Mg, phycocyanin needs Cu, etc.)

### 5.3 Penalty Thresholds

| Compatibility Range | Category | Energy Penalty | Health Loss | Reproduction Penalty |
|---------------------|----------|----------------|-------------|----------------------|
| **0.8 - 1.0** | Excellent | 1.0x (none) | 0.0/s | 1.0x (none) |
| **0.6 - 0.8** | Good | 1.0x (none) | 0.0/s | 1.0x (none) |
| **0.4 - 0.6** | Moderate | 1.0-1.2x | 0.0/s | 0.9-1.0x |
| **0.2 - 0.4** | Poor | 1.2-1.5x | 0.0-0.5/s | 0.6-0.9x |
| **< 0.2** | Lethal | 1.5-2.0x+ | 0.5-2.0/s+ | 0.0-0.6x |

---

## 6. Genome Initialization with Presets

### 6.1 Spawn-Time Application
When creatures spawn, genomes are initialized using:
```cpp
genome.initializeForPreset(preset, bias, planetChemistry);
```

This:
1. Applies preset trait ranges
2. Applies guidance bias multipliers
3. Calls `adaptToChemistry()` to align biochemistry traits

### 6.2 Chemistry Adaptation
`adaptToChemistry()` adjusts biochemistry genes to match planet:

**Solvent Affinity**:
```cpp
switch (chemistry.solventType) {
    case WATER: target = 0.5; break;
    case AMMONIA/METHANE: target = 0.15; break;
    case SULFURIC_ACID/ETHANOL: target = 0.85; break;
}
genome.solventAffinity = target + Random::range(-0.1, 0.1);
```

**Membrane Fluidity** (temperature adaptation):
```cpp
tempNormalized = (chemistry.temperatureBase + 50) / 200;
genome.membraneFluidity = clamp(tempNormalized + Random::range(-0.15, 0.15), 0, 1);
```

**Oxygen Tolerance**:
```cpp
oxygenNormalized = chemistry.atmosphere.oxygen / 0.3;
genome.oxygenTolerance = clamp(oxygenNormalized + Random::range(-0.2, 0.2), 0, 1);
```

**Radiation Resistance**:
```cpp
radiationNeed = max(0, chemistry.radiationLevel - 1.0);
genome.radiationResistance = clamp(radiationNeed + Random::range(-0.15, 0.15), 0, 1);
```

**pH Preference**:
```cpp
if (chemistry.acidity < 5) {
    genome.pHPreference = 0.15;  // Acidophile
} else if (chemistry.acidity > 9) {
    genome.pHPreference = 0.85;  // Alkaliphile
} else {
    genome.pHPreference = 0.5;   // Neutrophile
}
```

**Biopigment Family** (discrete choice):
Based on radiation, oxygen, and minerals:
- High radiation → Carotenoid (1) or Melanin (4)
- Low light → Phycocyanin (2)
- High oxygen → Chlorophyll (0)
- High sulfur → Flavin (5)
- Extreme conditions → Bacteriorhodopsin (3)

### 6.3 Regional Evolution Config
For multi-region worlds:
```cpp
struct RegionEvolutionConfig {
    EvolutionStartPreset preset;
    EvolutionGuidanceBias bias;
    float mutationRateModifier;      // 1.0 = normal
    float selectionPressure;         // 1.0 = normal
    bool allowExoticBiochemistry;
};
```

---

## 7. Compatibility Cache System

### 7.1 Species-Level Caching
To optimize performance, compatibility is cached per species:

```cpp
const SpeciesAffinity& getSpeciesAffinity(
    uint32_t speciesId,
    const Genome& representativeGenome,
    const PlanetChemistry& chemistry
);
```

Cache entry:
```cpp
struct SpeciesAffinity {
    uint32_t speciesId;
    BiochemistryCompatibility compatibility;
    PigmentHint pigmentHint;
    uint64_t computedFrame;
    bool isValid;
};
```

### 7.2 Cache Management
- **Lifetime**: 3600 frames (60 seconds @ 60 FPS)
- **Invalidation**: Call `invalidateSpeciesCache(speciesId)` when species genome changes
- **World Reset**: Call `clearAllCaches()` when world resets or chemistry changes

### 7.3 Per-Species Update Strategy
```cpp
// Once per species per frame (not per creature):
if (frameNumber % 60 == speciesId % 60) {  // Stagger updates
    auto& affinity = biochemSystem.getSpeciesAffinity(
        speciesId,
        species.representativeGenome,
        world.planetChemistry
    );
    species.averageCompatibility = affinity.compatibility.overall;
}
```

---

## 8. Chemistry-Aware Mutation

### 8.1 Method Signature
```cpp
void Genome::mutateWithChemistry(
    float mutationRate,
    float mutationStrength,
    const PlanetChemistry& chemistry
);
```

### 8.2 Mutation Bias Rules
Mutations are biased toward chemistry-compatible values:

**Standard mutation** (90% of mutations):
```cpp
trait += Random::gaussian(0, mutationStrength);
```

**Chemistry-biased mutation** (10% of mutations):
```cpp
float targetValue = computeChemistryTarget(trait, chemistry);
float bias = (targetValue - currentValue) * 0.3;
trait += Random::gaussian(bias, mutationStrength);
```

**Example - Solvent Affinity**:
```cpp
// Current: 0.3, Target: 0.5 (water world)
// Bias: (0.5 - 0.3) * 0.3 = 0.06
// Mutation: gaussian(0.06, mutationStrength)
// More likely to increase toward 0.5
```

### 8.3 Constraints
- Temperature tolerance: doesn't shrink below 5.0 degrees
- Oxygen tolerance: stays within 0-1 range
- pH preference: discrete zones (acid/neutral/alkaline)
- Biopigment family: rare chance to change (2%)

---

## 9. Handoff to Agent 1 (Runtime Integration)

### 9.1 Required Changes in Creature.cpp

Agent 1 must apply penalties during creature update cycle.

#### 9.1.1 Include Headers
```cpp
#include "../core/BiochemistrySystem.h"
#include "../environment/PlanetChemistry.h"
```

#### 9.1.2 Access World Chemistry
Creature update needs access to `world.planetChemistry`. Options:
- **Option A**: Pass as parameter to `Creature::update(float dt, const PlanetChemistry& chemistry)`
- **Option B**: Store reference in CreatureManager
- **Option C**: Global accessor (least preferred)

**Recommended**: Option A - pass chemistry to update

#### 9.1.3 Apply Energy Penalty
In energy consumption code:
```cpp
void Creature::update(float dt, const PlanetChemistry& chemistry) {
    // ... existing update code ...

    // Get biochemistry penalty
    float energyPenalty = BIOCHEM_ENERGY_PENALTY(m_genome, chemistry);

    // Apply to energy consumption
    float baseEnergyUse = calculateBaseEnergyUse();
    m_energy -= baseEnergyUse * energyPenalty * dt;

    // ... rest of update ...
}
```

#### 9.1.4 Apply Health Penalty
For poor compatibility environments:
```cpp
float healthLoss = BIOCHEM_HEALTH_PENALTY(m_genome, chemistry);
if (healthLoss > 0.0f) {
    m_health -= healthLoss * dt;

    // Optional: visual feedback (damage flash, color change)
    if (healthLoss > 1.0f) {
        // Creature is dying from environment
        m_environmentalDamageTint = glm::vec3(0.8f, 0.2f, 0.2f);
    }
}
```

#### 9.1.5 Apply Reproduction Penalty
In reproduction check:
```cpp
bool Creature::canReproduce() const {
    if (m_energy < m_reproductionThreshold) return false;
    if (m_age < m_maturityAge) return false;

    // Apply biochemistry penalty
    float reproPenalty = BIOCHEM_REPRO_PENALTY(m_genome, m_worldChemistry);
    float effectiveChance = m_baseReproductionChance * reproPenalty;

    return Random::chance(effectiveChance);
}
```

### 9.2 Macros Available
BiochemistrySystem.h provides convenience macros:
```cpp
BIOCHEM_ENERGY_PENALTY(genome, chemistry)   // Returns float multiplier (1.0-2.0+)
BIOCHEM_HEALTH_PENALTY(genome, chemistry)   // Returns float health/sec loss (0-2.0+)
BIOCHEM_REPRO_PENALTY(genome, chemistry)    // Returns float multiplier (0-1.0)
```

### 9.3 Optional: Species-Level Optimization
For large populations, use species cache instead of per-creature computation:
```cpp
// In CreatureManager or SpeciesManager:
void updateSpeciesCompatibility(Species& species) {
    auto& affinity = BiochemistrySystem::getInstance().getSpeciesAffinity(
        species.id,
        species.representativeGenome,
        m_worldChemistry
    );

    species.avgEnergyPenalty = affinity.compatibility.energyPenaltyMultiplier;
    species.avgHealthPenalty = affinity.compatibility.healthPenaltyRate;
    species.avgReproPenalty = affinity.compatibility.reproductionPenalty;
}

// Then in Creature::update():
float energyPenalty = m_species->avgEnergyPenalty;  // Cached value
```

### 9.4 Logging and Debugging
Add optional logging for chemistry impact:
```cpp
if (debugChemistry && frameCount % 300 == 0) {  // Every 5 seconds
    BiochemistrySystem::getInstance().logStatistics();
}
```

---

## 10. Validation Testing

### 10.1 Test Procedure
Run a 5-10 minute simulation with these parameters:

**World Setup**:
- Seed: 42
- Theme: Random (to test chemistry variety)
- Preset: EARLY_LIMB
- Bias: LAND

**Creature Setup**:
- Spawn 50 creatures of mixed types
- Enable chemistry logging
- Monitor for 300-600 seconds

### 10.2 Expected Observations

**Logs to Check**:
1. World generation logs show chemistry profile
2. Creatures spawn with biochemistry traits adapted to planet
3. Compatibility statistics logged every 60 seconds
4. Average compatibility: 0.5-0.8 (some struggle, most adapted)
5. Distribution:
   - Lethal: 0-10%
   - Poor: 10-20%
   - Moderate: 20-30%
   - Good: 30-40%
   - Excellent: 20-30%

**Behavioral Observations**:
- Creatures with poor compatibility die faster
- Well-adapted creatures have higher reproduction rates
- Species with <0.4 compatibility should struggle to survive
- Evolution over generations should improve compatibility

### 10.3 Test Scenarios

#### Test 1: Earth-Like World
```
Expected: Most creatures do well (0.6-0.9 compatibility)
Chemistry: Water, 21% O2, pH 7, 15°C
```

#### Test 2: Toxic World
```
Expected: Most creatures struggle (0.2-0.5 compatibility)
Chemistry: Sulfuric Acid, 5% O2, pH 2, 80°C
```

#### Test 3: Frozen World
```
Expected: Temperature-adapted creatures thrive, others freeze
Chemistry: Ammonia, 2% O2, pH 8, -80°C
```

### 10.4 Success Criteria
- ✅ Chemistry profile logs correctly
- ✅ Preset ranges applied at spawn
- ✅ Guidance bias affects initial population
- ✅ Compatibility calculated without crashes
- ✅ Penalties applied (energy drain visible)
- ✅ Poor-compatibility creatures die faster
- ✅ Cache system reduces computation load
- ✅ No frame rate drops from compatibility checks

---

## 11. File Modifications Summary

### Files Modified:
1. **src/environment/ProceduralWorld.h**
   - Add `PlanetChemistry planetChemistry` to `GeneratedWorld` struct

2. **src/environment/ProceduralWorld.cpp**
   - Generate chemistry in `generate()` method
   - Add chemistry logging

3. **src/entities/Genome.h**
   - Preset and bias enums already exist ✓
   - `initializeForPreset()` already declared ✓
   - `adaptToChemistry()` already declared ✓

4. **src/entities/Genome.cpp**
   - Implement preset range tables
   - Implement bias multipliers
   - Implement `adaptToChemistry()` logic
   - Implement `mutateWithChemistry()`

5. **src/core/BiochemistrySystem.h**
   - Already complete ✓
   - Macros already defined ✓

6. **src/core/BiochemistrySystem.cpp**
   - Already implemented ✓
   - Compatibility formulas complete ✓

### Files for Agent 1 (Handoff):
7. **src/entities/Creature.h**
   - Add chemistry reference or parameter
   - Store environmental damage tint (optional)

8. **src/entities/Creature.cpp**
   - Apply energy penalty
   - Apply health penalty
   - Apply reproduction penalty

9. **src/main.cpp** (or CreatureManager)
   - Pass `world.planetChemistry` to creature updates

---

## 12. Future Enhancements

### Phase 11+ Ideas:
1. **Adaptive Evolution**
   - Track species compatibility over generations
   - Accelerate adaptation mutations for struggling species

2. **Extreme Chemistry Events**
   - Volcanic eruptions change pH temporarily
   - Solar flares increase radiation
   - Seasonal solvent changes (tidal methane lakes)

3. **Hybrid Zones**
   - Regions with gradient chemistry between biomes
   - Species specialization at boundaries

4. **Biochemistry Visualization**
   - Color creatures by compatibility (green=good, red=poor)
   - Particle effects for environmental damage
   - UI panel showing chemistry impact

5. **Terraforming**
   - Player can modify planet chemistry over time
   - Watch species adapt or go extinct
   - Track evolutionary response to environmental change

---

## 13. Troubleshooting

### Issue: Creatures die instantly
**Cause**: Lethal compatibility (<0.2)
**Fix**: Increase `adaptToChemistry()` alignment strength

### Issue: No penalties observed
**Cause**: Agent 1 not applying penalties
**Fix**: Check `BIOCHEM_*_PENALTY` macros are called in `Creature::update()`

### Issue: All creatures identical
**Cause**: No variation in `adaptToChemistry()`
**Fix**: Ensure random ranges (±0.1) are applied to targets

### Issue: Cache never updates
**Cause**: `setCurrentFrame()` not called
**Fix**: Call `BiochemistrySystem::getInstance().setCurrentFrame(frameNum)` each frame

### Issue: Performance drops
**Cause**: Computing compatibility every frame for every creature
**Fix**: Use species cache, not per-creature computation

---

## 14. References

### Code Anchors:
- `src/environment/PlanetChemistry.h:117` - PlanetChemistry struct
- `src/core/BiochemistrySystem.h:94` - computeCompatibility()
- `src/entities/Genome.h:440` - initializeForPreset()
- `src/entities/Genome.h:449` - adaptToChemistry()
- `src/core/BiochemistrySystem.h:199` - BIOCHEM_ENERGY_PENALTY macro

### Related Documentation:
- Phase 2: World Generation (PlanetChemistry generation)
- Phase 7: Morphology Diversity (Genome trait structure)
- Phase 8: Genetics System (Species and evolution)

---

## Changelog
- **2026-01-16**: Initial documentation created
- **Implementation Status**:
  - ✅ PlanetChemistry system complete
  - ✅ BiochemistrySystem complete
  - ✅ Genome preset/bias enums declared
  - ⏳ Preset initialization implementation (in progress)
  - ⏳ ProceduralWorld integration (pending)
  - ❌ Agent 1 runtime integration (handoff required)
