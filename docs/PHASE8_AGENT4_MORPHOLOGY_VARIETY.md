# Phase 8: Morphology Diversity and Family Differentiation

## Overview

This document describes the Family Archetype System implemented in Phase 8 to dramatically increase visual diversity among creatures. The system ensures creatures look distinctly different across families while maintaining biological plausibility.

## Family Archetype System

### 8 Archetype Definitions

| Archetype | Description | Key Visual Traits | Inspiration |
|-----------|-------------|-------------------|-------------|
| **SEGMENTED** | Centipede/worm-like | Long body (2-4x aspect), 5-12 segments, 4-8 leg pairs, antennae | Centipedes, millipedes |
| **PLATED** | Armadillo/turtle-like | Compact body, heavy armor (60-100% coverage), thick legs | Armadillos, pangolins |
| **FINNED** | Fish/ray-like | Streamlined body (1.5-3x aspect), multiple fins, fan tail | Fish, rays, eels |
| **LONG_LIMBED** | Spider/crane-like | Small compact body, 3-6 pairs of very long thin legs (1.2-2x body length) | Spiders, crane flies |
| **RADIAL** | Starfish/jellyfish-like | Circular body, radial symmetry, 5-8 arms, tendrils | Starfish, jellyfish |
| **BURROWING** | Mole/wombat-like | Torpedo-shaped, small eyes, massive digging claws, thick limbs | Moles, wombats |
| **GLIDING** | Flying squirrel-like | Flat profile, membrane flaps between limbs, lightweight | Flying squirrels, sugar gliders |
| **SPINED** | Porcupine/hedgehog-like | Round/domed body, dense spine coverage (3-6 rows), defensive posture | Porcupines, hedgehogs |

### Archetype Selection Logic

Archetypes are selected deterministically based on `speciesId` and `planetSeed`:

```cpp
FamilyArchetype MorphologyBuilder::determineArchetype(uint32_t speciesId, uint32_t planetSeed) {
    uint32_t combined = speciesId ^ (planetSeed * 0x9E3779B9);  // Golden ratio hash

    // Weight distribution (percentages)
    // SEGMENTED: 12%, PLATED: 12%, FINNED: 15%, LONG_LIMBED: 12%
    // RADIAL: 10%, BURROWING: 13%, GLIDING: 11%, SPINED: 15%
}
```

This ensures:
- Consistent archetype for each species across saves
- Different planet seeds yield different archetype distributions
- No archetype is underrepresented (<5%) or overrepresented (>25%)

## Archetype Constraints

Each archetype defines specific gene ranges:

### Body Proportions
| Archetype | Body Aspect | Width | Height |
|-----------|-------------|-------|--------|
| SEGMENTED | 2.0 - 4.0 | 0.2 - 0.4 | 0.2 - 0.35 |
| PLATED | 0.8 - 1.5 | 0.5 - 0.8 | 0.4 - 0.7 |
| FINNED | 1.5 - 3.0 | 0.3 - 0.6 | 0.3 - 0.5 |
| LONG_LIMBED | 0.8 - 1.2 | 0.2 - 0.35 | 0.2 - 0.35 |
| RADIAL | 0.8 - 1.2 | 0.4 - 0.7 | 0.3 - 0.5 |
| BURROWING | 1.2 - 1.8 | 0.4 - 0.6 | 0.35 - 0.5 |
| GLIDING | 1.0 - 1.6 | 0.3 - 0.5 | 0.2 - 0.35 |
| SPINED | 0.9 - 1.5 | 0.4 - 0.6 | 0.4 - 0.6 |

### Limb Configuration
| Archetype | Leg Pairs | Leg Segments | Leg Length | Leg Thickness |
|-----------|-----------|--------------|------------|---------------|
| SEGMENTED | 4-8 | 2-3 | 0.3-0.5 | 0.05-0.1 |
| PLATED | 2-3 | 2-3 | 0.4-0.6 | 0.12-0.2 |
| FINNED | 0-2 | 2 | 0.2-0.4 | 0.05-0.1 |
| LONG_LIMBED | 3-6 | 3-5 | 1.2-2.0 | 0.03-0.06 |
| RADIAL | 0 | 0 | 0 | 0 |
| BURROWING | 2-3 | 2-3 | 0.3-0.5 | 0.15-0.25 |
| GLIDING | 2-3 | 3-4 | 0.7-1.0 | 0.04-0.08 |
| SPINED | 2-3 | 2-3 | 0.4-0.6 | 0.08-0.15 |

### Feature Probabilities
| Archetype | Armor | Spines | Fins | Crests | Antennae | Tentacles |
|-----------|-------|--------|------|--------|----------|-----------|
| SEGMENTED | 30% | 20% | 10% | 10% | 70% | 10% |
| PLATED | 95% | 30% | 5% | 20% | 10% | 0% |
| FINNED | 20% | 30% | 100% | 40% | 20% | 10% |
| LONG_LIMBED | 15% | 20% | 10% | 10% | 50% | 20% |
| RADIAL | 20% | 40% | 30% | 30% | 30% | 90% |
| BURROWING | 40% | 20% | 5% | 10% | 40% | 10% |
| GLIDING | 5% | 10% | 30% | 30% | 20% | 10% |
| SPINED | 30% | 100% | 10% | 40% | 10% | 0% |

## Geometry Modules

### New Archetype-Specific Builders

#### 1. Tendrils (RADIAL archetype)
```cpp
void buildTendrils(MetaballSystem& metaballs, const MorphologyGenes& genes,
                   const glm::vec3& bodyCenter, int tendrilCount, float tendrilLength);
```
- Hangs from underside of body
- 6-8 segments per tendril with taper
- Curling wave motion applied

#### 2. Membrane Flaps (GLIDING archetype)
```cpp
void buildMembraneFlaps(MetaballSystem& metaballs, const MorphologyGenes& genes,
                        const glm::vec3& bodyCenter);
```
- Stretched between front and back limbs
- Grid of thin metaballs (5x8)
- Curved droop at edges

#### 3. Radial Arms (RADIAL archetype)
```cpp
void buildRadialArms(MetaballSystem& metaballs, const MorphologyGenes& genes,
                     const glm::vec3& bodyCenter, int armCount);
```
- 5-8 arms distributed radially
- 5 segments per arm with taper
- Sucker bumps on underside

#### 4. Digging Claws (BURROWING archetype)
```cpp
void buildDiggingClaws(MetaballSystem& metaballs, const MorphologyGenes& genes,
                       const glm::vec3& limbEnd, float clawSize);
```
- 3 large curved claws per limb
- 4 segments with sharp taper
- Forward-facing for digging motion

#### 5. Articulated Plates (PLATED archetype)
```cpp
void buildArticulatedPlates(MetaballSystem& metaballs, const MorphologyGenes& genes,
                            const glm::vec3& bodyCenter);
```
- 4-12 overlapping rows
- Width varies (narrower at ends)
- Raised ridges on alternate plates

#### 6. Dense Spines (SPINED archetype)
```cpp
void buildDenseSpines(MetaballSystem& metaballs, const MorphologyGenes& genes,
                      const glm::vec3& bodyCenter);
```
- Covers dorsal surface (excludes belly)
- 3-6 rows x 15 density = 45-90 spines
- 3 metaballs per spine (base, mid, tip)

## Pattern Families

### New Phase 8 Patterns (5 additions)

| Pattern | Archetype Match | Description |
|---------|-----------------|-------------|
| **Segmented** | SEGMENTED | Horizontal segment bands with joint darkening, chitin sheen |
| **PlatedArmor** | PLATED | Overlapping plates with raised centers, brick-pattern offset |
| **RadialBurst** | RADIAL | Radial rays from center with concentric rings |
| **Reticulated** | LONG_LIMBED | Voronoi network/mesh pattern like giraffe |
| **Bioluminescent** | RADIAL (deep-sea) | Glowing photophores on dark base, pulsing veins |

### Pattern Assignment by Archetype

Each archetype has 4 preferred patterns:

| Archetype | Preferred Patterns |
|-----------|-------------------|
| SEGMENTED | Spots, Scales, Speckled, Bands |
| PLATED | Scales, Patches, Marbled, Camouflage |
| FINNED | Scales, Countershading, Stripes, Bands |
| LONG_LIMBED | Solid, Spots, Speckled, Tribal |
| RADIAL | Gradient, Rings, Eyespots, Rosettes |
| BURROWING | Solid, Camouflage, Mottled, Brindle |
| GLIDING | Stripes, Gradient, Countershading, Feathers |
| SPINED | Solid, Speckled, Brindle, Bands |

## Performance and LOD

### Vertex Budgets

| LOD Level | Max Vertices | Distance Range |
|-----------|--------------|----------------|
| FULL | 18,000 | < 10m |
| REDUCED | 8,000 | 10-30m |
| SIMPLIFIED | 2,000 | 30-100m |
| MINIMAL | 200 | > 100m |

### Archetype Complexity Estimates

| Archetype | Typical Metaballs | Est. Vertices (FULL) |
|-----------|-------------------|----------------------|
| SEGMENTED | 80-150 | 8,000-15,000 |
| PLATED | 100-180 | 10,000-17,000 |
| FINNED | 60-100 | 6,000-10,000 |
| LONG_LIMBED | 90-160 | 9,000-14,000 |
| RADIAL | 70-120 | 7,000-12,000 |
| BURROWING | 80-130 | 8,000-13,000 |
| GLIDING | 100-170 | 10,000-16,000 |
| SPINED | 120-200 | 12,000-18,000 |

### Debug Reporting

Two debug functions are available:

```cpp
// Generate detailed report for N creatures
void MorphologyBuilder::generateDebugReport(
    int creatureCount,
    uint32_t planetSeed,
    std::string& outReport);

// Validate archetype distribution
void MorphologyBuilder::validateArchetypeDistribution(
    int creatureCount,
    uint32_t planetSeed,
    std::string& outReport);
```

Sample output:
```
=== ARCHETYPE DISTRIBUTION VALIDATION ===

Sample Size: 100
Planet Seed: 12345

Archetype     Count   Percentage Distribution
------------------------------------------------------------
Segmented     12      12.0%      ######
Plated        11      11.0%      #####
Finned        16      16.0%      ########
Long-Limbed   13      13.0%      ######
Radial        9       9.0%       ####
Burrowing     14      14.0%      #######
Gliding       10      10.0%      #####
Spined        15      15.0%      #######

=== DISTRIBUTION QUALITY ===
Distribution is well-balanced across all archetypes.
```

## Integration Notes

### Applying Archetype to New Creatures

```cpp
// Determine archetype
FamilyArchetype archetype = MorphologyBuilder::determineArchetype(speciesId, planetSeed);

// Generate morphology with archetype constraints
MorphologyGenes genes;
genes.randomize();
MorphologyBuilder::applyArchetypeToMorphology(genes, archetype, speciesId);

// Build creature with archetype-specific features
MetaballSystem metaballs;
MorphologyBuilder::buildFromMorphology(metaballs, genes, creatureType, visualState);
MorphologyBuilder::buildArchetypeFeatures(metaballs, genes, archetype, bodyCenter);

// Get appropriate texture pattern
uint8_t patternIndex = MorphologyBuilder::getArchetypePreferredPattern(archetype, speciesId);
```

### Files Modified

- `src/graphics/procedural/MorphologyBuilder.h` - Added FamilyArchetype enum, ArchetypeConstraints struct, new methods
- `src/graphics/procedural/MorphologyBuilder.cpp` - Implemented archetype system, geometry modules, debug reporting
- `src/graphics/procedural/CreatureTextureGenerator.h` - Added 5 new PatternType entries
- `src/graphics/procedural/CreatureTextureGenerator.cpp` - Implemented 5 archetype-specific pattern generators

### Parallel Safety

This implementation only modifies files in `src/graphics/procedural/*`:
- MorphologyBuilder.*
- CreatureTextureGenerator.*

It does NOT modify:
- `src/entities/Genome.*` (genome traits)
- `src/entities/genetics/Species.*` (species system)
- `src/ui/*` (user interface)

## Validation Checklist

- [x] 8 archetypes defined with distinct visual profiles
- [x] Deterministic archetype selection per species + planet
- [x] Archetype-specific gene range modifiers
- [x] 6 new geometry module builders
- [x] 5 new archetype-specific texture patterns
- [x] Vertex budgets defined (FULL: 18k, REDUCED: 8k, SIMPLIFIED: 2k)
- [x] Debug reporting for archetype + vertex counts
- [x] Distribution validation function

## Future Enhancements

1. **Hybrid Archetypes**: Allow rare creatures to blend two archetypes (e.g., PLATED + FINNED = armored fish)
2. **Archetype Evolution**: Allow species to shift archetypes over many generations under strong selection pressure
3. **Environment-Biased Distribution**: Aquatic biomes favor FINNED/RADIAL, caves favor BURROWING/SEGMENTED
4. **Archetype-Specific Animations**: Different locomotion cycles per archetype family
