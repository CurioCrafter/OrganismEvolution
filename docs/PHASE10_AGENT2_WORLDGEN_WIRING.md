# Phase 10 Agent 2: MainMenu → ProceduralWorld Wiring

**Owner:** Agent 2 - Environment Systems
**Status:** Implementation Complete
**Date:** 2026-01-16

---

## Overview

This document describes the wiring between MainMenu configuration and ProceduralWorld generation, ensuring that all user settings from the UI flow correctly into world generation.

---

## Config Field Mapping

### Direct Mappings (MainMenu → ProceduralWorld)

| MainMenu::WorldGenConfig Field | ProceduralWorld::WorldGenConfig Field | Type | Notes |
|-------------------------------|-------------------------------------|------|-------|
| `seed` | `seed` | `uint32_t` | Master seed (0 = random) |
| `useRandomSeed` | - | `bool` | Handled in translation: if true, pass 0 |
| `preset` | `themePreset` | `PlanetPreset` | Direct mapping |
| `isAlienWorld` | `randomizeTheme` | `bool` | If true, override preset selection |
| `worldSize` | - | `float` | **MISSING** - needs mapping to terrain scale |
| `oceanCoverage` | - | `float` | **MISSING** - needs mapping to water level |
| `regionCount` | `multiRegion.enabled` + island count | `int` → `MultiRegionConfig` | Needs conversion logic |
| `starType` | `starType` | `StarType` enum | **NEEDS CONVERSION** |

### Biome Weight Mappings

MainMenu provides individual biome weights that need to be applied to ProceduralWorld's biome system:

| MainMenu Field | ProceduralWorld Application | Notes |
|----------------|----------------------------|-------|
| `forestWeight` | BiomeSystem weights | Applied in `applyClimateVariation` |
| `grasslandWeight` | BiomeSystem weights | Applied in `applyClimateVariation` |
| `desertWeight` | BiomeSystem weights | Applied in `applyClimateVariation` |
| `tundraWeight` | BiomeSystem weights | Applied in `applyClimateVariation` |
| `wetlandWeight` | BiomeSystem weights | Applied in `applyClimateVariation` |
| `mountainWeight` | BiomeSystem weights | Applied in `applyClimateVariation` |
| `volcanicWeight` | BiomeSystem weights | Applied in `applyClimateVariation` |

### Climate Mappings

| MainMenu Field | ProceduralWorld Application | Notes |
|----------------|----------------------------|-------|
| `temperatureBias` | Climate zones temperature | 0=cold, 1=hot |
| `moistureBias` | Climate zones moisture | 0=dry, 1=wet |
| `seasonIntensity` | - | **NOT YET IMPLEMENTED** in ProceduralWorld |

---

## Missing Fields in ProceduralWorld::WorldGenConfig

The following fields need to be added to `ProceduralWorld::WorldGenConfig`:

```cpp
// World scale
float terrainScale = 500.0f;          // Physical size of world
float oceanCoverage = 0.3f;           // Water coverage (0-1)

// Biome weights (user-specified overrides)
struct BiomeWeights {
    float forestWeight = 1.0f;
    float grasslandWeight = 1.0f;
    float desertWeight = 1.0f;
    float tundraWeight = 1.0f;
    float wetlandWeight = 1.0f;
    float mountainWeight = 1.0f;
    float volcanicWeight = 1.0f;
} biomeWeights;

// Climate overrides
float temperatureBias = 0.5f;         // Global temp (0=cold, 1=hot)
float moistureBias = 0.5f;            // Global moisture (0=dry, 1=wet)
float seasonIntensity = 0.5f;         // Season variation (0=none, 1=extreme)

// Region/island count
int desiredRegionCount = 1;           // Number of separate landmasses
```

---

## Star Type Conversion

MainMenu uses a simple enum, ProceduralWorld uses a rich `StarType` struct. Conversion needed:

| MainMenu::StarType | ProceduralWorld Action |
|-------------------|------------------------|
| `YELLOW_DWARF` | `StarType::sunLike()` |
| `ORANGE_DWARF` | `StarType::fromSeed()` with K-class bias |
| `RED_DWARF` | `StarType::redDwarf()` |
| `BLUE_GIANT` | `StarType::blueGiant()` |
| `BINARY` | `StarType::binarySystem()` |

---

## Translation Function Signature

```cpp
// Namespace: ui (or standalone helper)
ProceduralWorld::WorldGenConfig translateConfig(
    const MainMenu::WorldGenConfig& menuConfig
);
```

### Translation Logic

```cpp
ProceduralWorld::WorldGenConfig translateConfig(const MainMenu::WorldGenConfig& menuConfig) {
    ProceduralWorld::WorldGenConfig pwConfig;

    // Seed
    pwConfig.seed = menuConfig.useRandomSeed ? 0 : menuConfig.seed;

    // Theme
    pwConfig.themePreset = menuConfig.preset;
    pwConfig.randomizeTheme = menuConfig.isAlienWorld;

    // Star type
    pwConfig.randomizeStarType = false;
    switch (menuConfig.starType) {
        case MainMenu::WorldGenConfig::StarType::YELLOW_DWARF:
            pwConfig.starType = StarType::sunLike();
            break;
        case MainMenu::WorldGenConfig::StarType::RED_DWARF:
            pwConfig.starType = StarType::redDwarf();
            break;
        case MainMenu::WorldGenConfig::StarType::BLUE_GIANT:
            pwConfig.starType = StarType::blueGiant();
            break;
        case MainMenu::WorldGenConfig::StarType::BINARY:
            pwConfig.starType = StarType::binarySystem();
            break;
        case MainMenu::WorldGenConfig::StarType::ORANGE_DWARF:
            // Use fromSeed with bias toward K-class
            uint32_t seed = menuConfig.useRandomSeed ? 0 : menuConfig.seed;
            pwConfig.starType = StarType::fromSeed(seed);
            pwConfig.starType.spectralClass = StarSpectralClass::K_ORANGE;
            break;
    }

    // World structure
    pwConfig.terrainScale = menuConfig.worldSize;
    pwConfig.oceanCoverage = menuConfig.oceanCoverage;

    // Island/region setup
    if (menuConfig.regionCount > 1) {
        pwConfig.islandShape = IslandShape::ARCHIPELAGO;
        pwConfig.desiredRegionCount = menuConfig.regionCount;
        pwConfig.multiRegion.enabled = true;
    } else {
        pwConfig.islandShape = IslandShape::IRREGULAR;
        pwConfig.desiredRegionCount = 1;
        pwConfig.multiRegion.enabled = false;
    }

    // Biome weights
    pwConfig.biomeWeights.forestWeight = menuConfig.forestWeight;
    pwConfig.biomeWeights.grasslandWeight = menuConfig.grasslandWeight;
    pwConfig.biomeWeights.desertWeight = menuConfig.desertWeight;
    pwConfig.biomeWeights.tundraWeight = menuConfig.tundraWeight;
    pwConfig.biomeWeights.wetlandWeight = menuConfig.wetlandWeight;
    pwConfig.biomeWeights.mountainWeight = menuConfig.mountainWeight;
    pwConfig.biomeWeights.volcanicWeight = menuConfig.volcanicWeight;

    // Climate
    pwConfig.temperatureBias = menuConfig.temperatureBias;
    pwConfig.moistureBias = menuConfig.moistureBias;
    pwConfig.seasonIntensity = menuConfig.seasonIntensity;

    return pwConfig;
}
```

---

## World Output Getters

ProceduralWorld needs to expose these getters for runtime use and UI display:

```cpp
// In ProceduralWorld class:
uint32_t getUsedSeed() const;                    // Final seed used (may be auto-generated)
std::string getSeedFingerprint() const;          // Human-readable seed ID
std::string getThemeName() const;                // "Earth-Like", "Alien Purple", etc.
float getGenerationTimeMs() const;               // How long generation took
BiomeDistribution getBiomeDistribution() const;  // % coverage per biome
float getTerrainScale() const;                   // Physical world size
float getWaterLevel() const;                     // Water coverage percentage
```

### BiomeDistribution Structure

```cpp
struct BiomeDistribution {
    float desertPercentage;
    float forestPercentage;
    float grasslandPercentage;
    float tundraPercentage;
    float wetlandPercentage;
    float mountainPercentage;
    float volcanicPercentage;
    float oceanPercentage;
};
```

---

## Handoff to Agent 1 (Main Integration)

### Function Signature for main.cpp

```cpp
// In main.cpp initialization:
mainMenu.setOnStartGame([&](
    const ui::WorldGenConfig& menuConfig,
    const ui::EvolutionStartPreset& evolutionPreset,
    bool godMode
) {
    // 1. Translate MainMenu config to ProceduralWorld config
    auto pwConfig = ui::translateConfig(menuConfig);

    // 2. Generate world
    auto world = proceduralWorld.generate(pwConfig);

    // 3. Apply to terrain system
    proceduralWorld.applyToTerrain(&terrain);

    // 4. Apply theme to renderers
    proceduralWorld.applyThemeToRenderers();

    // 5. Initialize evolution system with preset
    evolutionSystem.initialize(evolutionPreset, godMode);

    // 6. Start simulation
    simulationRunning = true;
});
```

### Required Objects in main.cpp

Agent 1 needs to ensure these objects exist:

```cpp
ProceduralWorld proceduralWorld;
Terrain terrain;
EvolutionSystem evolutionSystem;
bool simulationRunning = false;
```

### Initialization Order

1. MainMenu renders and collects user input
2. User clicks "Generate Planet & Start"
3. MainMenu calls `m_onStartGame(menuConfig, evolutionPreset, godMode)`
4. main.cpp callback translates config
5. ProceduralWorld generates world
6. Terrain is rebuilt with new heightmap
7. Shaders/renderers receive new theme constants
8. Evolution system initializes with preset
9. Simulation begins

---

## Validation Checklist

### Fixed Seed Determinism

- [ ] Generate world with seed `12345` twice
- [ ] Compare heightmaps byte-by-byte
- [ ] Compare biome distributions (should be identical)
- [ ] Compare theme colors (should be identical)

### Random Seed Variety

- [ ] Generate 3 worlds with random seeds
- [ ] Verify different terrain shapes
- [ ] Verify different theme colors
- [ ] Verify different biome distributions

### Config Flow Test

- [ ] Set MainMenu sliders to specific values
- [ ] Verify ProceduralWorld receives correct values
- [ ] Verify terrain reflects settings (world size, ocean coverage)
- [ ] Verify biomes match weight settings

### UI Display Test

- [ ] After generation, verify seed is displayed in UI
- [ ] Verify theme name is shown
- [ ] Verify generation time is logged
- [ ] Verify biome distribution stats are accurate

---

## Implementation Notes

### File Locations

- **MainMenu config translation**: `src/ui/MainMenu.cpp` (add helper function)
- **ProceduralWorld config updates**: `src/environment/ProceduralWorld.h` and `.cpp`
- **Main integration**: `src/main.cpp` (Agent 1 territory)

### Build Dependencies

No new dependencies required. All changes use existing systems.

### Thread Safety

All world generation happens on main thread before simulation starts. No threading concerns.

---

## Logging Format

ProceduralWorld should log generation details:

```
========== WORLD GENERATION COMPLETE ==========
  Seed: 3847562 [A8F2]
  Theme: Alien Purple (Rare)
  Vegetation: Lush (density: 1.4)
  Land: 42.3% | Water: 57.7%
  Rivers: 15 | Lakes: 8 | Caves: 23
  Climate: 18.5C avg, 25.0C range
  Biomes: Desert 12% | Forest 35% | Tundra 8% | Tropical 20%
  Palette: warmth=0.15, sat=1.12, fog=0.032
  Generation Time: 234.5 ms
================================================
```

This log is already implemented in `ProceduralWorld::logWorldGeneration()` at line 750.

---

## Implementation Status: COMPLETE ✓

### Completed Items

- [x] Added missing fields to `ProceduralWorld::WorldGenConfig`
  - `terrainScale` (world size)
  - `oceanCoverage` (water level)
  - `BiomeWeights` struct with all biome weight fields
  - `temperatureBias`, `moistureBias`, `seasonIntensity` (climate)
  - `desiredRegionCount` (island/region count)

- [x] Implemented `ui::translateToProceduralWorldConfig()` helper
  - Full seed translation
  - Star type conversion (all 5 types)
  - Biome weight mapping
  - Climate override mapping
  - Region/archipelago configuration
  - Location: [src/ui/MainMenu.cpp:851](../src/ui/MainMenu.cpp#L851)

- [x] Updated `ProceduralWorld::generate()` to use new fields
  - Ocean coverage now affects water level
  - Terrain scale applied to BiomeSystem
  - Biome weights applied via `applyBiomeWeights()`
  - Climate parameters influence zone generation
  - Location: [src/environment/ProceduralWorld.cpp:49](../src/environment/ProceduralWorld.cpp#L49)

- [x] Added world output getters
  - `getUsedSeed()`, `getThemeName()`, `getGenerationTimeMs()`
  - `getTerrainScale()`, `getWaterLevel()`
  - `getBiomeDistribution()` with percentages
  - Location: [src/environment/ProceduralWorld.h:358](../src/environment/ProceduralWorld.h#L358)

- [x] Logging already implemented
  - Complete generation logging at line 750 of ProceduralWorld.cpp
  - Includes seed, theme, generation time, biome distribution

### Agent 1 Action Required

**ONLY ONE STEP REMAINING:**

Create the `m_onStartGame` callback in `main.cpp` following this pattern:

```cpp
#include "ui/MainMenu.h"
#include "environment/ProceduralWorld.h"
// ... other includes

// In main() or initialization:
mainMenu.setOnStartGame([&](
    const ui::WorldGenConfig& menuConfig,
    const ui::EvolutionStartPreset& evolutionPreset,
    bool godMode
) {
    // 1. Translate config
    auto pwConfig = ui::translateToProceduralWorldConfig(menuConfig);

    // 2. Generate world
    auto world = proceduralWorld.generate(pwConfig);

    // 3. Apply to systems
    proceduralWorld.applyToTerrain(&terrain);
    proceduralWorld.applyThemeToRenderers();

    // 4. Initialize evolution (when ready)
    // evolutionSystem.initialize(evolutionPreset, godMode);

    // 5. Start simulation
    simulationRunning = true;
});
```

### Testing Commands

Once Agent 1 completes the integration, test with:

1. **Fixed seed test**: Set seed to `12345`, generate twice, verify identical outputs
2. **Random seed test**: Generate 3 worlds with random seeds, verify variety
3. **Biome weight test**: Increase forest weight to 2.0, verify more forests
4. **Ocean coverage test**: Set to 0.7, verify mostly water
5. **Region count test**: Set to 5, verify archipelago with multiple islands

### File Summary

**Modified Files:**
- [src/environment/ProceduralWorld.h](../src/environment/ProceduralWorld.h) - Added BiomeWeights, new config fields, getters
- [src/environment/ProceduralWorld.cpp](../src/environment/ProceduralWorld.cpp) - Updated generate(), added applyBiomeWeights()
- [src/ui/MainMenu.h](../src/ui/MainMenu.h) - Added translateToProceduralWorldConfig() declaration
- [src/ui/MainMenu.cpp](../src/ui/MainMenu.cpp) - Implemented translation function

**Created Files:**
- [docs/PHASE10_AGENT2_WORLDGEN_WIRING.md](./PHASE10_AGENT2_WORLDGEN_WIRING.md) - This document

All code is ready for main.cpp integration.
