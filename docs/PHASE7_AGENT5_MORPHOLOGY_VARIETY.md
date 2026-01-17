# Phase 7 Agent 5: Morphology Variety and Evolution Diversity

## Overview

This document describes the expanded genetic and morphological diversity system implemented in Phase 7, designed to make creatures look and feel unique across all creature types including aquatic and small creatures.

## New Genes and Mutation Ranges

### Body Structure Genes

| Gene | Range | Description | Mutation Rate |
|------|-------|-------------|---------------|
| `segmentCount` | 1-8 | Number of body segments | Standard (2% macro-mutation) |
| `bodyAspect` | 0.3-3.0 | Length/width ratio (compact vs elongated) | Heavy-tailed distribution |
| `bodyTaper` | 0.5-1.5 | Front-to-back body taper | Standard |

### Fin Configuration (Aquatic)

| Gene | Range | Description | Mutation Rate |
|------|-------|-------------|---------------|
| `dorsalFinCount` | 0-3 | Number of dorsal fins | 30% of standard |
| `pectoralFinCount` | 0-4 | Pectoral fin pairs | 30% of standard |
| `ventralFinCount` | 0-2 | Ventral fins | 30% of standard |
| `finAspect` | 0.3-3.0 | Fin shape (rounded vs swept) | Heavy-tailed |
| `finRayCount` | 3-12 | Ray density for ray-finned appearance | Standard |

### Crests and Dorsal Features

| Gene | Range | Description | Mutation Rate |
|------|-------|-------------|---------------|
| `crestHeight` | 0.0-0.8 | Dorsal crest/ridge height | Heavy-tailed |
| `crestExtent` | 0.0-1.0 | Body coverage of crest | Standard |
| `crestType` | 0-4 | Type: none/ridge/sail/frill/spiny | 15% of standard |

### Horns and Antennae

| Gene | Range | Description | Mutation Rate |
|------|-------|-------------|---------------|
| `hornCount` | 0-6 | Total number of horns | 20% (2% macro) |
| `hornLength` | 0.1-1.5 | Relative to head size | Heavy-tailed |
| `hornCurvature` | -1.0-1.0 | Forward/backward curve | Standard |
| `hornType` | 0-3 | Straight/curved/spiral/branched | 10% of standard |
| `antennaeCount` | 0-4 | Sensory antennae | 20% of standard |
| `antennaeLength` | 0.2-2.0 | Relative to body length | Heavy-tailed |

### Tail Variants

| Gene | Range | Description | Mutation Rate |
|------|-------|-------------|---------------|
| `tailVariant` | 0-6 | Type: standard/club/fan/whip/fork/prehensile/spiked | 15% (2% macro) |
| `tailFinHeight` | 0.0-0.5 | For fan-type tails | Standard |
| `tailBulbSize` | 0.0-0.4 | For club-type tails | Standard |

### Mouth and Feeding

| Gene | Range | Description | Mutation Rate |
|------|-------|-------------|---------------|
| `jawType` | 0-4 | Standard/underslung/protruding/beak/filter | 10% of standard |
| `jawProtrusion` | -0.3-0.5 | Jaw extension/recession | Standard |
| `barbels` | 0.0-1.0 | Whisker-like sensory organs | Standard |

### Limb Variation

| Gene | Range | Description | Mutation Rate |
|------|-------|-------------|---------------|
| `limbSegments` | 2-5 | Segments per limb | 20% of standard |
| `limbTaper` | 0.3-1.0 | Thickness reduction along limb | Standard |
| `footSpread` | 0.3-2.0 | Webbing/toe spread | Heavy-tailed |
| `hasClaws` | bool | Presence of claws | 10% toggle |
| `clawLength` | 0.0-0.5 | Relative to foot size | Standard |

### Spikes and Protrusions

| Gene | Range | Description | Mutation Rate |
|------|-------|-------------|---------------|
| `spikeRows` | 0-4 | Rows of spikes along body | 20% of standard |
| `spikeLength` | 0.0-0.6 | Relative to body width | Heavy-tailed |
| `spikeDensity` | 0.1-1.0 | Spikes per unit area | Standard |

### Shell and Armor

| Gene | Range | Description | Mutation Rate |
|------|-------|-------------|---------------|
| `shellCoverage` | 0.0-1.0 | Body coverage by armor | Heavy-tailed |
| `shellSegmentation` | 0.0-1.0 | Articulation of shell | Standard |
| `shellTexture` | 0-3 | Smooth/ridged/bumpy/plated | 15% of standard |

### Frills and Displays

| Gene | Range | Description | Mutation Rate |
|------|-------|-------------|---------------|
| `hasNeckFrill` | bool | Frill around neck/head | 10% toggle |
| `frillSize` | 0.1-1.5 | Relative to head size | Heavy-tailed |
| `hasBodyFrills` | bool | Frills along body sides | 10% toggle |
| `displayFeatherSize` | 0.0-1.0 | Display feathers/fins | Standard |

### Eye Diversity

| Gene | Range | Description | Mutation Rate |
|------|-------|-------------|---------------|
| `eyeArrangement` | 0-4 | Paired/forward/stalked/compound/wide-set | 10% of standard |
| `eyeProtrusion` | 0.0-0.5 | How much eyes bulge | Standard |
| `hasEyeSpots` | bool | False eye patterns | 5% toggle |
| `eyeSpotCount` | 0-8 | Number of eye spots | 20% of standard |

## Mutation Distributions

### Standard Mutation
Normal Gaussian-like distribution around current value.

### Heavy-Tailed Distribution
2% chance of macro-mutation (jump to random value in range), plus 1.5% chance of extreme trait value (push toward min or max). This creates rare but dramatic variations:

```cpp
if (Random::chance(MACRO_MUTATION_CHANCE)) {
    // Macro-mutation: jump to new random value
    return Random::range(min, max);
} else if (Random::chance(EXTREME_TRAIT_CHANCE)) {
    // Extreme trait: push toward min or max
    float direction = Random::chance(0.5f) ? 1.0f : -1.0f;
    return value + direction * (max - min) * 0.4f;
}
```

## Mapping: Genes to Features

### Genome -> MorphologyGenes

The `MorphologyBuilder::genomeToMorphology()` function maps genome traits to morphology:

| Genome Trait | MorphologyGenes Field |
|--------------|----------------------|
| `segmentCount` | `segmentCount` |
| `bodyAspect` | `bodyAspect` |
| `dorsalFinCount` | `dorsalFinCount` |
| `crestHeight` + `crestType` | `crestType`, `crestHeight`, `crestExtent` |
| `hornCount` + `hornType` | `hornCount`, `hornLength`, `hornCurvature`, `hornType` |
| `tailVariant` | `tailType` |

### MorphologyGenes -> Metaballs

The `MorphologyBuilder::buildFromMorphology()` function converts genes to metaballs:

1. **Body Torso**: `segmentCount` segments with `segmentTaper` tapering
2. **Head**: Scaled by `headSize`, with snout based on `jawProtrusion`
3. **Limbs**: `legPairs` legs with `limbSegments` segments each
4. **Features**: Built based on `primaryFeature`/`secondaryFeature`
5. **Extended Features**: Crests, horns, spines, frills, etc.

### Metaballs -> Mesh

Marching Cubes algorithm converts metaballs to triangle mesh:
- Resolution 24 (default) produces ~120 vertices per metaball
- LOD reduces detail at distance

## Example Silhouettes

### Standard Land Creature (4-legged)
```
     /\      <- crest (if crestType > 0)
    /  \
   /head\___________________
  /     |      body        |
 |  o   |==================|    <- spines (if spikeRows > 0)
  \     |__________________|
   \   /|    |    |    |  \
    \_/ |leg |leg |leg |   tail (variant based on tailVariant)
        |    |    |    |
```

### Aquatic Creature (fish-like)
```
        _____/\___________
   /\  /     ||  dorsal   \
  /  \/      ||  fin(s)    \____
 |   head    ||   body     |tail|  <- caudal fin
  \  /\      ||            /~~~~
   \/  \_____|/___________/
       pectoral fins
```

### Heavily Armored Creature
```
    [===]   <- shell plates
   /     \
  |[=====]|  <- segmented armor
  |[=====]|
  |[=====]|
   \_____/
   /\  /\
  legs with claws
```

### Display Creature (frill + horns)
```
        /\  /\   <- horns (hornCount=2)
       /  \/  \
      |  head  |
     /|~~~~~~~~|\  <- neck frill
    / |        | \
   |  |  body  |  |
   |  |________|  |
      |  |  |  |
```

## New Texture Patterns (Phase 7)

8 new pattern families added:

1. **Marbled** - Organic vein patterns like marble stone
2. **Mottled** - Irregular blotches at multiple scales
3. **Rosettes** - Jaguar-style spots with dark outlines
4. **Lightning** - Branching patterns like lightning bolts
5. **Countershading** - Natural camouflage (dark top, light bottom)
6. **Eyespots** - Intimidating false eye patterns
7. **Tribal** - Bold geometric patterns
8. **Brindle** - Subtle diagonal streaks

Total patterns: 19 (up from 11)

## Performance Notes

### LOD System

Four LOD levels based on camera distance:

| LOD Level | Distance | Features Rendered |
|-----------|----------|-------------------|
| FULL | < 10m | All features |
| REDUCED | 10-30m | Skip small features, reduce segments |
| SIMPLIFIED | 30-100m | Basic silhouette only |
| MINIMAL | > 100m | Sphere approximation |

### Vertex Count Estimates

| Creature Type | FULL LOD | REDUCED | SIMPLIFIED | MINIMAL |
|--------------|----------|---------|------------|---------|
| Simple (4 legs) | ~2,400 | ~1,700 | ~960 | ~100 |
| Complex (features) | ~4,200 | ~2,900 | ~1,200 | ~100 |
| Aquatic (fins) | ~3,000 | ~2,100 | ~1,000 | ~100 |

### Validation Utility

Use `MorphologyBuilder::validateRandomCreatures()` to test performance:

```cpp
std::vector<MorphologyStats> stats;
MorphologyBuilder::validateRandomCreatures(20, stats);

for (const auto& s : stats) {
    printf("Verts: %d, Metaballs: %d, Features: %d, Radius: %.2f\n",
           s.vertexCount, s.metaballCount, s.featureCount, s.boundingRadius);
}
```

### Cache Considerations

- Texture generation is deterministic (seeded by speciesId)
- Metaball configurations can be cached per species
- LOD meshes can be pre-generated for common creatures

## Files Modified

### Core Genome System
- `src/entities/Genome.h` - Added 40+ new morphology genes
- `src/entities/Genome.cpp` - Crossover, mutation, and initialization

### Morphology System
- `src/physics/Morphology.h` - New enums and MorphologyGenes fields
- `src/graphics/procedural/MorphologyBuilder.h` - New builder methods
- `src/graphics/procedural/MorphologyBuilder.cpp` - Feature implementations

### Texture System
- `src/graphics/procedural/CreatureTextureGenerator.h` - 8 new pattern types
- `src/graphics/procedural/CreatureTextureGenerator.cpp` - Pattern implementations

## Integration Notes

### For Agent 3 (Metamorphosis)
Amphibious traits use the new `tailVariant` and `limbSegments` genes. Handoff note: The `tailType` enum in Morphology.h includes `PREHENSILE` for amphibious creatures.

### For Agent 8 (Shaders)
New patterns are generated CPU-side. Texture upload uses existing `TextureUploader` class. No shader changes required.

## Testing Recommendations

1. Run validation with 20+ random creatures
2. Check vertex counts stay under 5,000 at FULL LOD
3. Verify LOD transitions are smooth
4. Test extreme gene combinations (max horns + max spines + armor)
5. Verify crossover produces viable offspring
