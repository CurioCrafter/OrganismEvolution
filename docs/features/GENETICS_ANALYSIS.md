# Genetics Systems Analysis

## Overview

This document analyzes the dual genome systems currently in the OrganismEvolution project and outlines the unification strategy.

---

## 1. Current Systems Comparison

### Haploid Genome System (`src/entities/Genome.h`)

**Purpose**: Core trait storage for all creatures with mutation and crossover support.

**Traits Stored (40+ genes)**:
| Category | Traits |
|----------|--------|
| Physical | size, speed, visionRange, efficiency, color (RGB) |
| Vision | visionFOV, visionAcuity, colorPerception, motionDetection |
| Hearing | hearingRange, hearingDirectionality, echolocationAbility |
| Smell | smellRange, smellSensitivity, pheromoneProduction |
| Tactile | touchRange, vibrationSensitivity |
| Behavioral | camouflageLevel, alarmCallVolume, displayIntensity |
| Memory | memoryCapacity, memoryRetention |
| Flying | wingSpan, flapFrequency, glideRatio, preferredAltitude |
| Aquatic | finSize, tailSize, swimFrequency, swimAmplitude, preferredDepth, schoolingStrength |
| Neural | neuralWeights[24-120] |

**Mutation System**:
- Per-gene probability with independent mutation chance
- Different strength multipliers per trait category (0.1x to 15x)
- Clamped to valid ranges

**Crossover Strategies**:
1. Discrete inheritance (50/50 parent selection)
2. Color blending (averaging)
3. Uniform crossover (neural weights)
4. Linked gene groups (sensory systems inherited together)
5. Aquatic blending (averaging with variation)

---

### Diploid Genome System (`src/entities/genetics/DiploidGenome.h`)

**Purpose**: Advanced genetics with diploid inheritance, meiosis, epigenetics, and mate preferences.

**Structure**:
- 4 chromosome pairs (maternal/paternal)
- Each chromosome contains multiple genes with two alleles
- Dominance coefficients (0=recessive, 0.5=additive, 1=dominant)

**Chromosome Organization**:
| Chromosome | Traits |
|------------|--------|
| 0 - Physical | SIZE, SPEED, VISION_RANGE, EFFICIENCY, METABOLIC_RATE, FERTILITY, MATURATION_RATE |
| 1 - Display | COLOR_RED, COLOR_GREEN, COLOR_BLUE, ORNAMENT_INTENSITY, DISPLAY_FREQUENCY |
| 2 - Behavioral | AGGRESSION, SOCIALITY, CURIOSITY, FEAR_RESPONSE, MATE_SIZE_PREF, MATE_ORNAMENT_PREF, MATE_SIMILARITY_PREF, CHOOSINESS |
| 3 - Niche | DIET_SPECIALIZATION, HABITAT_PREFERENCE, ACTIVITY_TIME, HEAT_TOLERANCE, COLD_TOLERANCE |
| + Neural | 24 NEURAL_WEIGHT genes distributed across all chromosomes |

**Meiosis/Gamete Creation**:
- Recombination with crossover (70% double, 30% single)
- Random selection of recombinant chromosomes
- Haploid gamete produced

**Epigenetic System**:
- Methylation (silencing, -50%)
- Acetylation (activating, +30%)
- Phosphorylation (signal, +10%)
- Imprinting (parent-of-origin, -40%)
- Environmental stress response
- 30% inheritance probability per parent

**Mate Preferences**:
- sizePreference (-1 to 1)
- ornamentPreference (-1 to 1)
- similarityPreference (-1 to 1)
- choosiness (0 to 1)

**Species Tracking**:
- speciesId (categorical)
- lineageId (unique per organism)
- hybrid flag
- Genetic distance calculation (80% allelic, 20% structural)

---

## 2. Gap Analysis

### Missing from Haploid System:
- [ ] Diploid inheritance with dominance
- [ ] Meiosis and proper gamete creation
- [ ] Epigenetic marks
- [ ] Mate preferences
- [ ] Species tracking
- [ ] Inbreeding coefficient
- [ ] Genetic load calculation

### Missing from Diploid System:
- [ ] Flying traits (wingSpan, flapFrequency, glideRatio, preferredAltitude)
- [ ] Aquatic traits (finSize, tailSize, swimFrequency, etc.)
- [ ] Advanced sensory traits (echolocation, camouflage, etc.)
- [ ] Memory traits
- [ ] Creature type aptitudes (terrestrial, aquatic, aerial)
- [ ] Linked gene inheritance for sensory systems

---

## 3. Unification Strategy

### Decision: Use DiploidGenome as Base, Extend with Haploid Traits

**Rationale**:
1. Diploid genetics is more realistic
2. Epigenetic system already built
3. Mate preferences and species tracking in place
4. Just need to add missing traits

### New Chromosome Layout (Extended)

| Chromosome | Genes |
|------------|-------|
| 0 - Physical | size, speed, visionRange, efficiency, metabolicRate, fertility, maturationRate |
| 1 - Coloration | colorR, colorG, colorB, ornamentIntensity, displayFrequency, patternType |
| 2 - Sensory | visionFOV, visionAcuity, colorPerception, motionDetection, hearingRange, hearingDirectionality, echolocationAbility, smellRange, smellSensitivity, pheromoneProduction, touchRange, vibrationSensitivity |
| 3 - Behavioral | aggression, sociability, curiosity, fearResponse, camouflageLevel, alarmCallVolume |
| 4 - Memory | memoryCapacity, memoryRetention |
| 5 - Mate Prefs | mateSizePref, mateOrnamentPref, mateSimilarityPref, choosiness |
| 6 - Aptitudes | terrestrialAptitude, aquaticAptitude, aerialAptitude |
| 7 - Flying | wingSpan, flapFrequency, glideRatio, preferredAltitude |
| 8 - Aquatic | finSize, tailSize, swimFrequency, swimAmplitude, preferredDepth, schoolingStrength |
| 9 - Niche | dietSpecialization, habitatPreference, activityTime, heatTolerance, coldTolerance |
| 10-13 - Neural | 24 neural weights (6 per chromosome) |

### Key Changes

1. **CreatureType from Genotype (C-15)**:
   - Use aptitude genes (terrestrial, aquatic, aerial)
   - Combine with behavioral genes (aggression, size)
   - Determine type at phenotype expression time

2. **Enhanced Fitness (C-16)**:
   - Multi-component fitness calculation
   - Survival (40%), Reproduction (30%), Resources (20%), Social (10%)
   - Type-specific bonuses (predator kills, prey avoidance)

3. **Mate Preferences Used (C-17)**:
   - Integrate existing MatePreferences struct
   - Add willMateWith() method to Creature
   - Probabilistic acceptance based on preference matching

---

## 4. Implementation Plan

### Phase 1: Extend DiploidGenome
- Add new GeneTypes for all missing traits
- Add new chromosomes for organization
- Implement trait accessors

### Phase 2: Gene Expression
- Phenotype struct with all expressed traits
- expressGene() using dominance coefficients
- Apply epigenetic modifiers

### Phase 3: CreatureType Determination
- determineCreatureType() based on aptitudes
- Return appropriate CreatureType enum
- Consider aggression and size for subtype

### Phase 4: Enhanced Fitness
- calculateFitness() with multiple components
- Type-specific weighting
- Track survival stats

### Phase 5: Mate Preferences Integration
- willMateWith() in Creature class
- Attraction scoring
- Probabilistic acceptance

### Phase 6: Creature Migration
- Replace Genome with UnifiedGenome in Creature
- Update constructors
- Update reproduction logic

---

## 5. Files to Modify

| File | Changes |
|------|---------|
| `src/entities/genetics/DiploidGenome.h` | Add new GeneTypes, extend structure |
| `src/entities/genetics/DiploidGenome.cpp` | Implement new traits, expression |
| `src/entities/genetics/Gene.h` | Add new gene types enum |
| `src/entities/genetics/Gene.cpp` | Value ranges for new genes |
| `src/entities/Creature.h` | Switch to UnifiedGenome, add willMateWith() |
| `src/entities/Creature.cpp` | Update to use phenotype expression |
| `src/entities/CreatureType.h` | Ensure all types present |

---

## 6. Backward Compatibility

### Strategy: Conversion Functions

```cpp
// Convert old Genome to UnifiedGenome
UnifiedGenome UnifiedGenome::fromLegacyGenome(const Genome& old);

// Convert UnifiedGenome to Genome (for any legacy code)
Genome UnifiedGenome::toLegacyGenome() const;
```

### Deprecation Timeline
1. Add UnifiedGenome with full functionality
2. Update Creature to use UnifiedGenome
3. Mark old Genome as deprecated
4. Remove in future version

---

## 7. Testing Strategy

1. **Unit Tests**: Gene expression, mutation, crossover
2. **Integration Tests**: Creature creation with genomes
3. **Evolution Tests**: Run simulation, verify traits evolve
4. **Speciation Tests**: Verify species tracking works
5. **Performance Tests**: Compare with old system

---

## 8. Implementation Summary

### Completed Changes

#### Gene.h Extensions
Added 30+ new GeneTypes organized into categories:
- **Creature Type Aptitudes**: TERRESTRIAL_APTITUDE, AQUATIC_APTITUDE, AERIAL_APTITUDE
- **Flying Traits**: WING_SPAN, FLAP_FREQUENCY, GLIDE_RATIO, PREFERRED_ALTITUDE
- **Aquatic Traits**: FIN_SIZE, TAIL_SIZE, SWIM_FREQUENCY, SWIM_AMPLITUDE, PREFERRED_DEPTH, SCHOOLING_STRENGTH
- **Sensory Traits**: VISION_FOV, VISION_ACUITY, COLOR_PERCEPTION, MOTION_DETECTION, HEARING_RANGE, HEARING_DIRECTIONALITY, ECHOLOCATION_ABILITY, SMELL_RANGE, SMELL_SENSITIVITY, PHEROMONE_PRODUCTION, TOUCH_RANGE, VIBRATION_SENSITIVITY
- **Defense/Communication**: CAMOUFLAGE_LEVEL, ALARM_CALL_VOLUME
- **Memory**: MEMORY_CAPACITY, MEMORY_RETENTION

#### Gene.cpp Extensions
Added value ranges for all new gene types with biologically-inspired defaults.

#### DiploidGenome.h Extensions
- Added **Phenotype** struct with all expressed traits
- Added **CreatureStats** struct for fitness calculation input
- Extended chromosome count from 4 to 9:
  - Chromosome 0: Physical traits
  - Chromosome 1: Color and display
  - Chromosome 2: Behavioral and mate preferences
  - Chromosome 3: Niche and tolerance
  - Chromosome 4: Creature type aptitudes (NEW)
  - Chromosome 5: Flying traits (NEW)
  - Chromosome 6: Aquatic traits (NEW)
  - Chromosome 7: Sensory traits (NEW)
  - Chromosome 8: Defense, communication, memory (NEW)

#### DiploidGenome.cpp Extensions
- **express()**: Genotype to phenotype conversion
- **determineCreatureType()**: C-15 fix - determines creature type from aptitude genes
- **calculateFitness(CreatureStats)**: C-16 fix - multi-component fitness with genetic quality modifiers

#### Creature.h/cpp Extensions
- **canMateWith()**: Basic compatibility checks
- **evaluateMateAttraction()**: Attraction scoring based on preferences
- **willMateWith()**: C-17 fix - probabilistic mate acceptance using preferences

### Issues Fixed

| Issue | Description | Solution |
|-------|-------------|----------|
| C-15 | CreatureType not determined from genotype | `determineCreatureType()` uses aptitude genes |
| C-16 | Simple fitness function | Multi-component fitness with survival, reproduction, resources, social factors |
| C-17 | MatePreferences defined but not used | `willMateWith()` evaluates attraction and choosiness |

### Testing Recommendations

1. Verify `determineCreatureType()` returns expected types for various aptitude combinations
2. Test `calculateFitness()` produces higher values for successful creatures
3. Verify `willMateWith()` respects size, ornament, and similarity preferences
4. Test that genetic distance affects mate acceptance
5. Verify chromosomes initialize correctly with all new gene types

---

*Document created: Phase 4 - Unified Genetics System*
*Issues addressed: C-13, C-14, C-15, C-16, C-17*
*Implementation completed: 2026-01-14*
