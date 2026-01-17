# Biome System Design Document

## Overview

This document details the enhanced biome system for OrganismEvolution, implementing climate-based biome selection following the Whittaker diagram approach with additional factors for realistic terrain variety.

---

## 1. Biome Types

### 1.1 Water Biomes
| Biome | Description | Height Range | Temperature | Moisture |
|-------|-------------|--------------|-------------|----------|
| DEEP_OCEAN | Open water, dark blue | < 0.005 | Any | Any |
| SHALLOW_WATER | Coastal water, turquoise | 0.005 - 0.012 | Any | Any |
| BEACH | Sandy shore | 0.012 - 0.03 | Any | Any |

### 1.2 Tropical Biomes
| Biome | Description | Temperature | Moisture |
|-------|-------------|-------------|----------|
| TROPICAL_RAINFOREST | Dense canopy, high biodiversity | > 0.85 | > 0.7 |
| TROPICAL_SEASONAL | Monsoon forest, wet/dry seasons | > 0.65 | 0.4 - 0.7 |

### 1.3 Temperate Biomes
| Biome | Description | Temperature | Moisture |
|-------|-------------|-------------|----------|
| TEMPERATE_FOREST | Deciduous trees, four seasons | 0.35 - 0.65 | > 0.4 |
| TEMPERATE_GRASSLAND | Prairies, meadows | 0.35 - 0.65 | 0.2 - 0.4 |

### 1.4 Cold Biomes
| Biome | Description | Temperature | Moisture |
|-------|-------------|-------------|----------|
| BOREAL_FOREST | Coniferous taiga | 0.15 - 0.35 | > 0.5 |
| TUNDRA | Permafrost, lichens | < 0.35 | 0.2 - 0.5 |
| ICE | Permanent ice/snow | < 0.15 | > 0.3 |

### 1.5 Arid Biomes
| Biome | Description | Temperature | Moisture |
|-------|-------------|-------------|----------|
| DESERT_HOT | Sandy dunes, cacti | > 0.5 | < 0.2 |
| DESERT_COLD | Rocky desert, sparse | 0.2 - 0.5 | < 0.2 |
| SAVANNA | Grassland with scattered trees | > 0.6 | 0.2 - 0.4 |

### 1.6 Wetland Biomes
| Biome | Description | Special Conditions |
|-------|-------------|-------------------|
| SWAMP | Waterlogged, humid | Near water + low elevation + high moisture |

### 1.7 Mountain Biomes
| Biome | Description | Elevation | Temperature |
|-------|-------------|-----------|-------------|
| MOUNTAIN_MEADOW | Alpine flowers | > 0.65 | > 0.35 |
| MOUNTAIN_ROCK | Exposed rock | > 0.7 | 0.2 - 0.35 |
| MOUNTAIN_SNOW | Snow cap | > 0.85 | < 0.2 |

---

## 2. Climate Calculation

### 2.1 Temperature Model

```cpp
float calculateBaseTemperature(float elevation, float latitude) {
    // Lapse rate: ~6.5°C per 1000m
    float elevationEffect = elevation * 0.6f;  // Max 0.6 cooling

    // Latitude effect: poles are colder
    float latitudeEffect = abs(latitude) * 0.5f;

    // Base at equator, sea level = 0.85 (hot)
    return 0.85f - elevationEffect - latitudeEffect;
}
```

### 2.2 Moisture Model

Moisture is determined by:
1. **Distance to water** - Closer = more moisture
2. **Prevailing wind** - Windward slopes get rain
3. **Orographic effects** - Rain shadow behind mountains

```cpp
void simulateMoisture() {
    // Propagate moisture from water sources
    // Following prevailing wind direction
    // Reduce at mountain barriers (rain shadow)
}
```

### 2.3 Slope Calculation

```cpp
float calculateSlope(float x, float z) {
    float eps = 1.0f;
    float h0 = terrain->getHeight(x, z);
    float hx = terrain->getHeight(x + eps, z);
    float hz = terrain->getHeight(x, z + eps);

    float dx = (hx - h0) / eps;
    float dz = (hz - h0) / eps;

    return sqrt(dx * dx + dz * dz);  // Gradient magnitude
}
```

---

## 3. Biome Transitions

### 3.1 Blend System

Smooth transitions between biomes using noise-modulated boundaries:

```cpp
struct BiomeBlend {
    BiomeType primary;
    BiomeType secondary;
    float blendFactor;   // 0-1
    float noiseOffset;   // Prevents straight lines
};

BiomeBlend calculateBiomeBlend(const ClimateData& climate) {
    // Find biome boundary distance
    // Add noise perturbation
    // Return blend information
}
```

### 3.2 Transition Colors

In shader, blend between biome colors:
```hlsl
float3 terrainColor = lerp(
    getBiomeColor(blend.primary),
    getBiomeColor(blend.secondary),
    blend.blendFactor + noise * 0.1
);
```

---

## 4. Vegetation Density

### 4.1 Per-Biome Vegetation

```cpp
VegetationDensity VegetationDensity::forBiome(BiomeType biome) {
    VegetationDensity density = {0, 0, 0, 0, 0, 0};

    switch (biome) {
        case TROPICAL_RAINFOREST:
            density.treeDensity = 0.9f;
            density.shrubDensity = 0.8f;
            density.fernDensity = 0.7f;
            break;

        case TEMPERATE_GRASSLAND:
            density.grassDensity = 0.9f;
            density.flowerDensity = 0.5f;
            density.treeDensity = 0.05f;
            break;

        case DESERT_HOT:
            density.cactusDensity = 0.15f;
            break;
        // ...
    }
    return density;
}
```

### 4.2 Seasonal Modifiers

Vegetation density changes with seasons:
- **Spring**: Growing (0.7 → 1.0)
- **Summer**: Full (1.0)
- **Fall**: Declining (1.0 → 0.7)
- **Winter**: Dormant (0.5)

---

## 5. Biome Colors

### 5.1 Primary Colors

```cpp
glm::vec3 ClimateData::getPrimaryColor() const {
    switch (getBiome()) {
        case DEEP_OCEAN:          return {0.02, 0.08, 0.18};
        case TROPICAL_RAINFOREST: return {0.15, 0.45, 0.12};
        case TEMPERATE_FOREST:    return {0.18, 0.42, 0.15};
        case TEMPERATE_GRASSLAND: return {0.42, 0.58, 0.25};
        case DESERT_HOT:          return {0.88, 0.75, 0.48};
        case TUNDRA:              return {0.55, 0.52, 0.42};
        case MOUNTAIN_SNOW:       return {0.95, 0.95, 0.98};
        // ...
    }
}
```

### 5.2 Seasonal Color Variations

Colors shift with seasons:
- **Spring**: Fresh greens, flowers
- **Summer**: Deep greens
- **Fall**: Yellows, oranges, browns
- **Winter**: Browns, grays, whites

---

## 6. Shader Integration

### 6.1 Current Terrain.hlsl Biome Thresholds

```hlsl
static const float WATER_THRESH = 0.012;
static const float WET_SAND_THRESH = 0.025;
static const float BEACH_THRESH = 0.05;
static const float GRASS_THRESH = 0.35;
static const float FOREST_THRESH = 0.67;
static const float MOUNTAIN_THRESH = 0.85;
```

### 6.2 Enhanced Climate-Based Selection

Future shader enhancement could include:
```hlsl
cbuffer ClimateBuffer : register(b2) {
    Texture2D<float> temperatureMap;
    Texture2D<float> moistureMap;
};

BiomeType getBiomeFromClimate(float2 worldXZ) {
    float temp = temperatureMap.Sample(sampler, worldXZ / terrainSize);
    float moisture = moistureMap.Sample(sampler, worldXZ / terrainSize);

    // Whittaker diagram lookup
    return whittakerLookup(temp, moisture);
}
```

---

## 7. Implementation Files

| File | Purpose |
|------|---------|
| `ClimateSystem.h/.cpp` | Temperature, moisture, biome calculation |
| `VegetationDensity` | Per-biome vegetation parameters |
| `BiomeBlend` | Transition blending data |
| `Terrain.hlsl` | Current height-based biome rendering |

---

## 8. Future Enhancements

### 8.1 GPU Climate Maps
- Generate temperature/moisture textures on GPU
- Sample in shader for real-time biome blending

### 8.2 Biome-Specific Features
- Different rock types per biome
- Water color variations
- Unique vegetation species

### 8.3 Dynamic Biomes
- Climate change over time
- Player-influenced biome shifts
- Fire/flood effects

---

*Document created for OrganismEvolution Phase 3*
*Files: ClimateSystem.h/.cpp, GrassSystem.h/.cpp*
