# Phase 8 Agent 6: Ocean Ecosystem Simulation

## Overview

Agent 6 implements the ocean ecosystem simulation, populating underwater environments with visible life and simulating aquatic resource loops. This includes depth bands, underwater flora, fish schooling, and integration with the existing food web system.

## Recent Updates

### Aquatic Spawn Zone System (Latest)

Added comprehensive aquatic spawn zone management to EcosystemManager:

**New Structures:**
```cpp
// Spawn zone definition
struct AquaticSpawnZone {
    glm::vec3 center;
    float radius;
    float minDepth, maxDepth;
    DepthBand primaryBand;

    // Environmental factors
    float temperature;       // Cooler at depth
    float oxygenLevel;       // Less O2 at depth
    float foodDensity;       // Plankton/algae availability
    float shelterDensity;    // Kelp/coral cover

    // Spawn weights
    float herbivoreWeight, predatorWeight, apexWeight;
    int maxCapacity, currentPopulation;
};

// Population tracking
struct AquaticPopulationStats {
    std::array<int, 5> countByDepth;  // Per depth band
    int herbivoreCount, predatorCount, apexCount;
    float avgDepth, depthVariance;
    float herbivorePreyRatio, predatorPreyRatio, apexPredatorRatio;
};
```

**New EcosystemManager Methods:**
```cpp
const std::vector<AquaticSpawnZone>& getAquaticSpawnZones() const;
const AquaticSpawnZone* findBestSpawnZone(CreatureType type) const;
glm::vec3 getAquaticSpawnPosition(const AquaticSpawnZone& zone, CreatureType type) const;
const AquaticPopulationStats& getAquaticPopulationStats() const;
int getPopulationInDepthBand(DepthBand band) const;
bool isAquaticEcosystemHealthy() const;
float getAquaticEcosystemHealth() const;
float getAquaticFoodWebBalance() const;
std::string getDepthBandHistogram() const;  // Debug output
```

**Spawn Zone Generation:**
- Zones auto-generated during init by scanning terrain for water
- Each zone sized to radius where water depth > 2m
- Environmental factors set based on depth (temp, O2, food, shelter)
- Spawn weights vary by depth band

**Spawn Weights by Depth:**
| Depth Band | Herbivore | Predator | Apex | Capacity |
|------------|-----------|----------|------|----------|
| Surface/Shallow | 70% | 25% | 5% | 40 |
| Mid-Water | 55% | 35% | 10% | 60 |
| Deep | 40% | 40% | 20% | 30 |
| Abyss | 30% | 35% | 35% | 15 |

## Implementation Summary

### Phase 1: Depth Bands and Spawn Zones

**Files Modified:**
- [SwimBehavior.h](../src/entities/SwimBehavior.h) - Added `DepthBand` enum and helper functions
- [CreatureType.h](../src/entities/CreatureType.h) - Added `getAquaticSpawnDepthRange()` and `calculateAquaticSpawnDepth()`
- [main.cpp](../src/main.cpp) - Improved `GetSurfaceHeight()` for species-specific depth ranges
- [StatisticsDataManager.h/cpp](../src/ui/statistics/StatisticsDataManager.h) - Added aquatic depth histogram tracking
- [EcosystemDashboard.cpp](../src/ui/statistics/EcosystemDashboard.cpp) - Added `renderAquaticDepthHistogram()`

**Depth Band Definitions:**
| Band | Depth Range | Description |
|------|-------------|-------------|
| SURFACE | 0-2m | Air-breathing creatures, surface feeders |
| SHALLOW | 2-5m | Small fish, reef dwellers, kelp forests |
| MID_WATER | 5-25m | Schooling fish, most common zone |
| DEEP | 25-50m | Larger predators, pressure-adapted species |
| ABYSS | 50m+ | Deep sea creatures, bioluminescent species |

**Key Functions Added:**
```cpp
DepthBand getDepthBand(float depth);
void getDepthBandRange(DepthBand band, float& minDepth, float& maxDepth);
float calculateAquaticSpawnDepth(CreatureType type, float availableWaterDepth, float randomValue01);
```

### Phase 2: Underwater Flora System Integration

**Files Modified:**
- [VegetationManager.h](../src/environment/VegetationManager.h) - Added `AquaticPlantSystem` integration
- [VegetationManager.cpp](../src/environment/VegetationManager.cpp) - Implemented aquatic plant methods
- [main.cpp](../src/main.cpp) - Added aquatic plant initialization

**Existing Systems Integrated:**
- `AquaticPlantSystem` - Full kelp forests, coral reefs, lily pads, seaweed
- 30+ aquatic plant types with depth/temperature/salinity requirements
- Food value, shelter factor, oxygen production per plant

**New Methods:**
```cpp
void VegetationManager::initializeAquaticPlants(ID3D12Device* dx12Device, unsigned int seed);
void VegetationManager::updateAquaticPlants(float deltaTime, const glm::vec3& cameraPos);
VegetationManager::AquaticStats VegetationManager::getAquaticStats() const;
```

### Phase 3: Food Web Integration

**Files Modified:**
- [EcosystemManager.cpp](../src/environment/EcosystemManager.cpp) - Added aquatic food sources to `getFoodPositionsFor()` and `getFoodPressure()`

**Food Web Connections:**
| Creature Type | Food Sources |
|---------------|--------------|
| AQUATIC / AQUATIC_HERBIVORE | Plankton, algae, seaweed (via `getAllAquaticFoodPositions()`) |
| AMPHIBIAN | Aquatic food + terrestrial insects (proxy via bushes) |
| AQUATIC_PREDATOR / AQUATIC_APEX | Hunt smaller fish (no plant food) |

**Producer System Aquatic Patches:**
- `planktonPatches` - Floating in water column
- `algaePatches` - On sea floor/rocks
- `seaweedPatches` - Larger underwater plants

### Phase 4: Swimming and Schooling Behaviors

**Existing Infrastructure:**
- `SwimBehavior` class with buoyancy, drag, thrust, pressure effects
- `FishSchoolingManager` with spatial hash grid for O(n) neighbor finding
- GPU compute shader (`SteeringCompute.hlsl`) with boids algorithm support

**GPU Steering Supports:**
- Separation, alignment, cohesion (flocking parameters)
- `TYPE_AQUATIC` and `TYPE_AQUATIC_PREDATOR` in compute shader
- `CanSwim()` and `AvoidLand()` functions for aquatic creatures
- Predator-prey relationships for aquatic types

**SteeringConstants Flocking Parameters:**
```cpp
float separationDistance = 3.0f;
float alignmentDistance = 8.0f;
float cohesionDistance = 10.0f;
float separationWeight = 1.5f;
float alignmentWeight = 1.0f;
float cohesionWeight = 1.0f;
```

## File Ownership

**Owned by Agent 6:**
- `src/environment/AquaticPlants.cpp` / `.h`
- `src/environment/EcosystemManager.cpp` / `.h`
- `src/environment/ProducerSystem.cpp` / `.h`
- `src/entities/SwimBehavior.cpp` / `.h`
- `src/entities/aquatic/*`

**Do NOT Edit:**
- `src/graphics/WaterRenderer.*`
- `Runtime/Shaders/*`

## Key Constants

| Constant | Value | Location |
|----------|-------|----------|
| WATER_SURFACE_HEIGHT | 10.5f | `SwimBehavior.h` |
| WATER_LEVEL | 0.35 (normalized) | `TerrainSampler.h` |
| HEIGHT_SCALE | 30.0f | `TerrainSampler.h` |

## Validation Checklist

- [x] Depth bands defined with explicit ranges
- [x] Aquatic spawn positions use species-specific depth ranges
- [x] Depth histogram visible in UI (Ecosystem Dashboard)
- [x] Underwater flora system initialized at startup
- [x] Aquatic food sources integrated with creature food-finding
- [x] Food pressure calculation includes aquatic types
- [x] GPU steering shader supports aquatic creature types
- [x] Schooling behavior infrastructure exists (FishSchoolingManager)

## Testing Notes

1. **Population Check**: Run simulation for 5+ minutes, verify aquatic creatures persist
2. **Depth Distribution**: Check "Aquatic Depth Distribution" panel in Ecosystem Dashboard
3. **Food Web**: Observe herbivore fish seeking algae/plankton patches
4. **Schooling**: Watch for fish grouping behaviors in mid-water zone

## Known Limitations

1. FishSchoolingManager runs CPU-side; GPU schooling uses generic flocking
2. Coral reef health and bleaching system exists but may need visual indicators
3. Bioluminescent creatures exist in data but rendering may need integration

## Future Enhancements

1. Per-species schooling parameters (tighter schools for small fish)
2. Migration patterns for aquatic creatures
3. Seasonal plankton blooms affecting fish populations
4. Aquatic plant rendering through VegetationManager
