#include "GrassSystem.h"
#include "Terrain.h"
#include "TerrainSampler.h"
#include "ClimateSystem.h"
#include "BiomeSystem.h"
#include "BiomePalette.h"
#include "SeasonManager.h"
#include "WeatherSystem.h"
#ifndef USE_FORGE_ENGINE
#include "../graphics/DX12Device.h"
#endif
#include <random>
#include <algorithm>
#include <cmath>

// Global BiomePaletteManager instance for unified colors
static BiomePaletteManager g_biomePaletteManager;

constexpr float GRASS_HEIGHT_MULTIPLIER = 2.5f;
constexpr float GRASS_WIDTH_MULTIPLIER = 1.7f;
constexpr float GRASS_DENSITY_SCALE = 0.08f;
constexpr int MAX_BLADES_PER_PATCH = 350;
constexpr int MAX_FLOWERS_PER_PATCH = 50;
constexpr int MAX_GROUND_COVER_PER_PATCH = 30;

// ===== Configuration Functions =====

GrassTypeConfig getGrassTypeConfig(GrassType type) {
    GrassTypeConfig config;
    config.type = type;
    config.isAlien = false;
    config.glowIntensity = 0.0f;

    switch (type) {
        case GrassType::LAWN_GRASS:
            config.baseHeight = 0.15f;
            config.heightVariation = 0.05f;
            config.baseWidth = 0.02f;
            config.widthVariation = 0.01f;
            config.stiffness = 0.7f;
            config.baseColor = {0.25f, 0.45f, 0.18f};
            config.tipColor = {0.35f, 0.55f, 0.22f};
            break;

        case GrassType::MEADOW_GRASS:
            config.baseHeight = 0.35f;
            config.heightVariation = 0.15f;
            config.baseWidth = 0.025f;
            config.widthVariation = 0.01f;
            config.stiffness = 0.5f;
            config.baseColor = {0.28f, 0.48f, 0.2f};
            config.tipColor = {0.38f, 0.58f, 0.25f};
            break;

        case GrassType::TALL_GRASS:
            config.baseHeight = 0.7f;
            config.heightVariation = 0.3f;
            config.baseWidth = 0.03f;
            config.widthVariation = 0.015f;
            config.stiffness = 0.3f;
            config.baseColor = {0.45f, 0.5f, 0.25f};
            config.tipColor = {0.55f, 0.58f, 0.3f};
            break;

        case GrassType::WILD_GRASS:
            config.baseHeight = 0.4f;
            config.heightVariation = 0.25f;
            config.baseWidth = 0.028f;
            config.widthVariation = 0.02f;
            config.stiffness = 0.4f;
            config.baseColor = {0.32f, 0.45f, 0.2f};
            config.tipColor = {0.42f, 0.52f, 0.25f};
            break;

        case GrassType::TROPICAL_GRASS:
            config.baseHeight = 0.5f;
            config.heightVariation = 0.2f;
            config.baseWidth = 0.04f;
            config.widthVariation = 0.02f;
            config.stiffness = 0.35f;
            config.baseColor = {0.15f, 0.45f, 0.12f};
            config.tipColor = {0.22f, 0.52f, 0.18f};
            break;

        case GrassType::TUNDRA_GRASS:
            config.baseHeight = 0.1f;
            config.heightVariation = 0.05f;
            config.baseWidth = 0.015f;
            config.widthVariation = 0.005f;
            config.stiffness = 0.8f;
            config.baseColor = {0.4f, 0.42f, 0.3f};
            config.tipColor = {0.48f, 0.5f, 0.35f};
            break;

        case GrassType::MARSH_GRASS:
            config.baseHeight = 0.6f;
            config.heightVariation = 0.2f;
            config.baseWidth = 0.035f;
            config.widthVariation = 0.015f;
            config.stiffness = 0.25f;
            config.baseColor = {0.22f, 0.38f, 0.18f};
            config.tipColor = {0.28f, 0.42f, 0.2f};
            break;

        case GrassType::DUNE_GRASS:
            config.baseHeight = 0.45f;
            config.heightVariation = 0.15f;
            config.baseWidth = 0.02f;
            config.widthVariation = 0.01f;
            config.stiffness = 0.6f;
            config.baseColor = {0.5f, 0.52f, 0.35f};
            config.tipColor = {0.58f, 0.6f, 0.4f};
            break;

        case GrassType::ALPINE_GRASS:
            config.baseHeight = 0.2f;
            config.heightVariation = 0.1f;
            config.baseWidth = 0.018f;
            config.widthVariation = 0.008f;
            config.stiffness = 0.65f;
            config.baseColor = {0.32f, 0.5f, 0.25f};
            config.tipColor = {0.42f, 0.58f, 0.3f};
            break;

        case GrassType::BAMBOO_GRASS:
            config.baseHeight = 0.8f;
            config.heightVariation = 0.3f;
            config.baseWidth = 0.015f;
            config.widthVariation = 0.005f;
            config.stiffness = 0.7f;
            config.baseColor = {0.35f, 0.55f, 0.25f};
            config.tipColor = {0.45f, 0.62f, 0.32f};
            break;

        case GrassType::PAMPAS_GRASS:
            config.baseHeight = 1.2f;
            config.heightVariation = 0.4f;
            config.baseWidth = 0.025f;
            config.widthVariation = 0.01f;
            config.stiffness = 0.2f;
            config.baseColor = {0.65f, 0.62f, 0.5f};
            config.tipColor = {0.85f, 0.82f, 0.75f};
            break;

        case GrassType::FOUNTAIN_GRASS:
            config.baseHeight = 0.6f;
            config.heightVariation = 0.2f;
            config.baseWidth = 0.012f;
            config.widthVariation = 0.005f;
            config.stiffness = 0.15f;
            config.baseColor = {0.4f, 0.45f, 0.28f};
            config.tipColor = {0.6f, 0.55f, 0.4f};
            break;

        case GrassType::BLUE_GRASS:
            config.baseHeight = 0.3f;
            config.heightVariation = 0.1f;
            config.baseWidth = 0.022f;
            config.widthVariation = 0.008f;
            config.stiffness = 0.55f;
            config.baseColor = {0.25f, 0.35f, 0.45f};
            config.tipColor = {0.35f, 0.45f, 0.55f};
            break;

        case GrassType::RED_GRASS:
            config.baseHeight = 0.35f;
            config.heightVariation = 0.12f;
            config.baseWidth = 0.024f;
            config.widthVariation = 0.01f;
            config.stiffness = 0.5f;
            config.baseColor = {0.55f, 0.25f, 0.2f};
            config.tipColor = {0.7f, 0.35f, 0.25f};
            break;

        // Alien grasses
        case GrassType::BIOLUMINESCENT_GRASS:
            config.baseHeight = 0.4f;
            config.heightVariation = 0.15f;
            config.baseWidth = 0.025f;
            config.widthVariation = 0.01f;
            config.stiffness = 0.4f;
            config.baseColor = {0.1f, 0.4f, 0.5f};
            config.tipColor = {0.2f, 0.8f, 0.9f};
            config.isAlien = true;
            config.glowIntensity = 0.6f;
            break;

        case GrassType::CRYSTAL_GRASS:
            config.baseHeight = 0.25f;
            config.heightVariation = 0.1f;
            config.baseWidth = 0.03f;
            config.widthVariation = 0.015f;
            config.stiffness = 0.9f;
            config.baseColor = {0.7f, 0.75f, 0.85f};
            config.tipColor = {0.85f, 0.9f, 1.0f};
            config.isAlien = true;
            config.glowIntensity = 0.3f;
            break;

        case GrassType::TENDRIL_GRASS:
            config.baseHeight = 0.5f;
            config.heightVariation = 0.2f;
            config.baseWidth = 0.02f;
            config.widthVariation = 0.01f;
            config.stiffness = 0.1f;
            config.baseColor = {0.3f, 0.15f, 0.35f};
            config.tipColor = {0.5f, 0.25f, 0.55f};
            config.isAlien = true;
            config.glowIntensity = 0.2f;
            break;

        case GrassType::SPORE_GRASS:
            config.baseHeight = 0.35f;
            config.heightVariation = 0.15f;
            config.baseWidth = 0.028f;
            config.widthVariation = 0.012f;
            config.stiffness = 0.45f;
            config.baseColor = {0.4f, 0.35f, 0.2f};
            config.tipColor = {0.6f, 0.5f, 0.3f};
            config.isAlien = true;
            config.glowIntensity = 0.15f;
            break;

        default:
            config.baseHeight = 0.35f;
            config.heightVariation = 0.15f;
            config.baseWidth = 0.025f;
            config.widthVariation = 0.01f;
            config.stiffness = 0.5f;
            config.baseColor = {0.28f, 0.48f, 0.2f};
            config.tipColor = {0.38f, 0.58f, 0.25f};
            break;
    }

    return config;
}

FlowerSpeciesConfig getFlowerSpeciesConfig(FlowerType type) {
    FlowerSpeciesConfig config;
    config.type = type;
    config.isBioluminescent = false;
    config.glowIntensity = 0.0f;
    config.attractsPollinators = true;
    config.attractionRadius = 5.0f;

    switch (type) {
        case FlowerType::DAISY:
            config.name = "Daisy";
            config.defaultPetalColor = {1.0f, 1.0f, 1.0f};
            config.defaultCenterColor = {1.0f, 0.85f, 0.0f};
            config.minSize = 0.08f;
            config.maxSize = 0.15f;
            config.petalCount = 21;
            config.bloomDuration = 14.0f;
            config.pollenProduction = 0.1f;
            config.nectarProduction = 0.15f;
            config.seedProduction = 50.0f;
            config.minTemperature = 5.0f;
            config.maxTemperature = 30.0f;
            config.minMoisture = 0.3f;
            config.maxMoisture = 0.7f;
            config.bloomSeasonStart = 90.0f;   // April
            config.bloomSeasonEnd = 270.0f;    // September
            break;

        case FlowerType::POPPY:
            config.name = "Poppy";
            config.defaultPetalColor = {0.9f, 0.15f, 0.1f};
            config.defaultCenterColor = {0.1f, 0.1f, 0.1f};
            config.minSize = 0.1f;
            config.maxSize = 0.2f;
            config.petalCount = 4;
            config.bloomDuration = 7.0f;
            config.pollenProduction = 0.2f;
            config.nectarProduction = 0.05f;
            config.seedProduction = 1000.0f;
            config.minTemperature = 10.0f;
            config.maxTemperature = 28.0f;
            config.minMoisture = 0.2f;
            config.maxMoisture = 0.5f;
            config.bloomSeasonStart = 120.0f;  // May
            config.bloomSeasonEnd = 210.0f;    // July
            break;

        case FlowerType::DANDELION:
            config.name = "Dandelion";
            config.defaultPetalColor = {1.0f, 0.9f, 0.0f};
            config.defaultCenterColor = {0.9f, 0.8f, 0.0f};
            config.minSize = 0.06f;
            config.maxSize = 0.12f;
            config.petalCount = 100;
            config.bloomDuration = 3.0f;
            config.pollenProduction = 0.3f;
            config.nectarProduction = 0.2f;
            config.seedProduction = 200.0f;
            config.minTemperature = 0.0f;
            config.maxTemperature = 35.0f;
            config.minMoisture = 0.2f;
            config.maxMoisture = 0.8f;
            config.bloomSeasonStart = 60.0f;   // March
            config.bloomSeasonEnd = 300.0f;    // October
            break;

        case FlowerType::BUTTERCUP:
            config.name = "Buttercup";
            config.defaultPetalColor = {1.0f, 0.95f, 0.0f};
            config.defaultCenterColor = {0.9f, 0.85f, 0.0f};
            config.minSize = 0.05f;
            config.maxSize = 0.1f;
            config.petalCount = 5;
            config.bloomDuration = 10.0f;
            config.pollenProduction = 0.15f;
            config.nectarProduction = 0.1f;
            config.seedProduction = 30.0f;
            config.minTemperature = 5.0f;
            config.maxTemperature = 25.0f;
            config.minMoisture = 0.4f;
            config.maxMoisture = 0.8f;
            config.bloomSeasonStart = 100.0f;
            config.bloomSeasonEnd = 240.0f;
            break;

        case FlowerType::CLOVER:
            config.name = "Clover";
            config.defaultPetalColor = {0.95f, 0.85f, 0.95f};
            config.defaultCenterColor = {0.9f, 0.75f, 0.9f};
            config.minSize = 0.04f;
            config.maxSize = 0.08f;
            config.petalCount = 60;
            config.bloomDuration = 21.0f;
            config.pollenProduction = 0.25f;
            config.nectarProduction = 0.3f;
            config.seedProduction = 4.0f;
            config.minTemperature = 5.0f;
            config.maxTemperature = 30.0f;
            config.minMoisture = 0.3f;
            config.maxMoisture = 0.7f;
            config.bloomSeasonStart = 120.0f;
            config.bloomSeasonEnd = 270.0f;
            break;

        case FlowerType::BLUEBELL:
            config.name = "Bluebell";
            config.defaultPetalColor = {0.3f, 0.4f, 0.9f};
            config.defaultCenterColor = {0.2f, 0.3f, 0.7f};
            config.minSize = 0.06f;
            config.maxSize = 0.12f;
            config.petalCount = 6;
            config.bloomDuration = 14.0f;
            config.pollenProduction = 0.1f;
            config.nectarProduction = 0.2f;
            config.seedProduction = 20.0f;
            config.minTemperature = 8.0f;
            config.maxTemperature = 20.0f;
            config.minMoisture = 0.5f;
            config.maxMoisture = 0.8f;
            config.bloomSeasonStart = 90.0f;
            config.bloomSeasonEnd = 150.0f;
            break;

        case FlowerType::LAVENDER:
            config.name = "Lavender";
            config.defaultPetalColor = {0.6f, 0.5f, 0.85f};
            config.defaultCenterColor = {0.5f, 0.4f, 0.7f};
            config.minSize = 0.08f;
            config.maxSize = 0.15f;
            config.petalCount = 30;
            config.bloomDuration = 30.0f;
            config.pollenProduction = 0.15f;
            config.nectarProduction = 0.35f;
            config.seedProduction = 40.0f;
            config.minTemperature = 15.0f;
            config.maxTemperature = 35.0f;
            config.minMoisture = 0.2f;
            config.maxMoisture = 0.5f;
            config.bloomSeasonStart = 150.0f;
            config.bloomSeasonEnd = 240.0f;
            break;

        case FlowerType::SUNFLOWER:
            config.name = "Sunflower";
            config.defaultPetalColor = {1.0f, 0.85f, 0.0f};
            config.defaultCenterColor = {0.4f, 0.25f, 0.1f};
            config.minSize = 0.3f;
            config.maxSize = 0.6f;
            config.petalCount = 34;
            config.bloomDuration = 14.0f;
            config.pollenProduction = 0.4f;
            config.nectarProduction = 0.3f;
            config.seedProduction = 1500.0f;
            config.minTemperature = 15.0f;
            config.maxTemperature = 35.0f;
            config.minMoisture = 0.3f;
            config.maxMoisture = 0.6f;
            config.bloomSeasonStart = 180.0f;
            config.bloomSeasonEnd = 270.0f;
            break;

        case FlowerType::VIOLET:
            config.name = "Violet";
            config.defaultPetalColor = {0.5f, 0.3f, 0.8f};
            config.defaultCenterColor = {1.0f, 1.0f, 1.0f};
            config.minSize = 0.04f;
            config.maxSize = 0.08f;
            config.petalCount = 5;
            config.bloomDuration = 10.0f;
            config.pollenProduction = 0.08f;
            config.nectarProduction = 0.12f;
            config.seedProduction = 15.0f;
            config.minTemperature = 5.0f;
            config.maxTemperature = 22.0f;
            config.minMoisture = 0.4f;
            config.maxMoisture = 0.7f;
            config.bloomSeasonStart = 60.0f;
            config.bloomSeasonEnd = 150.0f;
            break;

        case FlowerType::MARIGOLD:
            config.name = "Marigold";
            config.defaultPetalColor = {1.0f, 0.6f, 0.0f};
            config.defaultCenterColor = {0.9f, 0.5f, 0.0f};
            config.minSize = 0.1f;
            config.maxSize = 0.2f;
            config.petalCount = 50;
            config.bloomDuration = 21.0f;
            config.pollenProduction = 0.12f;
            config.nectarProduction = 0.08f;
            config.seedProduction = 100.0f;
            config.minTemperature = 15.0f;
            config.maxTemperature = 35.0f;
            config.minMoisture = 0.3f;
            config.maxMoisture = 0.6f;
            config.bloomSeasonStart = 150.0f;
            config.bloomSeasonEnd = 300.0f;
            break;

        case FlowerType::TULIP:
            config.name = "Tulip";
            config.defaultPetalColor = {0.9f, 0.2f, 0.3f};
            config.defaultCenterColor = {0.2f, 0.2f, 0.2f};
            config.minSize = 0.12f;
            config.maxSize = 0.2f;
            config.petalCount = 6;
            config.bloomDuration = 10.0f;
            config.pollenProduction = 0.15f;
            config.nectarProduction = 0.1f;
            config.seedProduction = 200.0f;
            config.minTemperature = 5.0f;
            config.maxTemperature = 20.0f;
            config.minMoisture = 0.4f;
            config.maxMoisture = 0.7f;
            config.bloomSeasonStart = 75.0f;
            config.bloomSeasonEnd = 135.0f;
            break;

        case FlowerType::DAFFODIL:
            config.name = "Daffodil";
            config.defaultPetalColor = {1.0f, 1.0f, 0.7f};
            config.defaultCenterColor = {1.0f, 0.8f, 0.0f};
            config.minSize = 0.1f;
            config.maxSize = 0.18f;
            config.petalCount = 6;
            config.bloomDuration = 14.0f;
            config.pollenProduction = 0.12f;
            config.nectarProduction = 0.15f;
            config.seedProduction = 30.0f;
            config.minTemperature = 5.0f;
            config.maxTemperature = 18.0f;
            config.minMoisture = 0.4f;
            config.maxMoisture = 0.7f;
            config.bloomSeasonStart = 45.0f;
            config.bloomSeasonEnd = 120.0f;
            break;

        case FlowerType::CHRYSANTHEMUM:
            config.name = "Chrysanthemum";
            config.defaultPetalColor = {0.9f, 0.7f, 0.2f};
            config.defaultCenterColor = {0.8f, 0.6f, 0.1f};
            config.minSize = 0.15f;
            config.maxSize = 0.3f;
            config.petalCount = 80;
            config.bloomDuration = 28.0f;
            config.pollenProduction = 0.1f;
            config.nectarProduction = 0.08f;
            config.seedProduction = 150.0f;
            config.minTemperature = 10.0f;
            config.maxTemperature = 25.0f;
            config.minMoisture = 0.4f;
            config.maxMoisture = 0.7f;
            config.bloomSeasonStart = 240.0f;
            config.bloomSeasonEnd = 330.0f;
            break;

        case FlowerType::SNOWDROP:
            config.name = "Snowdrop";
            config.defaultPetalColor = {1.0f, 1.0f, 1.0f};
            config.defaultCenterColor = {0.8f, 1.0f, 0.8f};
            config.minSize = 0.04f;
            config.maxSize = 0.08f;
            config.petalCount = 6;
            config.bloomDuration = 14.0f;
            config.pollenProduction = 0.05f;
            config.nectarProduction = 0.08f;
            config.seedProduction = 10.0f;
            config.minTemperature = -5.0f;
            config.maxTemperature = 15.0f;
            config.minMoisture = 0.5f;
            config.maxMoisture = 0.8f;
            config.bloomSeasonStart = 15.0f;
            config.bloomSeasonEnd = 75.0f;
            break;

        case FlowerType::HIBISCUS:
            config.name = "Hibiscus";
            config.defaultPetalColor = {0.95f, 0.2f, 0.4f};
            config.defaultCenterColor = {0.9f, 0.85f, 0.2f};
            config.minSize = 0.2f;
            config.maxSize = 0.35f;
            config.petalCount = 5;
            config.bloomDuration = 2.0f;
            config.pollenProduction = 0.25f;
            config.nectarProduction = 0.4f;
            config.seedProduction = 50.0f;
            config.minTemperature = 20.0f;
            config.maxTemperature = 40.0f;
            config.minMoisture = 0.5f;
            config.maxMoisture = 0.9f;
            config.bloomSeasonStart = 0.0f;
            config.bloomSeasonEnd = 365.0f;
            break;

        case FlowerType::ORCHID:
            config.name = "Orchid";
            config.defaultPetalColor = {0.9f, 0.6f, 0.85f};
            config.defaultCenterColor = {0.95f, 0.95f, 0.8f};
            config.minSize = 0.1f;
            config.maxSize = 0.2f;
            config.petalCount = 6;
            config.bloomDuration = 60.0f;
            config.pollenProduction = 0.02f;
            config.nectarProduction = 0.5f;
            config.seedProduction = 100000.0f;
            config.minTemperature = 18.0f;
            config.maxTemperature = 30.0f;
            config.minMoisture = 0.6f;
            config.maxMoisture = 0.9f;
            config.bloomSeasonStart = 0.0f;
            config.bloomSeasonEnd = 365.0f;
            break;

        case FlowerType::BIRD_OF_PARADISE:
            config.name = "Bird of Paradise";
            config.defaultPetalColor = {1.0f, 0.5f, 0.0f};
            config.defaultCenterColor = {0.2f, 0.3f, 0.8f};
            config.minSize = 0.25f;
            config.maxSize = 0.4f;
            config.petalCount = 5;
            config.bloomDuration = 14.0f;
            config.pollenProduction = 0.15f;
            config.nectarProduction = 0.35f;
            config.seedProduction = 80.0f;
            config.minTemperature = 20.0f;
            config.maxTemperature = 35.0f;
            config.minMoisture = 0.5f;
            config.maxMoisture = 0.8f;
            config.bloomSeasonStart = 0.0f;
            config.bloomSeasonEnd = 365.0f;
            break;

        case FlowerType::PLUMERIA:
            config.name = "Plumeria";
            config.defaultPetalColor = {1.0f, 1.0f, 0.9f};
            config.defaultCenterColor = {1.0f, 0.9f, 0.3f};
            config.minSize = 0.1f;
            config.maxSize = 0.18f;
            config.petalCount = 5;
            config.bloomDuration = 5.0f;
            config.pollenProduction = 0.08f;
            config.nectarProduction = 0.3f;
            config.seedProduction = 20.0f;
            config.minTemperature = 22.0f;
            config.maxTemperature = 38.0f;
            config.minMoisture = 0.3f;
            config.maxMoisture = 0.7f;
            config.bloomSeasonStart = 0.0f;
            config.bloomSeasonEnd = 365.0f;
            break;

        case FlowerType::EDELWEISS:
            config.name = "Edelweiss";
            config.defaultPetalColor = {0.95f, 0.95f, 0.9f};
            config.defaultCenterColor = {0.9f, 0.88f, 0.75f};
            config.minSize = 0.06f;
            config.maxSize = 0.1f;
            config.petalCount = 9;
            config.bloomDuration = 21.0f;
            config.pollenProduction = 0.05f;
            config.nectarProduction = 0.03f;
            config.seedProduction = 500.0f;
            config.minTemperature = -10.0f;
            config.maxTemperature = 18.0f;
            config.minMoisture = 0.2f;
            config.maxMoisture = 0.5f;
            config.bloomSeasonStart = 150.0f;
            config.bloomSeasonEnd = 240.0f;
            break;

        case FlowerType::ALPINE_ASTER:
            config.name = "Alpine Aster";
            config.defaultPetalColor = {0.6f, 0.5f, 0.9f};
            config.defaultCenterColor = {1.0f, 0.9f, 0.0f};
            config.minSize = 0.08f;
            config.maxSize = 0.15f;
            config.petalCount = 21;
            config.bloomDuration = 14.0f;
            config.pollenProduction = 0.1f;
            config.nectarProduction = 0.12f;
            config.seedProduction = 100.0f;
            config.minTemperature = -5.0f;
            config.maxTemperature = 20.0f;
            config.minMoisture = 0.3f;
            config.maxMoisture = 0.6f;
            config.bloomSeasonStart = 150.0f;
            config.bloomSeasonEnd = 270.0f;
            break;

        case FlowerType::MOUNTAIN_AVENS:
            config.name = "Mountain Avens";
            config.defaultPetalColor = {1.0f, 1.0f, 1.0f};
            config.defaultCenterColor = {1.0f, 0.95f, 0.0f};
            config.minSize = 0.05f;
            config.maxSize = 0.1f;
            config.petalCount = 8;
            config.bloomDuration = 10.0f;
            config.pollenProduction = 0.08f;
            config.nectarProduction = 0.1f;
            config.seedProduction = 40.0f;
            config.minTemperature = -15.0f;
            config.maxTemperature = 15.0f;
            config.minMoisture = 0.3f;
            config.maxMoisture = 0.6f;
            config.bloomSeasonStart = 135.0f;
            config.bloomSeasonEnd = 210.0f;
            break;

        // Alien flowers
        case FlowerType::GLOW_BLOOM:
            config.name = "Glow Bloom";
            config.defaultPetalColor = {0.2f, 0.9f, 0.8f};
            config.defaultCenterColor = {0.1f, 0.6f, 0.5f};
            config.minSize = 0.12f;
            config.maxSize = 0.22f;
            config.petalCount = 7;
            config.bloomDuration = 30.0f;
            config.pollenProduction = 0.2f;
            config.nectarProduction = 0.4f;
            config.seedProduction = 10.0f;
            config.minTemperature = 10.0f;
            config.maxTemperature = 35.0f;
            config.minMoisture = 0.4f;
            config.maxMoisture = 0.8f;
            config.bloomSeasonStart = 0.0f;
            config.bloomSeasonEnd = 365.0f;
            config.isBioluminescent = true;
            config.glowIntensity = 0.8f;
            break;

        case FlowerType::CRYSTAL_FLOWER:
            config.name = "Crystal Flower";
            config.defaultPetalColor = {0.8f, 0.85f, 1.0f};
            config.defaultCenterColor = {0.6f, 0.7f, 0.95f};
            config.minSize = 0.15f;
            config.maxSize = 0.25f;
            config.petalCount = 5;
            config.bloomDuration = 90.0f;
            config.pollenProduction = 0.01f;
            config.nectarProduction = 0.01f;
            config.seedProduction = 5.0f;
            config.minTemperature = -20.0f;
            config.maxTemperature = 40.0f;
            config.minMoisture = 0.0f;
            config.maxMoisture = 1.0f;
            config.bloomSeasonStart = 0.0f;
            config.bloomSeasonEnd = 365.0f;
            config.isBioluminescent = true;
            config.glowIntensity = 0.4f;
            break;

        case FlowerType::VOID_BLOSSOM:
            config.name = "Void Blossom";
            config.defaultPetalColor = {0.15f, 0.05f, 0.2f};
            config.defaultCenterColor = {0.4f, 0.0f, 0.5f};
            config.minSize = 0.2f;
            config.maxSize = 0.35f;
            config.petalCount = 13;
            config.bloomDuration = 7.0f;
            config.pollenProduction = 0.5f;
            config.nectarProduction = 0.0f;
            config.seedProduction = 1.0f;
            config.minTemperature = 5.0f;
            config.maxTemperature = 30.0f;
            config.minMoisture = 0.3f;
            config.maxMoisture = 0.7f;
            config.bloomSeasonStart = 0.0f;
            config.bloomSeasonEnd = 365.0f;
            config.isBioluminescent = true;
            config.glowIntensity = 0.6f;
            break;

        case FlowerType::PLASMA_FLOWER:
            config.name = "Plasma Flower";
            config.defaultPetalColor = {1.0f, 0.4f, 0.8f};
            config.defaultCenterColor = {0.9f, 0.9f, 1.0f};
            config.minSize = 0.18f;
            config.maxSize = 0.3f;
            config.petalCount = 8;
            config.bloomDuration = 3.0f;
            config.pollenProduction = 0.8f;
            config.nectarProduction = 0.6f;
            config.seedProduction = 3.0f;
            config.minTemperature = 15.0f;
            config.maxTemperature = 40.0f;
            config.minMoisture = 0.2f;
            config.maxMoisture = 0.6f;
            config.bloomSeasonStart = 0.0f;
            config.bloomSeasonEnd = 365.0f;
            config.isBioluminescent = true;
            config.glowIntensity = 1.0f;
            break;

        default:
            config.name = "Unknown Flower";
            config.defaultPetalColor = {0.8f, 0.8f, 0.8f};
            config.defaultCenterColor = {0.6f, 0.6f, 0.0f};
            config.minSize = 0.08f;
            config.maxSize = 0.15f;
            config.petalCount = 5;
            config.bloomDuration = 14.0f;
            config.pollenProduction = 0.1f;
            config.nectarProduction = 0.1f;
            config.seedProduction = 50.0f;
            config.minTemperature = 5.0f;
            config.maxTemperature = 30.0f;
            config.minMoisture = 0.3f;
            config.maxMoisture = 0.7f;
            config.bloomSeasonStart = 90.0f;
            config.bloomSeasonEnd = 270.0f;
            break;
    }

    return config;
}

// ===== Helper Functions =====

namespace {
    float sampleHeightForGrass(const Terrain* terrain, float x, float z) {
        if (terrain && terrain->isInBounds(x, z)) {
            return terrain->getHeight(x, z);
        }
        return TerrainSampler::SampleHeight(x, z);
    }

    bool isWaterForGrass(const Terrain* terrain, float x, float z) {
        if (terrain && terrain->isInBounds(x, z)) {
            return terrain->isWater(x, z);
        }
        return TerrainSampler::IsWater(x, z);
    }

    bool isInWorldBounds(const Terrain* terrain, float x, float z) {
        if (terrain) {
            return terrain->isInBounds(x, z);
        }
        float halfWorld = TerrainSampler::WORLD_SIZE * 0.5f;
        return std::abs(x) <= halfWorld && std::abs(z) <= halfWorld;
    }
}

// ===== GrassSystem Implementation =====

GrassSystem::GrassSystem() = default;

GrassSystem::~GrassSystem() {
    // DX12 resources cleaned up by ComPtr
}

void GrassSystem::initialize(DX12Device* device, const Terrain* t) {
    dx12Device = device;
    terrain = t;
}

void GrassSystem::generate(unsigned int seed) {
    patches.clear();
    allInstances.clear();
    allFlowers.clear();
    allGroundCover.clear();

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

    // Calculate world bounds
    float worldWidth = TerrainSampler::WORLD_SIZE;
    float worldDepth = TerrainSampler::WORLD_SIZE;
    if (terrain) {
        worldWidth = terrain->getWidth() * terrain->getScale();
        worldDepth = terrain->getDepth() * terrain->getScale();
    }
    float minX = -worldWidth / 2.0f;
    float minZ = -worldDepth / 2.0f;

    // Generate patches in a grid pattern
    float patchSize = 24.0f;
    int patchesX = static_cast<int>(worldWidth / patchSize);
    int patchesZ = static_cast<int>(worldDepth / patchSize);

    for (int pz = 0; pz < patchesZ; pz++) {
        for (int px = 0; px < patchesX; px++) {
            float centerX = minX + (px + 0.5f) * patchSize;
            float centerZ = minZ + (pz + 0.5f) * patchSize;

            if (!isSuitableForGrass(centerX, centerZ)) {
                continue;
            }

            float height = sampleHeightForGrass(terrain, centerX, centerZ);
            glm::vec3 center = {centerX, height, centerZ};

            GrassConfig config = getLocalConfig(centerX, centerZ);
            config.density *= densityMultiplier;

            if (config.density > 0.01f) {
                generatePatch(center, patchSize * 0.5f, config, rng());
            }
        }
    }

    // Flatten all instances for rendering
    allInstances.clear();
    allFlowers.clear();
    allGroundCover.clear();

    for (const auto& patch : patches) {
        for (const auto& blade : patch.blades) {
            allInstances.push_back(blade);
        }
        for (const auto& flower : patch.flowers) {
            allFlowers.push_back(flower);
        }
        for (const auto& cover : patch.groundCover) {
            allGroundCover.push_back(cover);
        }
    }

    if (dx12Device && !allInstances.empty()) {
        createBuffers();
    }
}

void GrassSystem::generatePatch(const glm::vec3& center, float radius, const GrassConfig& config, unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);

    GrassPatch patch;
    patch.center = center;
    patch.radius = radius;
    patch.maxBiomass = radius * radius * config.density * 0.5f;
    patch.biomass = patch.maxBiomass;

    // Calculate blade count
    float area = 3.14159f * radius * radius;
    int numBlades = static_cast<int>(area * config.density * GRASS_DENSITY_SCALE);
    numBlades = std::min(numBlades, MAX_BLADES_PER_PATCH);

    // Generate grass blades
    for (int i = 0; i < numBlades; i++) {
        float r = std::sqrt(dist01(rng)) * radius;
        float theta = angleDist(rng);
        float offsetX = r * std::cos(theta);
        float offsetZ = r * std::sin(theta);

        float worldX = center.x + offsetX;
        float worldZ = center.z + offsetZ;

        if (!isInWorldBounds(terrain, worldX, worldZ)) continue;
        float height = sampleHeightForGrass(terrain, worldX, worldZ);

        if (isWaterForGrass(terrain, worldX, worldZ)) continue;

        float normalizedHeight = height / TerrainSampler::HEIGHT_SCALE;
        if (normalizedHeight < TerrainSampler::BEACH_LEVEL + 0.02f || normalizedHeight > 0.85f) continue;

        // Select grass type
        GrassType selectedType = selectGrassType(config, worldX, worldZ, rng());
        GrassTypeConfig typeConfig = getGrassTypeConfig(selectedType);

        GrassBladeInstance blade;
        blade.position = {worldX, height, worldZ};
        blade.rotation = angleDist(rng);
        blade.height = (typeConfig.baseHeight + dist01(rng) * typeConfig.heightVariation) * GRASS_HEIGHT_MULTIPLIER;
        blade.width = (typeConfig.baseWidth + dist01(rng) * typeConfig.widthVariation) * GRASS_WIDTH_MULTIPLIER;
        blade.bendFactor = 0.3f + dist01(rng) * 0.4f * (1.0f - typeConfig.stiffness);
        blade.colorVariation = dist01(rng);
        blade.type = selectedType;
        blade.windPhase = dist01(rng) * 6.28318f;
        blade.grazedAmount = 0.0f;
        blade.regrowthProgress = 1.0f;

        patch.blades.push_back(blade);
    }

    // Generate flowers if configured
    if (config.hasFlowers && config.flowerDensity > 0.0f) {
        generateFlowersInPatch(patch, config, rng());
    }

    // Generate ground cover
    if (config.hasGroundCover && config.groundCoverDensity > 0.0f) {
        generateGroundCoverInPatch(patch, config, rng());
    }

    if (!patch.blades.empty()) {
        patches.push_back(std::move(patch));
    }
}

void GrassSystem::generateFlowersInPatch(GrassPatch& patch, const GrassConfig& config, unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);

    float area = 3.14159f * patch.radius * patch.radius;
    int numFlowers = static_cast<int>(area * config.flowerDensity * 0.1f);
    numFlowers = std::min(numFlowers, MAX_FLOWERS_PER_PATCH);

    // Determine biome for palette lookup
    BiomeType paletteType = BiomeType::GRASSLAND;
    if (climateSystem) {
        ClimateData climate = climateSystem->getClimateAt(patch.center.x, patch.center.z);
        ClimateBiome biome = climate.getBiome();
        switch (biome) {
            case ClimateBiome::TROPICAL_RAINFOREST: paletteType = BiomeType::TROPICAL_RAINFOREST; break;
            case ClimateBiome::TEMPERATE_FOREST: paletteType = BiomeType::TEMPERATE_FOREST; break;
            case ClimateBiome::TEMPERATE_GRASSLAND: paletteType = BiomeType::GRASSLAND; break;
            case ClimateBiome::SAVANNA: paletteType = BiomeType::SAVANNA; break;
            case ClimateBiome::BOREAL_FOREST: paletteType = BiomeType::BOREAL_FOREST; break;
            case ClimateBiome::TUNDRA: paletteType = BiomeType::TUNDRA; break;
            case ClimateBiome::SWAMP: paletteType = BiomeType::SWAMP; break;
            case ClimateBiome::MOUNTAIN_MEADOW: paletteType = BiomeType::ALPINE_MEADOW; break;
            default: break;
        }
    }

    for (int i = 0; i < numFlowers; i++) {
        float r = std::sqrt(dist01(rng)) * patch.radius;
        float theta = angleDist(rng);
        float offsetX = r * std::cos(theta);
        float offsetZ = r * std::sin(theta);

        float worldX = patch.center.x + offsetX;
        float worldZ = patch.center.z + offsetZ;

        if (!isInWorldBounds(terrain, worldX, worldZ)) continue;
        float height = sampleHeightForGrass(terrain, worldX, worldZ);

        if (isWaterForGrass(terrain, worldX, worldZ)) continue;

        // Select flower type
        FlowerType selectedType = selectFlowerType(config, worldX, worldZ, rng());
        FlowerSpeciesConfig speciesConfig = getFlowerSpeciesConfig(selectedType);

        FlowerInstance flower;
        flower.position = {worldX, height, worldZ};
        flower.rotation = angleDist(rng);
        flower.scale = speciesConfig.minSize + dist01(rng) * (speciesConfig.maxSize - speciesConfig.minSize);
        flower.type = selectedType;

        // Use BiomePalette for flower colors - flowers in patches share the same color
        // This creates natural-looking color clusters
        FlowerPatchColor patchColor = g_biomePaletteManager.sampleFlowerColor(paletteType, worldX, worldZ);

        // Blend palette color with species default (palette dominates for cohesion)
        flower.petalColor = glm::mix(speciesConfig.defaultPetalColor, patchColor.petalColor, 0.7f);
        flower.centerColor = glm::mix(speciesConfig.defaultCenterColor, patchColor.centerColor, 0.5f);

        // Apply slight variation within the patch to avoid perfect uniformity
        float colorVar = (dist01(rng) - 0.5f) * 0.1f;
        flower.petalColor += glm::vec3(colorVar);
        flower.petalColor = glm::clamp(flower.petalColor, glm::vec3(0.0f), glm::vec3(1.0f));

        // Initialize lifecycle state based on season
        if (isFlowerInBloomSeason(selectedType, currentDayOfYear)) {
            flower.state = PollinationState::BLOOMING;
            flower.bloomProgress = 0.5f + dist01(rng) * 0.5f;
        } else {
            flower.state = PollinationState::CLOSED;
            flower.bloomProgress = 0.0f;
        }

        flower.pollenAmount = speciesConfig.pollenProduction * flower.bloomProgress;
        flower.nectarAmount = speciesConfig.nectarProduction * flower.bloomProgress;
        flower.age = dist01(rng) * speciesConfig.bloomDuration * 0.5f;
        flower.health = 0.8f + dist01(rng) * 0.2f;
        flower.hasBeenVisited = false;
        flower.seedsProduced = 0;

        patch.flowers.push_back(flower);
    }
}

void GrassSystem::generateGroundCoverInPatch(GrassPatch& patch, const GrassConfig& config, unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);

    int numCover = static_cast<int>(patch.radius * config.groundCoverDensity);
    numCover = std::min(numCover, MAX_GROUND_COVER_PER_PATCH);

    // Determine biome for palette lookup
    BiomeType paletteType = BiomeType::GRASSLAND;
    if (climateSystem) {
        ClimateData climate = climateSystem->getClimateAt(patch.center.x, patch.center.z);
        ClimateBiome biome = climate.getBiome();
        switch (biome) {
            case ClimateBiome::TROPICAL_RAINFOREST: paletteType = BiomeType::TROPICAL_RAINFOREST; break;
            case ClimateBiome::TEMPERATE_FOREST: paletteType = BiomeType::TEMPERATE_FOREST; break;
            case ClimateBiome::TEMPERATE_GRASSLAND: paletteType = BiomeType::GRASSLAND; break;
            case ClimateBiome::SAVANNA: paletteType = BiomeType::SAVANNA; break;
            case ClimateBiome::BOREAL_FOREST: paletteType = BiomeType::BOREAL_FOREST; break;
            case ClimateBiome::TUNDRA: paletteType = BiomeType::TUNDRA; break;
            case ClimateBiome::SWAMP: paletteType = BiomeType::SWAMP; break;
            case ClimateBiome::MOUNTAIN_MEADOW: paletteType = BiomeType::ALPINE_MEADOW; break;
            default: break;
        }
    }

    const BiomePalette& palette = g_biomePaletteManager.getPalette(paletteType);

    for (int i = 0; i < numCover; i++) {
        float r = std::sqrt(dist01(rng)) * patch.radius;
        float theta = angleDist(rng);
        float offsetX = r * std::cos(theta);
        float offsetZ = r * std::sin(theta);

        float worldX = patch.center.x + offsetX;
        float worldZ = patch.center.z + offsetZ;

        if (!isInWorldBounds(terrain, worldX, worldZ)) continue;
        float height = sampleHeightForGrass(terrain, worldX, worldZ);

        // Select ground cover type and color using BiomePalette
        GroundCoverType coverType = GroundCoverType::MOSS_GREEN;
        glm::vec3 coverColor = palette.mossColor;

        if (climateSystem) {
            ClimateData climate = climateSystem->getClimateAt(worldX, worldZ);
            float moisture = climate.moisture;
            float temp = climate.temperature;

            if (moisture > 0.7f) {
                coverType = GroundCoverType::MOSS_GREEN;
                coverColor = palette.mossColor;
            } else if (moisture > 0.5f) {
                int typeChoice = static_cast<int>(dist01(rng) * 3);
                switch (typeChoice) {
                    case 0:
                        coverType = GroundCoverType::MOSS_GREEN;
                        coverColor = palette.mossColor;
                        break;
                    case 1:
                        coverType = GroundCoverType::MOSS_BROWN;
                        coverColor = glm::mix(palette.mossColor, palette.groundColor, 0.4f);
                        break;
                    default:
                        coverType = GroundCoverType::FALLEN_LEAVES;
                        coverColor = glm::mix(palette.groundColor, palette.grassDryColor, 0.5f);
                        break;
                }
            } else if (temp < 5.0f) {
                coverType = GroundCoverType::LICHEN_CRUSTOSE;
                coverColor = palette.lichenColor;
            } else {
                coverType = GroundCoverType::FALLEN_LEAVES;
                coverColor = glm::mix(palette.groundColor, palette.grassDryColor, 0.5f);
            }
        }

        // Add slight color variation
        float colorVar = (dist01(rng) - 0.5f) * 0.1f;
        coverColor += glm::vec3(colorVar);
        coverColor = glm::clamp(coverColor, glm::vec3(0.0f), glm::vec3(1.0f));

        GroundCoverInstance cover;
        cover.position = {worldX, height, worldZ};
        cover.rotation = angleDist(rng);
        cover.scale = 0.3f + dist01(rng) * 0.5f;
        cover.type = coverType;
        cover.color = coverColor;
        cover.density = 0.5f + dist01(rng) * 0.5f;
        cover.moisture = 0.5f;

        patch.groundCover.push_back(cover);
    }
}

GrassType GrassSystem::selectGrassType(const GrassConfig& config, float x, float z, unsigned int seed) const {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

    // If specific types are allowed, choose from them
    if (!config.allowedTypes.empty()) {
        int index = static_cast<int>(dist01(rng) * config.allowedTypes.size());
        index = std::min(index, static_cast<int>(config.allowedTypes.size()) - 1);
        return config.allowedTypes[index];
    }

    // Otherwise use primary type with some variation
    float variation = dist01(rng);
    if (variation < 0.7f) {
        return config.primaryType;
    } else if (variation < 0.9f) {
        return GrassType::WILD_GRASS;
    } else {
        return GrassType::MEADOW_GRASS;
    }
}

FlowerType GrassSystem::selectFlowerType(const GrassConfig& config, float x, float z, unsigned int seed) const {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

    // If specific flowers are allowed, choose from them
    if (!config.allowedFlowers.empty()) {
        int index = static_cast<int>(dist01(rng) * config.allowedFlowers.size());
        index = std::min(index, static_cast<int>(config.allowedFlowers.size()) - 1);
        return config.allowedFlowers[index];
    }

    // Default flower selection based on common wildflowers
    float choice = dist01(rng);
    if (choice < 0.2f) return FlowerType::DAISY;
    if (choice < 0.35f) return FlowerType::DANDELION;
    if (choice < 0.5f) return FlowerType::CLOVER;
    if (choice < 0.65f) return FlowerType::BUTTERCUP;
    if (choice < 0.75f) return FlowerType::POPPY;
    if (choice < 0.85f) return FlowerType::VIOLET;
    if (choice < 0.92f) return FlowerType::BLUEBELL;
    return FlowerType::MARIGOLD;
}

bool GrassSystem::isSuitableForGrass(float x, float z) const {
    if (!terrain) return false;

    if (!isInWorldBounds(terrain, x, z)) return false;
    if (isWaterForGrass(terrain, x, z)) return false;

    float height = sampleHeightForGrass(terrain, x, z);
    float normalizedHeight = height / TerrainSampler::HEIGHT_SCALE;

    if (normalizedHeight < TerrainSampler::BEACH_LEVEL + 0.02f) return false;
    if (normalizedHeight > 0.85f) return false;

    if (climateSystem) {
        ClimateData climate = climateSystem->getClimateAt(x, z);
        ClimateBiome biome = climate.getBiome();

        switch (biome) {
            case ClimateBiome::DEEP_OCEAN:
            case ClimateBiome::SHALLOW_WATER:
            case ClimateBiome::BEACH:
            case ClimateBiome::DESERT_HOT:
            case ClimateBiome::DESERT_COLD:
            case ClimateBiome::MOUNTAIN_ROCK:
            case ClimateBiome::MOUNTAIN_SNOW:
            case ClimateBiome::ICE:
                return false;
            default:
                break;
        }
    }

    return true;
}

GrassConfig GrassSystem::getLocalConfig(float x, float z) const {
    GrassConfig config;

    // Get seasonal value (0-1, where 0.5 is peak summer)
    float seasonValue = 0.5f;
    if (seasonManager) {
        Season season = seasonManager->getCurrentSeason();
        float progress = seasonManager->getSeasonProgress();
        switch (season) {
            case Season::SPRING: seasonValue = 0.0f + progress * 0.25f; break;
            case Season::SUMMER: seasonValue = 0.25f + progress * 0.25f; break;
            case Season::FALL: seasonValue = 0.5f + progress * 0.25f; break;
            case Season::WINTER: seasonValue = 0.75f + progress * 0.25f; break;
        }
    }

    // Default configuration
    config.density = 15.0f;
    config.minHeight = 0.15f;
    config.maxHeight = 0.4f;
    config.primaryType = GrassType::MEADOW_GRASS;
    config.hasGroundCover = true;
    config.groundCoverDensity = 0.2f;

    // Use BiomePalette for unified colors
    BiomeType paletteType = BiomeType::GRASSLAND;

    if (climateSystem) {
        ClimateData climate = climateSystem->getClimateAt(x, z);
        ClimateBiome biome = climate.getBiome();

        // Map ClimateBiome to BiomeType for palette lookup
        switch (biome) {
            case ClimateBiome::TROPICAL_RAINFOREST:
                paletteType = BiomeType::TROPICAL_RAINFOREST;
                config.density = 25.0f;
                config.maxHeight = 0.6f;
                config.primaryType = GrassType::TROPICAL_GRASS;
                config.hasFlowers = true;
                config.flowerDensity = 0.15f;
                config.allowedFlowers = {FlowerType::HIBISCUS, FlowerType::ORCHID, FlowerType::BIRD_OF_PARADISE, FlowerType::PLUMERIA};
                config.allowedTypes = {GrassType::TROPICAL_GRASS, GrassType::BAMBOO_GRASS};
                break;

            case ClimateBiome::TROPICAL_SEASONAL:
                paletteType = BiomeType::SAVANNA;
                config.density = 18.0f;
                config.maxHeight = 0.5f;
                config.primaryType = GrassType::TROPICAL_GRASS;
                config.hasFlowers = true;
                config.flowerDensity = 0.1f;
                config.allowedFlowers = {FlowerType::HIBISCUS, FlowerType::PLUMERIA, FlowerType::MARIGOLD};
                break;

            case ClimateBiome::TEMPERATE_FOREST:
                paletteType = BiomeType::TEMPERATE_FOREST;
                config.density = 12.0f;
                config.maxHeight = 0.35f;
                config.primaryType = GrassType::MEADOW_GRASS;
                config.hasFlowers = true;
                config.flowerDensity = 0.08f;
                config.allowedFlowers = {FlowerType::BLUEBELL, FlowerType::VIOLET, FlowerType::SNOWDROP};
                config.groundCoverDensity = 0.4f;
                break;

            case ClimateBiome::TEMPERATE_GRASSLAND:
                paletteType = BiomeType::GRASSLAND;
                config.density = 30.0f;
                config.maxHeight = 0.55f;
                config.primaryType = GrassType::TALL_GRASS;
                config.hasFlowers = true;
                config.flowerDensity = 0.2f;
                config.allowedFlowers = {FlowerType::DAISY, FlowerType::DANDELION, FlowerType::CLOVER, FlowerType::BUTTERCUP, FlowerType::POPPY};
                config.allowedTypes = {GrassType::TALL_GRASS, GrassType::MEADOW_GRASS, GrassType::WILD_GRASS, GrassType::PAMPAS_GRASS};
                break;

            case ClimateBiome::SAVANNA:
                paletteType = BiomeType::SAVANNA;
                config.density = 20.0f;
                config.minHeight = 0.4f;
                config.maxHeight = 0.8f;
                config.primaryType = GrassType::TALL_GRASS;
                config.hasFlowers = false;
                config.allowedTypes = {GrassType::TALL_GRASS, GrassType::FOUNTAIN_GRASS};
                config.groundCoverDensity = 0.1f;
                break;

            case ClimateBiome::BOREAL_FOREST:
                paletteType = BiomeType::BOREAL_FOREST;
                config.density = 8.0f;
                config.maxHeight = 0.25f;
                config.primaryType = GrassType::WILD_GRASS;
                config.hasFlowers = true;
                config.flowerDensity = 0.05f;
                config.allowedFlowers = {FlowerType::VIOLET, FlowerType::CLOVER};
                config.groundCoverDensity = 0.5f;
                break;

            case ClimateBiome::TUNDRA:
                paletteType = BiomeType::TUNDRA;
                config.density = 5.0f;
                config.minHeight = 0.05f;
                config.maxHeight = 0.15f;
                config.primaryType = GrassType::TUNDRA_GRASS;
                config.hasFlowers = true;
                config.flowerDensity = 0.03f;
                config.allowedFlowers = {FlowerType::EDELWEISS, FlowerType::MOUNTAIN_AVENS};
                config.groundCoverDensity = 0.6f;
                break;

            case ClimateBiome::SWAMP:
                paletteType = BiomeType::SWAMP;
                config.density = 15.0f;
                config.maxHeight = 0.45f;
                config.primaryType = GrassType::MARSH_GRASS;
                config.hasFlowers = true;
                config.flowerDensity = 0.05f;
                config.allowedTypes = {GrassType::MARSH_GRASS, GrassType::WILD_GRASS};
                config.groundCoverDensity = 0.3f;
                break;

            case ClimateBiome::MOUNTAIN_MEADOW:
                paletteType = BiomeType::ALPINE_MEADOW;
                config.density = 20.0f;
                config.maxHeight = 0.3f;
                config.primaryType = GrassType::ALPINE_GRASS;
                config.hasFlowers = true;
                config.flowerDensity = 0.25f;
                config.allowedFlowers = {FlowerType::EDELWEISS, FlowerType::ALPINE_ASTER, FlowerType::MOUNTAIN_AVENS, FlowerType::DAISY};
                break;

            default:
                break;
        }
    }

    // Get unified colors from BiomePalette
    const BiomePalette& palette = g_biomePaletteManager.getPalette(paletteType);

    // Sample grass color with spatial variation and seasonal tinting
    config.baseColor = g_biomePaletteManager.sampleGrassColor(paletteType, x, z, seasonValue);
    config.tipColor = palette.grassTipColor;

    // Blend tip color with seasonal variation
    if (seasonValue > 0.5f) {
        // Autumn/Winter - blend towards dry color
        float autumnFactor = (seasonValue - 0.5f) * 2.0f;
        config.baseColor = glm::mix(config.baseColor, palette.grassDryColor, autumnFactor * 0.5f);
        config.tipColor = glm::mix(config.tipColor, palette.grassDryColor, autumnFactor * 0.3f);
    }

    // Apply seasonal modifiers
    if (seasonManager) {
        float leafMult = seasonManager->getLeafMultiplier();
        float growthMult = seasonManager->getGrowthMultiplier();
        Season season = seasonManager->getCurrentSeason();

        config.density *= growthMult;
        config.maxHeight *= (0.5f + leafMult * 0.5f);

        switch (season) {
            case Season::SPRING:
                config.baseColor.g *= 1.1f;
                config.tipColor.g *= 1.1f;
                config.flowerDensity *= 1.5f;
                break;

            case Season::SUMMER:
                config.flowerDensity *= 1.2f;
                break;

            case Season::FALL:
                config.baseColor = glm::mix(config.baseColor, glm::vec3(0.55f, 0.48f, 0.25f), 0.3f);
                config.tipColor = glm::mix(config.tipColor, glm::vec3(0.62f, 0.52f, 0.28f), 0.3f);
                config.flowerDensity *= 0.5f;
                break;

            case Season::WINTER:
                config.baseColor = glm::mix(config.baseColor, glm::vec3(0.45f, 0.4f, 0.28f), 0.5f);
                config.tipColor = glm::mix(config.tipColor, glm::vec3(0.5f, 0.45f, 0.3f), 0.5f);
                config.density *= 0.5f;
                config.flowerDensity *= 0.1f;
                break;
        }
    }

    return config;
}

void GrassSystem::update(float deltaTime, const glm::vec3& cameraPos) {
    simulationTime += deltaTime;
    windTime += deltaTime;

    // Update day of year (assuming ~10 minutes per day for simulation)
    currentDayOfYear += deltaTime / 600.0f;
    if (currentDayOfYear >= 365.0f) {
        currentDayOfYear -= 365.0f;
    }

    if (weatherSystem) {
        const WeatherState& weather = weatherSystem->getCurrentWeather();
        windStrength = weather.windStrength;
        windDirection = weather.windDirection;
    }

    updateSeasonalEffects();
    updateGrazingRecovery(deltaTime);
    updateFlowerLifecycles(deltaTime);
    updateWindAnimation(deltaTime);
    updateVisibility(cameraPos);
}

void GrassSystem::updateSeasonalEffects() {
    if (!seasonManager) return;

    Season season = seasonManager->getCurrentSeason();
    float progress = seasonManager->getSeasonProgress();

    switch (season) {
        case Season::SPRING:
            seasonalDensity = 0.7f + progress * 0.3f;
            seasonalColorTint = glm::mix(glm::vec3(0.8f, 0.85f, 0.7f), glm::vec3(1.0f, 1.0f, 1.0f), progress);
            break;

        case Season::SUMMER:
            seasonalDensity = 1.0f;
            seasonalColorTint = {1.0f, 1.0f, 1.0f};
            break;

        case Season::FALL:
            seasonalDensity = 1.0f - progress * 0.3f;
            seasonalColorTint = glm::mix(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.1f, 0.9f, 0.7f), progress);
            break;

        case Season::WINTER:
            seasonalDensity = 0.5f + (1.0f - progress) * 0.2f;
            seasonalColorTint = {0.9f, 0.88f, 0.85f};
            break;
    }
}

void GrassSystem::updateGrazingRecovery(float deltaTime) {
    float dayDelta = deltaTime / 600.0f;

    for (auto& patch : patches) {
        if (patch.grazedAmount > 0.0f) {
            patch.grazedAmount -= patch.regrowthRate * dayDelta;
            patch.grazedAmount = std::max(0.0f, patch.grazedAmount);

            // Update biomass
            patch.biomass = patch.maxBiomass * (1.0f - patch.grazedAmount);

            // Update individual blades
            for (auto& blade : patch.blades) {
                if (blade.grazedAmount > 0.0f) {
                    blade.regrowthProgress += 0.1f * dayDelta;
                    blade.regrowthProgress = std::min(1.0f, blade.regrowthProgress);

                    if (blade.regrowthProgress >= 1.0f) {
                        blade.grazedAmount = 0.0f;
                    }
                }
            }
        }
    }

    // Clean up old grazing events
    recentGrazingEvents.erase(
        std::remove_if(recentGrazingEvents.begin(), recentGrazingEvents.end(),
            [this](const GrazingEvent& event) {
                return (simulationTime - event.time) > 300.0f;
            }),
        recentGrazingEvents.end()
    );
}

void GrassSystem::updateFlowerLifecycles(float deltaTime) {
    float dayDelta = deltaTime / 600.0f;

    for (auto& patch : patches) {
        for (auto& flower : patch.flowers) {
            flower.age += dayDelta;
            updateFlowerBloom(flower, dayDelta);
            updateFlowerPollination(flower, dayDelta);
        }
    }
}

void GrassSystem::updateFlowerBloom(FlowerInstance& flower, float dayDelta) {
    FlowerSpeciesConfig config = getFlowerSpeciesConfig(flower.type);
    bool inSeason = isFlowerInBloomSeason(flower.type, currentDayOfYear);

    switch (flower.state) {
        case PollinationState::CLOSED:
            if (inSeason && flower.health > 0.5f) {
                flower.bloomProgress += 0.1f * dayDelta;
                if (flower.bloomProgress >= 1.0f) {
                    flower.state = PollinationState::BLOOMING;
                    flower.bloomProgress = 1.0f;
                }
            }
            break;

        case PollinationState::BLOOMING:
            // Produce pollen and nectar
            flower.pollenAmount = std::min(1.0f, flower.pollenAmount + config.pollenProduction * dayDelta);
            flower.nectarAmount = std::min(1.0f, flower.nectarAmount + config.nectarProduction * dayDelta);

            // Check if bloom duration exceeded
            if (flower.age > config.bloomDuration) {
                if (flower.hasBeenVisited) {
                    flower.state = PollinationState::POLLINATED;
                } else {
                    flower.state = PollinationState::WILTED;
                    flower.bloomProgress = 0.5f;
                }
            }
            break;

        case PollinationState::POLLINATED:
            flower.bloomProgress -= 0.05f * dayDelta;
            if (flower.bloomProgress <= 0.5f) {
                flower.state = PollinationState::PRODUCING_SEEDS;
                flower.seedsProduced = static_cast<int>(config.seedProduction);
            }
            break;

        case PollinationState::PRODUCING_SEEDS:
            flower.bloomProgress -= 0.02f * dayDelta;
            if (flower.bloomProgress <= 0.2f) {
                flower.state = PollinationState::DISPERSING;
            }
            break;

        case PollinationState::DISPERSING:
            flower.seedsProduced = std::max(0, flower.seedsProduced - static_cast<int>(dayDelta * 10));
            if (flower.seedsProduced <= 0) {
                flower.state = PollinationState::WILTED;
            }
            break;

        case PollinationState::WILTED:
            flower.health -= 0.1f * dayDelta;
            flower.bloomProgress = std::max(0.0f, flower.bloomProgress - 0.05f * dayDelta);
            break;
    }
}

void GrassSystem::updateFlowerPollination(FlowerInstance& flower, float dayDelta) {
    // Natural pollination chance (wind, etc.)
    if (flower.state == PollinationState::BLOOMING && !flower.hasBeenVisited) {
        // Small chance of wind pollination
        std::mt19937 rng(static_cast<unsigned int>(simulationTime * 1000 + flower.position.x * 100));
        std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

        if (dist01(rng) < 0.001f * dayDelta * windStrength) {
            flower.hasBeenVisited = true;
        }
    }
}

void GrassSystem::updateWindAnimation(float deltaTime) {
    // Wind animation is primarily handled in shaders
    // This updates phase offsets for individual variation
    for (auto& patch : patches) {
        if (!patch.isVisible) continue;

        for (auto& blade : patch.blades) {
            GrassTypeConfig typeConfig = getGrassTypeConfig(blade.type);
            float windEffect = windStrength * (1.0f - typeConfig.stiffness);
            blade.bendFactor = 0.2f + windEffect * 0.5f;
        }
    }
}

void GrassSystem::updateVisibility(const glm::vec3& cameraPos) {
    visibleInstanceCount = 0;
    visibleFlowerCount = 0;

    for (auto& patch : patches) {
        float dist = glm::length(glm::vec2(patch.center.x - cameraPos.x, patch.center.z - cameraPos.z));

        if (dist > maxRenderDistance) {
            patch.isVisible = false;
            continue;
        }

        patch.isVisible = true;

        if (dist < lodDistance1) {
            patch.lodLevel = 0.0f;
        } else if (dist < lodDistance2) {
            patch.lodLevel = (dist - lodDistance1) / (lodDistance2 - lodDistance1);
        } else {
            patch.lodLevel = 1.0f;
        }

        float lodFactor = 1.0f - patch.lodLevel * 0.5f;
        visibleInstanceCount += static_cast<size_t>(patch.blades.size() * lodFactor);
        visibleFlowerCount += patch.flowers.size();
    }
}

void GrassSystem::render(ID3D12GraphicsCommandList* commandList) {
    if (allInstances.empty() || !dx12Device) return;
    uploadInstanceData();
}

void GrassSystem::createBuffers() {
    if (!dx12Device || allInstances.empty()) return;
    // Buffer creation handled by DX12Device
}

void GrassSystem::uploadInstanceData() {
    if (!instanceBuffer || allInstances.empty()) return;
    // Data upload handled by DX12Device
}

// ===== Grazing System =====

void GrassSystem::applyGrazing(const glm::vec3& position, float radius, float intensity) {
    GrazingEvent event;
    event.position = position;
    event.radius = radius;
    event.intensity = intensity;
    event.time = simulationTime;
    recentGrazingEvents.push_back(event);

    for (auto& patch : patches) {
        float dist = glm::length(glm::vec2(patch.center.x - position.x, patch.center.z - position.z));

        if (dist < radius + patch.radius) {
            // Patch is affected
            float patchEffect = 1.0f - std::min(1.0f, dist / (radius + patch.radius));
            patch.grazedAmount = std::min(1.0f, patch.grazedAmount + intensity * patchEffect);
            patch.lastGrazedTime = simulationTime;

            // Update individual blades
            for (auto& blade : patch.blades) {
                float bladeDist = glm::length(glm::vec2(blade.position.x - position.x, blade.position.z - position.z));
                if (bladeDist < radius) {
                    float bladeEffect = 1.0f - (bladeDist / radius);
                    blade.grazedAmount = std::min(1.0f, blade.grazedAmount + intensity * bladeEffect);
                    blade.regrowthProgress = 0.0f;
                }
            }

            // Update biomass
            patch.biomass = patch.maxBiomass * (1.0f - patch.grazedAmount);
        }
    }
}

float GrassSystem::getGrazingFoodValue(const glm::vec3& position, float radius) const {
    float totalFood = 0.0f;

    for (const auto& patch : patches) {
        float dist = glm::length(glm::vec2(patch.center.x - position.x, patch.center.z - position.z));

        if (dist < radius + patch.radius) {
            float availability = 1.0f - patch.grazedAmount;
            float overlap = std::max(0.0f, 1.0f - dist / (radius + patch.radius));
            totalFood += patch.biomass * availability * overlap;
        }
    }

    return totalFood;
}

bool GrassSystem::hasGrazeableGrass(const glm::vec3& position, float radius) const {
    return getGrazingFoodValue(position, radius) > 1.0f;
}

float GrassSystem::getBiomassAt(const glm::vec3& position, float radius) const {
    float totalBiomass = 0.0f;

    for (const auto& patch : patches) {
        float dist = glm::length(glm::vec2(patch.center.x - position.x, patch.center.z - position.z));

        if (dist < radius + patch.radius) {
            float overlap = std::max(0.0f, 1.0f - dist / (radius + patch.radius));
            totalBiomass += patch.biomass * overlap;
        }
    }

    return totalBiomass;
}

// ===== Pollination System =====

std::vector<FlowerInstance*> GrassSystem::findPollinatingFlowers(const glm::vec3& position, float radius) {
    std::vector<FlowerInstance*> result;

    for (auto& patch : patches) {
        float dist = glm::length(glm::vec2(patch.center.x - position.x, patch.center.z - position.z));

        if (dist < radius + patch.radius) {
            for (auto& flower : patch.flowers) {
                if (flower.state == PollinationState::BLOOMING) {
                    float flowerDist = glm::length(flower.position - position);
                    if (flowerDist < radius) {
                        result.push_back(&flower);
                    }
                }
            }
        }
    }

    return result;
}

GrassSystem::PollinatorReward GrassSystem::pollinatorVisit(FlowerInstance* flower, float pollenCarried) {
    PollinatorReward reward;
    reward.pollenCollected = 0.0f;
    reward.nectarCollected = 0.0f;
    reward.causedPollination = false;

    if (!flower || flower->state != PollinationState::BLOOMING) {
        return reward;
    }

    // Collect nectar
    reward.nectarCollected = flower->nectarAmount * 0.5f;
    flower->nectarAmount -= reward.nectarCollected;

    // Collect pollen
    reward.pollenCollected = flower->pollenAmount * 0.3f;
    flower->pollenAmount -= reward.pollenCollected;

    // Pollination occurs if pollinator carries pollen from another flower
    if (pollenCarried > 0.1f && !flower->hasBeenVisited) {
        flower->hasBeenVisited = true;
        reward.causedPollination = true;
    }

    return reward;
}

std::vector<glm::vec3> GrassSystem::getSeedPositions() const {
    std::vector<glm::vec3> positions;

    for (const auto& patch : patches) {
        for (const auto& flower : patch.flowers) {
            if (flower.state == PollinationState::DISPERSING && flower.seedsProduced > 0) {
                positions.push_back(flower.position);
            }
        }
    }

    return positions;
}

bool GrassSystem::plantSeed(const glm::vec3& position, FlowerType type) {
    // Find nearest patch to plant in
    GrassPatch* nearestPatch = nullptr;
    float nearestDist = std::numeric_limits<float>::max();

    for (auto& patch : patches) {
        float dist = glm::length(glm::vec2(patch.center.x - position.x, patch.center.z - position.z));
        if (dist < patch.radius && dist < nearestDist) {
            nearestPatch = &patch;
            nearestDist = dist;
        }
    }

    if (!nearestPatch || nearestPatch->flowers.size() >= MAX_FLOWERS_PER_PATCH) {
        return false;
    }

    FlowerSpeciesConfig config = getFlowerSpeciesConfig(type);

    FlowerInstance newFlower;
    newFlower.position = position;
    newFlower.rotation = static_cast<float>(std::rand()) / RAND_MAX * 6.28318f;
    newFlower.scale = config.minSize;
    newFlower.type = type;
    newFlower.state = PollinationState::CLOSED;
    newFlower.petalColor = config.defaultPetalColor;
    newFlower.centerColor = config.defaultCenterColor;
    newFlower.bloomProgress = 0.0f;
    newFlower.pollenAmount = 0.0f;
    newFlower.nectarAmount = 0.0f;
    newFlower.age = 0.0f;
    newFlower.health = 1.0f;
    newFlower.hasBeenVisited = false;
    newFlower.seedsProduced = 0;

    nearestPatch->flowers.push_back(newFlower);
    allFlowers.push_back(newFlower);

    return true;
}

// ===== Ecosystem Integration =====

GrassSystem::GrassEcosystemStats GrassSystem::getEcosystemStats() const {
    GrassEcosystemStats stats;
    stats.totalBiomass = 0.0f;
    stats.totalGrazedArea = 0.0f;
    stats.totalFlowers = 0;
    stats.bloomingFlowers = 0;
    stats.pollinatedFlowers = 0;
    stats.averageGrassHealth = 0.0f;

    int healthyBlades = 0;

    for (const auto& patch : patches) {
        stats.totalBiomass += patch.biomass;
        stats.totalGrazedArea += patch.grazedAmount * patch.radius * patch.radius * 3.14159f;

        for (const auto& flower : patch.flowers) {
            stats.totalFlowers++;
            if (flower.state == PollinationState::BLOOMING) {
                stats.bloomingFlowers++;
            }
            if (flower.hasBeenVisited) {
                stats.pollinatedFlowers++;
            }
        }

        for (const auto& blade : patch.blades) {
            if (blade.grazedAmount < 0.5f) {
                healthyBlades++;
            }
        }
    }

    if (!allInstances.empty()) {
        stats.averageGrassHealth = static_cast<float>(healthyBlades) / allInstances.size();
    }

    return stats;
}

std::vector<std::pair<glm::vec3, glm::vec3>> GrassSystem::getBioluminescentGrassPositions() const {
    std::vector<std::pair<glm::vec3, glm::vec3>> positions;

    for (const auto& patch : patches) {
        for (const auto& blade : patch.blades) {
            GrassTypeConfig config = getGrassTypeConfig(blade.type);
            if (config.isAlien && config.glowIntensity > 0.1f) {
                glm::vec3 color = config.tipColor * config.glowIntensity;
                positions.push_back({blade.position, color});
            }
        }

        for (const auto& flower : patch.flowers) {
            FlowerSpeciesConfig config = getFlowerSpeciesConfig(flower.type);
            if (config.isBioluminescent && flower.state == PollinationState::BLOOMING) {
                glm::vec3 color = flower.petalColor * config.glowIntensity;
                positions.push_back({flower.position, color});
            }
        }
    }

    return positions;
}

std::vector<glm::vec3> GrassSystem::getAlienGrassPositions() const {
    std::vector<glm::vec3> positions;

    for (const auto& patch : patches) {
        for (const auto& blade : patch.blades) {
            GrassTypeConfig config = getGrassTypeConfig(blade.type);
            if (config.isAlien) {
                positions.push_back(blade.position);
            }
        }
    }

    return positions;
}

bool GrassSystem::isFlowerInBloomSeason(FlowerType type, float dayOfYear) const {
    FlowerSpeciesConfig config = getFlowerSpeciesConfig(type);

    if (config.bloomSeasonStart <= config.bloomSeasonEnd) {
        return dayOfYear >= config.bloomSeasonStart && dayOfYear <= config.bloomSeasonEnd;
    } else {
        // Wraps around year end
        return dayOfYear >= config.bloomSeasonStart || dayOfYear <= config.bloomSeasonEnd;
    }
}

glm::vec3 GrassSystem::getFlowerSeasonalColor(FlowerType type, float dayOfYear) const {
    FlowerSpeciesConfig config = getFlowerSpeciesConfig(type);

    if (!isFlowerInBloomSeason(type, dayOfYear)) {
        return config.defaultPetalColor * 0.5f;
    }

    return config.defaultPetalColor;
}

GrassConfig GrassSystem::getConfigForBiome(int biomeType) {
    GrassConfig config;
    ClimateBiome biome = static_cast<ClimateBiome>(biomeType);

    switch (biome) {
        case ClimateBiome::TEMPERATE_GRASSLAND:
            config.density = 30.0f;
            config.maxHeight = 0.55f;
            config.primaryType = GrassType::TALL_GRASS;
            config.hasFlowers = true;
            config.flowerDensity = 0.2f;
            break;

        case ClimateBiome::SAVANNA:
            config.density = 20.0f;
            config.maxHeight = 0.8f;
            config.primaryType = GrassType::TALL_GRASS;
            break;

        case ClimateBiome::TUNDRA:
            config.density = 5.0f;
            config.maxHeight = 0.15f;
            config.primaryType = GrassType::TUNDRA_GRASS;
            break;

        case ClimateBiome::TROPICAL_RAINFOREST:
            config.density = 25.0f;
            config.maxHeight = 0.6f;
            config.primaryType = GrassType::TROPICAL_GRASS;
            config.hasFlowers = true;
            config.flowerDensity = 0.15f;
            break;

        default:
            config.density = 15.0f;
            config.maxHeight = 0.4f;
            config.primaryType = GrassType::MEADOW_GRASS;
            break;
    }

    return config;
}
