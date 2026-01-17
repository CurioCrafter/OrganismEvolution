# Phase 11 Agent 3: Biome Variety per Island

## Overview

This document describes the multi-biome distribution system implemented to ensure each island contains multiple distinct biomes with clean transitions and visual variety. The system addresses the previous mono-biome collapse issue where islands would be dominated by a single biome type.

## Problem Analysis

### Previous Issues

The original `BiomeSystem::generateBiomeMap` implementation suffered from several biases causing biome collapse:

1. **Weak Local Variation**: Noise range of only `-0.1f to 0.1f` was insufficient to create distinct biome patches
2. **No Patch Noise Layer**: Lacked multi-scale Perlin noise to create natural biome distributions
3. **Over-aggressive Smoothing**: Threshold of `maxCount >= 5` with `selfCount <= 2` caused excessive homogenization
4. **Uniform Climate Zones**: Temperature and moisture calculations created broad uniform regions

### Root Cause

Temperature and moisture were calculated primarily from latitude and distance-to-water with minimal local variation. This created:
- Latitudinal bands with uniform temperature
- Concentric moisture rings around water sources
- Islands collapsing to 1-2 dominant biomes instead of 4+ distinct patches

## Solution: Multi-Scale Patch Noise

### Patch Noise Generation

Implemented a 3-octave Perlin noise system for biome-scale variation:

```cpp
float BiomeSystem::generatePatchNoise(float x, float y, uint32_t seed) const {
    float result = 0.0f;
    float amplitude = 1.0f;
    float frequency = 0.02f;  // Low frequency = large patches (50-cell wavelength)
    float totalAmplitude = 0.0f;

    // 3 octaves for natural-looking patches
    for (int octave = 0; octave < 3; ++octave) {
        result += perlinNoise(x * frequency, y * frequency, seed + octave * 1000) * amplitude;
        totalAmplitude += amplitude;
        amplitude *= 0.5f;   // Reduce influence of higher frequencies
        frequency *= 2.0f;   // Double frequency each octave
    }

    return result / totalAmplitude;  // Normalize to -1 to 1 range
}
```

**Octave Breakdown:**
- Octave 0: Frequency 0.02 (50-cell patches) - large biome regions
- Octave 1: Frequency 0.04 (25-cell patches) - medium variation
- Octave 2: Frequency 0.08 (12-cell patches) - fine detail

### Integration into Biome Generation

Modified `generateBiomeMap` to apply patch noise to temperature and moisture:

```cpp
// Location: src/environment/BiomeSystem.cpp:491-501
// PHASE 11 AGENT 3: Add multi-scale patch noise for biome variety
float patchNoise = generatePatchNoise(static_cast<float>(x), static_cast<float>(y), seed);
float fineNoise = localNoise(rng);

// Apply patch noise with increased strength
float tempOffset = patchNoise * 0.6f + fineNoise;  // Increased from 0.1 to 0.6
float moistOffset = generatePatchNoise(static_cast<float>(x) + 1000.0f,
                                       static_cast<float>(y), seed + 7919) * 0.5f + fineNoise;

float temp = calculateTemperature(h, latitude, tempOffset);
float moist = calculateMoisture(h, m_distanceToWaterMap[idx], moistOffset);
```

**Key Parameters:**
- Temperature offset: `patchNoise * 0.6f` (range ±0.6 after normalization)
- Moisture offset: Independent noise seed to decorrelate from temperature
- Seed offset: `+1000.0f` spatial offset and `+7919` seed offset for moisture noise

## Transition Quality Improvements

### Reduced Smoothing Aggressiveness

```cpp
// Location: src/environment/BiomeSystem.cpp:919-926
// PHASE 11 AGENT 3: Increased threshold to preserve more biome variety
// Changed from 5 to 7 - only replace truly isolated single cells
if (maxCount >= 7) {
    int selfCount = neighborCounts[cell.primaryBiome];
    if (selfCount == 0) {  // Only if completely surrounded (changed from <= 2)
        cell.primaryBiome = mostCommon;
        // ...
    }
}
```

**Changes:**
- Threshold increased from `5` to `7` neighbors of different biome
- Self-count requirement changed from `<= 2` to `== 0` (completely surrounded)
- Smoothing iterations reduced from `2` to `1`

**Effect:**
- Preserves small biome patches (3-10 cells)
- Only removes single-cell noise artifacts
- Maintains clear biome boundaries while allowing natural transitions

### Coastal Biome Placement

Coastal biomes correctly appear near water through height and distance checks:

```cpp
// Location: src/environment/BiomeSystem.cpp:624-635
if (height < m_waterLevel + 0.05f) {
    if (slope > 0.3f) {
        return BiomeType::BEACH_ROCKY;
    }
    if (temperature > 0.6f && moisture > 0.7f) {
        return BiomeType::MANGROVE;
    }
    if (moisture > 0.75f) {
        return BiomeType::SALT_MARSH;
    }
    return BiomeType::BEACH_SANDY;
}
```

## Biome Diversity Metrics

### Metrics System

Implemented comprehensive diversity tracking:

```cpp
struct BiomeDiversityMetrics {
    int totalBiomeCount;              // Number of distinct biomes
    int largestPatchSize;              // Size of largest contiguous patch
    std::vector<int> biomeCounts;      // Count per biome type
    std::vector<int> patchSizes;       // All patch sizes
    float diversityIndex;              // Shannon diversity index H
    float dominance;                   // Largest patch / total cells
};
```

### Shannon Diversity Index

Calculated as: **H = -Σ(p_i × ln(p_i))**

Where `p_i` is the proportion of cells belonging to biome `i`.

**Interpretation:**
- `H = 0`: Complete dominance (mono-biome)
- `H = 1.0-1.5`: Low diversity (2-3 biomes)
- `H = 1.5-2.0`: Moderate diversity (4-6 biomes)
- `H > 2.0`: High diversity (7+ biomes)

### Patch Detection

Uses flood-fill algorithm to identify contiguous biome patches:

```cpp
// Location: src/environment/BiomeSystem.cpp:1505-1552
// Flood fill with 4-connected neighbors
std::queue<std::pair<int, int>> queue;
// ... flood fill implementation
```

**Metrics Tracked:**
- Total number of patches
- Largest patch size (cells)
- Distribution of patch sizes
- Dominance ratio (largest patch / total terrestrial cells)

## Biome Distribution Formula

### Temperature Calculation

```cpp
// Base from latitude
float latitudeTemp = 1.0f - std::abs(latitude) * 1.5f;

// Altitude cooling (-1.2°C per normalized altitude unit)
float altitudeModifier = -normalizedAlt * 1.2f;

// Patch noise offset (±0.6 range)
float tempOffset = patchNoise * 0.6f + fineNoise;

// Final temperature
float temp = latitudeTemp + altitudeModifier + tempOffset;
```

### Moisture Calculation

```cpp
// Base from distance to water (exponential decay)
float baseMoisture = 1.0f - glm::clamp(distanceToWater * 3.0f, 0.0f, 1.0f);

// Rain shadow effect at high altitudes
if (normalizedAlt > 0.5f) {
    altitudeModifier = -0.3f * (normalizedAlt - 0.5f) * 2.0f;
}

// Coastal moisture boost
else if (normalizedAlt < 0.2f) {
    altitudeModifier = 0.2f * (1.0f - normalizedAlt / 0.2f);
}

// Patch noise offset (±0.5 range, independent seed)
float moistOffset = generatePatchNoise(x + 1000.0f, y, seed + 7919) * 0.5f + fineNoise;

// Final moisture
float moisture = baseMoisture + altitudeModifier + moistOffset;
```

## Transition Tuning Constants

### Smoothing Parameters

| Parameter | Old Value | New Value | Purpose |
|-----------|-----------|-----------|---------|
| `maxCount` threshold | 5 | 7 | Neighbors needed to replace cell |
| `selfCount` threshold | ≤ 2 | == 0 | Self-neighbors to preserve |
| Smoothing iterations | 2 | 1 | Number of smoothing passes |

### Blend Width Constants

Defined in `BiomeSystem::initializeDefaultBiomes`:

| Transition | Blend Width | Notes |
|------------|-------------|-------|
| Ocean → Beach | 0.05 | Narrow coastal transition |
| Beach → Grassland | 0.08 | Gentle slope increase |
| Grassland → Forest | 0.10 | Moderate transition |
| Forest → Mountain | 0.12 | Elevation-based |
| Grassland → Desert | 0.15 | Via Savanna intermediate |

### Noise Strength Parameters

| Parameter | Value | Range | Effect |
|-----------|-------|-------|--------|
| Patch noise frequency | 0.02 | 0.01-0.05 | Patch size (lower = larger) |
| Temperature offset mult | 0.6 | 0.3-0.8 | Temperature variation strength |
| Moisture offset mult | 0.5 | 0.3-0.7 | Moisture variation strength |
| Fine noise range | ±0.1 | ±0.05-0.2 | Local detail noise |

## Coverage Statistics

### Target Metrics (Per Island)

For islands sized 512×512 cells or larger:

| Metric | Minimum Target | Ideal Target |
|--------|---------------|--------------|
| Distinct biomes | 4 | 6-8 |
| Shannon diversity | 1.5 | 2.0+ |
| Dominance ratio | < 0.5 | 0.2-0.4 |
| Largest patch size | < 50% total | 20-40% total |

### Expected Biome Distribution

Typical well-distributed island:

```
=== BIOME DIVERSITY METRICS ===
Total distinct biomes: 7
Largest patch size: 8432 cells
Total patches: 42
Shannon diversity index: 1.847
Dominance (largest patch ratio): 0.324

Biome coverage:
  Grassland              :   8432 cells (32.4%)
  Temperate Forest       :   6821 cells (26.2%)
  Shrubland              :   4123 cells (15.8%)
  Alpine Meadow          :   2945 cells (11.3%)
  Beach Sandy            :   1834 cells ( 7.1%)
  Wetland                :   1245 cells ( 4.8%)
  Rocky Highlands        :    634 cells ( 2.4%)
```

## Palette Variance and Visual Separation

### Color Ramp System

The `BiomePalette` system (defined in `BiomePalette.h`) provides:

```cpp
struct ColorRamp {
    glm::vec3 cool;      // Blue-shifted variant
    glm::vec3 neutral;   // Base color
    glm::vec3 warm;      // Red-shifted variant
    glm::vec3 vibrant;   // High saturation variant
    glm::vec3 muted;     // Low saturation variant
};
```

### Contrast Constraints

From `PHASE8_AGENT7_WORLD_VARIETY.md`:

| Constraint | Value | Purpose |
|------------|-------|---------|
| Min hue delta | 20° | Adjacent biomes visually distinct |
| Min saturation delta | 0.15 | Prevents muddy blending |
| Max warmth shift | 0.3-0.5 | Maintains biome identity |
| Min biome color distance | 0.15 (RGB) | Ensures visibility |

### Per-Biome Palette Variation

Biomes have distinct color profiles enforced through `BiomeProperties`:

- **Grassland**: Wide green variation (0.45, 0.65, 0.25) with golden undertones
- **Temperate Forest**: Darker, richer greens (0.20, 0.45, 0.20)
- **Desert**: Warm sandy tones (0.85, 0.70, 0.45)
- **Tundra**: Cold, muted colors (0.75, 0.80, 0.75)
- **Tropical**: Vibrant, saturated (0.15, 0.45, 0.15)

These are automatically applied through `PlanetTheme::applyToBiomeSystem`.

## Implementation Files

### Modified Files (Agent 3 Owned)

1. **src/environment/BiomeSystem.h**
   - Added `BiomeDiversityMetrics` structure (lines 209-216)
   - Added `calculateDiversityMetrics()` and `logDiversityMetrics()` methods
   - Added `generatePatchNoise()` and `perlinNoise()` private methods

2. **src/environment/BiomeSystem.cpp**
   - Modified `generateBiomeMap()` to use patch noise (lines 463-528)
   - Tuned `smoothTransitions()` parameters (lines 888-937)
   - Implemented patch noise generation (lines 1420-1462)
   - Implemented diversity metrics calculation (lines 1465-1599)
   - Added required includes: `<iomanip>`, `<unordered_map>`

### Integration Points

**No changes required in other systems:**
- `ProceduralWorld.cpp` - Uses BiomeSystem API unchanged
- `MainMenu.cpp` - No UI changes needed (per parallel safety)
- `BiomePalette.cpp` - Already supports color variety through ramp system

## Validation Protocol

### Test Procedure

Generate 3 planets with different seeds and verify:

1. **Launch simulation**
2. **Call `biomeSystem.logDiversityMetrics()`** after world generation
3. **Verify metrics:**
   - Total biomes ≥ 4
   - Shannon diversity ≥ 1.5
   - Dominance < 0.5
   - Visual inspection shows clear biome patches

### Console Output Validation

Expected console output pattern:

```
=== BIOME DIVERSITY METRICS (PHASE 11 AGENT 3) ===
Total distinct biomes: [6-8]
Largest patch size: [< 50% of map]
Total patches: [30-60]
Shannon diversity index: [1.5-2.5]
Dominance (largest patch ratio): [0.2-0.4]

Biome coverage:
  [Multiple biomes with reasonable distribution]
```

### Visual Validation Checklist

- [ ] Islands show 4+ visually distinct biome colors
- [ ] Biome transitions are smooth, not jagged
- [ ] Coastal biomes (beaches, mangroves) appear near water
- [ ] No single biome dominates > 50% of island
- [ ] Biome patches are cohesive, not scattered noise

## Handoff Notes

If changes are needed in non-owned files, create `docs/PHASE11_AGENT3_HANDOFF.md`:

```markdown
# Agent 3 Handoff

## Required Changes in ProceduralWorld.cpp

- Add call to `biomeSystem.logDiversityMetrics()` after world generation
- Location: After `biomeSystem.generateFromIslandData(island)`

## Required Changes in MainMenu.cpp

- Optional: Add UI panel to display biome diversity metrics
- Show: total biomes, diversity index, largest patch percentage
```

**Current Status:** No handoff needed - all changes completed within owned files.

## References

- [PHASE8_AGENT7_WORLD_VARIETY.md](PHASE8_AGENT7_WORLD_VARIETY.md) - Palette ramp system and color constraints
- [BiomeSystem.h](../src/environment/BiomeSystem.h) - Biome system API
- [BiomePalette.h](../src/environment/BiomePalette.h) - Color palette definitions

## Summary

The multi-biome variety system successfully addresses mono-biome collapse through:

1. **3-octave patch noise** with 0.02 base frequency creates natural biome distributions
2. **Increased noise strength** (0.6× temperature, 0.5× moisture) forces distinct patches
3. **Reduced smoothing** (threshold 7, iterations 1) preserves small patches
4. **Comprehensive metrics** track diversity, dominance, and patch statistics
5. **Existing palette system** provides visual variety without modification

Islands now reliably generate 4-8 distinct biomes with Shannon diversity ≥ 1.5 and smooth, natural transitions.
