#include "AquaticPlants.h"
#include "Terrain.h"
#include "TerrainSampler.h"
#include "ClimateSystem.h"
#include "SeasonManager.h"
#include "WeatherSystem.h"
#ifndef USE_FORGE_ENGINE
#include "../graphics/DX12Device.h"
#endif
#include <random>
#include <algorithm>
#include <cmath>

// ===== Configuration Functions =====

AquaticPlantConfig getAquaticPlantConfig(AquaticPlantType type) {
    AquaticPlantConfig config;
    config.type = type;
    config.isBioluminescent = false;
    config.glowIntensity = 0.0f;

    switch (type) {
        // Kelp types
        case AquaticPlantType::KELP_GIANT:
            config.name = "Giant Kelp";
            config.minHeight = 15.0f;
            config.maxHeight = 45.0f;
            config.minScale = 0.8f;
            config.maxScale = 1.2f;
            config.minSegments = 20;
            config.maxSegments = 60;
            config.preferredZone = WaterZone::MEDIUM;
            config.minDepth = 5.0f;
            config.maxDepth = 40.0f;
            config.minTemperature = 8.0f;
            config.maxTemperature = 20.0f;
            config.minSalinity = 0.8f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.3f;
            config.growthRate = 0.5f;  // Can grow 0.5m per day
            config.oxygenProduction = 2.0f;
            config.carbonSequestration = 1.5f;
            config.primaryColor = {0.2f, 0.35f, 0.15f};
            config.secondaryColor = {0.25f, 0.4f, 0.18f};
            config.swaySpeed = 0.5f;
            config.swayAmount = 0.3f;
            config.shelterFactor = 0.8f;
            config.foodValue = 0.6f;
            config.attractsFish = true;
            config.fishAttractionRadius = 10.0f;
            break;

        case AquaticPlantType::KELP_BULL:
            config.name = "Bull Kelp";
            config.minHeight = 8.0f;
            config.maxHeight = 20.0f;
            config.minScale = 0.7f;
            config.maxScale = 1.1f;
            config.minSegments = 10;
            config.maxSegments = 30;
            config.preferredZone = WaterZone::SHALLOW;
            config.minDepth = 2.0f;
            config.maxDepth = 20.0f;
            config.minTemperature = 6.0f;
            config.maxTemperature = 18.0f;
            config.minSalinity = 0.7f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.4f;
            config.growthRate = 0.3f;
            config.oxygenProduction = 1.5f;
            config.carbonSequestration = 1.0f;
            config.primaryColor = {0.3f, 0.28f, 0.12f};
            config.secondaryColor = {0.35f, 0.32f, 0.15f};
            config.swaySpeed = 0.4f;
            config.swayAmount = 0.25f;
            config.shelterFactor = 0.6f;
            config.foodValue = 0.5f;
            config.attractsFish = true;
            config.fishAttractionRadius = 8.0f;
            break;

        case AquaticPlantType::KELP_RIBBON:
            config.name = "Ribbon Kelp";
            config.minHeight = 3.0f;
            config.maxHeight = 10.0f;
            config.minScale = 0.6f;
            config.maxScale = 1.0f;
            config.minSegments = 5;
            config.maxSegments = 15;
            config.preferredZone = WaterZone::SHALLOW;
            config.minDepth = 1.0f;
            config.maxDepth = 15.0f;
            config.minTemperature = 8.0f;
            config.maxTemperature = 22.0f;
            config.minSalinity = 0.6f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.5f;
            config.growthRate = 0.2f;
            config.oxygenProduction = 1.0f;
            config.carbonSequestration = 0.8f;
            config.primaryColor = {0.15f, 0.3f, 0.1f};
            config.secondaryColor = {0.18f, 0.35f, 0.12f};
            config.swaySpeed = 0.6f;
            config.swayAmount = 0.4f;
            config.shelterFactor = 0.4f;
            config.foodValue = 0.4f;
            config.attractsFish = true;
            config.fishAttractionRadius = 5.0f;
            break;

        // Seaweed types
        case AquaticPlantType::SEAWEED_GREEN:
            config.name = "Green Seaweed";
            config.minHeight = 0.3f;
            config.maxHeight = 1.5f;
            config.minScale = 0.5f;
            config.maxScale = 1.0f;
            config.minSegments = 3;
            config.maxSegments = 8;
            config.preferredZone = WaterZone::TIDAL;
            config.minDepth = 0.0f;
            config.maxDepth = 8.0f;
            config.minTemperature = 5.0f;
            config.maxTemperature = 28.0f;
            config.minSalinity = 0.3f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.6f;
            config.growthRate = 0.15f;
            config.oxygenProduction = 0.5f;
            config.carbonSequestration = 0.3f;
            config.primaryColor = {0.2f, 0.5f, 0.15f};
            config.secondaryColor = {0.25f, 0.55f, 0.2f};
            config.swaySpeed = 0.8f;
            config.swayAmount = 0.3f;
            config.shelterFactor = 0.2f;
            config.foodValue = 0.7f;
            config.attractsFish = false;
            config.fishAttractionRadius = 2.0f;
            break;

        case AquaticPlantType::SEAWEED_BROWN:
            config.name = "Brown Seaweed";
            config.minHeight = 0.5f;
            config.maxHeight = 2.0f;
            config.minScale = 0.6f;
            config.maxScale = 1.1f;
            config.minSegments = 4;
            config.maxSegments = 10;
            config.preferredZone = WaterZone::TIDAL;
            config.minDepth = 0.0f;
            config.maxDepth = 10.0f;
            config.minTemperature = 4.0f;
            config.maxTemperature = 25.0f;
            config.minSalinity = 0.5f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.5f;
            config.growthRate = 0.12f;
            config.oxygenProduction = 0.6f;
            config.carbonSequestration = 0.4f;
            config.primaryColor = {0.35f, 0.28f, 0.12f};
            config.secondaryColor = {0.4f, 0.32f, 0.15f};
            config.swaySpeed = 0.7f;
            config.swayAmount = 0.25f;
            config.shelterFactor = 0.25f;
            config.foodValue = 0.6f;
            config.attractsFish = false;
            config.fishAttractionRadius = 2.0f;
            break;

        case AquaticPlantType::SEAWEED_RED:
            config.name = "Red Seaweed";
            config.minHeight = 0.2f;
            config.maxHeight = 0.8f;
            config.minScale = 0.4f;
            config.maxScale = 0.9f;
            config.minSegments = 2;
            config.maxSegments = 6;
            config.preferredZone = WaterZone::SHALLOW;
            config.minDepth = 1.0f;
            config.maxDepth = 30.0f;
            config.minTemperature = 10.0f;
            config.maxTemperature = 25.0f;
            config.minSalinity = 0.7f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.2f;  // Can grow in low light
            config.growthRate = 0.08f;
            config.oxygenProduction = 0.4f;
            config.carbonSequestration = 0.3f;
            config.primaryColor = {0.5f, 0.15f, 0.18f};
            config.secondaryColor = {0.55f, 0.2f, 0.22f};
            config.swaySpeed = 0.9f;
            config.swayAmount = 0.2f;
            config.shelterFactor = 0.15f;
            config.foodValue = 0.8f;
            config.attractsFish = false;
            config.fishAttractionRadius = 1.0f;
            break;

        // Surface plants
        case AquaticPlantType::LILY_PAD:
            config.name = "Lily Pad";
            config.minHeight = 0.02f;
            config.maxHeight = 0.05f;
            config.minScale = 0.3f;
            config.maxScale = 0.8f;
            config.minSegments = 1;
            config.maxSegments = 1;
            config.preferredZone = WaterZone::FRESHWATER;
            config.minDepth = 0.5f;
            config.maxDepth = 3.0f;
            config.minTemperature = 15.0f;
            config.maxTemperature = 30.0f;
            config.minSalinity = 0.0f;
            config.maxSalinity = 0.1f;
            config.minLight = 0.7f;
            config.growthRate = 0.1f;
            config.oxygenProduction = 0.3f;
            config.carbonSequestration = 0.2f;
            config.primaryColor = {0.2f, 0.45f, 0.15f};
            config.secondaryColor = {0.25f, 0.5f, 0.18f};
            config.swaySpeed = 0.2f;
            config.swayAmount = 0.05f;
            config.shelterFactor = 0.5f;  // Good shade for fish
            config.foodValue = 0.3f;
            config.attractsFish = true;
            config.fishAttractionRadius = 3.0f;
            break;

        case AquaticPlantType::WATER_LILY:
            config.name = "Water Lily";
            config.minHeight = 0.05f;
            config.maxHeight = 0.15f;
            config.minScale = 0.4f;
            config.maxScale = 1.0f;
            config.minSegments = 1;
            config.maxSegments = 1;
            config.preferredZone = WaterZone::FRESHWATER;
            config.minDepth = 0.5f;
            config.maxDepth = 2.5f;
            config.minTemperature = 18.0f;
            config.maxTemperature = 32.0f;
            config.minSalinity = 0.0f;
            config.maxSalinity = 0.05f;
            config.minLight = 0.8f;
            config.growthRate = 0.08f;
            config.oxygenProduction = 0.4f;
            config.carbonSequestration = 0.25f;
            config.primaryColor = {0.22f, 0.48f, 0.18f};
            config.secondaryColor = {1.0f, 1.0f, 1.0f};  // White flower
            config.swaySpeed = 0.15f;
            config.swayAmount = 0.03f;
            config.shelterFactor = 0.6f;
            config.foodValue = 0.4f;
            config.attractsFish = true;
            config.fishAttractionRadius = 4.0f;
            break;

        case AquaticPlantType::LOTUS:
            config.name = "Lotus";
            config.minHeight = 0.1f;
            config.maxHeight = 0.3f;
            config.minScale = 0.5f;
            config.maxScale = 1.2f;
            config.minSegments = 1;
            config.maxSegments = 1;
            config.preferredZone = WaterZone::FRESHWATER;
            config.minDepth = 0.3f;
            config.maxDepth = 2.0f;
            config.minTemperature = 20.0f;
            config.maxTemperature = 35.0f;
            config.minSalinity = 0.0f;
            config.maxSalinity = 0.02f;
            config.minLight = 0.85f;
            config.growthRate = 0.06f;
            config.oxygenProduction = 0.5f;
            config.carbonSequestration = 0.3f;
            config.primaryColor = {0.25f, 0.5f, 0.2f};
            config.secondaryColor = {1.0f, 0.7f, 0.8f};  // Pink flower
            config.swaySpeed = 0.1f;
            config.swayAmount = 0.02f;
            config.shelterFactor = 0.5f;
            config.foodValue = 0.5f;
            config.attractsFish = true;
            config.fishAttractionRadius = 5.0f;
            break;

        // Coral types
        case AquaticPlantType::CORAL_BRAIN:
            config.name = "Brain Coral";
            config.minHeight = 0.3f;
            config.maxHeight = 1.5f;
            config.minScale = 0.5f;
            config.maxScale = 2.0f;
            config.minSegments = 1;
            config.maxSegments = 1;
            config.preferredZone = WaterZone::SHALLOW;
            config.minDepth = 2.0f;
            config.maxDepth = 25.0f;
            config.minTemperature = 23.0f;
            config.maxTemperature = 29.0f;
            config.minSalinity = 0.85f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.5f;
            config.growthRate = 0.01f;  // Very slow
            config.oxygenProduction = 0.8f;
            config.carbonSequestration = 0.5f;
            config.primaryColor = {0.6f, 0.5f, 0.35f};
            config.secondaryColor = {0.55f, 0.45f, 0.3f};
            config.swaySpeed = 0.0f;
            config.swayAmount = 0.0f;
            config.shelterFactor = 0.7f;
            config.foodValue = 0.1f;
            config.attractsFish = true;
            config.fishAttractionRadius = 8.0f;
            break;

        case AquaticPlantType::CORAL_STAGHORN:
            config.name = "Staghorn Coral";
            config.minHeight = 0.5f;
            config.maxHeight = 3.0f;
            config.minScale = 0.4f;
            config.maxScale = 1.5f;
            config.minSegments = 1;
            config.maxSegments = 1;
            config.preferredZone = WaterZone::SHALLOW;
            config.minDepth = 1.0f;
            config.maxDepth = 20.0f;
            config.minTemperature = 24.0f;
            config.maxTemperature = 29.0f;
            config.minSalinity = 0.9f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.6f;
            config.growthRate = 0.02f;
            config.oxygenProduction = 1.0f;
            config.carbonSequestration = 0.6f;
            config.primaryColor = {0.9f, 0.75f, 0.5f};
            config.secondaryColor = {0.85f, 0.7f, 0.45f};
            config.swaySpeed = 0.0f;
            config.swayAmount = 0.0f;
            config.shelterFactor = 0.9f;  // Excellent shelter
            config.foodValue = 0.05f;
            config.attractsFish = true;
            config.fishAttractionRadius = 12.0f;
            break;

        case AquaticPlantType::CORAL_TABLE:
            config.name = "Table Coral";
            config.minHeight = 0.2f;
            config.maxHeight = 1.0f;
            config.minScale = 0.8f;
            config.maxScale = 3.0f;
            config.minSegments = 1;
            config.maxSegments = 1;
            config.preferredZone = WaterZone::SHALLOW;
            config.minDepth = 3.0f;
            config.maxDepth = 15.0f;
            config.minTemperature = 25.0f;
            config.maxTemperature = 29.0f;
            config.minSalinity = 0.9f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.7f;
            config.growthRate = 0.015f;
            config.oxygenProduction = 1.2f;
            config.carbonSequestration = 0.7f;
            config.primaryColor = {0.45f, 0.55f, 0.4f};
            config.secondaryColor = {0.4f, 0.5f, 0.35f};
            config.swaySpeed = 0.0f;
            config.swayAmount = 0.0f;
            config.shelterFactor = 0.85f;
            config.foodValue = 0.08f;
            config.attractsFish = true;
            config.fishAttractionRadius = 15.0f;
            break;

        case AquaticPlantType::CORAL_FAN:
            config.name = "Sea Fan";
            config.minHeight = 0.3f;
            config.maxHeight = 2.0f;
            config.minScale = 0.5f;
            config.maxScale = 1.5f;
            config.minSegments = 1;
            config.maxSegments = 1;
            config.preferredZone = WaterZone::MEDIUM;
            config.minDepth = 5.0f;
            config.maxDepth = 40.0f;
            config.minTemperature = 20.0f;
            config.maxTemperature = 28.0f;
            config.minSalinity = 0.85f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.3f;
            config.growthRate = 0.008f;
            config.oxygenProduction = 0.6f;
            config.carbonSequestration = 0.4f;
            config.primaryColor = {0.8f, 0.4f, 0.5f};
            config.secondaryColor = {0.75f, 0.35f, 0.45f};
            config.swaySpeed = 0.3f;  // Fans sway
            config.swayAmount = 0.15f;
            config.shelterFactor = 0.5f;
            config.foodValue = 0.02f;
            config.attractsFish = true;
            config.fishAttractionRadius = 6.0f;
            break;

        case AquaticPlantType::ANEMONE:
            config.name = "Sea Anemone";
            config.minHeight = 0.1f;
            config.maxHeight = 0.5f;
            config.minScale = 0.3f;
            config.maxScale = 1.0f;
            config.minSegments = 1;
            config.maxSegments = 1;
            config.preferredZone = WaterZone::SHALLOW;
            config.minDepth = 0.5f;
            config.maxDepth = 30.0f;
            config.minTemperature = 15.0f;
            config.maxTemperature = 28.0f;
            config.minSalinity = 0.7f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.4f;
            config.growthRate = 0.005f;
            config.oxygenProduction = 0.3f;
            config.carbonSequestration = 0.2f;
            config.primaryColor = {0.9f, 0.3f, 0.4f};
            config.secondaryColor = {0.95f, 0.6f, 0.2f};
            config.swaySpeed = 0.5f;
            config.swayAmount = 0.1f;
            config.shelterFactor = 0.4f;  // Home for clownfish
            config.foodValue = 0.0f;  // Predatory
            config.attractsFish = true;
            config.fishAttractionRadius = 3.0f;
            break;

        // Emergent plants
        case AquaticPlantType::CATTAIL:
            config.name = "Cattail";
            config.minHeight = 1.5f;
            config.maxHeight = 3.0f;
            config.minScale = 0.7f;
            config.maxScale = 1.0f;
            config.minSegments = 1;
            config.maxSegments = 1;
            config.preferredZone = WaterZone::FRESHWATER;
            config.minDepth = 0.1f;
            config.maxDepth = 1.5f;
            config.minTemperature = 5.0f;
            config.maxTemperature = 35.0f;
            config.minSalinity = 0.0f;
            config.maxSalinity = 0.2f;
            config.minLight = 0.6f;
            config.growthRate = 0.2f;
            config.oxygenProduction = 0.8f;
            config.carbonSequestration = 0.5f;
            config.primaryColor = {0.35f, 0.45f, 0.2f};
            config.secondaryColor = {0.4f, 0.25f, 0.15f};  // Brown cattail head
            config.swaySpeed = 0.3f;
            config.swayAmount = 0.1f;
            config.shelterFactor = 0.7f;
            config.foodValue = 0.6f;
            config.attractsFish = true;
            config.fishAttractionRadius = 5.0f;
            break;

        case AquaticPlantType::REED:
            config.name = "Reed";
            config.minHeight = 2.0f;
            config.maxHeight = 4.0f;
            config.minScale = 0.6f;
            config.maxScale = 1.0f;
            config.minSegments = 1;
            config.maxSegments = 1;
            config.preferredZone = WaterZone::FRESHWATER;
            config.minDepth = 0.0f;
            config.maxDepth = 2.0f;
            config.minTemperature = 0.0f;
            config.maxTemperature = 35.0f;
            config.minSalinity = 0.0f;
            config.maxSalinity = 0.3f;
            config.minLight = 0.5f;
            config.growthRate = 0.25f;
            config.oxygenProduction = 1.0f;
            config.carbonSequestration = 0.6f;
            config.primaryColor = {0.4f, 0.5f, 0.25f};
            config.secondaryColor = {0.45f, 0.55f, 0.28f};
            config.swaySpeed = 0.4f;
            config.swayAmount = 0.15f;
            config.shelterFactor = 0.8f;
            config.foodValue = 0.4f;
            config.attractsFish = true;
            config.fishAttractionRadius = 6.0f;
            break;

        case AquaticPlantType::MANGROVE_PROP_ROOT:
            config.name = "Mangrove Roots";
            config.minHeight = 1.0f;
            config.maxHeight = 3.0f;
            config.minScale = 0.8f;
            config.maxScale = 2.0f;
            config.minSegments = 5;
            config.maxSegments = 15;
            config.preferredZone = WaterZone::BRACKISH;
            config.minDepth = 0.0f;
            config.maxDepth = 2.0f;
            config.minTemperature = 20.0f;
            config.maxTemperature = 35.0f;
            config.minSalinity = 0.3f;
            config.maxSalinity = 0.8f;
            config.minLight = 0.6f;
            config.growthRate = 0.05f;
            config.oxygenProduction = 1.5f;
            config.carbonSequestration = 2.0f;  // Excellent carbon sink
            config.primaryColor = {0.3f, 0.22f, 0.15f};
            config.secondaryColor = {0.35f, 0.25f, 0.18f};
            config.swaySpeed = 0.0f;
            config.swayAmount = 0.0f;
            config.shelterFactor = 0.95f;  // Nursery habitat
            config.foodValue = 0.2f;
            config.attractsFish = true;
            config.fishAttractionRadius = 20.0f;
            break;

        // Alien aquatic plants
        case AquaticPlantType::BIOLUMINESCENT_KELP:
            config.name = "Bioluminescent Kelp";
            config.minHeight = 10.0f;
            config.maxHeight = 30.0f;
            config.minScale = 0.7f;
            config.maxScale = 1.3f;
            config.minSegments = 15;
            config.maxSegments = 40;
            config.preferredZone = WaterZone::DEEP;
            config.minDepth = 20.0f;
            config.maxDepth = 100.0f;
            config.minTemperature = 4.0f;
            config.maxTemperature = 15.0f;
            config.minSalinity = 0.8f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.0f;  // Doesn't need light
            config.growthRate = 0.3f;
            config.oxygenProduction = 1.5f;
            config.carbonSequestration = 1.0f;
            config.primaryColor = {0.1f, 0.3f, 0.4f};
            config.secondaryColor = {0.2f, 0.8f, 0.9f};
            config.swaySpeed = 0.4f;
            config.swayAmount = 0.35f;
            config.isBioluminescent = true;
            config.glowIntensity = 0.7f;
            config.shelterFactor = 0.7f;
            config.foodValue = 0.4f;
            config.attractsFish = true;
            config.fishAttractionRadius = 15.0f;
            break;

        case AquaticPlantType::CRYSTAL_CORAL:
            config.name = "Crystal Coral";
            config.minHeight = 0.5f;
            config.maxHeight = 2.5f;
            config.minScale = 0.6f;
            config.maxScale = 2.0f;
            config.minSegments = 1;
            config.maxSegments = 1;
            config.preferredZone = WaterZone::MEDIUM;
            config.minDepth = 10.0f;
            config.maxDepth = 50.0f;
            config.minTemperature = 5.0f;
            config.maxTemperature = 25.0f;
            config.minSalinity = 0.5f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.1f;
            config.growthRate = 0.005f;
            config.oxygenProduction = 0.2f;
            config.carbonSequestration = 0.1f;
            config.primaryColor = {0.7f, 0.8f, 1.0f};
            config.secondaryColor = {0.8f, 0.9f, 1.0f};
            config.swaySpeed = 0.0f;
            config.swayAmount = 0.0f;
            config.isBioluminescent = true;
            config.glowIntensity = 0.5f;
            config.shelterFactor = 0.6f;
            config.foodValue = 0.0f;
            config.attractsFish = true;
            config.fishAttractionRadius = 10.0f;
            break;

        case AquaticPlantType::PLASMA_POLYP:
            config.name = "Plasma Polyp";
            config.minHeight = 0.2f;
            config.maxHeight = 0.8f;
            config.minScale = 0.3f;
            config.maxScale = 1.0f;
            config.minSegments = 1;
            config.maxSegments = 1;
            config.preferredZone = WaterZone::DEEP;
            config.minDepth = 30.0f;
            config.maxDepth = 200.0f;
            config.minTemperature = 2.0f;
            config.maxTemperature = 15.0f;
            config.minSalinity = 0.8f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.0f;
            config.growthRate = 0.002f;
            config.oxygenProduction = 0.5f;
            config.carbonSequestration = 0.3f;
            config.primaryColor = {1.0f, 0.3f, 0.7f};
            config.secondaryColor = {0.9f, 0.9f, 1.0f};
            config.swaySpeed = 0.8f;
            config.swayAmount = 0.2f;
            config.isBioluminescent = true;
            config.glowIntensity = 1.0f;
            config.shelterFactor = 0.3f;
            config.foodValue = 0.0f;
            config.attractsFish = true;
            config.fishAttractionRadius = 8.0f;
            break;

        case AquaticPlantType::VOID_ANEMONE:
            config.name = "Void Anemone";
            config.minHeight = 0.3f;
            config.maxHeight = 1.0f;
            config.minScale = 0.4f;
            config.maxScale = 1.2f;
            config.minSegments = 1;
            config.maxSegments = 1;
            config.preferredZone = WaterZone::ABYSS;
            config.minDepth = 50.0f;
            config.maxDepth = 500.0f;
            config.minTemperature = 1.0f;
            config.maxTemperature = 10.0f;
            config.minSalinity = 0.9f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.0f;
            config.growthRate = 0.001f;
            config.oxygenProduction = 0.1f;
            config.carbonSequestration = 0.05f;
            config.primaryColor = {0.1f, 0.0f, 0.15f};
            config.secondaryColor = {0.3f, 0.0f, 0.4f};
            config.swaySpeed = 0.6f;
            config.swayAmount = 0.15f;
            config.isBioluminescent = true;
            config.glowIntensity = 0.6f;
            config.shelterFactor = 0.5f;
            config.foodValue = 0.0f;
            config.attractsFish = true;
            config.fishAttractionRadius = 5.0f;
            break;

        case AquaticPlantType::TENDRIL_SEAWEED:
            config.name = "Tendril Seaweed";
            config.minHeight = 2.0f;
            config.maxHeight = 8.0f;
            config.minScale = 0.5f;
            config.maxScale = 1.5f;
            config.minSegments = 10;
            config.maxSegments = 25;
            config.preferredZone = WaterZone::MEDIUM;
            config.minDepth = 5.0f;
            config.maxDepth = 40.0f;
            config.minTemperature = 10.0f;
            config.maxTemperature = 25.0f;
            config.minSalinity = 0.6f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.2f;
            config.growthRate = 0.15f;
            config.oxygenProduction = 0.8f;
            config.carbonSequestration = 0.5f;
            config.primaryColor = {0.25f, 0.1f, 0.3f};
            config.secondaryColor = {0.4f, 0.2f, 0.45f};
            config.swaySpeed = 1.0f;
            config.swayAmount = 0.5f;  // Lots of movement
            config.isBioluminescent = true;
            config.glowIntensity = 0.3f;
            config.shelterFactor = 0.6f;
            config.foodValue = 0.3f;
            config.attractsFish = true;
            config.fishAttractionRadius = 10.0f;
            break;

        default:
            config.name = "Unknown Aquatic Plant";
            config.minHeight = 0.5f;
            config.maxHeight = 2.0f;
            config.minScale = 0.5f;
            config.maxScale = 1.0f;
            config.minSegments = 1;
            config.maxSegments = 5;
            config.preferredZone = WaterZone::SHALLOW;
            config.minDepth = 1.0f;
            config.maxDepth = 10.0f;
            config.minTemperature = 15.0f;
            config.maxTemperature = 25.0f;
            config.minSalinity = 0.5f;
            config.maxSalinity = 1.0f;
            config.minLight = 0.5f;
            config.growthRate = 0.1f;
            config.oxygenProduction = 0.5f;
            config.carbonSequestration = 0.3f;
            config.primaryColor = {0.3f, 0.4f, 0.2f};
            config.secondaryColor = {0.35f, 0.45f, 0.25f};
            config.swaySpeed = 0.5f;
            config.swayAmount = 0.2f;
            config.shelterFactor = 0.3f;
            config.foodValue = 0.3f;
            config.attractsFish = false;
            config.fishAttractionRadius = 3.0f;
            break;
    }

    return config;
}

bool isCoralType(AquaticPlantType type) {
    return type == AquaticPlantType::CORAL_BRAIN ||
           type == AquaticPlantType::CORAL_STAGHORN ||
           type == AquaticPlantType::CORAL_TABLE ||
           type == AquaticPlantType::CORAL_FAN ||
           type == AquaticPlantType::CORAL_PILLAR ||
           type == AquaticPlantType::CORAL_MUSHROOM ||
           type == AquaticPlantType::ANEMONE ||
           type == AquaticPlantType::CRYSTAL_CORAL ||
           type == AquaticPlantType::PLASMA_POLYP ||
           type == AquaticPlantType::VOID_ANEMONE;
}

bool isSurfacePlant(AquaticPlantType type) {
    return type == AquaticPlantType::LILY_PAD ||
           type == AquaticPlantType::WATER_LILY ||
           type == AquaticPlantType::LOTUS ||
           type == AquaticPlantType::DUCKWEED ||
           type == AquaticPlantType::WATER_HYACINTH ||
           type == AquaticPlantType::WATER_LETTUCE ||
           type == AquaticPlantType::SEAWEED_SARGASSUM;
}

bool isAlienAquaticPlant(AquaticPlantType type) {
    return type == AquaticPlantType::BIOLUMINESCENT_KELP ||
           type == AquaticPlantType::CRYSTAL_CORAL ||
           type == AquaticPlantType::PLASMA_POLYP ||
           type == AquaticPlantType::VOID_ANEMONE ||
           type == AquaticPlantType::TENDRIL_SEAWEED;
}

// ===== AquaticPlantSystem Implementation =====

AquaticPlantSystem::AquaticPlantSystem() = default;

AquaticPlantSystem::~AquaticPlantSystem() = default;

void AquaticPlantSystem::initialize(DX12Device* device, const Terrain* t) {
    dx12Device = device;
    terrain = t;
}

void AquaticPlantSystem::generate(unsigned int seed) {
    kelpForests.clear();
    coralReefs.clear();
    lilyPadClusters.clear();
    allInstances.clear();

    generateKelpForests(seed);
    generateCoralReefs(seed + 1000);
    generateSurfacePlants(seed + 2000);
    generateEmergentPlants(seed + 3000);

    if (dx12Device && !allInstances.empty()) {
        createBuffers();
    }
}

void AquaticPlantSystem::generateKelpForests(unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);

    float worldSize = TerrainSampler::WORLD_SIZE;
    if (terrain) {
        worldSize = terrain->getWidth() * terrain->getScale();
    }
    float halfWorld = worldSize * 0.5f;

    // Generate kelp forests in suitable ocean areas
    int numForests = 8 + static_cast<int>(dist01(rng) * 6);

    for (int f = 0; f < numForests; f++) {
        float x = (dist01(rng) - 0.5f) * worldSize;
        float z = (dist01(rng) - 0.5f) * worldSize;

        float depth = getWaterDepth(x, z);
        float temp = getWaterTemperature(x, z);
        float salinity = getWaterSalinity(x, z);

        // Kelp needs: depth 5-40m, temp 8-20C, high salinity
        if (depth < 5.0f || depth > 40.0f) continue;
        if (temp < 8.0f || temp > 20.0f) continue;
        if (salinity < 0.7f) continue;

        KelpForest forest;
        forest.center = {x, -depth, z};
        forest.radius = 20.0f + dist01(rng) * 30.0f;
        forest.temperature = temp;
        forest.currentStrength = 0.2f + dist01(rng) * 0.3f;

        // Generate kelp plants in forest
        int numPlants = 30 + static_cast<int>(dist01(rng) * 50);

        for (int p = 0; p < numPlants; p++) {
            float r = std::sqrt(dist01(rng)) * forest.radius;
            float theta = angleDist(rng);
            float px = forest.center.x + r * std::cos(theta);
            float pz = forest.center.z + r * std::sin(theta);

            float localDepth = getWaterDepth(px, pz);
            if (localDepth < 3.0f) continue;

            // Select kelp type
            AquaticPlantType kelpType;
            float typeChoice = dist01(rng);
            if (typeChoice < 0.5f) {
                kelpType = AquaticPlantType::KELP_GIANT;
            } else if (typeChoice < 0.8f) {
                kelpType = AquaticPlantType::KELP_BULL;
            } else {
                kelpType = AquaticPlantType::KELP_RIBBON;
            }

            // Small chance of alien kelp
            if (dist01(rng) < 0.05f) {
                kelpType = AquaticPlantType::BIOLUMINESCENT_KELP;
            }

            AquaticPlantConfig config = getAquaticPlantConfig(kelpType);

            AquaticPlantInstance kelp;
            kelp.position = {px, -localDepth, pz};
            kelp.rotation = angleDist(rng);
            kelp.scale = config.minScale + dist01(rng) * (config.maxScale - config.minScale);
            kelp.type = kelpType;
            kelp.swayPhase = dist01(rng) * 6.28318f;
            kelp.swayAmplitude = config.swayAmount;
            kelp.currentHeight = config.minHeight + dist01(rng) * (config.maxHeight - config.minHeight);
            kelp.health = 0.8f + dist01(rng) * 0.2f;
            kelp.oxygenProduction = config.oxygenProduction;
            kelp.shelterValue = config.shelterFactor;
            kelp.foodValue = config.foodValue;

            generateKelpSegments(kelp, localDepth, rng());

            forest.plants.push_back(kelp);
            allInstances.push_back(kelp);
            forest.totalBiomass += kelp.currentHeight * kelp.scale * 10.0f;
            forest.oxygenOutput += kelp.oxygenProduction;
        }

        if (!forest.plants.empty()) {
            kelpForests.push_back(std::move(forest));
        }
    }
}

void AquaticPlantSystem::generateCoralReefs(unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);

    float worldSize = TerrainSampler::WORLD_SIZE;
    if (terrain) {
        worldSize = terrain->getWidth() * terrain->getScale();
    }

    // Generate coral reefs in warm, shallow waters
    int numReefs = 5 + static_cast<int>(dist01(rng) * 5);

    for (int r = 0; r < numReefs; r++) {
        float x = (dist01(rng) - 0.5f) * worldSize;
        float z = (dist01(rng) - 0.5f) * worldSize;

        float depth = getWaterDepth(x, z);
        float temp = getWaterTemperature(x, z);
        float salinity = getWaterSalinity(x, z);

        // Coral needs: depth 2-30m, temp 23-29C, high salinity
        if (depth < 2.0f || depth > 30.0f) continue;
        if (temp < 20.0f || temp > 32.0f) continue;
        if (salinity < 0.8f) continue;

        CoralReef reef;
        reef.center = {x, -depth, z};
        reef.radius = 15.0f + dist01(rng) * 25.0f;
        reef.temperature = temp;
        reef.overallHealth = 0.8f + dist01(rng) * 0.2f;
        reef.biodiversityScore = 0.5f + dist01(rng) * 0.5f;
        reef.calcificationRate = 0.01f + dist01(rng) * 0.01f;
        reef.isProtected = dist01(rng) < 0.2f;

        // Generate corals in reef
        int numCorals = 40 + static_cast<int>(dist01(rng) * 60);

        std::vector<AquaticPlantType> coralTypes = {
            AquaticPlantType::CORAL_BRAIN,
            AquaticPlantType::CORAL_STAGHORN,
            AquaticPlantType::CORAL_TABLE,
            AquaticPlantType::CORAL_FAN,
            AquaticPlantType::ANEMONE
        };

        for (int c = 0; c < numCorals; c++) {
            float cr = std::sqrt(dist01(rng)) * reef.radius;
            float ctheta = angleDist(rng);
            float cx = reef.center.x + cr * std::cos(ctheta);
            float cz = reef.center.z + cr * std::sin(ctheta);

            float localDepth = getWaterDepth(cx, cz);
            if (localDepth < 1.0f || localDepth > 35.0f) continue;

            // Select coral type
            AquaticPlantType coralType = coralTypes[static_cast<int>(dist01(rng) * coralTypes.size()) % coralTypes.size()];

            // Small chance of alien coral
            if (dist01(rng) < 0.03f) {
                float alienChoice = dist01(rng);
                if (alienChoice < 0.4f) {
                    coralType = AquaticPlantType::CRYSTAL_CORAL;
                } else if (alienChoice < 0.7f) {
                    coralType = AquaticPlantType::PLASMA_POLYP;
                } else {
                    coralType = AquaticPlantType::VOID_ANEMONE;
                }
            }

            AquaticPlantConfig config = getAquaticPlantConfig(coralType);

            AquaticPlantInstance coral;
            coral.position = {cx, -localDepth + 0.1f, cz};
            coral.rotation = angleDist(rng);
            coral.scale = config.minScale + dist01(rng) * (config.maxScale - config.minScale);
            coral.type = coralType;
            coral.swayPhase = dist01(rng) * 6.28318f;
            coral.swayAmplitude = config.swayAmount;
            coral.currentHeight = config.minHeight + dist01(rng) * (config.maxHeight - config.minHeight);
            coral.health = 0.7f + dist01(rng) * 0.3f;
            coral.coralHealth = CoralHealthState::THRIVING;
            coral.bleachProgress = 0.0f;
            coral.originalColor = config.primaryColor;
            coral.currentColor = config.primaryColor;
            coral.oxygenProduction = config.oxygenProduction;
            coral.shelterValue = config.shelterFactor;
            coral.foodValue = config.foodValue;

            reef.corals.push_back(coral);
            allInstances.push_back(coral);
        }

        if (!reef.corals.empty()) {
            coralReefs.push_back(std::move(reef));
        }
    }
}

void AquaticPlantSystem::generateSurfacePlants(unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);

    float worldSize = TerrainSampler::WORLD_SIZE;
    if (terrain) {
        worldSize = terrain->getWidth() * terrain->getScale();
    }

    // Generate lily pad clusters in freshwater areas
    int numClusters = 10 + static_cast<int>(dist01(rng) * 10);

    for (int c = 0; c < numClusters; c++) {
        float x = (dist01(rng) - 0.5f) * worldSize;
        float z = (dist01(rng) - 0.5f) * worldSize;

        float depth = getWaterDepth(x, z);
        float salinity = getWaterSalinity(x, z);

        // Lily pads need: shallow freshwater
        if (depth < 0.3f || depth > 3.0f) continue;
        if (salinity > 0.2f) continue;  // Freshwater only

        LilyPadCluster cluster;
        cluster.center = {x, 0.0f, z};
        cluster.radius = 5.0f + dist01(rng) * 10.0f;
        cluster.floweringCount = 0;
        cluster.coveragePercent = 0.0f;

        // Generate lily pads
        int numPads = 15 + static_cast<int>(dist01(rng) * 25);

        for (int p = 0; p < numPads; p++) {
            float pr = std::sqrt(dist01(rng)) * cluster.radius;
            float ptheta = angleDist(rng);
            float px = cluster.center.x + pr * std::cos(ptheta);
            float pz = cluster.center.z + pr * std::sin(ptheta);

            float localDepth = getWaterDepth(px, pz);
            if (localDepth < 0.2f || localDepth > 4.0f) continue;

            // Select type
            AquaticPlantType padType;
            float typeChoice = dist01(rng);
            if (typeChoice < 0.5f) {
                padType = AquaticPlantType::LILY_PAD;
            } else if (typeChoice < 0.8f) {
                padType = AquaticPlantType::WATER_LILY;
            } else {
                padType = AquaticPlantType::LOTUS;
            }

            AquaticPlantConfig config = getAquaticPlantConfig(padType);

            AquaticPlantInstance pad;
            pad.position = {px, 0.02f, pz};  // Slightly above water
            pad.rotation = angleDist(rng);
            pad.scale = config.minScale + dist01(rng) * (config.maxScale - config.minScale);
            pad.type = padType;
            pad.swayPhase = dist01(rng) * 6.28318f;
            pad.swayAmplitude = config.swayAmount;
            pad.waterSurfaceHeight = 0.0f;
            pad.padSize = 0.3f + dist01(rng) * 0.5f;
            pad.hasFlower = (padType == AquaticPlantType::WATER_LILY || padType == AquaticPlantType::LOTUS) && dist01(rng) < 0.4f;
            pad.flowerColor = config.secondaryColor;
            pad.health = 0.8f + dist01(rng) * 0.2f;
            pad.oxygenProduction = config.oxygenProduction;
            pad.shelterValue = config.shelterFactor;
            pad.foodValue = config.foodValue;

            if (pad.hasFlower) {
                cluster.floweringCount++;
            }

            cluster.pads.push_back(pad);
            allInstances.push_back(pad);
            cluster.coveragePercent += pad.padSize * pad.padSize * 3.14159f;
        }

        // Normalize coverage
        float areaOfCluster = 3.14159f * cluster.radius * cluster.radius;
        cluster.coveragePercent = std::min(1.0f, cluster.coveragePercent / areaOfCluster);

        if (!cluster.pads.empty()) {
            lilyPadClusters.push_back(std::move(cluster));
        }
    }
}

void AquaticPlantSystem::generateEmergentPlants(unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);

    float worldSize = TerrainSampler::WORLD_SIZE;
    if (terrain) {
        worldSize = terrain->getWidth() * terrain->getScale();
    }

    // Generate emergent plants (cattails, reeds) along shorelines
    int numPatches = 20 + static_cast<int>(dist01(rng) * 15);

    std::vector<AquaticPlantType> emergentTypes = {
        AquaticPlantType::CATTAIL,
        AquaticPlantType::REED,
        AquaticPlantType::BULRUSH
    };

    for (int p = 0; p < numPatches; p++) {
        float x = (dist01(rng) - 0.5f) * worldSize;
        float z = (dist01(rng) - 0.5f) * worldSize;

        float depth = getWaterDepth(x, z);

        // Emergent plants need very shallow water
        if (depth < 0.0f || depth > 1.5f) continue;

        AquaticPlantType plantType = emergentTypes[static_cast<int>(dist01(rng) * emergentTypes.size()) % emergentTypes.size()];
        AquaticPlantConfig config = getAquaticPlantConfig(plantType);

        // Generate a small cluster
        int clusterSize = 5 + static_cast<int>(dist01(rng) * 10);
        float clusterRadius = 2.0f + dist01(rng) * 3.0f;

        for (int i = 0; i < clusterSize; i++) {
            float r = std::sqrt(dist01(rng)) * clusterRadius;
            float theta = angleDist(rng);
            float px = x + r * std::cos(theta);
            float pz = z + r * std::sin(theta);

            float localDepth = getWaterDepth(px, pz);
            if (localDepth < -0.2f || localDepth > 2.0f) continue;

            float baseY = 0.0f;
            if (terrain && terrain->isInBounds(px, pz)) {
                baseY = terrain->getHeight(px, pz);
            }

            AquaticPlantInstance plant;
            plant.position = {px, baseY, pz};
            plant.rotation = angleDist(rng);
            plant.scale = config.minScale + dist01(rng) * (config.maxScale - config.minScale);
            plant.type = plantType;
            plant.swayPhase = dist01(rng) * 6.28318f;
            plant.swayAmplitude = config.swayAmount;
            plant.currentHeight = config.minHeight + dist01(rng) * (config.maxHeight - config.minHeight);
            plant.health = 0.8f + dist01(rng) * 0.2f;
            plant.oxygenProduction = config.oxygenProduction;
            plant.shelterValue = config.shelterFactor;
            plant.foodValue = config.foodValue;

            allInstances.push_back(plant);
        }
    }

    // Generate mangrove roots in brackish areas
    int numMangroves = 5 + static_cast<int>(dist01(rng) * 5);

    for (int m = 0; m < numMangroves; m++) {
        float x = (dist01(rng) - 0.5f) * worldSize;
        float z = (dist01(rng) - 0.5f) * worldSize;

        float depth = getWaterDepth(x, z);
        float salinity = getWaterSalinity(x, z);

        // Mangroves need brackish shallow water
        if (depth < 0.0f || depth > 2.0f) continue;
        if (salinity < 0.2f || salinity > 0.8f) continue;

        AquaticPlantConfig config = getAquaticPlantConfig(AquaticPlantType::MANGROVE_PROP_ROOT);

        int rootCount = 10 + static_cast<int>(dist01(rng) * 15);
        float patchRadius = 5.0f + dist01(rng) * 8.0f;

        for (int r = 0; r < rootCount; r++) {
            float rr = std::sqrt(dist01(rng)) * patchRadius;
            float rtheta = angleDist(rng);
            float rx = x + rr * std::cos(rtheta);
            float rz = z + rr * std::sin(rtheta);

            float localDepth = getWaterDepth(rx, rz);
            if (localDepth < -0.5f || localDepth > 2.5f) continue;

            float baseY = 0.0f;
            if (terrain && terrain->isInBounds(rx, rz)) {
                baseY = terrain->getHeight(rx, rz);
            }

            AquaticPlantInstance root;
            root.position = {rx, baseY - localDepth * 0.5f, rz};
            root.rotation = angleDist(rng);
            root.scale = config.minScale + dist01(rng) * (config.maxScale - config.minScale);
            root.type = AquaticPlantType::MANGROVE_PROP_ROOT;
            root.swayPhase = 0.0f;
            root.swayAmplitude = 0.0f;
            root.currentHeight = config.minHeight + dist01(rng) * (config.maxHeight - config.minHeight);
            root.health = 0.9f + dist01(rng) * 0.1f;
            root.segmentCount = config.minSegments + static_cast<int>(dist01(rng) * (config.maxSegments - config.minSegments));
            root.oxygenProduction = config.oxygenProduction;
            root.shelterValue = config.shelterFactor;
            root.foodValue = config.foodValue;

            allInstances.push_back(root);
        }
    }
}

void AquaticPlantSystem::generateKelpSegments(AquaticPlantInstance& kelp, float waterDepth, unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

    AquaticPlantConfig config = getAquaticPlantConfig(kelp.type);

    int segments = config.minSegments + static_cast<int>(dist01(rng) * (config.maxSegments - config.minSegments));
    kelp.segmentCount = segments;
    kelp.segmentPositions.clear();

    float segmentHeight = kelp.currentHeight / segments;
    glm::vec3 currentPos = kelp.position;

    for (int i = 0; i < segments; i++) {
        // Add some randomness to segment positions
        float offsetX = (dist01(rng) - 0.5f) * 0.3f;
        float offsetZ = (dist01(rng) - 0.5f) * 0.3f;

        currentPos.x += offsetX;
        currentPos.y += segmentHeight;
        currentPos.z += offsetZ;

        kelp.segmentPositions.push_back(currentPos);
    }
}

void AquaticPlantSystem::update(float deltaTime, const glm::vec3& cameraPos) {
    simulationTime += deltaTime;

    updateKelpAnimation(deltaTime);
    updateCoralHealth(deltaTime);
    updateSurfacePlants(deltaTime);
    updateWaterCurrent(deltaTime);

    // Update visibility
    visibleInstanceCount = 0;
    for (const auto& instance : allInstances) {
        float dist = glm::length(glm::vec2(instance.position.x - cameraPos.x, instance.position.z - cameraPos.z));
        if (dist < maxRenderDistance) {
            visibleInstanceCount++;
        }
    }
}

void AquaticPlantSystem::updateKelpAnimation(float deltaTime) {
    for (auto& forest : kelpForests) {
        glm::vec3 current = getWaterCurrent(forest.center.x, forest.center.z);

        for (auto& kelp : forest.plants) {
            kelp.swayPhase += deltaTime * getAquaticPlantConfig(kelp.type).swaySpeed;

            // Update segments if present
            if (!kelp.segmentPositions.empty()) {
                updateKelpSegments(kelp, deltaTime, current);
            }
        }
    }
}

void AquaticPlantSystem::updateKelpSegments(AquaticPlantInstance& kelp, float deltaTime, const glm::vec3& current) {
    float swayFactor = std::sin(kelp.swayPhase) * kelp.swayAmplitude;
    float currentEffect = glm::length(glm::vec2(current.x, current.z));

    for (size_t i = 0; i < kelp.segmentPositions.size(); i++) {
        float heightFactor = static_cast<float>(i) / kelp.segmentPositions.size();
        float sway = swayFactor * heightFactor;

        kelp.segmentPositions[i].x = kelp.position.x + sway + current.x * heightFactor * 0.5f;
        kelp.segmentPositions[i].z = kelp.position.z + std::cos(kelp.swayPhase * 0.7f) * kelp.swayAmplitude * heightFactor + current.z * heightFactor * 0.5f;
    }
}

void AquaticPlantSystem::updateCoralHealth(float deltaTime) {
    for (auto& reef : coralReefs) {
        float temperature = getWaterTemperature(reef.center.x, reef.center.z);
        reef.temperature = temperature;

        float healthSum = 0.0f;
        int healthyCount = 0;
        int bleachedCount = 0;

        for (auto& coral : reef.corals) {
            updateCoralBleaching(coral, temperature, deltaTime);

            healthSum += coral.health;
            if (coral.coralHealth == CoralHealthState::THRIVING || coral.coralHealth == CoralHealthState::STRESSED) {
                healthyCount++;
            } else if (coral.coralHealth == CoralHealthState::BLEACHED || coral.coralHealth == CoralHealthState::DEAD) {
                bleachedCount++;
            }
        }

        if (!reef.corals.empty()) {
            reef.overallHealth = healthSum / reef.corals.size();
        }
    }
}

void AquaticPlantSystem::updateCoralBleaching(AquaticPlantInstance& coral, float temperature, float deltaTime) {
    float dayDelta = deltaTime / 600.0f;
    AquaticPlantConfig config = getAquaticPlantConfig(coral.type);

    // Temperature stress
    float tempStress = 0.0f;
    if (temperature > config.maxTemperature) {
        tempStress = (temperature - config.maxTemperature) / 5.0f;
    } else if (temperature < config.minTemperature) {
        tempStress = (config.minTemperature - temperature) / 10.0f;
    }

    switch (coral.coralHealth) {
        case CoralHealthState::THRIVING:
            if (tempStress > 0.2f) {
                coral.coralHealth = CoralHealthState::STRESSED;
            }
            coral.health = std::min(1.0f, coral.health + 0.01f * dayDelta);
            break;

        case CoralHealthState::STRESSED:
            if (tempStress > 0.5f) {
                coral.coralHealth = CoralHealthState::BLEACHING;
                coral.bleachProgress = 0.1f;
            } else if (tempStress < 0.1f) {
                coral.coralHealth = CoralHealthState::THRIVING;
            }
            break;

        case CoralHealthState::BLEACHING:
            coral.bleachProgress += tempStress * 0.1f * dayDelta;
            if (coral.bleachProgress >= 1.0f) {
                coral.coralHealth = CoralHealthState::BLEACHED;
                coral.bleachProgress = 1.0f;
            } else if (tempStress < 0.2f) {
                coral.coralHealth = CoralHealthState::RECOVERING;
            }
            coral.health -= 0.05f * dayDelta;
            break;

        case CoralHealthState::BLEACHED:
            if (tempStress < 0.2f && coral.health > 0.3f) {
                coral.coralHealth = CoralHealthState::RECOVERING;
            } else {
                coral.health -= 0.02f * dayDelta;
                if (coral.health <= 0.0f) {
                    coral.coralHealth = CoralHealthState::DEAD;
                    coral.health = 0.0f;
                }
            }
            break;

        case CoralHealthState::RECOVERING:
            coral.bleachProgress -= 0.05f * dayDelta;
            coral.health += 0.02f * dayDelta;
            if (coral.bleachProgress <= 0.0f) {
                coral.coralHealth = CoralHealthState::THRIVING;
                coral.bleachProgress = 0.0f;
            }
            break;

        case CoralHealthState::DEAD:
            // No recovery from death
            break;
    }

    // Update color based on bleach progress
    coral.currentColor = getCoralBleachedColor(coral.originalColor, coral.bleachProgress);
}

glm::vec3 AquaticPlantSystem::getCoralBleachedColor(const glm::vec3& originalColor, float bleachAmount) {
    glm::vec3 white(0.95f, 0.95f, 0.9f);
    return glm::mix(originalColor, white, bleachAmount);
}

void AquaticPlantSystem::updateSurfacePlants(float deltaTime) {
    for (auto& cluster : lilyPadClusters) {
        for (auto& pad : cluster.pads) {
            // Gentle bobbing motion
            pad.swayPhase += deltaTime * getAquaticPlantConfig(pad.type).swaySpeed;
            pad.position.y = 0.02f + std::sin(pad.swayPhase) * 0.01f;
        }
    }
}

void AquaticPlantSystem::updateWaterCurrent(float deltaTime) {
    // Update current strength based on weather
    if (weatherSystem) {
        const WeatherState& weather = weatherSystem->getCurrentWeather();
        for (auto& forest : kelpForests) {
            forest.currentStrength = 0.1f + weather.windStrength * 0.4f;
        }
    }
}

void AquaticPlantSystem::render(ID3D12GraphicsCommandList* commandList) {
    if (allInstances.empty() || !dx12Device) return;
    updateInstanceBuffer();
}

// ===== Query Functions =====

KelpForest* AquaticPlantSystem::findKelpForestAt(const glm::vec3& position, float radius) {
    for (auto& forest : kelpForests) {
        float dist = glm::length(glm::vec2(forest.center.x - position.x, forest.center.z - position.z));
        if (dist < forest.radius + radius) {
            return &forest;
        }
    }
    return nullptr;
}

float AquaticPlantSystem::getKelpDensity(const glm::vec3& position, float radius) const {
    float totalDensity = 0.0f;

    for (const auto& forest : kelpForests) {
        float dist = glm::length(glm::vec2(forest.center.x - position.x, forest.center.z - position.z));
        if (dist < forest.radius + radius) {
            float overlap = 1.0f - (dist / (forest.radius + radius));
            totalDensity += forest.plants.size() * overlap / (forest.radius * forest.radius * 3.14159f);
        }
    }

    return totalDensity;
}

CoralReef* AquaticPlantSystem::findCoralReefAt(const glm::vec3& position, float radius) {
    for (auto& reef : coralReefs) {
        float dist = glm::length(glm::vec2(reef.center.x - position.x, reef.center.z - position.z));
        if (dist < reef.radius + radius) {
            return &reef;
        }
    }
    return nullptr;
}

float AquaticPlantSystem::getCoralHealth(const glm::vec3& position, float radius) const {
    float totalHealth = 0.0f;
    float totalWeight = 0.0f;

    for (const auto& reef : coralReefs) {
        float dist = glm::length(glm::vec2(reef.center.x - position.x, reef.center.z - position.z));
        if (dist < reef.radius + radius) {
            float weight = 1.0f - (dist / (reef.radius + radius));
            totalHealth += reef.overallHealth * weight;
            totalWeight += weight;
        }
    }

    return totalWeight > 0.0f ? totalHealth / totalWeight : 0.0f;
}

void AquaticPlantSystem::applyTemperatureStress(float temperature, const glm::vec3& position, float radius) {
    for (auto& reef : coralReefs) {
        float dist = glm::length(glm::vec2(reef.center.x - position.x, reef.center.z - position.z));
        if (dist < reef.radius + radius) {
            // Apply temperature change
            float effect = 1.0f - (dist / (reef.radius + radius));
            reef.temperature = glm::mix(reef.temperature, temperature, effect * 0.5f);
        }
    }
}

bool AquaticPlantSystem::hasSurfaceCoverage(const glm::vec3& position, float radius) const {
    for (const auto& cluster : lilyPadClusters) {
        float dist = glm::length(glm::vec2(cluster.center.x - position.x, cluster.center.z - position.z));
        if (dist < cluster.radius + radius && cluster.coveragePercent > 0.1f) {
            return true;
        }
    }
    return false;
}

float AquaticPlantSystem::getOxygenProduction(const glm::vec3& position, float radius) const {
    float total = 0.0f;

    for (const auto& instance : allInstances) {
        float dist = glm::length(instance.position - position);
        if (dist < radius) {
            total += instance.oxygenProduction * (1.0f - dist / radius);
        }
    }

    return total;
}

float AquaticPlantSystem::getShelterValue(const glm::vec3& position, float radius) const {
    float total = 0.0f;

    for (const auto& instance : allInstances) {
        float dist = glm::length(instance.position - position);
        if (dist < radius) {
            total += instance.shelterValue * (1.0f - dist / radius);
        }
    }

    return std::min(1.0f, total);
}

float AquaticPlantSystem::getFoodValue(const glm::vec3& position, float radius) const {
    float total = 0.0f;

    for (const auto& instance : allInstances) {
        float dist = glm::length(instance.position - position);
        if (dist < radius) {
            total += instance.foodValue * instance.health * (1.0f - dist / radius);
        }
    }

    return total;
}

float AquaticPlantSystem::consumePlant(const glm::vec3& position, float amount) {
    float consumed = 0.0f;

    for (auto& instance : allInstances) {
        float dist = glm::length(instance.position - position);
        if (dist < 2.0f && instance.foodValue > 0.0f && instance.health > 0.0f) {
            float canConsume = std::min(amount - consumed, instance.health * instance.foodValue);
            instance.health -= canConsume / instance.foodValue;
            consumed += canConsume;

            if (consumed >= amount) break;
        }
    }

    return consumed;
}

AquaticPlantSystem::AquaticEcosystemStats AquaticPlantSystem::getStats() const {
    AquaticEcosystemStats stats;
    stats.totalKelpBiomass = 0.0f;
    stats.totalCoralHealth = 0.0f;
    stats.healthyCoralCount = 0;
    stats.bleachedCoralCount = 0;
    stats.totalOxygenProduction = 0.0f;
    stats.lilyPadCount = 0;
    stats.totalPlantCount = static_cast<int>(allInstances.size());

    for (const auto& forest : kelpForests) {
        stats.totalKelpBiomass += forest.totalBiomass;
    }

    for (const auto& reef : coralReefs) {
        stats.totalCoralHealth += reef.overallHealth;

        for (const auto& coral : reef.corals) {
            if (coral.coralHealth == CoralHealthState::THRIVING ||
                coral.coralHealth == CoralHealthState::STRESSED) {
                stats.healthyCoralCount++;
            } else if (coral.coralHealth == CoralHealthState::BLEACHED ||
                       coral.coralHealth == CoralHealthState::DEAD) {
                stats.bleachedCoralCount++;
            }
        }
    }

    if (!coralReefs.empty()) {
        stats.totalCoralHealth /= coralReefs.size();
    }

    for (const auto& cluster : lilyPadClusters) {
        stats.lilyPadCount += static_cast<int>(cluster.pads.size());
    }

    for (const auto& instance : allInstances) {
        stats.totalOxygenProduction += instance.oxygenProduction;
    }

    return stats;
}

std::vector<std::pair<glm::vec3, glm::vec3>> AquaticPlantSystem::getBioluminescentPositions() const {
    std::vector<std::pair<glm::vec3, glm::vec3>> positions;

    for (const auto& instance : allInstances) {
        AquaticPlantConfig config = getAquaticPlantConfig(instance.type);
        if (config.isBioluminescent && instance.health > 0.3f) {
            glm::vec3 color = config.secondaryColor * config.glowIntensity;
            positions.push_back({instance.position, color});
        }
    }

    return positions;
}

// ===== Helper Functions =====

bool AquaticPlantSystem::isValidPlantLocation(float x, float z, WaterZone zone) const {
    float depth = getWaterDepth(x, z);
    float salinity = getWaterSalinity(x, z);

    switch (zone) {
        case WaterZone::SURFACE:
            return depth > 0.0f && depth < 5.0f;
        case WaterZone::SHALLOW:
            return depth >= 0.0f && depth <= 5.0f;
        case WaterZone::MEDIUM:
            return depth > 5.0f && depth <= 20.0f;
        case WaterZone::DEEP:
            return depth > 20.0f && depth <= 50.0f;
        case WaterZone::ABYSS:
            return depth > 50.0f;
        case WaterZone::TIDAL:
            return depth >= -1.0f && depth <= 3.0f;
        case WaterZone::FRESHWATER:
            return depth > 0.0f && salinity < 0.2f;
        case WaterZone::BRACKISH:
            return depth > 0.0f && salinity >= 0.2f && salinity <= 0.8f;
        default:
            return false;
    }
}

float AquaticPlantSystem::getWaterDepth(float x, float z) const {
    if (terrain && terrain->isInBounds(x, z)) {
        float height = terrain->getHeight(x, z);
        float waterLevel = terrain->getWaterLevel();
        return waterLevel - height;
    }

    float height = TerrainSampler::SampleHeight(x, z);
    float waterLevel = TerrainSampler::HEIGHT_SCALE * TerrainSampler::WATER_LEVEL;
    return waterLevel - height;
}

float AquaticPlantSystem::getWaterTemperature(float x, float z) const {
    float baseTemp = 20.0f;

    if (climateSystem) {
        ClimateData climate = climateSystem->getClimateAt(x, z);
        baseTemp = climate.temperature;

        // Water moderates temperature
        float depth = getWaterDepth(x, z);
        if (depth > 0.0f) {
            // Deeper water is cooler
            float depthEffect = std::min(1.0f, depth / 50.0f);
            baseTemp = glm::mix(baseTemp, 4.0f, depthEffect * 0.5f);
        }
    }

    if (seasonManager) {
        // Seasonal variation
        Season season = seasonManager->getCurrentSeason();
        float progress = seasonManager->getSeasonProgress();

        switch (season) {
            case Season::SUMMER:
                baseTemp += 3.0f * progress;
                break;
            case Season::WINTER:
                baseTemp -= 5.0f * progress;
                break;
            default:
                break;
        }
    }

    return baseTemp;
}

float AquaticPlantSystem::getWaterSalinity(float x, float z) const {
    // Default to ocean salinity
    float salinity = 0.9f;

    if (climateSystem) {
        ClimateData climate = climateSystem->getClimateAt(x, z);
        ClimateBiome biome = climate.getBiome();

        switch (biome) {
            case ClimateBiome::DEEP_OCEAN:
            case ClimateBiome::SHALLOW_WATER:
                salinity = 0.95f;
                break;
            case ClimateBiome::SWAMP:
                salinity = 0.1f;  // Freshwater
                break;
            case ClimateBiome::BEACH:
                salinity = 0.5f;  // Brackish
                break;
            default:
                // Check if it's a lake/river (far from ocean)
                float distToOcean = std::abs(x) + std::abs(z);
                if (distToOcean < TerrainSampler::WORLD_SIZE * 0.3f) {
                    salinity = 0.85f;
                } else {
                    salinity = 0.1f;  // Inland = freshwater
                }
                break;
        }
    }

    return salinity;
}

float AquaticPlantSystem::getLightLevel(float x, float z, float depth) const {
    // Light decreases exponentially with depth
    float surfaceLight = 1.0f;

    if (weatherSystem) {
        const WeatherState& weather = weatherSystem->getCurrentWeather();
        surfaceLight = 1.0f - weather.cloudCoverage * 0.5f;
    }

    // Light attenuation coefficient (clear water ~0.05, turbid ~0.3)
    float attenuation = 0.1f;
    float lightAtDepth = surfaceLight * std::exp(-attenuation * depth);

    return lightAtDepth;
}

glm::vec3 AquaticPlantSystem::getWaterCurrent(float x, float z) const {
    glm::vec3 current(0.0f);

    // Base ocean current
    current.x = std::sin(x * 0.01f + simulationTime * 0.1f) * 0.2f;
    current.z = std::cos(z * 0.01f + simulationTime * 0.08f) * 0.15f;

    // Wind influence
    if (weatherSystem) {
        const WeatherState& weather = weatherSystem->getCurrentWeather();
        current.x += weather.windDirection.x * weather.windStrength * 0.3f;
        current.z += weather.windDirection.y * weather.windStrength * 0.3f;
    }

    return current;
}

void AquaticPlantSystem::createBuffers() {
    if (!dx12Device || allInstances.empty()) return;
    // Buffer creation handled by DX12Device
}

void AquaticPlantSystem::updateInstanceBuffer() {
    // Update instance data for rendering
}
