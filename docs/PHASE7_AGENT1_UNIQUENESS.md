# Phase 7 - Agent 1: Run-to-Run Uniqueness

## Overview

This document describes the planet seed system and run-to-run uniqueness features implemented to make each simulation run feel distinct, similar to No Man's Sky's planet variety. The system ensures that different seeds produce clearly different worlds while maintaining deterministic reproducibility.

## Files Modified/Created

### New Files
- `src/environment/PlanetSeed.h` - Planet seed system and theme profiles
- `src/environment/PlanetSeed.cpp` - Implementation of seed derivation and theme registry
- `docs/PHASE7_AGENT1_UNIQUENESS.md` - This documentation

### Modified Files
- `src/environment/ProceduralWorld.h` - Extended WorldGenConfig and GeneratedWorld
- `src/environment/ProceduralWorld.cpp` - Integrated seed-driven variation

---

## 1. Seed System and Sub-Seed Derivation

### PlanetSeed Structure

The `PlanetSeed` struct provides deterministic sub-seed derivation from a master seed using splitmix64 hashing:

```cpp
struct PlanetSeed {
    uint32_t masterSeed;      // Base seed for all derivations

    // Derived sub-seeds
    uint32_t paletteSeed;      // Theme colors, atmosphere, color grading
    uint32_t climateSeed;      // Temperature/humidity gradients, seasons
    uint32_t terrainSeed;      // Noise layers, elevation, mountains
    uint32_t lifeSeed;         // Creature spawning, behavior patterns
    uint32_t oceanSeed;        // Ocean coverage, currents, shoreline
    uint32_t biomeSeed;        // Biome distribution and mixing
    uint32_t vegetationSeed;   // Plant placement, tree variation
    uint32_t creatureSeed;     // Creature genetics, evolution parameters
    uint32_t weatherSeed;      // Weather patterns, events
    uint32_t archipelagoSeed;  // Island count, placement, connections

    std::string fingerprint;   // Human-readable code (e.g., "A7X-Q3M")
};
```

### Sub-Seed Derivation

Each sub-seed is derived using a unique offset constant:

| Sub-Seed | Offset | Purpose |
|----------|--------|---------|
| paletteSeed | 0x1A2B3C4D | Visual theme and colors |
| climateSeed | 0x5E6F7A8B | Temperature/moisture |
| terrainSeed | 0x9C0D1E2F | Heightmap generation |
| lifeSeed | 0x3A4B5C6D | Creature spawning |
| oceanSeed | 0x7E8F9A0B | Ocean parameters |
| biomeSeed | 0xC1D2E3F4 | Biome placement |
| vegetationSeed | 0x5A6B7C8D | Plant distribution |
| creatureSeed | 0x9E0F1A2B | Creature genetics |
| weatherSeed | 0x3C4D5E6F | Weather events |
| archipelagoSeed | 0x7A8B9C0D | Island layout |

### Helper Functions

```cpp
// Get further sub-seed for specific subsystems
static uint32_t getSubSeed(uint32_t baseSeed, uint32_t index);

// Convert seed to normalized float [0, 1]
static float seedToFloat(uint32_t seed);

// Get value in range
static float seedToRange(uint32_t seed, float min, float max);
static int seedToInt(uint32_t seed, int min, int max);
```

---

## 2. Theme Profiles Table

### Rarity Distribution
- **Common**: 40% total
- **Uncommon**: 35% total
- **Rare**: 20% total
- **Legendary**: 5% total

### Theme Profiles

| Name | Base Preset | Rarity | Weight | Key Characteristics |
|------|-------------|--------|--------|---------------------|
| Terra Prime | EARTH_LIKE | Common | 2.0 | Standard Earth colors, balanced biomes |
| Aqua Depths | OCEAN_WORLD | Common | 1.5 | Blue skies, high tropical/wetland |
| Verdant Haven | EARTH_LIKE | Common | 1.0 | Saturated greens, forest-heavy |
| Dune Expanse | DESERT_WORLD | Uncommon | 1.2 | Orange skies, low fog, high sun |
| Glacial Reach | FROZEN_WORLD | Uncommon | 1.0 | Pale skies, dense fog, cool tones |
| Infernal Forge | VOLCANIC_WORLD | Uncommon | 0.8 | Red/orange atmosphere, warm colors |
| Elder Realm | ANCIENT_WORLD | Uncommon | 0.8 | Muted, weathered appearance |
| Violet Nexus | ALIEN_PURPLE | Rare | 1.2 | Purple vegetation, high saturation |
| Crimson Horizon | ALIEN_RED | Rare | 1.0 | Red vegetation, warm tones |
| Venomous Mire | TOXIC_WORLD | Rare | 0.8 | Green atmosphere, dense fog |
| Luminous Abyss | BIOLUMINESCENT | Legendary | 1.5 | Dark atmosphere, glow effects |
| Prismatic Spires | CRYSTAL_WORLD | Legendary | 1.0 | Pink/purple, high clarity |

### Parameter Ranges Per Theme

Each theme defines ranges for seed-driven variation:

```cpp
struct ParameterRanges {
    // Sky
    float skyHueMin, skyHueMax;       // Hue range (0-360)
    float skySatMin, skySatMax;       // Saturation (0-1)
    float skyBrightMin, skyBrightMax; // Value/brightness (0-1)

    // Fog
    float fogDensityMin, fogDensityMax;
    float fogDistanceMin, fogDistanceMax;

    // Water
    float waterHueMin, waterHueMax;
    float waterClarityMin, waterClarityMax;

    // Sun
    float sunHueMin, sunHueMax;
    float sunIntensityMin, sunIntensityMax;

    // Biome
    float biomeSaturationMin, biomeSaturationMax;
    float warmthMin, warmthMax;  // -1 cool, +1 warm
};
```

---

## 3. Biome Palette Examples and Contrast Rules

### Contrast Budget System

The `ContrastBudget` struct enforces minimum color distances:

```cpp
struct ContrastBudget {
    float minBiomeColorDistance = 0.15f;   // Between biomes
    float minSkyGroundContrast = 0.20f;    // Sky vs terrain
    float minWaterLandContrast = 0.25f;    // Water vs coast
    float minVegetationContrast = 0.10f;   // Vegetation types
};
```

### Enforcement Algorithm

1. Calculate perceptual color distance using weighted RGB
2. If distance < minimum, push colors apart along their difference vector
3. Clamp results to valid RGB range [0, 1]

```cpp
static void enforceContrast(TerrainPalette& palette,
                            AtmosphereSettings& atm,
                            float strength);
```

### Biome Weight Modifiers Per Theme

Each theme can bias biome distribution:

| Theme | Desert | Forest | Tundra | Tropical | Wetland | Mountain | Volcanic |
|-------|--------|--------|--------|----------|---------|----------|----------|
| Terra Prime | 1.0 | 1.0 | 1.0 | 1.0 | 1.0 | 1.0 | 1.0 |
| Aqua Depths | 0.3 | 1.0 | 1.0 | 1.5 | 1.3 | 1.0 | 1.0 |
| Dune Expanse | 2.5 | 0.2 | 0.5 | 0.3 | 0.1 | 1.0 | 1.0 |
| Glacial Reach | 0.5 | 0.4 | 3.0 | 0.0 | 1.0 | 1.5 | 1.0 |
| Infernal Forge | 1.2 | 0.3 | 0.0 | 1.0 | 1.0 | 1.5 | 3.0 |

---

## 4. Terrain/Archipelago Parameter Ranges

### TerrainVariation (from terrainSeed)

| Parameter | Min | Max | Description |
|-----------|-----|-----|-------------|
| noiseFrequency | 0.5 | 2.0 | Base noise scale |
| noiseOctaves | 4 | 8 | Noise detail levels |
| ridgeBias | 0.0 | 1.0 | Sharp ridges vs smooth |
| valleyBias | 0.2 | 0.8 | Valley depth tendency |
| plateauChance | 0.0 | 0.4 | Flat area probability |
| erosionStrength | 0.3 | 1.0 | Hydraulic erosion |
| thermalStrength | 0.2 | 0.8 | Thermal erosion |

### OceanVariation (from oceanSeed)

| Parameter | Min | Max | Description |
|-----------|-----|-----|-------------|
| coverage | 0.3 | 0.7 | Ocean percentage |
| shorelineComplexity | 0.2 | 0.8 | Coast jaggedness |
| coralReefDensity | 0.0 | 0.5 | Reef features |
| depthVariation | 0.3 | 1.0 | Seafloor variety |
| currentStrength | 0.2 | 0.8 | Current intensity |

### ArchipelagoVariation (from archipelagoSeed)

| Parameter | Min | Max | Description |
|-----------|-----|-----|-------------|
| islandCount | 3 | 12 | Number of islands |
| sizeDispersion | 0.3 | 0.9 | Size variation |
| coastIrregularity | 0.2 | 0.7 | Coastline jaggedness |
| lagoonProbability | 0.0 | 0.4 | Internal lagoons |
| volcanoChance | 0.0 | 0.3 | Volcanic islands |
| connectionDensity | 0.2 | 0.8 | Island connectivity |

### ClimateVariation (from climateSeed)

| Parameter | Min | Max | Description |
|-----------|-----|-----|-------------|
| temperatureBase | -10.0 | 10.0 | Global temp offset (°C) |
| temperatureRange | 20.0 | 50.0 | Hot-cold variation |
| moistureBase | 0.3 | 0.7 | Global moisture |
| moistureRange | 0.3 | 0.7 | Wet-dry variation |
| latitudinalStrength | 0.5 | 1.0 | Latitude effect |
| altitudeStrength | 0.5 | 1.0 | Altitude effect |

---

## 5. Debug Output

The system logs generation details to console:

```
========== WORLD GENERATION COMPLETE ==========
  Seed: 12345678 [A7X-Q3M]
  Theme: Violet Nexus (Rare)
  Land: 45.3% | Water: 54.7%
  Rivers: 12 | Lakes: 5 | Caves: 8
  Climate: 18.5C avg, 35.0C range
  Biomes: Desert 5% | Forest 32% | Tundra 3% | Tropical 28%
  Generation Time: 245.3 ms
================================================
```

### Seed Fingerprint

Human-readable 6-character code for easy sharing:
- Format: `XXX-XXX` (e.g., "A7X-Q3M")
- Uses alphanumeric characters without ambiguous ones (no O/0, I/1)
- Derived deterministically from master seed

---

## 6. Integration Notes

### For Agent 8 (Shader Consumption)

The `PlanetTheme::getShaderConstants()` method returns all visual parameters needed for rendering:

```cpp
struct ShaderConstants {
    glm::vec4 skyZenithColor;
    glm::vec4 skyHorizonColor;
    glm::vec4 sunColor;
    glm::vec4 fogColor;
    glm::vec4 ambientColor;
    glm::vec4 waterDeepColor;
    glm::vec4 waterShallowColor;
    glm::vec4 colorFilter;
    glm::vec4 shadowColor;
    glm::vec4 highlightColor;
    float fogDensity;
    float fogStart;
    float sunIntensity;
    float contrast;
    float saturation;
    float brightness;
    float timeOfDay;
    float padding;
};
```

### For Agent 10 (UI Display)

New getters available on `ProceduralWorld`:

```cpp
// Get fingerprint for display
std::string getSeedFingerprint() const;

// Get current world details
const GeneratedWorld* getCurrentWorld() const;
```

`GeneratedWorld` now includes:
- `PlanetSeed planetSeed` - Full seed data
- `std::string themeName` - Theme profile name
- `ThemeRarity themeRarity` - Rarity tier
- `float desertCoverage, forestCoverage, ...` - Biome percentages
- `float averageTemperature, temperatureRange, averageMoisture` - Climate stats

---

## 7. Usage Example

```cpp
// Generate with specific seed
WorldGenConfig config;
config.seed = 12345678;
config.useWeightedThemeSelection = true;  // Use rarity-weighted themes

ProceduralWorld world;
GeneratedWorld result = world.generate(config);

// Access unique world properties
std::cout << "Seed: " << result.planetSeed.fingerprint << std::endl;
std::cout << "Theme: " << result.themeName << std::endl;
std::cout << "Forest: " << result.forestCoverage << "%" << std::endl;

// Same seed = identical world (deterministic)
ProceduralWorld world2;
config.seed = 12345678;
GeneratedWorld result2 = world2.generate(config);
// result2 will be identical to result
```

---

## 8. Validation

To verify uniqueness:

1. **Same seed = same world**: Generate twice with same seed, compare all parameters
2. **Different seeds = visible differences**: Generate with sequential seeds, verify:
   - Different theme profiles (over many runs)
   - Different biome distributions
   - Different terrain features
   - Different color palettes

3. **Sub-seed independence**: Changing one sub-seed should not affect unrelated systems
