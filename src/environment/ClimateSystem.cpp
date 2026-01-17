#include "ClimateSystem.h"
#include "Terrain.h"
#include "SeasonManager.h"
#include <cmath>
#include <algorithm>
#include <random>

// ============================================================================
// ClimateData Implementation
// ============================================================================

ClimateBiome ClimateData::getBiome() const {
    // Water biomes take precedence
    if (elevation < 0.012f) {
        if (elevation < 0.005f) {
            return ClimateBiome::DEEP_OCEAN;
        }
        return ClimateBiome::SHALLOW_WATER;
    }

    // Beach/coastal zone
    if (elevation < 0.03f && distanceToWater < 10.0f) {
        return ClimateBiome::BEACH;
    }

    // Very high elevation (mountains)
    if (elevation > 0.85f) {
        if (temperature < 0.2f) {
            return ClimateBiome::MOUNTAIN_SNOW;
        }
        if (temperature < 0.35f) {
            return ClimateBiome::MOUNTAIN_ROCK;
        }
        return ClimateBiome::MOUNTAIN_MEADOW;
    }

    // Steep slopes favor rock
    if (slope > 0.7f) {
        return ClimateBiome::MOUNTAIN_ROCK;
    }

    // Whittaker diagram-style classification
    // temperature: 0=freezing, 0.5=temperate, 1=tropical
    // moisture: 0=desert, 0.5=moderate, 1=rainforest

    // Frozen regions
    if (temperature < 0.15f) {
        if (moisture > 0.3f) {
            return ClimateBiome::ICE;
        }
        return ClimateBiome::TUNDRA;
    }

    // Cold regions
    if (temperature < 0.35f) {
        if (moisture > 0.5f) {
            return ClimateBiome::BOREAL_FOREST;
        }
        if (moisture > 0.2f) {
            return ClimateBiome::TUNDRA;
        }
        return ClimateBiome::DESERT_COLD;
    }

    // Temperate regions
    if (temperature < 0.65f) {
        if (moisture > 0.7f) {
            return ClimateBiome::TEMPERATE_FOREST;
        }
        if (moisture > 0.4f) {
            if (distanceToWater < 15.0f && elevation < 0.15f) {
                return ClimateBiome::SWAMP;
            }
            return ClimateBiome::TEMPERATE_FOREST;
        }
        if (moisture > 0.2f) {
            return ClimateBiome::TEMPERATE_GRASSLAND;
        }
        return ClimateBiome::DESERT_HOT;
    }

    // Warm/tropical regions
    if (temperature < 0.85f) {
        if (moisture > 0.6f) {
            return ClimateBiome::TROPICAL_SEASONAL;
        }
        if (moisture > 0.35f) {
            return ClimateBiome::SAVANNA;
        }
        return ClimateBiome::DESERT_HOT;
    }

    // Hot tropical
    if (moisture > 0.7f) {
        return ClimateBiome::TROPICAL_RAINFOREST;
    }
    if (moisture > 0.4f) {
        return ClimateBiome::TROPICAL_SEASONAL;
    }
    if (moisture > 0.2f) {
        return ClimateBiome::SAVANNA;
    }
    return ClimateBiome::DESERT_HOT;
}

glm::vec3 ClimateData::getPrimaryColor() const {
    ClimateBiome biome = getBiome();

    switch (biome) {
        case ClimateBiome::DEEP_OCEAN:         return {0.02f, 0.08f, 0.18f};
        case ClimateBiome::SHALLOW_WATER:      return {0.1f, 0.35f, 0.45f};
        case ClimateBiome::BEACH:              return {0.85f, 0.78f, 0.58f};
        case ClimateBiome::TROPICAL_RAINFOREST:return {0.15f, 0.45f, 0.12f};
        case ClimateBiome::TROPICAL_SEASONAL:  return {0.25f, 0.55f, 0.18f};
        case ClimateBiome::TEMPERATE_FOREST:   return {0.18f, 0.42f, 0.15f};
        case ClimateBiome::TEMPERATE_GRASSLAND:return {0.42f, 0.58f, 0.25f};
        case ClimateBiome::BOREAL_FOREST:      return {0.12f, 0.32f, 0.15f};
        case ClimateBiome::TUNDRA:             return {0.55f, 0.52f, 0.42f};
        case ClimateBiome::ICE:                return {0.92f, 0.95f, 0.98f};
        case ClimateBiome::DESERT_HOT:         return {0.88f, 0.75f, 0.48f};
        case ClimateBiome::DESERT_COLD:        return {0.72f, 0.68f, 0.55f};
        case ClimateBiome::SAVANNA:            return {0.65f, 0.58f, 0.32f};
        case ClimateBiome::SWAMP:              return {0.22f, 0.35f, 0.18f};
        case ClimateBiome::MOUNTAIN_MEADOW:    return {0.38f, 0.52f, 0.28f};
        case ClimateBiome::MOUNTAIN_ROCK:      return {0.52f, 0.48f, 0.45f};
        case ClimateBiome::MOUNTAIN_SNOW:      return {0.95f, 0.95f, 0.98f};
        default:                            return {0.5f, 0.5f, 0.5f};
    }
}

glm::vec3 ClimateData::getSecondaryColor() const {
    ClimateBiome biome = getBiome();

    // Secondary colors for texture variation
    switch (biome) {
        case ClimateBiome::DEEP_OCEAN:         return {0.01f, 0.05f, 0.12f};
        case ClimateBiome::SHALLOW_WATER:      return {0.05f, 0.25f, 0.35f};
        case ClimateBiome::BEACH:              return {0.78f, 0.72f, 0.52f};
        case ClimateBiome::TROPICAL_RAINFOREST:return {0.08f, 0.35f, 0.08f};
        case ClimateBiome::TROPICAL_SEASONAL:  return {0.35f, 0.48f, 0.22f};
        case ClimateBiome::TEMPERATE_FOREST:   return {0.12f, 0.32f, 0.1f};
        case ClimateBiome::TEMPERATE_GRASSLAND:return {0.52f, 0.62f, 0.28f};
        case ClimateBiome::BOREAL_FOREST:      return {0.08f, 0.25f, 0.1f};
        case ClimateBiome::TUNDRA:             return {0.48f, 0.45f, 0.38f};
        case ClimateBiome::ICE:                return {0.85f, 0.88f, 0.95f};
        case ClimateBiome::DESERT_HOT:         return {0.82f, 0.65f, 0.4f};
        case ClimateBiome::DESERT_COLD:        return {0.65f, 0.58f, 0.48f};
        case ClimateBiome::SAVANNA:            return {0.72f, 0.62f, 0.35f};
        case ClimateBiome::SWAMP:              return {0.28f, 0.32f, 0.22f};
        case ClimateBiome::MOUNTAIN_MEADOW:    return {0.32f, 0.45f, 0.22f};
        case ClimateBiome::MOUNTAIN_ROCK:      return {0.42f, 0.4f, 0.38f};
        case ClimateBiome::MOUNTAIN_SNOW:      return {0.88f, 0.9f, 0.95f};
        default:                            return {0.4f, 0.4f, 0.4f};
    }
}

// ============================================================================
// VegetationDensity Implementation
// ============================================================================

VegetationDensity VegetationDensity::forBiome(ClimateBiome biome) {
    VegetationDensity density = {0, 0, 0, 0, 0, 0};

    switch (biome) {
        case ClimateBiome::TROPICAL_RAINFOREST:
            density.treeDensity = 0.9f;
            density.shrubDensity = 0.8f;
            density.fernDensity = 0.7f;
            density.flowerDensity = 0.4f;
            break;

        case ClimateBiome::TROPICAL_SEASONAL:
            density.treeDensity = 0.6f;
            density.grassDensity = 0.5f;
            density.shrubDensity = 0.4f;
            density.flowerDensity = 0.5f;
            break;

        case ClimateBiome::TEMPERATE_FOREST:
            density.treeDensity = 0.75f;
            density.shrubDensity = 0.5f;
            density.grassDensity = 0.3f;
            density.fernDensity = 0.4f;
            density.flowerDensity = 0.3f;
            break;

        case ClimateBiome::TEMPERATE_GRASSLAND:
            density.grassDensity = 0.9f;
            density.flowerDensity = 0.5f;
            density.treeDensity = 0.05f;
            density.shrubDensity = 0.1f;
            break;

        case ClimateBiome::BOREAL_FOREST:
            density.treeDensity = 0.7f;
            density.shrubDensity = 0.3f;
            density.fernDensity = 0.2f;
            break;

        case ClimateBiome::TUNDRA:
            density.grassDensity = 0.3f;
            density.shrubDensity = 0.15f;
            density.flowerDensity = 0.1f;
            break;

        case ClimateBiome::SAVANNA:
            density.grassDensity = 0.8f;
            density.treeDensity = 0.1f;
            density.shrubDensity = 0.2f;
            break;

        case ClimateBiome::DESERT_HOT:
            density.cactusDensity = 0.15f;
            density.shrubDensity = 0.05f;
            break;

        case ClimateBiome::DESERT_COLD:
            density.shrubDensity = 0.1f;
            density.grassDensity = 0.05f;
            break;

        case ClimateBiome::SWAMP:
            density.treeDensity = 0.4f;
            density.shrubDensity = 0.6f;
            density.fernDensity = 0.5f;
            density.grassDensity = 0.7f;
            break;

        case ClimateBiome::MOUNTAIN_MEADOW:
            density.grassDensity = 0.7f;
            density.flowerDensity = 0.6f;
            density.shrubDensity = 0.2f;
            break;

        case ClimateBiome::BEACH:
            density.grassDensity = 0.1f;
            density.shrubDensity = 0.05f;
            break;

        default:
            // Water, ice, rock - minimal vegetation
            break;
    }

    return density;
}

// ============================================================================
// ClimateSystem Implementation
// ============================================================================

ClimateSystem::ClimateSystem() = default;

void ClimateSystem::initialize(const Terrain* t, SeasonManager* sm) {
    terrain = t;
    seasonManager = sm;

    // Precompute moisture map for rain shadow effects
    if (terrain) {
        moistureMapWidth = terrain->getWidth();
        moistureMapDepth = terrain->getDepth();
        simulateMoisture();
    }
}

void ClimateSystem::update(float deltaTime) {
    // Initialize climate grid on first update if needed
    if (!m_gridInitialized && terrain) {
        initializeClimateGrid();
    }

    m_simulationTime += deltaTime;

    // Update global temperature based on long-term cycles
    updateGlobalTemperature(deltaTime);

    // Update moisture patterns (wind-driven transport)
    updateMoisturePatterns(deltaTime);

    // Check for random climate events
    m_eventCheckTimer += deltaTime;
    if (m_eventCheckTimer >= 60.0f) {  // Check once per game-minute
        triggerRandomEvent();
        m_eventCheckTimer = 0.0f;
    }

    // Apply active climate event effects
    if (m_activeEvent != ClimateEvent::NONE) {
        applyClimateEvent(deltaTime);
    }

    // Update biome transitions based on climate changes
    updateBiomeTransitions(deltaTime);

    // Record temperature history for graphing
    m_historyRecordTimer += deltaTime;
    if (m_historyRecordTimer >= HISTORY_RECORD_INTERVAL) {
        recordTemperatureHistory();
        m_historyRecordTimer = 0.0f;
    }
}

ClimateData ClimateSystem::getClimateAt(const glm::vec3& worldPos) const {
    return getClimateAt(worldPos.x, worldPos.z);
}

ClimateData ClimateSystem::getClimateAt(float x, float z) const {
    ClimateData data;

    if (!terrain) {
        // Default values if no terrain
        data.temperature = 0.5f;
        data.moisture = 0.5f;
        data.elevation = 0.3f;
        data.slope = 0.0f;
        data.distanceToWater = 100.0f;
        data.latitude = worldLatitude / 90.0f;
        return data;
    }

    // Get elevation from terrain (normalized 0-1)
    float height = terrain->getHeight(x, z);
    float maxHeight = 30.0f; // HEIGHT_SCALE from terrain
    data.elevation = std::clamp(height / maxHeight, 0.0f, 1.0f);

    // Calculate latitude effect (simulate position on globe)
    float normalizedZ = z / (terrain->getDepth() * terrain->getScale());
    data.latitude = (normalizedZ - 0.5f) * 2.0f; // -1 to 1

    // Temperature based on elevation and latitude
    data.temperature = calculateBaseTemperature(data.elevation, data.latitude);

    // Apply seasonal modifier
    if (seasonManager) {
        float seasonTemp = seasonManager->getTemperature();
        data.temperature = data.temperature * 0.7f + seasonTemp * 0.3f;
    }

    // Moisture from precomputed map + local factors
    data.moisture = calculateMoisture(x, z, data.elevation);

    // Terrain slope
    data.slope = calculateSlope(x, z);

    // Distance to water
    data.distanceToWater = calculateDistanceToWater(x, z);

    return data;
}

BiomeBlend ClimateSystem::calculateBiomeBlend(const ClimateData& climate) const {
    BiomeBlend blend;
    blend.primary = climate.getBiome();

    // Find neighboring biome by slightly perturbing climate values
    ClimateData perturbedClimate = climate;
    perturbedClimate.temperature += 0.1f;
    blend.secondary = perturbedClimate.getBiome();

    if (blend.primary == blend.secondary) {
        perturbedClimate.moisture += 0.1f;
        blend.secondary = perturbedClimate.getBiome();
    }

    // Calculate blend factor based on climate edge proximity
    // Use noise for natural-looking boundaries
    float noiseScale = 0.1f;
    blend.noiseOffset = std::sin(climate.elevation * 50.0f + climate.moisture * 30.0f) * noiseScale;

    // Distance to biome boundary
    float tempBoundary = std::fmod(climate.temperature * 5.0f, 1.0f);
    float moistBoundary = std::fmod(climate.moisture * 5.0f, 1.0f);
    float edgeDist = std::min(
        std::min(tempBoundary, 1.0f - tempBoundary),
        std::min(moistBoundary, 1.0f - moistBoundary)
    );

    blend.blendFactor = std::clamp(1.0f - edgeDist * 5.0f + blend.noiseOffset, 0.0f, 1.0f);

    return blend;
}

void ClimateSystem::simulateMoisture() {
    if (!terrain) return;

    int w = moistureMapWidth;
    int d = moistureMapDepth;
    moistureMap.resize(w * d, 0.5f);

    float scale = terrain->getScale();

    // Simulate rain shadow effect
    // Wind blows from prevailingWind direction
    // Mountains block moisture, creating dry areas behind them

    // Start with base moisture (higher near water)
    for (int z = 0; z < d; z++) {
        for (int x = 0; x < w; x++) {
            float worldX = x * scale;
            float worldZ = z * scale;
            float height = terrain->getHeight(worldX, worldZ);

            // Base moisture from proximity to water
            if (terrain->isWater(worldX, worldZ)) {
                moistureMap[z * w + x] = 1.0f;
            } else {
                // Start with moderate moisture
                moistureMap[z * w + x] = 0.6f;
            }
        }
    }

    // Propagate moisture in wind direction
    glm::ivec2 windStep = {
        static_cast<int>(std::round(prevailingWind.x)),
        static_cast<int>(std::round(prevailingWind.y))
    };

    // Multiple passes for diffusion
    for (int pass = 0; pass < 10; pass++) {
        std::vector<float> newMoisture = moistureMap;

        for (int z = 1; z < d - 1; z++) {
            for (int x = 1; x < w - 1; x++) {
                float worldX = x * scale;
                float worldZ = z * scale;

                // Skip water
                if (terrain->isWater(worldX, worldZ)) continue;

                // Get height gradient (mountains block moisture)
                float centerHeight = terrain->getHeight(worldX, worldZ);

                // Sample upwind moisture
                int upwindX = std::clamp(x - windStep.x, 0, w - 1);
                int upwindZ = std::clamp(z - windStep.y, 0, d - 1);
                float upwindMoisture = moistureMap[upwindZ * w + upwindX];
                float upwindHeight = terrain->getHeight(upwindX * scale, upwindZ * scale);

                // Orographic effect: rising air = more rain, sinking = drier
                float heightDiff = centerHeight - upwindHeight;
                float orographicFactor = 1.0f;
                if (heightDiff > 0.5f) {
                    // Rising air - precipitation
                    orographicFactor = 0.8f;
                } else if (heightDiff < -0.5f) {
                    // Sinking air - rain shadow
                    orographicFactor = 1.2f;
                }

                // High mountains block moisture significantly
                if (centerHeight > 20.0f) {
                    orographicFactor *= 0.5f;
                }

                // Blend with upwind moisture (simulate advection)
                float newMoist = moistureMap[z * w + x] * 0.7f + upwindMoisture * 0.3f * orographicFactor;
                newMoisture[z * w + x] = std::clamp(newMoist, 0.0f, 1.0f);
            }
        }

        moistureMap = newMoisture;
    }
}

VegetationDensity ClimateSystem::getVegetationDensity(const glm::vec3& worldPos) const {
    ClimateData climate = getClimateAt(worldPos);
    VegetationDensity baseDensity = VegetationDensity::forBiome(climate.getBiome());

    // Modify by slope (steep slopes have less vegetation)
    float slopeFactor = 1.0f - climate.slope * 0.8f;
    baseDensity.treeDensity *= slopeFactor;
    baseDensity.grassDensity *= slopeFactor;
    baseDensity.shrubDensity *= slopeFactor;

    // Modify by season
    if (seasonManager) {
        float leafMult = seasonManager->getLeafMultiplier();
        float growthMult = seasonManager->getGrowthMultiplier();

        baseDensity.grassDensity *= growthMult;
        baseDensity.flowerDensity *= growthMult;
        // Trees don't change density as much, but could affect foliage rendering
    }

    return baseDensity;
}

float ClimateSystem::getSeasonalTemperature(float baseTemp) const {
    if (!seasonManager) return baseTemp;

    float seasonTemp = seasonManager->getTemperature();
    // Season modifies base temperature by ±15 degrees
    return baseTemp + (seasonTemp - 0.5f) * 30.0f;
}

const char* ClimateSystem::getBiomeName(ClimateBiome biome) {
    switch (biome) {
        case ClimateBiome::DEEP_OCEAN:          return "Deep Ocean";
        case ClimateBiome::SHALLOW_WATER:       return "Shallow Water";
        case ClimateBiome::BEACH:               return "Beach";
        case ClimateBiome::TROPICAL_RAINFOREST: return "Tropical Rainforest";
        case ClimateBiome::TROPICAL_SEASONAL:   return "Tropical Seasonal";
        case ClimateBiome::TEMPERATE_FOREST:    return "Temperate Forest";
        case ClimateBiome::TEMPERATE_GRASSLAND: return "Temperate Grassland";
        case ClimateBiome::BOREAL_FOREST:       return "Boreal Forest";
        case ClimateBiome::TUNDRA:              return "Tundra";
        case ClimateBiome::ICE:                 return "Ice";
        case ClimateBiome::DESERT_HOT:          return "Hot Desert";
        case ClimateBiome::DESERT_COLD:         return "Cold Desert";
        case ClimateBiome::SAVANNA:             return "Savanna";
        case ClimateBiome::SWAMP:               return "Swamp";
        case ClimateBiome::MOUNTAIN_MEADOW:     return "Mountain Meadow";
        case ClimateBiome::MOUNTAIN_ROCK:       return "Mountain Rock";
        case ClimateBiome::MOUNTAIN_SNOW:       return "Mountain Snow";
        default:                             return "Unknown";
    }
}

// ============================================================================
// Private Helper Methods
// ============================================================================

float ClimateSystem::calculateBaseTemperature(float elevation, float latitude) const {
    // Base temperature decreases with elevation (lapse rate ~6.5°C per 1000m)
    // Assuming max elevation = 3000m (normalized 1.0)
    float elevationEffect = elevation * 0.6f; // 0 to 0.6 cooling effect

    // Latitude effect: equator is warmest, poles are coldest
    float latitudeEffect = std::abs(latitude) * 0.5f; // 0 to 0.5 cooling effect

    // Base temperature at sea level, equator = 1.0 (hot)
    float baseTemp = 0.85f;

    return std::clamp(baseTemp - elevationEffect - latitudeEffect, 0.0f, 1.0f);
}

float ClimateSystem::calculateMoisture(float x, float z, float elevation) const {
    // Get from precomputed map if available
    if (!moistureMap.empty() && terrain) {
        float scale = terrain->getScale();
        int ix = static_cast<int>(x / scale);
        int iz = static_cast<int>(z / scale);

        if (ix >= 0 && ix < moistureMapWidth && iz >= 0 && iz < moistureMapDepth) {
            return moistureMap[iz * moistureMapWidth + ix];
        }
    }

    // Fallback: simple elevation-based moisture
    // Higher elevations are drier (above tree line)
    return std::clamp(0.6f - elevation * 0.4f, 0.1f, 1.0f);
}

float ClimateSystem::calculateSlope(float x, float z) const {
    if (!terrain) return 0.0f;

    float eps = 1.0f;
    float h0 = terrain->getHeight(x, z);
    float hx = terrain->getHeight(x + eps, z);
    float hz = terrain->getHeight(x, z + eps);

    float dx = (hx - h0) / eps;
    float dz = (hz - h0) / eps;

    float slopeMag = std::sqrt(dx * dx + dz * dz);

    // Normalize: 0 = flat, 1 = 45 degree slope
    return std::clamp(slopeMag / 1.0f, 0.0f, 1.0f);
}

float ClimateSystem::calculateDistanceToWater(float x, float z) const {
    if (!terrain) return 100.0f;

    // Check if we're in water
    if (terrain->isWater(x, z)) return 0.0f;

    // Sample in expanding circles to find nearest water
    float maxDist = 50.0f;
    float step = 5.0f;

    for (float dist = step; dist <= maxDist; dist += step) {
        // Check 8 directions
        for (int angle = 0; angle < 8; angle++) {
            float rad = angle * 0.785398f; // PI/4
            float sx = x + std::cos(rad) * dist;
            float sz = z + std::sin(rad) * dist;

            if (terrain->isWater(sx, sz)) {
                return dist;
            }
        }
    }

    return maxDist;
}

ClimateBiome ClimateSystem::whittakerDiagram(float temperature, float precipitation) const {
    // Implementation follows standard Whittaker biome classification
    // Already handled in ClimateData::getBiome()
    ClimateData data;
    data.temperature = temperature;
    data.moisture = precipitation;
    data.elevation = 0.3f;
    data.slope = 0.0f;
    data.distanceToWater = 50.0f;
    data.latitude = 0.0f;

    return data.getBiome();
}

// ============================================================================
// Dynamic Climate System Implementation
// ============================================================================

void ClimateSystem::initializeClimateGrid() {
    if (!terrain) return;

    // Calculate grid dimensions based on terrain size
    float terrainWidth = terrain->getWidth() * terrain->getScale();
    float terrainDepth = terrain->getDepth() * terrain->getScale();

    m_gridWidth = static_cast<int>(terrainWidth / m_gridCellSize) + 1;
    m_gridHeight = static_cast<int>(terrainDepth / m_gridCellSize) + 1;

    m_climateGrid.resize(m_gridWidth * m_gridHeight);

    // Initialize each cell with base climate values
    for (int z = 0; z < m_gridHeight; z++) {
        for (int x = 0; x < m_gridWidth; x++) {
            float worldX = x * m_gridCellSize;
            float worldZ = z * m_gridCellSize;

            ClimateData baseClimate = getClimateAt(worldX, worldZ);

            ClimateGridCell& cell = m_climateGrid[z * m_gridWidth + x];
            cell.baseTemperature = baseClimate.temperature;
            cell.currentTemperature = baseClimate.temperature;
            cell.baseMoisture = baseClimate.moisture;
            cell.currentMoisture = baseClimate.moisture;
            cell.primaryBiome = baseClimate.getBiome();
            cell.previousBiome = cell.primaryBiome;
            cell.transitionProgress = 0.0f;
            cell.isTransitioning = false;
        }
    }

    m_gridInitialized = true;
}

void ClimateSystem::updateGlobalTemperature(float deltaTime) {
    // Long-term cycle (simplified Milankovitch cycles)
    // Period: ~1000 game-years (assuming 360 days/year, 60s/day = 21600s/year)
    // So 1000 years = 21,600,000 seconds simulation time
    float longCyclePeriod = 21600000.0f;
    float longCycle = std::sin(m_simulationTime * 6.28318f / longCyclePeriod) * 0.07f;  // ±7% temperature

    // Medium cycle (centuries) - ~100 game-years
    float mediumCyclePeriod = 2160000.0f;
    float mediumCycle = std::sin(m_simulationTime * 6.28318f / mediumCyclePeriod) * 0.03f;  // ±3%

    // Seasonal variation from SeasonManager
    float seasonalOffset = 0.0f;
    if (seasonManager) {
        // SeasonManager returns 0.1-0.9, center at 0.5
        seasonalOffset = (seasonManager->getTemperature() - 0.5f) * 0.4f;  // ±20%
    }

    // Ice age modifier (accumulates slowly when in ice age)
    if (m_inIceAge) {
        m_iceAgeModifier = std::max(m_iceAgeModifier - deltaTime * 0.00001f, -0.15f);
    } else {
        m_iceAgeModifier = std::min(m_iceAgeModifier + deltaTime * 0.00001f, 0.0f);
    }

    // Check for ice age transitions based on long cycle
    if (longCycle < -0.05f && !m_inIceAge) {
        m_inIceAge = true;
    } else if (longCycle > 0.03f && m_inIceAge) {
        m_inIceAge = false;
    }

    m_globalTemperatureOffset = longCycle + mediumCycle + seasonalOffset + m_iceAgeModifier;
}

void ClimateSystem::updateMoisturePatterns(float deltaTime) {
    if (!m_gridInitialized || m_climateGrid.empty()) return;

    // Simulate wind-driven moisture transport
    // This is a simplified version that runs periodically

    static float moistureUpdateTimer = 0.0f;
    moistureUpdateTimer += deltaTime;

    // Only update moisture every few seconds for performance
    if (moistureUpdateTimer < 2.0f) return;
    moistureUpdateTimer = 0.0f;

    // Apply event modifiers
    float moistureModifier = 1.0f;
    if (m_activeEvent == ClimateEvent::DROUGHT) {
        moistureModifier = 0.5f;
    } else if (m_activeEvent == ClimateEvent::MONSOON) {
        moistureModifier = 1.5f;
    }

    // Wind transport direction (from prevailing wind)
    int windStepX = static_cast<int>(std::round(prevailingWind.x));
    int windStepZ = static_cast<int>(std::round(prevailingWind.y));

    // Single pass moisture update
    for (int z = 1; z < m_gridHeight - 1; z++) {
        for (int x = 1; x < m_gridWidth - 1; x++) {
            ClimateGridCell& cell = m_climateGrid[z * m_gridWidth + x];

            float worldX = x * m_gridCellSize;
            float worldZ = z * m_gridCellSize;

            // Skip water cells (they maintain moisture)
            if (terrain && terrain->isWater(worldX, worldZ)) {
                cell.currentMoisture = 1.0f;
                continue;
            }

            // Get upwind moisture
            int upwindX = std::clamp(x - windStepX, 0, m_gridWidth - 1);
            int upwindZ = std::clamp(z - windStepZ, 0, m_gridHeight - 1);
            float upwindMoisture = m_climateGrid[upwindZ * m_gridWidth + upwindX].currentMoisture;

            // Get elevation for rain shadow
            float elevation = 0.0f;
            float upwindElevation = 0.0f;
            if (terrain) {
                elevation = terrain->getHeight(worldX, worldZ);
                upwindElevation = terrain->getHeight(upwindX * m_gridCellSize, upwindZ * m_gridCellSize);
            }

            // Orographic effect
            float heightDiff = elevation - upwindElevation;
            float orographicFactor = 1.0f;
            if (heightDiff > 2.0f) {
                orographicFactor = 0.85f;  // Rising air = precipitation
            } else if (heightDiff < -2.0f) {
                orographicFactor = 1.15f;  // Sinking air = drier
            }

            // High mountains block moisture significantly
            if (elevation > 20.0f) {
                orographicFactor *= 0.7f;
            }

            // Calculate new moisture
            float newMoisture = cell.baseMoisture * 0.7f + upwindMoisture * 0.3f * orographicFactor;
            newMoisture *= moistureModifier;
            cell.currentMoisture = std::clamp(newMoisture, 0.0f, 1.0f);
        }
    }
}

void ClimateSystem::updateBiomeTransitions(float deltaTime) {
    if (!m_gridInitialized || m_climateGrid.empty()) return;

    // Transition speed (how fast biomes change)
    const float transitionSpeed = 0.05f;  // Takes ~20 seconds to fully transition

    for (int z = 0; z < m_gridHeight; z++) {
        for (int x = 0; x < m_gridWidth; x++) {
            ClimateGridCell& cell = m_climateGrid[z * m_gridWidth + x];

            // Apply global temperature offset to current temperature
            cell.currentTemperature = std::clamp(
                cell.baseTemperature + m_globalTemperatureOffset,
                0.0f, 1.0f
            );

            // Determine what biome this cell should be
            ClimateData tempData;
            tempData.temperature = cell.currentTemperature;
            tempData.moisture = cell.currentMoisture;

            float worldX = x * m_gridCellSize;
            float worldZ = z * m_gridCellSize;

            if (terrain) {
                float height = terrain->getHeight(worldX, worldZ);
                tempData.elevation = std::clamp(height / 30.0f, 0.0f, 1.0f);
                tempData.slope = calculateSlope(worldX, worldZ);
                tempData.distanceToWater = calculateDistanceToWater(worldX, worldZ);
            } else {
                tempData.elevation = 0.3f;
                tempData.slope = 0.0f;
                tempData.distanceToWater = 50.0f;
            }
            tempData.latitude = 0.0f;

            ClimateBiome newBiome = tempData.getBiome();

            // Check if biome should change
            if (newBiome != cell.primaryBiome && !cell.isTransitioning) {
                // Start transition
                cell.previousBiome = cell.primaryBiome;
                cell.primaryBiome = newBiome;
                cell.isTransitioning = true;
                cell.transitionProgress = 0.0f;
            }

            // Progress transition
            if (cell.isTransitioning) {
                cell.transitionProgress += deltaTime * transitionSpeed;
                if (cell.transitionProgress >= 1.0f) {
                    cell.transitionProgress = 1.0f;
                    cell.isTransitioning = false;
                }
            }
        }
    }
}

void ClimateSystem::triggerRandomEvent() {
    // Don't trigger new event if one is active
    if (m_activeEvent != ClimateEvent::NONE) return;

    // Random chance of event (~0.5% chance per minute)
    float roll = static_cast<float>(rand()) / RAND_MAX;
    if (roll > 0.005f) return;

    // Determine event type
    int eventRoll = rand() % 100;

    if (eventRoll < 20) {
        startEvent(ClimateEvent::DROUGHT, 30.0f * 60.0f);  // 30 game-days
    } else if (eventRoll < 40) {
        startEvent(ClimateEvent::MONSOON, 20.0f * 60.0f);  // 20 game-days
    } else if (eventRoll < 55) {
        startEvent(ClimateEvent::VOLCANIC_WINTER, 50.0f * 60.0f);  // 50 game-days
    } else if (eventRoll < 65) {
        startEvent(ClimateEvent::SOLAR_MAXIMUM, 100.0f * 60.0f);  // 100 game-days
    }
    // Ice ages are triggered by long-term cycles, not randomly
}

void ClimateSystem::startEvent(ClimateEvent event, float duration) {
    m_activeEvent = event;
    m_eventDuration = duration;
    m_eventTimeRemaining = duration;
}

void ClimateSystem::endEvent() {
    m_activeEvent = ClimateEvent::NONE;
    m_eventDuration = 0.0f;
    m_eventTimeRemaining = 0.0f;
}

void ClimateSystem::applyClimateEvent(float deltaTime) {
    // Update remaining time
    m_eventTimeRemaining -= deltaTime;
    if (m_eventTimeRemaining <= 0) {
        endEvent();
        return;
    }

    // Apply event effects (these modify the global offset temporarily)
    switch (m_activeEvent) {
        case ClimateEvent::VOLCANIC_WINTER:
            // Temperature reduction is applied in getGlobalTemperature()
            break;
        case ClimateEvent::SOLAR_MAXIMUM:
            // Temperature increase is applied in getGlobalTemperature()
            break;
        case ClimateEvent::DROUGHT:
        case ClimateEvent::MONSOON:
            // Moisture effects are applied in updateMoisturePatterns()
            break;
        case ClimateEvent::ICE_AGE_START:
            m_inIceAge = true;
            break;
        case ClimateEvent::ICE_AGE_END:
            m_inIceAge = false;
            break;
        default:
            break;
    }
}

const char* ClimateSystem::getEventName() const {
    switch (m_activeEvent) {
        case ClimateEvent::NONE:            return "None";
        case ClimateEvent::VOLCANIC_WINTER: return "Volcanic Winter";
        case ClimateEvent::SOLAR_MAXIMUM:   return "Solar Maximum";
        case ClimateEvent::DROUGHT:         return "Drought";
        case ClimateEvent::MONSOON:         return "Monsoon";
        case ClimateEvent::ICE_AGE_START:   return "Ice Age Beginning";
        case ClimateEvent::ICE_AGE_END:     return "Ice Age Ending";
        default:                            return "Unknown";
    }
}

float ClimateSystem::getGlobalTemperature() const {
    // Base temperature in Celsius (roughly world average)
    float baseTemp = 15.0f;

    // Apply global offset (normalized, so multiply by range)
    float tempRange = 30.0f;  // Can vary ±15°C from base
    float temp = baseTemp + m_globalTemperatureOffset * tempRange;

    // Apply active event modifiers
    if (m_activeEvent == ClimateEvent::VOLCANIC_WINTER) {
        temp -= 3.0f;
    } else if (m_activeEvent == ClimateEvent::SOLAR_MAXIMUM) {
        temp += 2.0f;
    }

    return temp;
}

const ClimateGridCell* ClimateSystem::getClimateGridCell(int x, int z) const {
    if (!m_gridInitialized) return nullptr;
    if (x < 0 || x >= m_gridWidth || z < 0 || z >= m_gridHeight) return nullptr;
    return &m_climateGrid[z * m_gridWidth + x];
}

void ClimateSystem::recordTemperatureHistory() {
    float temp = getGlobalTemperature();
    m_temperatureHistory.push_back(temp);

    // Keep history size bounded
    while (m_temperatureHistory.size() > MAX_HISTORY_SIZE) {
        m_temperatureHistory.pop_front();
    }
}
