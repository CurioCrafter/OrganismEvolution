# Procedural Island Generation System

## Overview

The procedural island generation system creates unique, visually distinct islands with diverse biomes and "alien planet" aesthetics. Each run produces a different island with varied terrain features, vegetation, and color schemes.

## Components

### 1. IslandGenerator (`src/environment/IslandGenerator.cpp/h`)
Generates island terrain with multiple shape algorithms:
- **Circular** - Classic round island
- **Archipelago** - Multiple connected islands
- **Crescent** - Moon-shaped island
- **Irregular** - Organic, noise-driven shape
- **Volcanic** - Central peak with crater
- **Atoll** - Ring-shaped with central lagoon
- **Continental** - Large landmass with varied coastline

Features:
- Coastal erosion simulation
- River carving using gradient descent
- Lake basin detection
- Cave entrance markers
- Underwater terrain for aquatic creatures

### 2. BiomeSystem (`src/environment/BiomeSystem.cpp/h`)
Manages 31 biome types with smooth transitions:

**Water Biomes:**
- Deep Ocean, Ocean, Shallow Water, Coral Reef, Kelp Forest

**Coastal Biomes:**
- Sandy Beach, Rocky Beach, Tidal Pool, Mangrove, Salt Marsh

**Lowland Biomes:**
- Grassland, Savanna, Tropical Rainforest, Temperate Forest, Swamp, Wetland

**Highland Biomes:**
- Shrubland, Boreal Forest, Alpine Meadow, Rocky Highlands, Mountain Forest

**Extreme Biomes:**
- Hot Desert, Cold Desert, Tundra, Glacier, Volcanic, Lava Field, Crater Lake

### 3. PlanetTheme (`src/environment/PlanetTheme.cpp/h`)
Controls visual aesthetics with 12 presets:

| Preset | Description |
|--------|-------------|
| EARTH_LIKE | Standard Earth colors |
| ALIEN_PURPLE | Purple vegetation, orange sky |
| ALIEN_RED | Red vegetation, yellow sky |
| ALIEN_BLUE | Blue vegetation, green sky |
| FROZEN_WORLD | Ice and snow dominant |
| DESERT_WORLD | Arid, orange/brown |
| OCEAN_WORLD | Tropical, water-dominant |
| VOLCANIC_WORLD | Lava, ash, dark rocks |
| BIOLUMINESCENT | Glowing vegetation |
| CRYSTAL_WORLD | Reflective surfaces |
| TOXIC_WORLD | Green/yellow toxic atmosphere |
| ANCIENT_WORLD | Weathered, mossy |

Features:
- Time-of-day interpolation (dawn/noon/dusk/night)
- Color grading/post-processing
- Shader constant generation

### 4. ProceduralWorld (`src/environment/ProceduralWorld.cpp/h`)
High-level manager that combines all systems.

### 5. TerrainBiome.hlsl (`shaders/hlsl/TerrainBiome.hlsl`)
GPU shader with:
- Biome-based texture blending
- PBR lighting
- Atmospheric fog
- Water effects with caustics and foam

## Integration Guide

### Basic Usage

```cpp
#include "environment/ProceduralWorld.h"

// Create the world generator
ProceduralWorld worldGen;

// Generate a random alien world
GeneratedWorld world = worldGen.generateAlienWorld();

// Or generate with specific settings
WorldGenConfig config;
config.islandShape = IslandShape::VOLCANIC;
config.themePreset = PlanetPreset::ALIEN_PURPLE;
config.seed = 12345;
config.generateRivers = true;
config.generateLakes = true;
GeneratedWorld world = worldGen.generate(config);

// Access the results
const auto& heightmap = world.islandData.heightmap;
const auto& rivers = world.islandData.rivers;
const auto& biomeSystem = world.biomeSystem;
const auto& theme = world.planetTheme;

// Get shader constants for GPU
auto shaderConstants = theme->getShaderConstants();
```

### Integration with main.cpp

Replace the terrain generation section (around line 4926) with:

```cpp
// Create procedural world generator
#include "environment/ProceduralWorld.h"

// In AppState struct, add:
std::unique_ptr<ProceduralWorld> proceduralWorld;
GeneratedWorld* currentWorld = nullptr;

// In initialization:
g_app.proceduralWorld = std::make_unique<ProceduralWorld>();

// Generate world (pick one method):
// 1. Random alien world
auto world = g_app.proceduralWorld->generateAlienWorld();

// 2. Specific preset
auto world = g_app.proceduralWorld->generatePreset(
    IslandShape::ARCHIPELAGO,
    PlanetPreset::OCEAN_WORLD,
    42  // seed
);

// 3. From description string
auto config = WorldGeneration::parseDescription("purple alien archipelago");
auto world = g_app.proceduralWorld->generate(config);

// Get the heightmap for terrain rendering
const auto& heightmap = g_app.proceduralWorld->getHeightmap();
const auto& biomeMap = g_app.proceduralWorld->getBiomeMapRGBA();

// Apply to existing terrain system
g_app.terrain = std::make_unique<Terrain>(256, 256, 2.0f);
g_app.proceduralWorld->applyToTerrain(g_app.terrain.get());

// Apply theme to renderers
g_app.proceduralWorld->applyThemeToRenderers();

// For creature spawning, use biome system
float herbivoreCapacity = world.biomeSystem->getHerbivoreCapacity(x, z);
float aquaticCapacity = world.biomeSystem->getAquaticCapacity(x, z);
```

### Shader Integration

Upload planet theme constants to GPU:

```cpp
// Get shader constants
auto constants = g_app.currentWorld->planetTheme->getShaderConstants();

// Upload to constant buffer (example with DX12)
void* mappedData;
constantBuffer->Map(0, nullptr, &mappedData);
memcpy(mappedData, &constants, sizeof(constants));
constantBuffer->Unmap(0, nullptr);
```

### VegetationManager Integration

The BiomeSystem provides vegetation density queries:

```cpp
// In VegetationManager::generate()
float treeDensity = biomeSystem->getTreeDensity(worldX, worldZ);
float grassDensity = biomeSystem->getGrassDensity(worldX, worldZ);

// Check if tree can be placed
float treeScale;
if (biomeSystem->canPlaceTree(worldX, worldZ, treeScale)) {
    placeTree(worldX, worldZ, treeScale);
}
```

### Creature Spawning Integration

Use biome capacities for creature placement:

```cpp
// For each spawn position
BiomeQuery query = biomeSystem->queryBiome(worldX, worldZ);

if (isAquaticBiome(query.biome)) {
    // Spawn aquatic creatures
    float capacity = biomeSystem->getAquaticCapacity(worldX, worldZ);
    // ...
} else {
    // Land creatures
    float herbivoreCapacity = biomeSystem->getHerbivoreCapacity(worldX, worldZ);
    float carnivoreCapacity = biomeSystem->getCarnivoreCapacity(worldX, worldZ);
    // ...
}
```

## Performance

- Generation time: < 2 seconds for 2048x2048 heightmap
- Memory: ~16MB for heightmap + ~4MB for biome map
- Supports resolutions from 256x256 to 2048x2048

## Curated Presets

For quick generation of interesting worlds:

```cpp
auto presets = WorldGeneration::getCuratedPresets();
// Returns:
// - Tropical Paradise
// - Volcanic Caldera
// - Mystic Archipelago
// - Frozen North
// - Desert Oasis
// - Bioluminescent Reef
// - Crystal Spires
// - Toxic Wasteland
// - Ancient Ruins
// - Red Alien World
// - Blue Twilight
// - Earth Prime

// Generate from preset
auto preset = WorldGeneration::getRandomPreset(seed);
WorldGenConfig config;
config.islandShape = preset.shape;
config.themePreset = preset.theme;
config.coastComplexity = preset.coastComplexity;
```

## Regeneration

Generate new world with same settings but new seed:

```cpp
auto newWorld = g_app.proceduralWorld->regenerate();
```

## Files Created

```
src/environment/
  IslandGenerator.h      - Island shape generation
  IslandGenerator.cpp    - Implementation (noise, shapes, erosion)
  BiomeSystem.h          - Biome types and transitions
  BiomeSystem.cpp        - Implementation (biome determination)
  PlanetTheme.h          - Alien aesthetics system
  PlanetTheme.cpp        - Implementation (12 presets)
  ProceduralWorld.h      - High-level manager
  ProceduralWorld.cpp    - Implementation

shaders/hlsl/
  TerrainBiome.hlsl      - Biome-based terrain shader
```
