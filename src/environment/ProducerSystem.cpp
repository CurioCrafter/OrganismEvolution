#include "ProducerSystem.h"
#include "Terrain.h"
#include "SeasonManager.h"
#include "VegetationManager.h"
#include <random>
#include <algorithm>
#include <cmath>
#include <iostream>

ProducerSystem::ProducerSystem(Terrain* terrain, int gridResolution)
    : terrain(terrain)
    , gridResolution(gridResolution)
    , soilTileSize(0.0f)
{
}

void ProducerSystem::init(unsigned int seed) {
    float terrainWidth = terrain->getWidth() * terrain->getScale();
    soilTileSize = terrainWidth / gridResolution;

    soilGrid.resize(gridResolution);
    for (int i = 0; i < gridResolution; i++) {
        soilGrid[i].resize(gridResolution);
        for (int j = 0; j < gridResolution; j++) {
            float x = (i - gridResolution/2) * soilTileSize;
            float z = (j - gridResolution/2) * soilTileSize;
            float height = terrain->getHeight(x, z);

            SoilTile& tile = soilGrid[i][j];

            if (height > 0.5f && height < 0.75f) {
                tile.nitrogen = 60.0f + (std::rand() % 20);
                tile.organicMatter = 50.0f + (std::rand() % 30);
            }
            else if (height > 0.35f && height < 0.5f) {
                tile.nitrogen = 40.0f + (std::rand() % 20);
                tile.organicMatter = 30.0f + (std::rand() % 20);
            }
            else if (height > 0.75f) {
                tile.nitrogen = 20.0f + (std::rand() % 15);
                tile.organicMatter = 10.0f + (std::rand() % 15);
            }

            tile.moisture = std::max(20.0f, 80.0f - height * 60.0f);
        }
    }

    generateGrassPatches(seed);
    generateBushPatches(seed);
    linkTreePatches();
    generateAquaticPatches(seed);
}

void ProducerSystem::generateGrassPatches(unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> posDist(-0.5f, 0.5f);

    float terrainWidth = terrain->getWidth() * terrain->getScale();
    float spacing = 8.0f;
    int patchCount = static_cast<int>(terrainWidth / spacing);

    for (int i = 0; i < patchCount; i++) {
        for (int j = 0; j < patchCount; j++) {
            float x = (i - patchCount/2) * spacing + posDist(rng) * spacing * 0.5f;
            float z = (j - patchCount/2) * spacing + posDist(rng) * spacing * 0.5f;

            if (terrain->isWater(x, z)) continue;

            float height = terrain->getHeight(x, z);
            if (height < 0.35f || height > 0.75f) continue;

            FoodPatch patch;
            patch.position = glm::vec3(x, height, z);
            patch.type = FoodSourceType::GRASS;
            patch.maxBiomass = 10.0f + (height - 0.35f) * 20.0f;
            patch.currentBiomass = patch.maxBiomass * 0.8f;
            patch.regrowthRate = 0.5f;
            patch.energyPerUnit = 2.0f;
            patch.lastConsumedTime = 0.0f;
            patch.consumptionPressure = 0.0f;

            const SoilTile& soil = getSoilAt(patch.position);
            patch.soilNitrogen = soil.nitrogen;
            patch.soilMoisture = soil.moisture;

            grassPatches.push_back(patch);
        }
    }
}

void ProducerSystem::generateBushPatches(unsigned int seed) {
    std::mt19937 rng(seed + 1000);
    std::uniform_real_distribution<float> posDist(-0.5f, 0.5f);

    float terrainWidth = terrain->getWidth() * terrain->getScale();
    float spacing = 15.0f;
    int patchCount = static_cast<int>(terrainWidth / spacing);

    for (int i = 0; i < patchCount; i++) {
        for (int j = 0; j < patchCount; j++) {
            float x = (i - patchCount/2) * spacing + posDist(rng) * spacing * 0.5f;
            float z = (j - patchCount/2) * spacing + posDist(rng) * spacing * 0.5f;

            if (terrain->isWater(x, z)) continue;

            float height = terrain->getHeight(x, z);
            if (height < 0.45f || height > 0.7f) continue;
            if ((rng() % 100) > 40) continue;

            FoodPatch patch;
            patch.position = glm::vec3(x, height, z);
            patch.type = FoodSourceType::BUSH_BERRY;
            patch.maxBiomass = 20.0f;
            patch.currentBiomass = patch.maxBiomass * 0.7f;
            patch.regrowthRate = 0.2f;
            patch.energyPerUnit = 5.0f;
            patch.lastConsumedTime = 0.0f;
            patch.consumptionPressure = 0.0f;

            const SoilTile& soil = getSoilAt(patch.position);
            patch.soilNitrogen = soil.nitrogen;
            patch.soilMoisture = soil.moisture;

            bushPatches.push_back(patch);
        }
    }
}

void ProducerSystem::linkTreePatches() {
    std::mt19937 rng(42);
    float terrainWidth = terrain->getWidth() * terrain->getScale();
    float spacing = 20.0f;
    int patchCount = static_cast<int>(terrainWidth / spacing);

    for (int i = 0; i < patchCount; i++) {
        for (int j = 0; j < patchCount; j++) {
            float x = (i - patchCount/2) * spacing;
            float z = (j - patchCount/2) * spacing;

            if (terrain->isWater(x, z)) continue;

            float height = terrain->getHeight(x, z);
            if (height < 0.55f || height > 0.8f) continue;
            if ((rng() % 100) > 50) continue;

            FoodPatch fruitPatch;
            fruitPatch.position = glm::vec3(x, height, z);
            fruitPatch.type = FoodSourceType::TREE_FRUIT;
            fruitPatch.maxBiomass = 30.0f;
            fruitPatch.currentBiomass = fruitPatch.maxBiomass * 0.5f;
            fruitPatch.regrowthRate = 0.1f;
            fruitPatch.energyPerUnit = 8.0f;
            fruitPatch.lastConsumedTime = 0.0f;
            fruitPatch.consumptionPressure = 0.0f;

            FoodPatch leafPatch;
            leafPatch.position = glm::vec3(x, height, z);
            leafPatch.type = FoodSourceType::TREE_LEAF;
            leafPatch.maxBiomass = 50.0f;
            leafPatch.currentBiomass = leafPatch.maxBiomass * 0.8f;
            leafPatch.regrowthRate = 0.15f;
            leafPatch.energyPerUnit = 4.0f;
            leafPatch.lastConsumedTime = 0.0f;
            leafPatch.consumptionPressure = 0.0f;

            treePatches.push_back(fruitPatch);
            treePatches.push_back(leafPatch);
        }
    }
}

void ProducerSystem::update(float deltaTime, const SeasonManager* seasonMgr) {
    float seasonMultiplier = 1.0f;
    if (seasonMgr) {
        seasonMultiplier = seasonMgr->getGrowthMultiplier();
    }

    updateSeasonalBlooms(deltaTime, seasonMgr);
    updateGrowth(deltaTime, seasonMultiplier * currentBloomMultiplier);
    updateSoilNutrients(deltaTime);
    updateDetritus(deltaTime);
}

void ProducerSystem::updateGrowth(float deltaTime, float seasonMultiplier) {
    for (auto& patch : grassPatches) {
        if (patch.currentBiomass < patch.maxBiomass) {
            const SoilTile& soil = getSoilAt(patch.position);
            float growthMult = soil.getGrowthMultiplier() * seasonMultiplier;
            float pressureMult = std::max(0.2f, 1.0f - patch.consumptionPressure);

            patch.currentBiomass += patch.regrowthRate * growthMult * pressureMult * deltaTime;
            patch.currentBiomass = std::min(patch.currentBiomass, patch.maxBiomass);
        }
        patch.consumptionPressure = std::max(0.0f, patch.consumptionPressure - 0.1f * deltaTime);
    }

    for (auto& patch : bushPatches) {
        if (patch.currentBiomass < patch.maxBiomass) {
            const SoilTile& soil = getSoilAt(patch.position);
            float growthMult = soil.getGrowthMultiplier() * seasonMultiplier;
            patch.currentBiomass += patch.regrowthRate * growthMult * deltaTime;
            patch.currentBiomass = std::min(patch.currentBiomass, patch.maxBiomass);
        }
        patch.consumptionPressure = std::max(0.0f, patch.consumptionPressure - 0.1f * deltaTime);
    }

    for (auto& patch : treePatches) {
        if (patch.currentBiomass < patch.maxBiomass) {
            const SoilTile& soil = getSoilAt(patch.position);
            float growthMult = soil.getGrowthMultiplier() * seasonMultiplier;
            patch.currentBiomass += patch.regrowthRate * growthMult * deltaTime;
            patch.currentBiomass = std::min(patch.currentBiomass, patch.maxBiomass);
        }
        patch.consumptionPressure = std::max(0.0f, patch.consumptionPressure - 0.05f * deltaTime);
    }

    // Update aquatic patches (plankton regrows fast, seaweed slower)
    for (auto& patch : planktonPatches) {
        if (patch.currentBiomass < patch.maxBiomass) {
            // Plankton grows fast, affected by sunlight (season)
            patch.currentBiomass += patch.regrowthRate * seasonMultiplier * deltaTime;
            patch.currentBiomass = std::min(patch.currentBiomass, patch.maxBiomass);
        }
        patch.consumptionPressure = std::max(0.0f, patch.consumptionPressure - 0.15f * deltaTime);
    }

    for (auto& patch : algaePatches) {
        if (patch.currentBiomass < patch.maxBiomass) {
            patch.currentBiomass += patch.regrowthRate * seasonMultiplier * deltaTime;
            patch.currentBiomass = std::min(patch.currentBiomass, patch.maxBiomass);
        }
        patch.consumptionPressure = std::max(0.0f, patch.consumptionPressure - 0.1f * deltaTime);
    }

    for (auto& patch : seaweedPatches) {
        if (patch.currentBiomass < patch.maxBiomass) {
            patch.currentBiomass += patch.regrowthRate * seasonMultiplier * deltaTime;
            patch.currentBiomass = std::min(patch.currentBiomass, patch.maxBiomass);
        }
        patch.consumptionPressure = std::max(0.0f, patch.consumptionPressure - 0.08f * deltaTime);
    }
}

void ProducerSystem::updateSoilNutrients(float deltaTime) {
    for (int i = 1; i < gridResolution - 1; i++) {
        for (int j = 1; j < gridResolution - 1; j++) {
            SoilTile& tile = soilGrid[i][j];

            tile.nitrogen += 0.001f * deltaTime;
            tile.nitrogen = std::min(tile.nitrogen, 100.0f);

            if (tile.organicMatter > 10.0f) {
                float decomposed = tile.organicMatter * 0.001f * deltaTime;
                tile.organicMatter -= decomposed;
                tile.nitrogen += decomposed * 0.5f;
                tile.phosphorus += decomposed * 0.3f;
            }

            tile.nitrogen -= 0.0005f * deltaTime;
            tile.nitrogen = std::max(tile.nitrogen, 5.0f);
        }
    }
}

float ProducerSystem::consumeAt(const glm::vec3& position, FoodSourceType preferredType, float amount) {
    FoodPatch* patch = findNearestPatch(position, preferredType, 5.0f);

    if (!patch || patch->currentBiomass < 0.1f) {
        return 0.0f;
    }

    float consumed = std::min(amount, patch->currentBiomass);
    patch->currentBiomass -= consumed;
    patch->consumptionPressure = std::min(1.0f, patch->consumptionPressure + 0.2f);

    return consumed * patch->energyPerUnit;
}

FoodPatch* ProducerSystem::findNearestPatch(const glm::vec3& pos, FoodSourceType type, float range) {
    std::vector<FoodPatch>* patches = nullptr;

    switch (type) {
        case FoodSourceType::GRASS:
            patches = &grassPatches;
            break;
        case FoodSourceType::BUSH_BERRY:
            patches = &bushPatches;
            break;
        case FoodSourceType::TREE_FRUIT:
        case FoodSourceType::TREE_LEAF:
            patches = &treePatches;
            break;
        case FoodSourceType::PLANKTON:
            patches = &planktonPatches;
            break;
        case FoodSourceType::ALGAE:
            patches = &algaePatches;
            break;
        case FoodSourceType::SEAWEED:
        case FoodSourceType::KELP:
            patches = &seaweedPatches;
            break;
        default:
            break;
    }

    if (!patches) return nullptr;

    FoodPatch* nearest = nullptr;
    float nearestDist = range;

    for (auto& patch : *patches) {
        if (type == FoodSourceType::TREE_FRUIT || type == FoodSourceType::TREE_LEAF) {
            if (patch.type != type) continue;
        }

        if (!patch.isAvailable()) continue;

        float dist = glm::length(glm::vec2(patch.position.x - pos.x, patch.position.z - pos.z));
        if (dist < nearestDist) {
            nearestDist = dist;
            nearest = &patch;
        }
    }

    return nearest;
}

std::vector<glm::vec3> ProducerSystem::getGrassPositions() const {
    std::vector<glm::vec3> positions;
    for (const auto& patch : grassPatches) {
        if (patch.isAvailable()) {
            positions.push_back(patch.position);
        }
    }
    return positions;
}

std::vector<glm::vec3> ProducerSystem::getBushPositions() const {
    std::vector<glm::vec3> positions;
    for (const auto& patch : bushPatches) {
        if (patch.isAvailable()) {
            positions.push_back(patch.position);
        }
    }
    return positions;
}

std::vector<glm::vec3> ProducerSystem::getTreeFruitPositions() const {
    std::vector<glm::vec3> positions;
    for (const auto& patch : treePatches) {
        if (patch.type == FoodSourceType::TREE_FRUIT && patch.isAvailable()) {
            positions.push_back(patch.position);
        }
    }
    return positions;
}

std::vector<glm::vec3> ProducerSystem::getTreeLeafPositions() const {
    std::vector<glm::vec3> positions;
    for (const auto& patch : treePatches) {
        if (patch.type == FoodSourceType::TREE_LEAF && patch.isAvailable()) {
            positions.push_back(patch.position);
        }
    }
    return positions;
}

std::vector<glm::vec3> ProducerSystem::getAllFoodPositions() const {
    std::vector<glm::vec3> positions;

    for (const auto& patch : grassPatches) {
        if (patch.isAvailable()) positions.push_back(patch.position);
    }
    for (const auto& patch : bushPatches) {
        if (patch.isAvailable()) positions.push_back(patch.position);
    }
    for (const auto& patch : treePatches) {
        if (patch.isAvailable()) positions.push_back(patch.position);
    }

    return positions;
}

void ProducerSystem::addNutrients(const glm::vec3& position, float nitrogen, float phosphorus, float organicMatter) {
    auto [i, j] = worldToSoilIndex(position.x, position.z);

    if (i >= 0 && i < gridResolution && j >= 0 && j < gridResolution) {
        SoilTile& tile = soilGrid[i][j];
        tile.nitrogen = std::min(100.0f, tile.nitrogen + nitrogen);
        tile.phosphorus = std::min(100.0f, tile.phosphorus + phosphorus);
        tile.organicMatter = std::min(100.0f, tile.organicMatter + organicMatter);
    }
}

SoilTile& ProducerSystem::getSoilAt(const glm::vec3& position) {
    auto [i, j] = worldToSoilIndex(position.x, position.z);
    i = std::clamp(i, 0, gridResolution - 1);
    j = std::clamp(j, 0, gridResolution - 1);
    return soilGrid[i][j];
}

const SoilTile& ProducerSystem::getSoilAt(const glm::vec3& position) const {
    auto [i, j] = worldToSoilIndex(position.x, position.z);
    i = std::clamp(i, 0, gridResolution - 1);
    j = std::clamp(j, 0, gridResolution - 1);
    return soilGrid[i][j];
}

std::pair<int, int> ProducerSystem::worldToSoilIndex(float x, float z) const {
    float halfGrid = gridResolution / 2.0f;
    int i = static_cast<int>((x / soilTileSize) + halfGrid);
    int j = static_cast<int>((z / soilTileSize) + halfGrid);
    return {i, j};
}

float ProducerSystem::getTotalBiomass() const {
    return getGrassBiomass() + getBushBiomass() + getTreeBiomass();
}

float ProducerSystem::getGrassBiomass() const {
    float total = 0.0f;
    for (const auto& patch : grassPatches) {
        total += patch.currentBiomass;
    }
    return total;
}

float ProducerSystem::getBushBiomass() const {
    float total = 0.0f;
    for (const auto& patch : bushPatches) {
        total += patch.currentBiomass;
    }
    return total;
}

float ProducerSystem::getTreeBiomass() const {
    float total = 0.0f;
    for (const auto& patch : treePatches) {
        total += patch.currentBiomass;
    }
    return total;
}

int ProducerSystem::getActivePatches() const {
    int count = 0;
    for (const auto& patch : grassPatches) {
        if (patch.isAvailable()) count++;
    }
    for (const auto& patch : bushPatches) {
        if (patch.isAvailable()) count++;
    }
    for (const auto& patch : treePatches) {
        if (patch.isAvailable()) count++;
    }
    for (const auto& patch : planktonPatches) {
        if (patch.isAvailable()) count++;
    }
    for (const auto& patch : algaePatches) {
        if (patch.isAvailable()) count++;
    }
    for (const auto& patch : seaweedPatches) {
        if (patch.isAvailable()) count++;
    }
    return count;
}

// ============================================================================
// Detritus System
// ============================================================================

void ProducerSystem::addDetritus(const glm::vec3& position, float amount) {
    auto [i, j] = worldToSoilIndex(position.x, position.z);
    if (i >= 0 && i < gridResolution && j >= 0 && j < gridResolution) {
        soilGrid[i][j].detritus = std::min(100.0f, soilGrid[i][j].detritus + amount);
    }
}

float ProducerSystem::getDetritusAt(const glm::vec3& position, float radius) const {
    auto [centerI, centerJ] = worldToSoilIndex(position.x, position.z);
    int tileRadius = static_cast<int>(radius / soilTileSize) + 1;

    float totalDetritus = 0.0f;
    int count = 0;

    for (int di = -tileRadius; di <= tileRadius; ++di) {
        for (int dj = -tileRadius; dj <= tileRadius; ++dj) {
            int i = centerI + di;
            int j = centerJ + dj;
            if (i >= 0 && i < gridResolution && j >= 0 && j < gridResolution) {
                totalDetritus += soilGrid[i][j].detritus;
                count++;
            }
        }
    }

    return count > 0 ? totalDetritus / count : 0.0f;
}

float ProducerSystem::consumeDetritus(const glm::vec3& position, float amount) {
    auto [i, j] = worldToSoilIndex(position.x, position.z);
    if (i >= 0 && i < gridResolution && j >= 0 && j < gridResolution) {
        float available = soilGrid[i][j].detritus;
        float consumed = std::min(amount, available);
        soilGrid[i][j].detritus -= consumed;

        // Consuming detritus releases some nutrients back to soil
        soilGrid[i][j].nitrogen += consumed * 0.2f;
        soilGrid[i][j].organicMatter += consumed * 0.3f;

        return consumed;
    }
    return 0.0f;
}

std::vector<glm::vec3> ProducerSystem::getDetritusHotspots() const {
    std::vector<glm::vec3> hotspots;
    const float threshold = 40.0f;  // High detritus threshold

    for (int i = 0; i < gridResolution; ++i) {
        for (int j = 0; j < gridResolution; ++j) {
            if (soilGrid[i][j].detritus > threshold) {
                float x = (i - gridResolution / 2.0f) * soilTileSize;
                float z = (j - gridResolution / 2.0f) * soilTileSize;
                float y = terrain->getHeight(x, z);
                hotspots.push_back(glm::vec3(x, y, z));
            }
        }
    }

    return hotspots;
}

void ProducerSystem::updateDetritus(float deltaTime) {
    // Detritus slowly converts to nutrients and organic matter
    // Also naturally accumulates from plant death/leaf fall
    for (int i = 0; i < gridResolution; ++i) {
        for (int j = 0; j < gridResolution; ++j) {
            SoilTile& tile = soilGrid[i][j];

            // Detritus decay: converts to nutrients over time
            if (tile.detritus > 5.0f) {
                float decayRate = 0.01f * deltaTime;  // 1% per second base
                // Faster decay in warm, moist conditions
                decayRate *= (0.5f + tile.moisture / 200.0f);

                float decayed = tile.detritus * decayRate;
                tile.detritus -= decayed;
                tile.nitrogen += decayed * 0.3f;
                tile.phosphorus += decayed * 0.15f;
                tile.organicMatter += decayed * 0.4f;
            }

            // Natural detritus accumulation (leaf fall, dead roots) - very slow
            // Higher near plants
            tile.detritus += 0.001f * deltaTime;
            tile.detritus = std::min(100.0f, tile.detritus);
        }
    }
}

// ============================================================================
// Seasonal Bloom System
// ============================================================================

void ProducerSystem::updateSeasonalBlooms(float deltaTime, const SeasonManager* seasonMgr) {
    if (!seasonMgr) {
        currentBloomMultiplier = 1.0f;
        activeBloomType = 0;
        return;
    }

    Season season = seasonMgr->getCurrentSeason();
    float progress = seasonMgr->getSeasonProgress();

    // Reset bloom state
    float targetBloom = 1.0f;
    int targetType = 0;

    switch (season) {
        case Season::SPRING:
            // Early spring: gradual bloom buildup
            // Mid-spring (progress 0.3-0.6): peak spring bloom
            if (progress >= 0.2f && progress <= 0.7f) {
                targetBloom = 1.0f + 0.5f * std::sin((progress - 0.2f) / 0.5f * 3.14159f);
                targetType = 1;  // Spring growth bloom
            }
            break;

        case Season::SUMMER:
            // Stable high production
            targetBloom = 1.1f;
            targetType = 0;
            break;

        case Season::FALL:
            // Fungal burst in early fall (mushroom season)
            if (progress >= 0.1f && progress <= 0.4f) {
                targetBloom = 1.0f + 0.3f * std::sin((progress - 0.1f) / 0.3f * 3.14159f);
                targetType = 2;  // Fungal burst

                // Add extra detritus during fall (leaf drop)
                for (int i = 0; i < gridResolution; i += 5) {
                    for (int j = 0; j < gridResolution; j += 5) {
                        soilGrid[i][j].detritus += 0.1f * deltaTime;
                    }
                }
            }
            break;

        case Season::WINTER:
            // Very early winter: brief plankton bloom (if aquatic areas)
            if (progress <= 0.15f) {
                targetBloom = 1.0f + 0.2f * (0.15f - progress) / 0.15f;
                targetType = 3;  // Plankton bloom
            } else {
                targetBloom = 0.5f;  // Winter dormancy
            }
            break;
    }

    // Smooth transition to target bloom
    float blendRate = 0.5f * deltaTime;
    currentBloomMultiplier += (targetBloom - currentBloomMultiplier) * blendRate;
    activeBloomType = targetType;
}

// ============================================================================
// Aquatic Food System
// ============================================================================

void ProducerSystem::generateAquaticPatches(unsigned int seed) {
    std::mt19937 rng(seed + 5000);
    std::uniform_real_distribution<float> posDist(-0.5f, 0.5f);

    float terrainWidth = terrain->getWidth() * terrain->getScale();
    const float WATER_LEVEL = 10.5f;  // Match terrain water level

    // Generate plankton patches (floating in water column, fast regrowth)
    float planktonSpacing = 12.0f;
    int patchCount = static_cast<int>(terrainWidth / planktonSpacing);

    for (int i = 0; i < patchCount; i++) {
        for (int j = 0; j < patchCount; j++) {
            float x = (i - patchCount/2) * planktonSpacing + posDist(rng) * planktonSpacing * 0.5f;
            float z = (j - patchCount/2) * planktonSpacing + posDist(rng) * planktonSpacing * 0.5f;

            // Only spawn in water
            float terrainHeight = terrain->getHeight(x, z);
            if (terrainHeight >= WATER_LEVEL) continue;  // Not water

            // Plankton floats at various depths in the water column
            float waterDepth = WATER_LEVEL - terrainHeight;
            if (waterDepth < 2.0f) continue;  // Too shallow

            // Random depth within water column (prefer upper water)
            float depth = 1.0f + (rng() % 100) / 100.0f * std::min(waterDepth - 2.0f, 8.0f);
            float y = WATER_LEVEL - depth;

            FoodPatch patch;
            patch.position = glm::vec3(x, y, z);
            patch.type = FoodSourceType::PLANKTON;
            patch.maxBiomass = 8.0f;
            patch.currentBiomass = patch.maxBiomass * 0.9f;
            patch.regrowthRate = 0.8f;  // Fast regrowth
            patch.energyPerUnit = 1.5f;  // Low energy per unit (need lots)
            patch.lastConsumedTime = 0.0f;
            patch.consumptionPressure = 0.0f;
            patch.soilNitrogen = 50.0f;
            patch.soilMoisture = 100.0f;

            planktonPatches.push_back(patch);
        }
    }

    // Generate algae patches (on sea floor/rocks, medium regrowth)
    float algaeSpacing = 10.0f;
    patchCount = static_cast<int>(terrainWidth / algaeSpacing);

    for (int i = 0; i < patchCount; i++) {
        for (int j = 0; j < patchCount; j++) {
            float x = (i - patchCount/2) * algaeSpacing + posDist(rng) * algaeSpacing * 0.5f;
            float z = (j - patchCount/2) * algaeSpacing + posDist(rng) * algaeSpacing * 0.5f;

            float terrainHeight = terrain->getHeight(x, z);
            if (terrainHeight >= WATER_LEVEL) continue;

            float waterDepth = WATER_LEVEL - terrainHeight;
            if (waterDepth < 1.0f || waterDepth > 15.0f) continue;  // Shallow to medium depth

            // Skip some randomly
            if ((rng() % 100) > 60) continue;

            // Algae grows on the sea floor
            float y = terrainHeight + 0.2f;

            FoodPatch patch;
            patch.position = glm::vec3(x, y, z);
            patch.type = FoodSourceType::ALGAE;
            patch.maxBiomass = 15.0f;
            patch.currentBiomass = patch.maxBiomass * 0.8f;
            patch.regrowthRate = 0.4f;
            patch.energyPerUnit = 3.0f;
            patch.lastConsumedTime = 0.0f;
            patch.consumptionPressure = 0.0f;
            patch.soilNitrogen = 50.0f;
            patch.soilMoisture = 100.0f;

            algaePatches.push_back(patch);
        }
    }

    // Generate seaweed/kelp patches (larger, slower regrowth, high energy)
    float seaweedSpacing = 18.0f;
    patchCount = static_cast<int>(terrainWidth / seaweedSpacing);

    for (int i = 0; i < patchCount; i++) {
        for (int j = 0; j < patchCount; j++) {
            float x = (i - patchCount/2) * seaweedSpacing + posDist(rng) * seaweedSpacing * 0.5f;
            float z = (j - patchCount/2) * seaweedSpacing + posDist(rng) * seaweedSpacing * 0.5f;

            float terrainHeight = terrain->getHeight(x, z);
            if (terrainHeight >= WATER_LEVEL) continue;

            float waterDepth = WATER_LEVEL - terrainHeight;
            if (waterDepth < 3.0f || waterDepth > 20.0f) continue;  // Medium depth

            // Skip many randomly (seaweed less dense)
            if ((rng() % 100) > 35) continue;

            // Seaweed grows from floor to various heights
            float y = terrainHeight + 1.0f;

            FoodPatch patch;
            patch.position = glm::vec3(x, y, z);
            patch.type = FoodSourceType::SEAWEED;
            patch.maxBiomass = 25.0f;
            patch.currentBiomass = patch.maxBiomass * 0.7f;
            patch.regrowthRate = 0.2f;
            patch.energyPerUnit = 5.0f;
            patch.lastConsumedTime = 0.0f;
            patch.consumptionPressure = 0.0f;
            patch.soilNitrogen = 50.0f;
            patch.soilMoisture = 100.0f;

            seaweedPatches.push_back(patch);
        }
    }

    std::cout << "[AQUATIC FOOD] Generated " << planktonPatches.size() << " plankton patches, "
              << algaePatches.size() << " algae patches, "
              << seaweedPatches.size() << " seaweed patches" << std::endl;
}

std::vector<glm::vec3> ProducerSystem::getPlanktonPositions() const {
    std::vector<glm::vec3> positions;
    for (const auto& patch : planktonPatches) {
        if (patch.isAvailable()) {
            positions.push_back(patch.position);
        }
    }
    return positions;
}

std::vector<glm::vec3> ProducerSystem::getAlgaePositions() const {
    std::vector<glm::vec3> positions;
    for (const auto& patch : algaePatches) {
        if (patch.isAvailable()) {
            positions.push_back(patch.position);
        }
    }
    return positions;
}

std::vector<glm::vec3> ProducerSystem::getSeaweedPositions() const {
    std::vector<glm::vec3> positions;
    for (const auto& patch : seaweedPatches) {
        if (patch.isAvailable()) {
            positions.push_back(patch.position);
        }
    }
    return positions;
}

std::vector<glm::vec3> ProducerSystem::getAllAquaticFoodPositions() const {
    std::vector<glm::vec3> positions;

    for (const auto& patch : planktonPatches) {
        if (patch.isAvailable()) positions.push_back(patch.position);
    }
    for (const auto& patch : algaePatches) {
        if (patch.isAvailable()) positions.push_back(patch.position);
    }
    for (const auto& patch : seaweedPatches) {
        if (patch.isAvailable()) positions.push_back(patch.position);
    }

    return positions;
}
