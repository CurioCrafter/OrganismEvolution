#include "FungiSystem.h"
#include "Terrain.h"
#include "TerrainSampler.h"
#include "ClimateSystem.h"
#include "SeasonManager.h"
#include "DecomposerSystem.h"
#include "../utils/Random.h"
#include <algorithm>
#include <cmath>

// ============================================================================
// Fungus Configuration
// ============================================================================

FungusConfig getFungusConfig(FungusType type) {
    FungusConfig config;
    config.type = type;
    config.isBioluminescent = false;
    config.glowIntensity = 0.0f;

    switch (type) {
        // Edible mushrooms
        case FungusType::BUTTON_MUSHROOM:
            config.name = "Button Mushroom";
            config.preferredMoisture = 0.6f;
            config.preferredTemperature = 18.0f;
            config.minLight = 0.0f;
            config.maxLight = 0.3f;
            config.growthRate = 1.5f;
            config.maxSize = 0.08f;
            config.lifespan = 14.0f;
            config.sporeProductionRate = 100.0f;
            config.sporeSpreadRadius = 3.0f;
            config.nutritionalValue = 5.0f;
            config.toxicity = ToxicityLevel::EDIBLE;
            config.capColor = glm::vec3(0.9f, 0.88f, 0.82f);
            config.stemColor = glm::vec3(0.95f, 0.92f, 0.88f);
            config.gillColor = glm::vec3(0.7f, 0.65f, 0.6f);
            config.decompositionPower = 0.5f;
            break;

        case FungusType::OYSTER_MUSHROOM:
            config.name = "Oyster Mushroom";
            config.preferredMoisture = 0.7f;
            config.preferredTemperature = 15.0f;
            config.minLight = 0.1f;
            config.maxLight = 0.4f;
            config.growthRate = 2.0f;
            config.maxSize = 0.15f;
            config.lifespan = 10.0f;
            config.sporeProductionRate = 200.0f;
            config.sporeSpreadRadius = 5.0f;
            config.nutritionalValue = 6.0f;
            config.toxicity = ToxicityLevel::EDIBLE;
            config.capColor = glm::vec3(0.75f, 0.7f, 0.65f);
            config.stemColor = glm::vec3(0.9f, 0.88f, 0.85f);
            config.gillColor = glm::vec3(0.85f, 0.82f, 0.78f);
            config.decompositionPower = 1.5f;  // Good decomposer
            break;

        case FungusType::CHANTERELLE:
            config.name = "Chanterelle";
            config.preferredMoisture = 0.5f;
            config.preferredTemperature = 16.0f;
            config.minLight = 0.1f;
            config.maxLight = 0.5f;
            config.growthRate = 0.8f;
            config.maxSize = 0.1f;
            config.lifespan = 21.0f;
            config.sporeProductionRate = 50.0f;
            config.sporeSpreadRadius = 2.0f;
            config.nutritionalValue = 8.0f;
            config.toxicity = ToxicityLevel::EDIBLE;
            config.capColor = glm::vec3(1.0f, 0.75f, 0.2f);
            config.stemColor = glm::vec3(0.95f, 0.8f, 0.4f);
            config.gillColor = glm::vec3(0.9f, 0.7f, 0.3f);
            config.decompositionPower = 0.3f;
            break;

        case FungusType::MOREL:
            config.name = "Morel";
            config.preferredMoisture = 0.55f;
            config.preferredTemperature = 12.0f;
            config.minLight = 0.2f;
            config.maxLight = 0.6f;
            config.growthRate = 0.5f;
            config.maxSize = 0.12f;
            config.lifespan = 14.0f;
            config.sporeProductionRate = 80.0f;
            config.sporeSpreadRadius = 4.0f;
            config.nutritionalValue = 10.0f;
            config.toxicity = ToxicityLevel::EDIBLE;
            config.capColor = glm::vec3(0.4f, 0.35f, 0.25f);
            config.stemColor = glm::vec3(0.85f, 0.8f, 0.7f);
            config.gillColor = glm::vec3(0.5f, 0.45f, 0.35f);
            config.decompositionPower = 0.8f;
            break;

        case FungusType::PORCINI:
            config.name = "Porcini";
            config.preferredMoisture = 0.5f;
            config.preferredTemperature = 14.0f;
            config.minLight = 0.1f;
            config.maxLight = 0.4f;
            config.growthRate = 0.6f;
            config.maxSize = 0.2f;
            config.lifespan = 18.0f;
            config.sporeProductionRate = 120.0f;
            config.sporeSpreadRadius = 6.0f;
            config.nutritionalValue = 12.0f;
            config.toxicity = ToxicityLevel::EDIBLE;
            config.capColor = glm::vec3(0.5f, 0.35f, 0.2f);
            config.stemColor = glm::vec3(0.9f, 0.85f, 0.75f);
            config.gillColor = glm::vec3(0.6f, 0.5f, 0.35f);
            config.decompositionPower = 0.6f;
            break;

        // Poisonous mushrooms
        case FungusType::DEATH_CAP:
            config.name = "Death Cap";
            config.preferredMoisture = 0.6f;
            config.preferredTemperature = 17.0f;
            config.minLight = 0.05f;
            config.maxLight = 0.35f;
            config.growthRate = 1.0f;
            config.maxSize = 0.15f;
            config.lifespan = 12.0f;
            config.sporeProductionRate = 150.0f;
            config.sporeSpreadRadius = 5.0f;
            config.nutritionalValue = -50.0f;  // Negative = damage
            config.toxicity = ToxicityLevel::DEADLY;
            config.capColor = glm::vec3(0.65f, 0.7f, 0.5f);
            config.stemColor = glm::vec3(0.9f, 0.9f, 0.85f);
            config.gillColor = glm::vec3(0.95f, 0.95f, 0.9f);
            config.decompositionPower = 0.7f;
            break;

        case FungusType::FLY_AGARIC:
            config.name = "Fly Agaric";
            config.preferredMoisture = 0.5f;
            config.preferredTemperature = 12.0f;
            config.minLight = 0.1f;
            config.maxLight = 0.5f;
            config.growthRate = 0.9f;
            config.maxSize = 0.18f;
            config.lifespan = 14.0f;
            config.sporeProductionRate = 100.0f;
            config.sporeSpreadRadius = 4.0f;
            config.nutritionalValue = -20.0f;
            config.toxicity = ToxicityLevel::TOXIC;
            config.capColor = glm::vec3(0.95f, 0.15f, 0.1f);  // Iconic red
            config.stemColor = glm::vec3(0.95f, 0.95f, 0.9f);
            config.gillColor = glm::vec3(0.98f, 0.98f, 0.95f);
            config.decompositionPower = 0.5f;
            break;

        case FungusType::DESTROYING_ANGEL:
            config.name = "Destroying Angel";
            config.preferredMoisture = 0.55f;
            config.preferredTemperature = 18.0f;
            config.minLight = 0.05f;
            config.maxLight = 0.3f;
            config.growthRate = 0.8f;
            config.maxSize = 0.2f;
            config.lifespan = 10.0f;
            config.sporeProductionRate = 180.0f;
            config.sporeSpreadRadius = 6.0f;
            config.nutritionalValue = -80.0f;
            config.toxicity = ToxicityLevel::DEADLY;
            config.capColor = glm::vec3(0.98f, 0.98f, 0.95f);
            config.stemColor = glm::vec3(1.0f, 1.0f, 0.98f);
            config.gillColor = glm::vec3(0.95f, 0.95f, 0.9f);
            config.decompositionPower = 0.9f;
            break;

        // Bracket fungi
        case FungusType::TURKEY_TAIL:
            config.name = "Turkey Tail";
            config.preferredMoisture = 0.6f;
            config.preferredTemperature = 15.0f;
            config.minLight = 0.1f;
            config.maxLight = 0.6f;
            config.growthRate = 0.3f;
            config.maxSize = 0.15f;
            config.lifespan = 365.0f;  // Very long-lived
            config.sporeProductionRate = 50.0f;
            config.sporeSpreadRadius = 3.0f;
            config.nutritionalValue = 2.0f;
            config.toxicity = ToxicityLevel::EDIBLE;
            config.capColor = glm::vec3(0.5f, 0.4f, 0.3f);
            config.stemColor = glm::vec3(0.4f, 0.35f, 0.28f);
            config.gillColor = glm::vec3(0.6f, 0.55f, 0.45f);
            config.decompositionPower = 3.0f;  // Excellent decomposer
            break;

        case FungusType::ARTISTS_CONK:
            config.name = "Artist's Conk";
            config.preferredMoisture = 0.5f;
            config.preferredTemperature = 14.0f;
            config.minLight = 0.05f;
            config.maxLight = 0.4f;
            config.growthRate = 0.1f;
            config.maxSize = 0.4f;
            config.lifespan = 1000.0f;
            config.sporeProductionRate = 30.0f;
            config.sporeSpreadRadius = 2.0f;
            config.nutritionalValue = 1.0f;
            config.toxicity = ToxicityLevel::EDIBLE;
            config.capColor = glm::vec3(0.6f, 0.55f, 0.45f);
            config.stemColor = glm::vec3(0.4f, 0.35f, 0.3f);
            config.gillColor = glm::vec3(0.9f, 0.85f, 0.75f);
            config.decompositionPower = 2.5f;
            break;

        case FungusType::CHICKEN_OF_WOODS:
            config.name = "Chicken of the Woods";
            config.preferredMoisture = 0.55f;
            config.preferredTemperature = 18.0f;
            config.minLight = 0.15f;
            config.maxLight = 0.5f;
            config.growthRate = 0.5f;
            config.maxSize = 0.35f;
            config.lifespan = 60.0f;
            config.sporeProductionRate = 80.0f;
            config.sporeSpreadRadius = 4.0f;
            config.nutritionalValue = 8.0f;
            config.toxicity = ToxicityLevel::EDIBLE;
            config.capColor = glm::vec3(1.0f, 0.6f, 0.15f);
            config.stemColor = glm::vec3(0.95f, 0.7f, 0.3f);
            config.gillColor = glm::vec3(0.98f, 0.95f, 0.5f);
            config.decompositionPower = 2.0f;
            break;

        // Bioluminescent fungi
        case FungusType::GHOST_MUSHROOM:
            config.name = "Ghost Mushroom";
            config.preferredMoisture = 0.7f;
            config.preferredTemperature = 16.0f;
            config.minLight = 0.0f;
            config.maxLight = 0.2f;
            config.growthRate = 0.6f;
            config.maxSize = 0.12f;
            config.lifespan = 7.0f;
            config.sporeProductionRate = 150.0f;
            config.sporeSpreadRadius = 4.0f;
            config.nutritionalValue = 3.0f;
            config.toxicity = ToxicityLevel::MILDLY_TOXIC;
            config.capColor = glm::vec3(0.85f, 0.9f, 0.85f);
            config.stemColor = glm::vec3(0.9f, 0.95f, 0.9f);
            config.gillColor = glm::vec3(0.7f, 0.9f, 0.75f);
            config.isBioluminescent = true;
            config.glowColor = glm::vec3(0.3f, 1.0f, 0.5f);
            config.glowIntensity = 0.8f;
            config.decompositionPower = 0.8f;
            break;

        case FungusType::JACK_O_LANTERN:
            config.name = "Jack O'Lantern";
            config.preferredMoisture = 0.6f;
            config.preferredTemperature = 18.0f;
            config.minLight = 0.0f;
            config.maxLight = 0.3f;
            config.growthRate = 0.8f;
            config.maxSize = 0.18f;
            config.lifespan = 10.0f;
            config.sporeProductionRate = 120.0f;
            config.sporeSpreadRadius = 5.0f;
            config.nutritionalValue = -15.0f;
            config.toxicity = ToxicityLevel::TOXIC;
            config.capColor = glm::vec3(1.0f, 0.5f, 0.1f);
            config.stemColor = glm::vec3(0.9f, 0.55f, 0.2f);
            config.gillColor = glm::vec3(0.95f, 0.6f, 0.15f);
            config.isBioluminescent = true;
            config.glowColor = glm::vec3(0.3f, 0.8f, 0.2f);
            config.glowIntensity = 0.5f;
            config.decompositionPower = 1.2f;
            break;

        case FungusType::FOXFIRE:
            config.name = "Foxfire";
            config.preferredMoisture = 0.75f;
            config.preferredTemperature = 14.0f;
            config.minLight = 0.0f;
            config.maxLight = 0.15f;
            config.growthRate = 0.2f;
            config.maxSize = 0.05f;
            config.lifespan = 30.0f;
            config.sporeProductionRate = 200.0f;
            config.sporeSpreadRadius = 2.0f;
            config.nutritionalValue = 1.0f;
            config.toxicity = ToxicityLevel::EDIBLE;
            config.capColor = glm::vec3(0.6f, 0.7f, 0.5f);
            config.stemColor = glm::vec3(0.5f, 0.6f, 0.45f);
            config.gillColor = glm::vec3(0.4f, 0.55f, 0.35f);
            config.isBioluminescent = true;
            config.glowColor = glm::vec3(0.2f, 0.9f, 0.4f);
            config.glowIntensity = 1.0f;
            config.decompositionPower = 2.0f;
            break;

        // Alien fungi
        case FungusType::CRYSTAL_SPORE:
            config.name = "Crystal Spore";
            config.preferredMoisture = 0.3f;
            config.preferredTemperature = 5.0f;
            config.minLight = 0.0f;
            config.maxLight = 1.0f;
            config.growthRate = 0.1f;
            config.maxSize = 0.25f;
            config.lifespan = 1000.0f;
            config.sporeProductionRate = 10.0f;
            config.sporeSpreadRadius = 10.0f;
            config.nutritionalValue = 15.0f;
            config.toxicity = ToxicityLevel::EDIBLE;
            config.capColor = glm::vec3(0.7f, 0.85f, 1.0f);
            config.stemColor = glm::vec3(0.6f, 0.75f, 0.95f);
            config.gillColor = glm::vec3(0.8f, 0.9f, 1.0f);
            config.isBioluminescent = true;
            config.glowColor = glm::vec3(0.5f, 0.7f, 1.0f);
            config.glowIntensity = 1.2f;
            config.decompositionPower = 0.1f;
            break;

        case FungusType::PLASMA_CAP:
            config.name = "Plasma Cap";
            config.preferredMoisture = 0.5f;
            config.preferredTemperature = 25.0f;
            config.minLight = 0.0f;
            config.maxLight = 0.8f;
            config.growthRate = 2.0f;
            config.maxSize = 0.15f;
            config.lifespan = 3.0f;
            config.sporeProductionRate = 500.0f;
            config.sporeSpreadRadius = 8.0f;
            config.nutritionalValue = 20.0f;
            config.toxicity = ToxicityLevel::MILDLY_TOXIC;
            config.capColor = glm::vec3(0.3f, 0.1f, 0.8f);
            config.stemColor = glm::vec3(0.2f, 0.15f, 0.5f);
            config.gillColor = glm::vec3(0.5f, 0.2f, 0.9f);
            config.isBioluminescent = true;
            config.glowColor = glm::vec3(0.5f, 0.2f, 1.0f);
            config.glowIntensity = 1.5f;
            config.decompositionPower = 0.5f;
            break;

        case FungusType::TENDRIL_FUNGUS:
            config.name = "Tendril Fungus";
            config.preferredMoisture = 0.8f;
            config.preferredTemperature = 22.0f;
            config.minLight = 0.0f;
            config.maxLight = 0.4f;
            config.growthRate = 1.5f;
            config.maxSize = 0.3f;
            config.lifespan = 20.0f;
            config.sporeProductionRate = 300.0f;
            config.sporeSpreadRadius = 6.0f;
            config.nutritionalValue = 5.0f;
            config.toxicity = ToxicityLevel::MILDLY_TOXIC;
            config.capColor = glm::vec3(0.5f, 0.2f, 0.4f);
            config.stemColor = glm::vec3(0.4f, 0.15f, 0.35f);
            config.gillColor = glm::vec3(0.6f, 0.25f, 0.5f);
            config.isBioluminescent = true;
            config.glowColor = glm::vec3(0.8f, 0.3f, 0.6f);
            config.glowIntensity = 0.7f;
            config.decompositionPower = 3.0f;
            break;

        case FungusType::HIVEMIND_CLUSTER:
            config.name = "Hivemind Cluster";
            config.preferredMoisture = 0.7f;
            config.preferredTemperature = 20.0f;
            config.minLight = 0.0f;
            config.maxLight = 0.3f;
            config.growthRate = 0.5f;
            config.maxSize = 0.5f;
            config.lifespan = 100.0f;
            config.sporeProductionRate = 50.0f;
            config.sporeSpreadRadius = 15.0f;
            config.nutritionalValue = -30.0f;  // Parasitic
            config.toxicity = ToxicityLevel::TOXIC;
            config.capColor = glm::vec3(0.3f, 0.4f, 0.35f);
            config.stemColor = glm::vec3(0.25f, 0.35f, 0.3f);
            config.gillColor = glm::vec3(0.2f, 0.3f, 0.25f);
            config.isBioluminescent = true;
            config.glowColor = glm::vec3(0.4f, 1.0f, 0.6f);
            config.glowIntensity = 0.4f;
            config.decompositionPower = 5.0f;  // Extremely efficient
            break;

        // Specialized types
        case FungusType::PUFFBALL:
            config.name = "Puffball";
            config.preferredMoisture = 0.5f;
            config.preferredTemperature = 16.0f;
            config.minLight = 0.1f;
            config.maxLight = 0.6f;
            config.growthRate = 0.8f;
            config.maxSize = 0.25f;
            config.lifespan = 14.0f;
            config.sporeProductionRate = 1000.0f;  // Huge spore production
            config.sporeSpreadRadius = 10.0f;
            config.nutritionalValue = 4.0f;
            config.toxicity = ToxicityLevel::EDIBLE;
            config.capColor = glm::vec3(0.95f, 0.92f, 0.85f);
            config.stemColor = glm::vec3(0.9f, 0.88f, 0.8f);
            config.gillColor = glm::vec3(0.85f, 0.8f, 0.7f);
            config.decompositionPower = 0.4f;
            break;

        case FungusType::STINKHORN:
            config.name = "Stinkhorn";
            config.preferredMoisture = 0.65f;
            config.preferredTemperature = 20.0f;
            config.minLight = 0.0f;
            config.maxLight = 0.5f;
            config.growthRate = 3.0f;  // Very fast growing
            config.maxSize = 0.2f;
            config.lifespan = 5.0f;
            config.sporeProductionRate = 200.0f;
            config.sporeSpreadRadius = 5.0f;
            config.nutritionalValue = 2.0f;
            config.toxicity = ToxicityLevel::EDIBLE;
            config.capColor = glm::vec3(0.45f, 0.35f, 0.25f);
            config.stemColor = glm::vec3(0.95f, 0.92f, 0.88f);
            config.gillColor = glm::vec3(0.3f, 0.25f, 0.15f);
            config.decompositionPower = 1.5f;
            break;

        case FungusType::CORAL_FUNGUS:
            config.name = "Coral Fungus";
            config.preferredMoisture = 0.6f;
            config.preferredTemperature = 14.0f;
            config.minLight = 0.1f;
            config.maxLight = 0.4f;
            config.growthRate = 0.4f;
            config.maxSize = 0.15f;
            config.lifespan = 21.0f;
            config.sporeProductionRate = 80.0f;
            config.sporeSpreadRadius = 3.0f;
            config.nutritionalValue = 3.0f;
            config.toxicity = ToxicityLevel::EDIBLE;
            config.capColor = glm::vec3(0.95f, 0.85f, 0.7f);
            config.stemColor = glm::vec3(0.9f, 0.8f, 0.65f);
            config.gillColor = glm::vec3(0.85f, 0.75f, 0.6f);
            config.decompositionPower = 0.6f;
            break;

        default:
            config.name = "Unknown Fungus";
            config.preferredMoisture = 0.5f;
            config.preferredTemperature = 15.0f;
            config.minLight = 0.0f;
            config.maxLight = 0.5f;
            config.growthRate = 1.0f;
            config.maxSize = 0.1f;
            config.lifespan = 14.0f;
            config.sporeProductionRate = 100.0f;
            config.sporeSpreadRadius = 3.0f;
            config.nutritionalValue = 3.0f;
            config.toxicity = ToxicityLevel::MILDLY_TOXIC;
            config.capColor = glm::vec3(0.6f, 0.5f, 0.4f);
            config.stemColor = glm::vec3(0.8f, 0.75f, 0.7f);
            config.gillColor = glm::vec3(0.7f, 0.65f, 0.55f);
            config.decompositionPower = 0.5f;
            break;
    }

    return config;
}

// ============================================================================
// FungiSystem Implementation
// ============================================================================

FungiSystem::FungiSystem() {
    // Initialize nutrient grid
    soilNutrients.resize(nutrientGridSize, std::vector<float>(nutrientGridSize, 0.0f));
}

FungiSystem::~FungiSystem() = default;

void FungiSystem::initialize(const Terrain* t, DX12Device* device) {
    terrain = t;
    dx12Device = device;
}

void FungiSystem::generate(unsigned int seed) {
    Random::init();
    mushrooms.clear();
    networks.clear();
    decomposingMatter.clear();

    if (!terrain) return;

    float worldWidth = terrain->getWidth() * terrain->getScale();
    float worldDepth = terrain->getDepth() * terrain->getScale();
    float minX = -worldWidth / 2.0f;
    float minZ = -worldDepth / 2.0f;

    // Generate mushroom clusters
    float clusterSpacing = 30.0f;
    int clustersX = (int)(worldWidth / clusterSpacing);
    int clustersZ = (int)(worldDepth / clusterSpacing);

    for (int cz = 0; cz < clustersZ; cz++) {
        for (int cx = 0; cx < clustersX; cx++) {
            float centerX = minX + (cx + 0.5f) * clusterSpacing + (Random::value() - 0.5f) * clusterSpacing * 0.5f;
            float centerZ = minZ + (cz + 0.5f) * clusterSpacing + (Random::value() - 0.5f) * clusterSpacing * 0.5f;

            if (!isSuitableForFungi(centerX, centerZ)) continue;

            // Random chance to spawn cluster
            if (Random::value() > 0.3f) continue;

            // Get biome for this location
            int biomeType = 0;
            if (climateSystem) {
                ClimateData climate = climateSystem->getClimateAt(centerX, centerZ);
                biomeType = static_cast<int>(climate.getBiome());
            }

            FungusType type = selectFungusTypeForBiome(biomeType);
            int clusterSize = 3 + (int)(Random::value() * 8);

            float height = terrain->getHeight(centerX, centerZ);
            generateMushroomCluster(glm::vec3(centerX, height, centerZ), type, clusterSize, seed + cx * 1000 + cz);
        }
    }

    // Create mycelium networks connecting nearby mushrooms
    connectMushroomsToNetworks();
}

void FungiSystem::generateMushroomCluster(const glm::vec3& center, FungusType type, int count, unsigned int seed) {
    Random::init();
    FungusConfig config = getFungusConfig(type);

    for (int i = 0; i < count; i++) {
        float angle = Random::value() * 2.0f * glm::pi<float>();
        float dist = Random::value() * 2.0f;

        float x = center.x + cos(angle) * dist;
        float z = center.z + sin(angle) * dist;

        if (!terrain->isInBounds(x, z)) continue;
        if (terrain->isWater(x, z)) continue;

        float height = terrain->getHeight(x, z);

        MushroomInstance mushroom;
        mushroom.position = glm::vec3(x, height, z);
        mushroom.rotation = Random::value() * 2.0f * glm::pi<float>();
        mushroom.scale = config.maxSize * (0.3f + Random::value() * 0.7f);
        mushroom.type = type;
        mushroom.age = Random::value() * config.lifespan * 0.5f;
        mushroom.maturity = 0.5f + Random::value() * 0.5f;
        mushroom.health = 0.8f + Random::value() * 0.2f;
        mushroom.isBioluminescent = config.isBioluminescent;
        mushroom.glowColor = config.glowColor;
        mushroom.glowIntensity = config.glowIntensity * (0.8f + Random::value() * 0.4f);
        mushroom.nutritionalValue = config.nutritionalValue;
        mushroom.toxicity = config.toxicity;
        mushroom.sporeTimer = 0.0f;
        mushroom.sporesProduced = 0;

        mushrooms.push_back(mushroom);
    }
}

bool FungiSystem::isSuitableForFungi(float x, float z) const {
    if (!terrain) return false;
    if (!terrain->isInBounds(x, z)) return false;
    if (terrain->isWater(x, z)) return false;

    float height = terrain->getHeight(x, z);
    float normalizedHeight = height / TerrainSampler::HEIGHT_SCALE;

    // No fungi on beaches or mountain peaks
    if (normalizedHeight < TerrainSampler::BEACH_LEVEL + 0.05f) return false;
    if (normalizedHeight > 0.85f) return false;

    // Check biome - prefer forests and damp areas
    if (climateSystem) {
        ClimateData climate = climateSystem->getClimateAt(x, z);
        ClimateBiome biome = climate.getBiome();

        switch (biome) {
            case ClimateBiome::DEEP_OCEAN:
            case ClimateBiome::SHALLOW_WATER:
            case ClimateBiome::BEACH:
            case ClimateBiome::DESERT_HOT:
            case ClimateBiome::DESERT_COLD:
            case ClimateBiome::ICE:
                return false;
            default:
                break;
        }
    }

    return true;
}

FungusType FungiSystem::selectFungusTypeForBiome(int biomeType) const {
    ClimateBiome biome = static_cast<ClimateBiome>(biomeType);

    // Weight different mushroom types based on biome
    std::vector<std::pair<FungusType, float>> candidates;

    switch (biome) {
        case ClimateBiome::TEMPERATE_FOREST:
            candidates = {
                {FungusType::BUTTON_MUSHROOM, 0.2f},
                {FungusType::CHANTERELLE, 0.15f},
                {FungusType::PORCINI, 0.1f},
                {FungusType::FLY_AGARIC, 0.1f},
                {FungusType::TURKEY_TAIL, 0.15f},
                {FungusType::PUFFBALL, 0.1f},
                {FungusType::DEATH_CAP, 0.05f},
                {FungusType::MOREL, 0.15f}
            };
            break;

        case ClimateBiome::BOREAL_FOREST:
            candidates = {
                {FungusType::FLY_AGARIC, 0.2f},
                {FungusType::CHANTERELLE, 0.15f},
                {FungusType::PORCINI, 0.2f},
                {FungusType::TURKEY_TAIL, 0.15f},
                {FungusType::DEATH_CAP, 0.1f},
                {FungusType::GHOST_MUSHROOM, 0.1f},
                {FungusType::ARTISTS_CONK, 0.1f}
            };
            break;

        case ClimateBiome::TROPICAL_RAINFOREST:
            candidates = {
                {FungusType::OYSTER_MUSHROOM, 0.2f},
                {FungusType::GHOST_MUSHROOM, 0.15f},
                {FungusType::JACK_O_LANTERN, 0.1f},
                {FungusType::CORAL_FUNGUS, 0.15f},
                {FungusType::STINKHORN, 0.15f},
                {FungusType::CHICKEN_OF_WOODS, 0.1f},
                {FungusType::DESTROYING_ANGEL, 0.05f},
                {FungusType::FOXFIRE, 0.1f}
            };
            break;

        case ClimateBiome::SWAMP:
            candidates = {
                {FungusType::FOXFIRE, 0.2f},
                {FungusType::GHOST_MUSHROOM, 0.2f},
                {FungusType::JACK_O_LANTERN, 0.15f},
                {FungusType::STINKHORN, 0.15f},
                {FungusType::DESTROYING_ANGEL, 0.1f},
                {FungusType::ARTISTS_CONK, 0.1f},
                {FungusType::TURKEY_TAIL, 0.1f}
            };
            break;

        default:
            candidates = {
                {FungusType::BUTTON_MUSHROOM, 0.25f},
                {FungusType::PUFFBALL, 0.2f},
                {FungusType::TURKEY_TAIL, 0.2f},
                {FungusType::FLY_AGARIC, 0.15f},
                {FungusType::CHANTERELLE, 0.1f},
                {FungusType::CORAL_FUNGUS, 0.1f}
            };
            break;
    }

    // Weighted random selection
    float totalWeight = 0.0f;
    for (const auto& c : candidates) {
        totalWeight += c.second;
    }

    float r = Random::value() * totalWeight;
    float cumulative = 0.0f;

    for (const auto& c : candidates) {
        cumulative += c.second;
        if (r <= cumulative) {
            return c.first;
        }
    }

    return candidates.back().first;
}

void FungiSystem::update(float deltaTime) {
    updateMushroomGrowth(deltaTime);
    updateSporeSpread(deltaTime);
    updateMyceliumNetworks(deltaTime);
    updateDecomposition(deltaTime);
}

void FungiSystem::updateMushroomGrowth(float deltaTime) {
    float dayDelta = deltaTime / 86400.0f;  // Convert seconds to days

    for (auto it = mushrooms.begin(); it != mushrooms.end(); ) {
        MushroomInstance& m = *it;
        FungusConfig config = getFungusConfig(m.type);

        // Age
        m.age += dayDelta;

        // Check if died of old age
        if (m.age > config.lifespan) {
            // Release nutrients when dying
            releaseNutrients(m.position, m.scale * 10.0f);
            it = mushrooms.erase(it);
            continue;
        }

        // Growth
        if (m.maturity < 1.0f) {
            float growthMultiplier = 1.0f;

            // Environmental factors
            if (seasonManager) {
                growthMultiplier *= seasonManager->getGrowthMultiplier();
            }

            m.maturity = std::min(1.0f, m.maturity + config.growthRate * dayDelta * growthMultiplier);
            m.scale = config.maxSize * m.maturity;
        }

        // Health decay
        if (m.age > config.lifespan * 0.7f) {
            float decayRate = (m.age - config.lifespan * 0.7f) / (config.lifespan * 0.3f);
            m.health = std::max(0.0f, m.health - decayRate * dayDelta);
        }

        // Spore production
        if (m.maturity > 0.5f && m.health > 0.3f) {
            m.sporeTimer += dayDelta;
            float sporeInterval = 1.0f / config.sporeProductionRate;
            while (m.sporeTimer >= sporeInterval) {
                m.sporeTimer -= sporeInterval;
                m.sporesProduced++;
            }
        }

        ++it;
    }
}

void FungiSystem::updateSporeSpread(float deltaTime) {
    sporeSpreadTimer += deltaTime;

    // Spread spores periodically
    if (sporeSpreadTimer < 10.0f) return;
    sporeSpreadTimer = 0.0f;

    for (const auto& m : mushrooms) {
        if (m.sporesProduced <= 0) continue;

        FungusConfig config = getFungusConfig(m.type);

        // Chance to spawn new mushroom from spores
        int sporesToProcess = std::min(m.sporesProduced, 10);
        for (int i = 0; i < sporesToProcess; i++) {
            if (Random::value() < 0.01f) {  // 1% chance per batch
                trySpawnFromSpore(m.position, m.type, config.sporeSpreadRadius);
            }
        }
    }
}

void FungiSystem::trySpawnFromSpore(const glm::vec3& origin, FungusType type, float distance) {
    float angle = Random::value() * 2.0f * glm::pi<float>();
    float dist = Random::value() * distance;

    float x = origin.x + cos(angle) * dist;
    float z = origin.z + sin(angle) * dist;

    if (!isSuitableForFungi(x, z)) return;

    // Check if too close to existing mushrooms
    for (const auto& m : mushrooms) {
        float dx = m.position.x - x;
        float dz = m.position.z - z;
        if (dx * dx + dz * dz < 0.25f) return;  // 0.5m minimum spacing
    }

    spawnMushroom(glm::vec3(x, terrain->getHeight(x, z), z), type);
}

void FungiSystem::spawnMushroom(const glm::vec3& position, FungusType type) {
    FungusConfig config = getFungusConfig(type);

    MushroomInstance mushroom;
    mushroom.position = position;
    mushroom.rotation = Random::value() * 2.0f * glm::pi<float>();
    mushroom.scale = config.maxSize * 0.1f;
    mushroom.type = type;
    mushroom.age = 0.0f;
    mushroom.maturity = 0.1f;
    mushroom.health = 1.0f;
    mushroom.isBioluminescent = config.isBioluminescent;
    mushroom.glowColor = config.glowColor;
    mushroom.glowIntensity = config.glowIntensity;
    mushroom.nutritionalValue = config.nutritionalValue;
    mushroom.toxicity = config.toxicity;
    mushroom.sporeTimer = 0.0f;
    mushroom.sporesProduced = 0;

    mushrooms.push_back(mushroom);
}

void FungiSystem::updateMyceliumNetworks(float deltaTime) {
    networkUpdateTimer += deltaTime;

    if (networkUpdateTimer < 5.0f) return;
    networkUpdateTimer = 0.0f;

    // Update nutrient transport through networks
    for (auto& network : networks) {
        // Calculate total nutrients in network
        network.totalNutrients = 0.0f;
        for (const auto& node : network.nodes) {
            network.totalNutrients += node.nutrientLevel;
        }

        // Equalize nutrients across network (nutrient sharing)
        if (network.nodes.size() > 0) {
            float avgNutrients = network.totalNutrients / network.nodes.size();
            for (auto& node : network.nodes) {
                node.nutrientLevel = glm::mix(node.nutrientLevel, avgNutrients, 0.3f);
            }
        }
    }
}

void FungiSystem::updateDecomposition(float deltaTime) {
    float dayDelta = deltaTime / 86400.0f;

    for (auto it = decomposingMatter.begin(); it != decomposingMatter.end(); ) {
        DecomposingMatter& matter = *it;

        // Find nearby fungi decomposition power
        float localDecompositionPower = 0.0f;
        for (const auto& m : mushrooms) {
            float dx = m.position.x - matter.position.x;
            float dz = m.position.z - matter.position.z;
            float dist = std::sqrt(dx * dx + dz * dz);

            if (dist < 5.0f) {
                FungusConfig config = getFungusConfig(m.type);
                localDecompositionPower += config.decompositionPower * (1.0f - dist / 5.0f);
            }
        }

        // Base decomposition rate
        float baseRate = 0.01f;  // 1% per day baseline
        switch (matter.type) {
            case DecomposingMatter::MatterType::LEAF_LITTER:
                baseRate = 0.05f;
                break;
            case DecomposingMatter::MatterType::DEAD_VEGETATION:
                baseRate = 0.03f;
                break;
            case DecomposingMatter::MatterType::DEAD_CREATURE:
                baseRate = 0.02f;
                break;
            case DecomposingMatter::MatterType::FALLEN_TREE:
                baseRate = 0.005f;
                break;
        }

        // Apply fungi boost
        matter.decompositionRate = baseRate * (1.0f + localDecompositionPower);

        // Decompose
        float decomposed = matter.remainingMass * matter.decompositionRate * dayDelta;
        matter.remainingMass -= decomposed;
        matter.nutrientsReleased += decomposed * 0.5f;
        matter.decayProgress = 1.0f - (matter.remainingMass / (matter.remainingMass + matter.nutrientsReleased));

        // Release nutrients to soil
        releaseNutrients(matter.position, decomposed * 0.5f);

        // Remove when fully decomposed
        if (matter.remainingMass < 0.01f) {
            it = decomposingMatter.erase(it);
        } else {
            ++it;
        }
    }
}

void FungiSystem::connectMushroomsToNetworks() {
    // Simple clustering algorithm to create mycelium networks
    float networkRadius = 15.0f;
    std::vector<bool> assigned(mushrooms.size(), false);

    for (size_t i = 0; i < mushrooms.size(); i++) {
        if (assigned[i]) continue;

        // Start new network
        MyceliumNetwork network;
        network.networkId = nextNetworkId++;
        network.center = mushrooms[i].position;
        network.radius = networkRadius;
        network.mushroomCount = 0;

        // Find all mushrooms in range
        std::vector<size_t> clusterMembers;
        clusterMembers.push_back(i);
        assigned[i] = true;

        for (size_t j = i + 1; j < mushrooms.size(); j++) {
            if (assigned[j]) continue;

            float dx = mushrooms[j].position.x - network.center.x;
            float dz = mushrooms[j].position.z - network.center.z;
            float dist = std::sqrt(dx * dx + dz * dz);

            if (dist < networkRadius) {
                clusterMembers.push_back(j);
                assigned[j] = true;
            }
        }

        // Create mycelium nodes connecting cluster
        if (clusterMembers.size() > 1) {
            for (size_t idx : clusterMembers) {
                MyceliumNode node;
                node.position = mushrooms[idx].position;
                node.position.y -= 0.1f;  // Underground
                node.nutrientLevel = 10.0f;
                node.decompositionRate = 0.5f;
                node.isActive = true;
                node.thickness = 0.02f;
                node.color = glm::vec3(0.9f, 0.85f, 0.7f);

                network.nodes.push_back(node);
            }

            // Connect nodes (simple linear chain)
            for (size_t n = 0; n < network.nodes.size() - 1; n++) {
                network.nodes[n].connections.push_back(n + 1);
                network.nodes[n + 1].connections.push_back(n);
            }

            network.mushroomCount = clusterMembers.size();
            network.decompositionPower = network.mushroomCount * 0.5f;

            networks.push_back(network);
        }
    }
}

void FungiSystem::createNetwork(const glm::vec3& center, float radius) {
    MyceliumNetwork network;
    network.networkId = nextNetworkId++;
    network.center = center;
    network.radius = radius;
    network.totalNutrients = 0.0f;
    network.decompositionPower = 0.0f;
    network.mushroomCount = 0;

    networks.push_back(network);
}

void FungiSystem::releaseNutrients(const glm::vec3& position, float amount) {
    auto [gx, gz] = worldToNutrientGrid(position.x, position.z);

    if (gx >= 0 && gx < nutrientGridSize && gz >= 0 && gz < nutrientGridSize) {
        soilNutrients[gx][gz] += amount;

        // Spread to neighbors
        for (int dx = -1; dx <= 1; dx++) {
            for (int dz = -1; dz <= 1; dz++) {
                int nx = gx + dx;
                int nz = gz + dz;
                if (nx >= 0 && nx < nutrientGridSize && nz >= 0 && nz < nutrientGridSize) {
                    soilNutrients[nx][nz] += amount * 0.1f;
                }
            }
        }
    }
}

std::pair<int, int> FungiSystem::worldToNutrientGrid(float x, float z) const {
    float worldWidth = nutrientGridSize * nutrientTileSize;
    int gx = (int)((x + worldWidth / 2.0f) / nutrientTileSize);
    int gz = (int)((z + worldWidth / 2.0f) / nutrientTileSize);
    return {gx, gz};
}

// ============================================================================
// Public Interface
// ============================================================================

void FungiSystem::addDecomposingMatter(const glm::vec3& position, float mass, DecomposingMatter::MatterType type) {
    DecomposingMatter matter;
    matter.position = position;
    matter.remainingMass = mass;
    matter.decompositionRate = 0.01f;
    matter.nutrientsReleased = 0.0f;
    matter.type = type;
    matter.decayProgress = 0.0f;

    decomposingMatter.push_back(matter);
}

float FungiSystem::getNutrientsAt(const glm::vec3& position, float radius) const {
    auto [gx, gz] = worldToNutrientGrid(position.x, position.z);

    float total = 0.0f;
    int tilesInRadius = (int)(radius / nutrientTileSize) + 1;

    for (int dx = -tilesInRadius; dx <= tilesInRadius; dx++) {
        for (int dz = -tilesInRadius; dz <= tilesInRadius; dz++) {
            int nx = gx + dx;
            int nz = gz + dz;
            if (nx >= 0 && nx < nutrientGridSize && nz >= 0 && nz < nutrientGridSize) {
                total += soilNutrients[nx][nz];
            }
        }
    }

    return total;
}

const MushroomInstance* FungiSystem::findNearestMushroom(const glm::vec3& position, float radius) const {
    const MushroomInstance* nearest = nullptr;
    float nearestDist = radius * radius;

    for (const auto& m : mushrooms) {
        float dx = m.position.x - position.x;
        float dz = m.position.z - position.z;
        float distSq = dx * dx + dz * dz;

        if (distSq < nearestDist) {
            nearestDist = distSq;
            nearest = &m;
        }
    }

    return nearest;
}

float FungiSystem::consumeMushroom(const glm::vec3& position, float radius) {
    for (auto it = mushrooms.begin(); it != mushrooms.end(); ++it) {
        float dx = it->position.x - position.x;
        float dz = it->position.z - position.z;
        float dist = std::sqrt(dx * dx + dz * dz);

        if (dist < radius) {
            float nutrition = it->nutritionalValue * it->maturity;
            mushrooms.erase(it);
            return nutrition;
        }
    }

    return 0.0f;
}

std::vector<glm::vec3> FungiSystem::getEdibleMushroomPositions() const {
    std::vector<glm::vec3> positions;

    for (const auto& m : mushrooms) {
        if (m.toxicity == ToxicityLevel::EDIBLE && m.maturity > 0.5f) {
            positions.push_back(m.position);
        }
    }

    return positions;
}

std::vector<std::pair<glm::vec3, glm::vec3>> FungiSystem::getBioluminescentPositions() const {
    std::vector<std::pair<glm::vec3, glm::vec3>> results;

    for (const auto& m : mushrooms) {
        if (m.isBioluminescent && m.maturity > 0.3f) {
            glm::vec3 effectiveColor = m.glowColor * m.glowIntensity * m.health;
            results.push_back({m.position, effectiveColor});
        }
    }

    return results;
}

float FungiSystem::getTotalDecompositionPower() const {
    float total = 0.0f;
    for (const auto& m : mushrooms) {
        FungusConfig config = getFungusConfig(m.type);
        total += config.decompositionPower * m.health;
    }
    return total;
}

float FungiSystem::getTotalNutrients() const {
    float total = 0.0f;
    for (const auto& row : soilNutrients) {
        for (float n : row) {
            total += n;
        }
    }
    return total;
}

void FungiSystem::render(ID3D12GraphicsCommandList* /*commandList*/, const glm::vec3& /*cameraPos*/) {
    // Fungi rendering is handled by main renderer via getMushrooms()
    // This function is intentionally a no-op - call getMushrooms() to get render data
}
