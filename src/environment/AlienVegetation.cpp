#include "AlienVegetation.h"
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

AlienPlantConfig getAlienPlantConfig(AlienPlantType type) {
    AlienPlantConfig config;
    config.type = type;
    config.isHostile = false;
    config.isPredatory = false;
    config.canFloat = false;
    config.canMove = false;
    config.emitsSpores = false;
    config.emitsSound = false;
    config.hasTendrils = false;
    config.isCrystalline = false;
    config.prefersRadiation = false;
    config.prefersDarkness = false;

    switch (type) {
        // Bioluminescent plants
        case AlienPlantType::GLOW_TENDRIL:
            config.name = "Glow Tendril";
            config.minHeight = 0.5f;
            config.maxHeight = 2.0f;
            config.minScale = 0.6f;
            config.maxScale = 1.2f;
            config.basePrimaryColor = {0.1f, 0.3f, 0.4f};
            config.baseSecondaryColor = {0.15f, 0.35f, 0.45f};
            config.baseGlowColor = {0.2f, 0.8f, 0.9f};
            config.colorVariation = 0.2f;
            config.glows = true;
            config.baseGlowIntensity = 0.6f;
            config.glowPulseSpeed = 0.5f;
            config.defaultGlowPattern = 1;
            config.energySource = AlienEnergySource::BIOLUMINESCENCE;
            config.dangerLevel = 0.0f;
            config.hasTendrils = true;
            config.baseTendrilCount = 5;
            config.prefersDarkness = true;
            break;

        case AlienPlantType::LIGHT_BULB_PLANT:
            config.name = "Light Bulb Plant";
            config.minHeight = 0.3f;
            config.maxHeight = 1.0f;
            config.minScale = 0.5f;
            config.maxScale = 1.0f;
            config.basePrimaryColor = {0.4f, 0.35f, 0.2f};
            config.baseSecondaryColor = {0.45f, 0.4f, 0.25f};
            config.baseGlowColor = {1.0f, 0.9f, 0.5f};
            config.colorVariation = 0.15f;
            config.glows = true;
            config.baseGlowIntensity = 0.8f;
            config.glowPulseSpeed = 0.3f;
            config.defaultGlowPattern = 2;
            config.energySource = AlienEnergySource::PHOTOSYNTHESIS;
            config.dangerLevel = 0.0f;
            config.prefersDarkness = true;
            break;

        case AlienPlantType::AURORA_FERN:
            config.name = "Aurora Fern";
            config.minHeight = 0.4f;
            config.maxHeight = 1.2f;
            config.minScale = 0.7f;
            config.maxScale = 1.3f;
            config.basePrimaryColor = {0.2f, 0.3f, 0.25f};
            config.baseSecondaryColor = {0.25f, 0.35f, 0.3f};
            config.baseGlowColor = {0.3f, 1.0f, 0.5f};
            config.colorVariation = 0.3f;
            config.glows = true;
            config.baseGlowIntensity = 0.5f;
            config.glowPulseSpeed = 0.8f;
            config.defaultGlowPattern = 3;  // Color-shifting
            config.energySource = AlienEnergySource::ELECTROMAGNETIC;
            config.dangerLevel = 0.0f;
            break;

        case AlienPlantType::NEON_MOSS:
            config.name = "Neon Moss";
            config.minHeight = 0.05f;
            config.maxHeight = 0.15f;
            config.minScale = 0.8f;
            config.maxScale = 2.0f;
            config.basePrimaryColor = {0.1f, 0.2f, 0.15f};
            config.baseSecondaryColor = {0.15f, 0.25f, 0.2f};
            config.baseGlowColor = {0.0f, 1.0f, 0.4f};
            config.colorVariation = 0.25f;
            config.glows = true;
            config.baseGlowIntensity = 0.7f;
            config.glowPulseSpeed = 0.2f;
            config.defaultGlowPattern = 1;
            config.energySource = AlienEnergySource::CHEMICAL;
            config.dangerLevel = 0.0f;
            config.prefersDarkness = true;
            break;

        case AlienPlantType::PHOTON_FLOWER:
            config.name = "Photon Flower";
            config.minHeight = 0.6f;
            config.maxHeight = 1.5f;
            config.minScale = 0.5f;
            config.maxScale = 1.0f;
            config.basePrimaryColor = {0.3f, 0.25f, 0.35f};
            config.baseSecondaryColor = {0.35f, 0.3f, 0.4f};
            config.baseGlowColor = {1.0f, 0.5f, 1.0f};
            config.colorVariation = 0.2f;
            config.glows = true;
            config.baseGlowIntensity = 1.0f;
            config.glowPulseSpeed = 1.5f;
            config.defaultGlowPattern = 4;  // Strobe
            config.energySource = AlienEnergySource::PHOTOSYNTHESIS;
            config.dangerLevel = 0.1f;  // Slightly disorienting
            break;

        case AlienPlantType::STARLIGHT_TREE:
            config.name = "Starlight Tree";
            config.minHeight = 5.0f;
            config.maxHeight = 15.0f;
            config.minScale = 0.8f;
            config.maxScale = 1.5f;
            config.basePrimaryColor = {0.15f, 0.12f, 0.2f};
            config.baseSecondaryColor = {0.2f, 0.17f, 0.25f};
            config.baseGlowColor = {0.7f, 0.8f, 1.0f};
            config.colorVariation = 0.15f;
            config.glows = true;
            config.baseGlowIntensity = 0.4f;
            config.glowPulseSpeed = 0.1f;
            config.defaultGlowPattern = 5;  // Twinkling
            config.energySource = AlienEnergySource::DARK_ENERGY;
            config.dangerLevel = 0.0f;
            config.prefersDarkness = true;
            break;

        // Crystal-based plants
        case AlienPlantType::CRYSTAL_SPIRE:
            config.name = "Crystal Spire";
            config.minHeight = 2.0f;
            config.maxHeight = 8.0f;
            config.minScale = 0.6f;
            config.maxScale = 1.5f;
            config.basePrimaryColor = {0.7f, 0.75f, 0.9f};
            config.baseSecondaryColor = {0.75f, 0.8f, 0.95f};
            config.baseGlowColor = {0.8f, 0.85f, 1.0f};
            config.colorVariation = 0.1f;
            config.glows = true;
            config.baseGlowIntensity = 0.3f;
            config.glowPulseSpeed = 0.05f;
            config.defaultGlowPattern = 0;
            config.energySource = AlienEnergySource::ELECTROMAGNETIC;
            config.dangerLevel = 0.2f;  // Sharp edges
            config.isCrystalline = true;
            config.minFacets = 4;
            config.maxFacets = 8;
            break;

        case AlienPlantType::GEM_FLOWER:
            config.name = "Gem Flower";
            config.minHeight = 0.3f;
            config.maxHeight = 0.8f;
            config.minScale = 0.4f;
            config.maxScale = 1.0f;
            config.basePrimaryColor = {0.9f, 0.3f, 0.5f};
            config.baseSecondaryColor = {0.95f, 0.35f, 0.55f};
            config.baseGlowColor = {1.0f, 0.4f, 0.6f};
            config.colorVariation = 0.25f;
            config.glows = true;
            config.baseGlowIntensity = 0.5f;
            config.glowPulseSpeed = 0.3f;
            config.defaultGlowPattern = 2;
            config.energySource = AlienEnergySource::PHOTOSYNTHESIS;
            config.dangerLevel = 0.0f;
            config.isCrystalline = true;
            config.minFacets = 5;
            config.maxFacets = 12;
            break;

        case AlienPlantType::PRISM_BUSH:
            config.name = "Prism Bush";
            config.minHeight = 0.8f;
            config.maxHeight = 2.0f;
            config.minScale = 0.7f;
            config.maxScale = 1.4f;
            config.basePrimaryColor = {0.85f, 0.9f, 0.95f};
            config.baseSecondaryColor = {0.9f, 0.95f, 1.0f};
            config.baseGlowColor = {1.0f, 1.0f, 1.0f};
            config.colorVariation = 0.05f;
            config.glows = true;
            config.baseGlowIntensity = 0.2f;
            config.glowPulseSpeed = 0.0f;  // Constant
            config.defaultGlowPattern = 0;
            config.energySource = AlienEnergySource::PHOTOSYNTHESIS;
            config.dangerLevel = 0.1f;
            config.isCrystalline = true;
            config.minFacets = 20;
            config.maxFacets = 50;
            break;

        // Energy-based plants
        case AlienPlantType::PLASMA_TREE:
            config.name = "Plasma Tree";
            config.minHeight = 4.0f;
            config.maxHeight = 12.0f;
            config.minScale = 0.8f;
            config.maxScale = 1.5f;
            config.basePrimaryColor = {0.2f, 0.1f, 0.25f};
            config.baseSecondaryColor = {0.25f, 0.15f, 0.3f};
            config.baseGlowColor = {0.8f, 0.2f, 1.0f};
            config.colorVariation = 0.2f;
            config.glows = true;
            config.baseGlowIntensity = 0.9f;
            config.glowPulseSpeed = 2.0f;
            config.defaultGlowPattern = 6;  // Plasma flow
            config.energySource = AlienEnergySource::ELECTROMAGNETIC;
            config.dangerLevel = 0.5f;
            config.isHostile = false;
            config.hasTendrils = true;
            config.baseTendrilCount = 8;
            break;

        case AlienPlantType::VOID_BLOSSOM:
            config.name = "Void Blossom";
            config.minHeight = 0.4f;
            config.maxHeight = 1.0f;
            config.minScale = 0.5f;
            config.maxScale = 1.2f;
            config.basePrimaryColor = {0.05f, 0.0f, 0.1f};
            config.baseSecondaryColor = {0.1f, 0.0f, 0.15f};
            config.baseGlowColor = {0.3f, 0.0f, 0.5f};
            config.colorVariation = 0.1f;
            config.glows = true;
            config.baseGlowIntensity = 0.6f;
            config.glowPulseSpeed = 0.4f;
            config.defaultGlowPattern = 7;  // Void pulse
            config.energySource = AlienEnergySource::DARK_ENERGY;
            config.dangerLevel = 0.3f;
            config.prefersDarkness = true;
            break;

        case AlienPlantType::LIGHTNING_FERN:
            config.name = "Lightning Fern";
            config.minHeight = 0.5f;
            config.maxHeight = 1.5f;
            config.minScale = 0.6f;
            config.maxScale = 1.3f;
            config.basePrimaryColor = {0.2f, 0.25f, 0.3f};
            config.baseSecondaryColor = {0.25f, 0.3f, 0.35f};
            config.baseGlowColor = {0.5f, 0.7f, 1.0f};
            config.colorVariation = 0.15f;
            config.glows = true;
            config.baseGlowIntensity = 0.7f;
            config.glowPulseSpeed = 5.0f;  // Fast flicker
            config.defaultGlowPattern = 8;  // Lightning
            config.energySource = AlienEnergySource::ELECTROMAGNETIC;
            config.dangerLevel = 0.4f;
            config.emitsSound = true;
            config.soundRange = 10.0f;
            break;

        // Organic alien plants
        case AlienPlantType::TENDRIL_TREE:
            config.name = "Tendril Tree";
            config.minHeight = 3.0f;
            config.maxHeight = 10.0f;
            config.minScale = 0.7f;
            config.maxScale = 1.5f;
            config.basePrimaryColor = {0.25f, 0.15f, 0.3f};
            config.baseSecondaryColor = {0.3f, 0.2f, 0.35f};
            config.baseGlowColor = {0.4f, 0.2f, 0.5f};
            config.colorVariation = 0.2f;
            config.glows = true;
            config.baseGlowIntensity = 0.3f;
            config.glowPulseSpeed = 0.2f;
            config.defaultGlowPattern = 1;
            config.energySource = AlienEnergySource::PSYCHIC;
            config.dangerLevel = 0.3f;
            config.hasTendrils = true;
            config.baseTendrilCount = 15;
            break;

        case AlienPlantType::SPORE_TOWER:
            config.name = "Spore Tower";
            config.minHeight = 2.0f;
            config.maxHeight = 6.0f;
            config.minScale = 0.6f;
            config.maxScale = 1.2f;
            config.basePrimaryColor = {0.35f, 0.3f, 0.25f};
            config.baseSecondaryColor = {0.4f, 0.35f, 0.3f};
            config.baseGlowColor = {0.6f, 0.5f, 0.3f};
            config.colorVariation = 0.15f;
            config.glows = true;
            config.baseGlowIntensity = 0.2f;
            config.glowPulseSpeed = 0.1f;
            config.defaultGlowPattern = 0;
            config.energySource = AlienEnergySource::CHEMICAL;
            config.dangerLevel = 0.2f;
            config.emitsSpores = true;
            config.sporeInterval = 30.0f;
            break;

        case AlienPlantType::EYE_STALK:
            config.name = "Eye Stalk";
            config.minHeight = 0.5f;
            config.maxHeight = 2.0f;
            config.minScale = 0.4f;
            config.maxScale = 1.0f;
            config.basePrimaryColor = {0.4f, 0.35f, 0.3f};
            config.baseSecondaryColor = {0.45f, 0.4f, 0.35f};
            config.baseGlowColor = {0.9f, 0.7f, 0.0f};
            config.colorVariation = 0.2f;
            config.glows = true;
            config.baseGlowIntensity = 0.4f;
            config.glowPulseSpeed = 0.05f;
            config.defaultGlowPattern = 9;  // Blink
            config.energySource = AlienEnergySource::PSYCHIC;
            config.dangerLevel = 0.1f;
            break;

        case AlienPlantType::TENTACLE_GRASS:
            config.name = "Tentacle Grass";
            config.minHeight = 0.3f;
            config.maxHeight = 0.8f;
            config.minScale = 0.5f;
            config.maxScale = 1.5f;
            config.basePrimaryColor = {0.3f, 0.2f, 0.35f};
            config.baseSecondaryColor = {0.35f, 0.25f, 0.4f};
            config.baseGlowColor = {0.5f, 0.3f, 0.6f};
            config.colorVariation = 0.25f;
            config.glows = true;
            config.baseGlowIntensity = 0.25f;
            config.glowPulseSpeed = 0.3f;
            config.defaultGlowPattern = 1;
            config.energySource = AlienEnergySource::KINETIC;
            config.dangerLevel = 0.15f;
            config.hasTendrils = true;
            config.baseTendrilCount = 20;
            break;

        // Floating plants
        case AlienPlantType::FLOAT_POD:
            config.name = "Float Pod";
            config.minHeight = 0.2f;
            config.maxHeight = 0.5f;
            config.minScale = 0.3f;
            config.maxScale = 0.8f;
            config.basePrimaryColor = {0.6f, 0.55f, 0.7f};
            config.baseSecondaryColor = {0.65f, 0.6f, 0.75f};
            config.baseGlowColor = {0.7f, 0.6f, 0.9f};
            config.colorVariation = 0.2f;
            config.glows = true;
            config.baseGlowIntensity = 0.3f;
            config.glowPulseSpeed = 0.2f;
            config.defaultGlowPattern = 1;
            config.energySource = AlienEnergySource::ELECTROMAGNETIC;
            config.dangerLevel = 0.0f;
            config.canFloat = true;
            break;

        case AlienPlantType::HOVER_BLOOM:
            config.name = "Hover Bloom";
            config.minHeight = 0.3f;
            config.maxHeight = 0.8f;
            config.minScale = 0.4f;
            config.maxScale = 1.0f;
            config.basePrimaryColor = {0.8f, 0.6f, 0.9f};
            config.baseSecondaryColor = {0.85f, 0.65f, 0.95f};
            config.baseGlowColor = {0.9f, 0.7f, 1.0f};
            config.colorVariation = 0.25f;
            config.glows = true;
            config.baseGlowIntensity = 0.5f;
            config.glowPulseSpeed = 0.4f;
            config.defaultGlowPattern = 2;
            config.energySource = AlienEnergySource::DARK_ENERGY;
            config.dangerLevel = 0.0f;
            config.canFloat = true;
            break;

        // Sound plants
        case AlienPlantType::SONIC_CHIME:
            config.name = "Sonic Chime";
            config.minHeight = 1.0f;
            config.maxHeight = 3.0f;
            config.minScale = 0.5f;
            config.maxScale = 1.2f;
            config.basePrimaryColor = {0.7f, 0.75f, 0.8f};
            config.baseSecondaryColor = {0.75f, 0.8f, 0.85f};
            config.baseGlowColor = {0.8f, 0.85f, 0.9f};
            config.colorVariation = 0.1f;
            config.glows = true;
            config.baseGlowIntensity = 0.2f;
            config.glowPulseSpeed = 0.0f;
            config.defaultGlowPattern = 0;
            config.energySource = AlienEnergySource::KINETIC;
            config.dangerLevel = 0.0f;
            config.emitsSound = true;
            config.soundRange = 20.0f;
            config.isCrystalline = true;
            config.minFacets = 6;
            config.maxFacets = 12;
            break;

        // Predatory plants
        case AlienPlantType::PREDATOR_PLANT:
            config.name = "Predator Plant";
            config.minHeight = 1.0f;
            config.maxHeight = 3.0f;
            config.minScale = 0.6f;
            config.maxScale = 1.5f;
            config.basePrimaryColor = {0.4f, 0.2f, 0.25f};
            config.baseSecondaryColor = {0.5f, 0.25f, 0.3f};
            config.baseGlowColor = {0.8f, 0.2f, 0.3f};
            config.colorVariation = 0.2f;
            config.glows = true;
            config.baseGlowIntensity = 0.4f;
            config.glowPulseSpeed = 1.0f;
            config.defaultGlowPattern = 10;  // Warning pulse
            config.energySource = AlienEnergySource::CHEMICAL;
            config.dangerLevel = 0.8f;
            config.isHostile = true;
            config.isPredatory = true;
            config.aggressiveness = 0.7f;
            config.hasTendrils = true;
            config.baseTendrilCount = 6;
            break;

        // Extreme environment
        case AlienPlantType::THERMAL_VENT_PLANT:
            config.name = "Thermal Vent Plant";
            config.minHeight = 0.5f;
            config.maxHeight = 2.0f;
            config.minScale = 0.6f;
            config.maxScale = 1.3f;
            config.basePrimaryColor = {0.5f, 0.3f, 0.2f};
            config.baseSecondaryColor = {0.6f, 0.35f, 0.25f};
            config.baseGlowColor = {1.0f, 0.5f, 0.2f};
            config.colorVariation = 0.15f;
            config.glows = true;
            config.baseGlowIntensity = 0.6f;
            config.glowPulseSpeed = 0.5f;
            config.defaultGlowPattern = 11;  // Heat shimmer
            config.energySource = AlienEnergySource::THERMAL;
            config.dangerLevel = 0.4f;
            config.preferredTemperature = 80.0f;
            config.temperatureTolerance = 50.0f;
            break;

        case AlienPlantType::ICE_CRYSTAL_PLANT:
            config.name = "Ice Crystal Plant";
            config.minHeight = 0.4f;
            config.maxHeight = 1.5f;
            config.minScale = 0.5f;
            config.maxScale = 1.2f;
            config.basePrimaryColor = {0.85f, 0.9f, 1.0f};
            config.baseSecondaryColor = {0.9f, 0.95f, 1.0f};
            config.baseGlowColor = {0.7f, 0.85f, 1.0f};
            config.colorVariation = 0.05f;
            config.glows = true;
            config.baseGlowIntensity = 0.3f;
            config.glowPulseSpeed = 0.1f;
            config.defaultGlowPattern = 0;
            config.energySource = AlienEnergySource::THERMAL;
            config.dangerLevel = 0.2f;
            config.isCrystalline = true;
            config.minFacets = 6;
            config.maxFacets = 20;
            config.preferredTemperature = -30.0f;
            config.temperatureTolerance = 20.0f;
            break;

        case AlienPlantType::RADIATION_FEEDER:
            config.name = "Radiation Feeder";
            config.minHeight = 0.6f;
            config.maxHeight = 2.0f;
            config.minScale = 0.5f;
            config.maxScale = 1.3f;
            config.basePrimaryColor = {0.3f, 0.4f, 0.2f};
            config.baseSecondaryColor = {0.35f, 0.45f, 0.25f};
            config.baseGlowColor = {0.4f, 1.0f, 0.3f};
            config.colorVariation = 0.2f;
            config.glows = true;
            config.baseGlowIntensity = 0.7f;
            config.glowPulseSpeed = 0.8f;
            config.defaultGlowPattern = 12;  // Geiger counter
            config.energySource = AlienEnergySource::RADIATION;
            config.dangerLevel = 0.3f;
            config.prefersRadiation = true;
            break;

        default:
            config.name = "Unknown Alien Plant";
            config.minHeight = 0.5f;
            config.maxHeight = 2.0f;
            config.minScale = 0.5f;
            config.maxScale = 1.0f;
            config.basePrimaryColor = {0.4f, 0.3f, 0.5f};
            config.baseSecondaryColor = {0.45f, 0.35f, 0.55f};
            config.baseGlowColor = {0.5f, 0.4f, 0.6f};
            config.colorVariation = 0.2f;
            config.glows = true;
            config.baseGlowIntensity = 0.3f;
            config.glowPulseSpeed = 0.3f;
            config.defaultGlowPattern = 1;
            config.energySource = AlienEnergySource::BIOLUMINESCENCE;
            config.dangerLevel = 0.1f;
            break;
    }

    return config;
}

bool isAlienPlantDangerous(AlienPlantType type) {
    AlienPlantConfig config = getAlienPlantConfig(type);
    return config.dangerLevel > 0.3f || config.isHostile || config.isPredatory;
}

bool doesAlienPlantGlow(AlienPlantType type) {
    AlienPlantConfig config = getAlienPlantConfig(type);
    return config.glows && config.baseGlowIntensity > 0.1f;
}

bool isAlienPlantCrystalline(AlienPlantType type) {
    AlienPlantConfig config = getAlienPlantConfig(type);
    return config.isCrystalline;
}

// ===== AlienVegetationSystem Implementation =====

AlienVegetationSystem::AlienVegetationSystem() = default;

AlienVegetationSystem::~AlienVegetationSystem() = default;

void AlienVegetationSystem::initialize(DX12Device* device, const Terrain* t) {
    dx12Device = device;
    terrain = t;
}

void AlienVegetationSystem::generate(unsigned int seed) {
    colonies.clear();
    allInstances.clear();

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

    float worldSize = TerrainSampler::WORLD_SIZE;
    if (terrain) {
        worldSize = terrain->getWidth() * terrain->getScale();
    }

    // Generate different alien zones
    int numBioluminescentZones = 3 + static_cast<int>(dist01(rng) * 3);
    int numCrystalFormations = 2 + static_cast<int>(dist01(rng) * 2);
    int numTendrilForests = 2 + static_cast<int>(dist01(rng) * 2);
    int numFloatingGardens = 1 + static_cast<int>(dist01(rng) * 2);
    int numPredatorPatches = 1 + static_cast<int>(dist01(rng) * 2);

    for (int i = 0; i < numBioluminescentZones; i++) {
        float x = (dist01(rng) - 0.5f) * worldSize;
        float z = (dist01(rng) - 0.5f) * worldSize;
        float radius = 20.0f + dist01(rng) * 30.0f;
        generateBioluminescentZone({x, 0.0f, z}, radius, rng());
    }

    for (int i = 0; i < numCrystalFormations; i++) {
        float x = (dist01(rng) - 0.5f) * worldSize;
        float z = (dist01(rng) - 0.5f) * worldSize;
        float radius = 15.0f + dist01(rng) * 25.0f;
        generateCrystalFormation({x, 0.0f, z}, radius, rng());
    }

    for (int i = 0; i < numTendrilForests; i++) {
        float x = (dist01(rng) - 0.5f) * worldSize;
        float z = (dist01(rng) - 0.5f) * worldSize;
        float radius = 25.0f + dist01(rng) * 35.0f;
        generateTendrilForest({x, 0.0f, z}, radius, rng());
    }

    for (int i = 0; i < numFloatingGardens; i++) {
        float x = (dist01(rng) - 0.5f) * worldSize;
        float z = (dist01(rng) - 0.5f) * worldSize;
        float radius = 15.0f + dist01(rng) * 20.0f;
        generateFloatingGarden({x, 0.0f, z}, radius, rng());
    }

    for (int i = 0; i < numPredatorPatches; i++) {
        float x = (dist01(rng) - 0.5f) * worldSize;
        float z = (dist01(rng) - 0.5f) * worldSize;
        float radius = 10.0f + dist01(rng) * 15.0f;
        generatePredatorPatch({x, 0.0f, z}, radius, rng());
    }

    if (dx12Device && !allInstances.empty()) {
        createBuffers();
    }
}

void AlienVegetationSystem::generateBioluminescentZone(const glm::vec3& center, float radius, unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);

    AlienPlantColony colony;
    colony.center = center;
    colony.radius = radius;
    colony.dominantType = AlienPlantType::GLOW_TENDRIL;
    colony.isHiveMind = false;
    colony.synchronization = 0.3f + dist01(rng) * 0.4f;

    std::vector<AlienPlantType> glowTypes = {
        AlienPlantType::GLOW_TENDRIL,
        AlienPlantType::LIGHT_BULB_PLANT,
        AlienPlantType::AURORA_FERN,
        AlienPlantType::NEON_MOSS,
        AlienPlantType::PHOTON_FLOWER
    };

    int numPlants = 20 + static_cast<int>(dist01(rng) * 30);

    for (int i = 0; i < numPlants; i++) {
        float r = std::sqrt(dist01(rng)) * radius;
        float theta = angleDist(rng);
        float x = center.x + r * std::cos(theta);
        float z = center.z + r * std::sin(theta);

        if (!isValidAlienPlantLocation(x, z)) continue;

        float height = 0.0f;
        if (terrain && terrain->isInBounds(x, z)) {
            height = terrain->getHeight(x, z);
        }

        AlienPlantType plantType = glowTypes[static_cast<int>(dist01(rng) * glowTypes.size()) % glowTypes.size()];
        AlienPlantConfig config = getAlienPlantConfig(plantType);

        AlienPlantInstance plant;
        plant.position = {x, height, z};
        plant.rotation = angleDist(rng);
        plant.scale = config.minScale + dist01(rng) * (config.maxScale - config.minScale);
        plant.type = plantType;
        plant.primaryColor = config.basePrimaryColor + glm::vec3((dist01(rng) - 0.5f) * config.colorVariation);
        plant.secondaryColor = config.baseSecondaryColor;
        plant.glowColor = generateGlowColor(plantType, rng());
        plant.glowIntensity = config.baseGlowIntensity * (0.7f + dist01(rng) * 0.6f);
        plant.glowPhase = dist01(rng) * 6.28318f;
        plant.animationPhase = dist01(rng) * 6.28318f;
        plant.animationSpeed = config.glowPulseSpeed;
        plant.behaviorState = AlienBehaviorState::IDLE;
        plant.health = 0.8f + dist01(rng) * 0.2f;
        plant.energy = 0.5f + dist01(rng) * 0.5f;
        plant.age = dist01(rng) * 100.0f;
        plant.growthStage = 0.7f + dist01(rng) * 0.3f;
        plant.dangerRadius = 0.0f;
        plant.attractionRadius = 5.0f;
        plant.isHostile = false;
        plant.isPredatory = false;
        plant.isFloating = false;
        plant.glowPattern = config.defaultGlowPattern;
        plant.glowCycleSpeed = config.glowPulseSpeed;

        if (config.hasTendrils) {
            initializeTendrils(plant, rng());
        }

        colony.plants.push_back(plant);
        allInstances.push_back(plant);
    }

    colony.colonyHealth = 0.9f;
    colony.colonyEnergy = 0.8f;
    colony.areaGlowIntensity = 0.5f;
    colony.areaDanger = 0.0f;
    colony.areaWeirdness = 0.6f;

    if (!colony.plants.empty()) {
        colonies.push_back(std::move(colony));
    }
}

void AlienVegetationSystem::generateCrystalFormation(const glm::vec3& center, float radius, unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);

    AlienPlantColony colony;
    colony.center = center;
    colony.radius = radius;
    colony.dominantType = AlienPlantType::CRYSTAL_SPIRE;
    colony.isHiveMind = true;
    colony.synchronization = 0.8f;

    std::vector<AlienPlantType> crystalTypes = {
        AlienPlantType::CRYSTAL_SPIRE,
        AlienPlantType::GEM_FLOWER,
        AlienPlantType::PRISM_BUSH,
        AlienPlantType::SONIC_CHIME,
        AlienPlantType::ICE_CRYSTAL_PLANT
    };

    int numPlants = 15 + static_cast<int>(dist01(rng) * 20);

    for (int i = 0; i < numPlants; i++) {
        float r = std::sqrt(dist01(rng)) * radius;
        float theta = angleDist(rng);
        float x = center.x + r * std::cos(theta);
        float z = center.z + r * std::sin(theta);

        if (!isValidAlienPlantLocation(x, z)) continue;

        float height = 0.0f;
        if (terrain && terrain->isInBounds(x, z)) {
            height = terrain->getHeight(x, z);
        }

        AlienPlantType plantType = crystalTypes[static_cast<int>(dist01(rng) * crystalTypes.size()) % crystalTypes.size()];
        AlienPlantConfig config = getAlienPlantConfig(plantType);

        AlienPlantInstance plant;
        plant.position = {x, height, z};
        plant.rotation = angleDist(rng);
        plant.scale = config.minScale + dist01(rng) * (config.maxScale - config.minScale);
        plant.type = plantType;
        plant.primaryColor = config.basePrimaryColor;
        plant.secondaryColor = config.baseSecondaryColor;
        plant.glowColor = config.baseGlowColor;
        plant.glowIntensity = config.baseGlowIntensity;
        plant.glowPhase = dist01(rng) * 6.28318f;
        plant.animationPhase = 0.0f;
        plant.animationSpeed = 0.0f;
        plant.behaviorState = AlienBehaviorState::IDLE;
        plant.health = 0.9f + dist01(rng) * 0.1f;
        plant.energy = 0.7f + dist01(rng) * 0.3f;
        plant.dangerRadius = 1.0f;
        plant.isHostile = false;
        plant.isPredatory = false;
        plant.isFloating = false;
        plant.emitsSound = config.emitsSound;
        plant.soundFrequency = 200.0f + dist01(rng) * 1000.0f;
        plant.facetCount = config.minFacets + static_cast<int>(dist01(rng) * (config.maxFacets - config.minFacets));
        plant.refractiveIndex = 1.5f + dist01(rng) * 0.5f;
        plant.crystalClarity = 0.7f + dist01(rng) * 0.3f;
        plant.glowPattern = config.defaultGlowPattern;

        colony.plants.push_back(plant);
        allInstances.push_back(plant);
    }

    colony.colonyHealth = 0.95f;
    colony.colonyEnergy = 0.9f;
    colony.areaGlowIntensity = 0.3f;
    colony.areaDanger = 0.2f;
    colony.areaWeirdness = 0.8f;

    if (!colony.plants.empty()) {
        colonies.push_back(std::move(colony));
    }
}

void AlienVegetationSystem::generateTendrilForest(const glm::vec3& center, float radius, unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);

    AlienPlantColony colony;
    colony.center = center;
    colony.radius = radius;
    colony.dominantType = AlienPlantType::TENDRIL_TREE;
    colony.isHiveMind = true;
    colony.synchronization = 0.6f;

    std::vector<AlienPlantType> tendrilTypes = {
        AlienPlantType::TENDRIL_TREE,
        AlienPlantType::TENTACLE_GRASS,
        AlienPlantType::SPORE_TOWER,
        AlienPlantType::EYE_STALK
    };

    int numPlants = 25 + static_cast<int>(dist01(rng) * 35);

    for (int i = 0; i < numPlants; i++) {
        float r = std::sqrt(dist01(rng)) * radius;
        float theta = angleDist(rng);
        float x = center.x + r * std::cos(theta);
        float z = center.z + r * std::sin(theta);

        if (!isValidAlienPlantLocation(x, z)) continue;

        float height = 0.0f;
        if (terrain && terrain->isInBounds(x, z)) {
            height = terrain->getHeight(x, z);
        }

        AlienPlantType plantType = tendrilTypes[static_cast<int>(dist01(rng) * tendrilTypes.size()) % tendrilTypes.size()];
        AlienPlantConfig config = getAlienPlantConfig(plantType);

        AlienPlantInstance plant;
        plant.position = {x, height, z};
        plant.rotation = angleDist(rng);
        plant.scale = config.minScale + dist01(rng) * (config.maxScale - config.minScale);
        plant.type = plantType;
        plant.primaryColor = config.basePrimaryColor + glm::vec3((dist01(rng) - 0.5f) * config.colorVariation);
        plant.secondaryColor = config.baseSecondaryColor;
        plant.glowColor = config.baseGlowColor;
        plant.glowIntensity = config.baseGlowIntensity;
        plant.glowPhase = dist01(rng) * 6.28318f;
        plant.animationPhase = dist01(rng) * 6.28318f;
        plant.animationSpeed = 0.5f + dist01(rng) * 0.5f;
        plant.behaviorState = AlienBehaviorState::IDLE;
        plant.health = 0.8f + dist01(rng) * 0.2f;
        plant.energy = 0.6f + dist01(rng) * 0.4f;
        plant.psychicRange = 10.0f + dist01(rng) * 10.0f;
        plant.dangerRadius = 2.0f;
        plant.isHostile = false;
        plant.isPredatory = false;
        plant.glowPattern = config.defaultGlowPattern;
        plant.hasSpores = config.emitsSpores;
        plant.sporeTimer = dist01(rng) * config.sporeInterval;

        if (config.hasTendrils) {
            initializeTendrils(plant, rng());
        }

        colony.plants.push_back(plant);
        allInstances.push_back(plant);
    }

    colony.colonyHealth = 0.85f;
    colony.colonyEnergy = 0.75f;
    colony.areaGlowIntensity = 0.3f;
    colony.areaDanger = 0.3f;
    colony.areaWeirdness = 0.9f;

    if (!colony.plants.empty()) {
        colonies.push_back(std::move(colony));
    }
}

void AlienVegetationSystem::generateFloatingGarden(const glm::vec3& center, float radius, unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);

    AlienPlantColony colony;
    colony.center = center;
    colony.radius = radius;
    colony.dominantType = AlienPlantType::HOVER_BLOOM;
    colony.isHiveMind = false;
    colony.synchronization = 0.2f;

    std::vector<AlienPlantType> floatTypes = {
        AlienPlantType::FLOAT_POD,
        AlienPlantType::HOVER_BLOOM,
        AlienPlantType::STARLIGHT_TREE
    };

    int numPlants = 15 + static_cast<int>(dist01(rng) * 20);

    for (int i = 0; i < numPlants; i++) {
        float r = std::sqrt(dist01(rng)) * radius;
        float theta = angleDist(rng);
        float x = center.x + r * std::cos(theta);
        float z = center.z + r * std::sin(theta);

        if (!isValidAlienPlantLocation(x, z)) continue;

        float groundHeight = 0.0f;
        if (terrain && terrain->isInBounds(x, z)) {
            groundHeight = terrain->getHeight(x, z);
        }

        AlienPlantType plantType = floatTypes[static_cast<int>(dist01(rng) * floatTypes.size()) % floatTypes.size()];
        AlienPlantConfig config = getAlienPlantConfig(plantType);

        AlienPlantInstance plant;
        plant.isFloating = config.canFloat;
        plant.floatHeight = config.canFloat ? (1.0f + dist01(rng) * 3.0f) : 0.0f;
        plant.position = {x, groundHeight + plant.floatHeight, z};
        plant.rotation = angleDist(rng);
        plant.scale = config.minScale + dist01(rng) * (config.maxScale - config.minScale);
        plant.type = plantType;
        plant.primaryColor = config.basePrimaryColor;
        plant.secondaryColor = config.baseSecondaryColor;
        plant.glowColor = config.baseGlowColor;
        plant.glowIntensity = config.baseGlowIntensity;
        plant.glowPhase = dist01(rng) * 6.28318f;
        plant.animationPhase = dist01(rng) * 6.28318f;
        plant.animationSpeed = 0.3f + dist01(rng) * 0.3f;
        plant.behaviorState = AlienBehaviorState::IDLE;
        plant.health = 0.9f + dist01(rng) * 0.1f;
        plant.energy = 0.8f + dist01(rng) * 0.2f;
        plant.dangerRadius = 0.0f;
        plant.attractionRadius = 8.0f;
        plant.isHostile = false;
        plant.isPredatory = false;
        plant.glowPattern = config.defaultGlowPattern;
        plant.movementOffset = {0.0f, 0.0f, 0.0f};

        colony.plants.push_back(plant);
        allInstances.push_back(plant);
    }

    colony.colonyHealth = 0.95f;
    colony.colonyEnergy = 0.9f;
    colony.areaGlowIntensity = 0.4f;
    colony.areaDanger = 0.0f;
    colony.areaWeirdness = 0.95f;

    if (!colony.plants.empty()) {
        colonies.push_back(std::move(colony));
    }
}

void AlienVegetationSystem::generatePredatorPatch(const glm::vec3& center, float radius, unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);

    AlienPlantColony colony;
    colony.center = center;
    colony.radius = radius;
    colony.dominantType = AlienPlantType::PREDATOR_PLANT;
    colony.isHiveMind = true;
    colony.synchronization = 0.9f;

    int numPlants = 5 + static_cast<int>(dist01(rng) * 8);

    for (int i = 0; i < numPlants; i++) {
        float r = std::sqrt(dist01(rng)) * radius;
        float theta = angleDist(rng);
        float x = center.x + r * std::cos(theta);
        float z = center.z + r * std::sin(theta);

        if (!isValidAlienPlantLocation(x, z)) continue;

        float height = 0.0f;
        if (terrain && terrain->isInBounds(x, z)) {
            height = terrain->getHeight(x, z);
        }

        AlienPlantConfig config = getAlienPlantConfig(AlienPlantType::PREDATOR_PLANT);

        AlienPlantInstance plant;
        plant.position = {x, height, z};
        plant.rotation = angleDist(rng);
        plant.scale = config.minScale + dist01(rng) * (config.maxScale - config.minScale);
        plant.type = AlienPlantType::PREDATOR_PLANT;
        plant.primaryColor = config.basePrimaryColor;
        plant.secondaryColor = config.baseSecondaryColor;
        plant.glowColor = config.baseGlowColor;
        plant.glowIntensity = config.baseGlowIntensity;
        plant.glowPhase = dist01(rng) * 6.28318f;
        plant.animationPhase = dist01(rng) * 6.28318f;
        plant.animationSpeed = 1.0f;
        plant.behaviorState = AlienBehaviorState::IDLE;
        plant.health = 0.9f + dist01(rng) * 0.1f;
        plant.energy = 0.5f + dist01(rng) * 0.5f;
        plant.dangerRadius = 3.0f;
        plant.attractionRadius = 8.0f;  // Lures prey
        plant.isHostile = true;
        plant.isPredatory = true;
        plant.glowPattern = config.defaultGlowPattern;

        initializeTendrils(plant, rng());

        colony.plants.push_back(plant);
        allInstances.push_back(plant);
    }

    colony.colonyHealth = 0.85f;
    colony.colonyEnergy = 0.6f;
    colony.areaGlowIntensity = 0.4f;
    colony.areaDanger = 0.9f;
    colony.areaWeirdness = 0.7f;

    if (!colony.plants.empty()) {
        colonies.push_back(std::move(colony));
    }
}

void AlienVegetationSystem::initializeTendrils(AlienPlantInstance& plant, unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);

    AlienPlantConfig config = getAlienPlantConfig(plant.type);
    plant.tendrilCount = config.baseTendrilCount;
    plant.tendrilPositions.clear();
    plant.tendrilLengths.clear();

    for (int i = 0; i < plant.tendrilCount; i++) {
        float angle = angleDist(rng);
        float length = 0.5f + dist01(rng) * 2.0f;
        float height = 0.2f + dist01(rng) * 0.8f;

        glm::vec3 tendrilEnd = plant.position + glm::vec3(
            std::cos(angle) * length,
            height * plant.scale,
            std::sin(angle) * length
        );

        plant.tendrilPositions.push_back(tendrilEnd);
        plant.tendrilLengths.push_back(length);
    }
}

void AlienVegetationSystem::update(float deltaTime, const glm::vec3& cameraPos) {
    simulationTime += deltaTime;
    dayNightCycle += deltaTime / 600.0f;  // 10 min day cycle
    if (dayNightCycle >= 1.0f) dayNightCycle -= 1.0f;

    updateGlowAnimations(deltaTime);
    updatePlantBehaviors(deltaTime);
    updateFloatingPlants(deltaTime);
    updateTendrils(deltaTime);
    updatePredatoryPlants(deltaTime);
    updateSporeRelease(deltaTime);
    updateColonyBehavior(deltaTime);

    // Update visibility
    visibleInstanceCount = 0;
    for (const auto& instance : allInstances) {
        float dist = glm::length(glm::vec2(instance.position.x - cameraPos.x, instance.position.z - cameraPos.z));
        if (dist < maxRenderDistance) {
            visibleInstanceCount++;
        }
    }
}

void AlienVegetationSystem::updateGlowAnimations(float deltaTime) {
    for (auto& plant : allInstances) {
        plant.glowPhase += deltaTime * plant.glowCycleSpeed;
        if (plant.glowPhase > 6.28318f) {
            plant.glowPhase -= 6.28318f;
        }

        // Night boost for bioluminescent plants
        float nightBoost = 1.0f;
        if (dayNightCycle > 0.5f) {
            nightBoost = 1.0f + (dayNightCycle - 0.5f) * 0.5f;
        }

        AlienPlantConfig config = getAlienPlantConfig(plant.type);
        if (config.prefersDarkness) {
            plant.glowIntensity = config.baseGlowIntensity * nightBoost;
        }
    }
}

void AlienVegetationSystem::updatePlantBehaviors(float deltaTime) {
    for (auto& plant : allInstances) {
        plant.behaviorTimer += deltaTime;
        plant.animationPhase += deltaTime * plant.animationSpeed;
    }
}

void AlienVegetationSystem::updateFloatingPlants(float deltaTime) {
    for (auto& plant : allInstances) {
        if (plant.isFloating) {
            // Gentle bobbing motion
            float bobOffset = std::sin(simulationTime * 0.5f + plant.glowPhase) * 0.2f;
            float swayX = std::sin(simulationTime * 0.3f + plant.position.x * 0.1f) * 0.1f;
            float swayZ = std::cos(simulationTime * 0.25f + plant.position.z * 0.1f) * 0.1f;

            plant.movementOffset = {swayX, bobOffset, swayZ};
        }
    }
}

void AlienVegetationSystem::updateTendrils(float deltaTime) {
    for (auto& plant : allInstances) {
        if (!plant.tendrilPositions.empty()) {
            updateTendrilPositions(plant, deltaTime);
        }
    }
}

void AlienVegetationSystem::updateTendrilPositions(AlienPlantInstance& plant, float deltaTime) {
    for (size_t i = 0; i < plant.tendrilPositions.size(); i++) {
        float phase = plant.animationPhase + static_cast<float>(i) * 0.5f;
        float waveX = std::sin(phase) * 0.3f;
        float waveZ = std::cos(phase * 0.7f) * 0.3f;
        float waveY = std::sin(phase * 0.5f) * 0.1f;

        float baseAngle = 6.28318f * static_cast<float>(i) / plant.tendrilCount;
        float length = plant.tendrilLengths[i];

        plant.tendrilPositions[i] = plant.position + glm::vec3(
            std::cos(baseAngle) * length + waveX,
            0.5f * plant.scale + waveY,
            std::sin(baseAngle) * length + waveZ
        );
    }
}

void AlienVegetationSystem::updatePredatoryPlants(float deltaTime) {
    for (auto& plant : allInstances) {
        if (plant.isPredatory) {
            // Hungry plants glow more intensely
            if (plant.energy < 0.3f) {
                plant.glowIntensity = getAlienPlantConfig(plant.type).baseGlowIntensity * 1.5f;
                plant.behaviorState = AlienBehaviorState::HUNTING;
            } else {
                plant.behaviorState = AlienBehaviorState::IDLE;
            }

            // Energy decay
            plant.energy -= 0.01f * deltaTime / 60.0f;
            plant.energy = std::max(0.0f, plant.energy);
        }
    }
}

void AlienVegetationSystem::updateSporeRelease(float deltaTime) {
    for (auto& plant : allInstances) {
        if (plant.hasSpores) {
            plant.sporeTimer -= deltaTime;
            if (plant.sporeTimer <= 0.0f) {
                // Release spores (visual effect would be handled elsewhere)
                AlienPlantConfig config = getAlienPlantConfig(plant.type);
                plant.sporeTimer = config.sporeInterval;
                plant.behaviorState = AlienBehaviorState::REPRODUCING;
            } else if (plant.behaviorState == AlienBehaviorState::REPRODUCING) {
                plant.behaviorState = AlienBehaviorState::IDLE;
            }
        }
    }
}

void AlienVegetationSystem::updateColonyBehavior(float deltaTime) {
    for (auto& colony : colonies) {
        if (colony.isHiveMind) {
            // Synchronize glow phases within colony
            float avgPhase = 0.0f;
            for (const auto& plant : colony.plants) {
                avgPhase += plant.glowPhase;
            }
            avgPhase /= colony.plants.size();

            for (auto& plant : colony.plants) {
                plant.glowPhase = glm::mix(plant.glowPhase, avgPhase, colony.synchronization * deltaTime);
            }
        }

        // Update colony stats
        float totalHealth = 0.0f;
        float totalEnergy = 0.0f;
        for (const auto& plant : colony.plants) {
            totalHealth += plant.health;
            totalEnergy += plant.energy;
        }
        colony.colonyHealth = totalHealth / colony.plants.size();
        colony.colonyEnergy = totalEnergy / colony.plants.size();
    }
}

void AlienVegetationSystem::render(ID3D12GraphicsCommandList* commandList) {
    if (allInstances.empty() || !dx12Device) return;
    updateInstanceBuffer();
}

// ===== Query Functions =====

AlienPlantColony* AlienVegetationSystem::findColonyAt(const glm::vec3& position, float radius) {
    for (auto& colony : colonies) {
        float dist = glm::length(glm::vec2(colony.center.x - position.x, colony.center.z - position.z));
        if (dist < colony.radius + radius) {
            return &colony;
        }
    }
    return nullptr;
}

float AlienVegetationSystem::getAliennessLevel(const glm::vec3& position, float radius) const {
    float maxWeirdness = 0.0f;

    for (const auto& colony : colonies) {
        float dist = glm::length(glm::vec2(colony.center.x - position.x, colony.center.z - position.z));
        if (dist < colony.radius + radius) {
            float effect = 1.0f - (dist / (colony.radius + radius));
            maxWeirdness = std::max(maxWeirdness, colony.areaWeirdness * effect);
        }
    }

    return maxWeirdness;
}

bool AlienVegetationSystem::isDangerousArea(const glm::vec3& position, float radius) const {
    return getDangerLevel(position, radius) > 0.5f;
}

float AlienVegetationSystem::getDangerLevel(const glm::vec3& position, float radius) const {
    float maxDanger = 0.0f;

    for (const auto& plant : allInstances) {
        if (plant.dangerRadius > 0.0f || plant.isPredatory) {
            float dist = glm::length(plant.position - position);
            if (dist < plant.dangerRadius + radius) {
                float danger = isAlienPlantDangerous(plant.type) ? 0.8f : 0.3f;
                maxDanger = std::max(maxDanger, danger);
            }
        }
    }

    return maxDanger;
}

float AlienVegetationSystem::getAttractionLevel(const glm::vec3& position, float radius) const {
    float totalAttraction = 0.0f;

    for (const auto& plant : allInstances) {
        if (plant.attractionRadius > 0.0f) {
            float dist = glm::length(plant.position - position);
            if (dist < plant.attractionRadius) {
                float attraction = plant.glowIntensity * (1.0f - dist / plant.attractionRadius);
                totalAttraction += attraction;
            }
        }
    }

    return std::min(1.0f, totalAttraction);
}

void AlienVegetationSystem::onCreatureNearby(const glm::vec3& creaturePos, float creatureSize) {
    for (auto& plant : allInstances) {
        float dist = glm::length(plant.position - creaturePos);

        // Reactive plants respond to proximity
        if (dist < plant.attractionRadius) {
            if (plant.isPredatory && plant.behaviorState == AlienBehaviorState::HUNTING) {
                plant.targetPosition = creaturePos;
                plant.behaviorState = AlienBehaviorState::ACTIVE;
            }

            // Eye stalks track creatures
            if (plant.type == AlienPlantType::EYE_STALK) {
                plant.targetPosition = creaturePos;
            }
        }
    }
}

float AlienVegetationSystem::plantAttack(const glm::vec3& position, float radius) {
    float totalDamage = 0.0f;

    for (auto& plant : allInstances) {
        if (plant.isPredatory && plant.behaviorState == AlienBehaviorState::ACTIVE) {
            float dist = glm::length(plant.position - position);
            if (dist < plant.dangerRadius + radius) {
                float damage = 10.0f * plant.health;
                totalDamage += damage;
                plant.energy += damage * 0.1f;  // Plant gains energy from attack
                plant.energy = std::min(1.0f, plant.energy);
            }
        }
    }

    return totalDamage;
}

std::vector<std::pair<glm::vec3, glm::vec3>> AlienVegetationSystem::getGlowingPlantPositions() const {
    std::vector<std::pair<glm::vec3, glm::vec3>> positions;

    for (const auto& plant : allInstances) {
        if (plant.glowIntensity > 0.1f) {
            glm::vec3 effectivePos = plant.position + plant.movementOffset;
            glm::vec3 glowColorIntensity = calculateGlowColorAnimated(plant, simulationTime) * plant.glowIntensity;
            positions.push_back({effectivePos, glowColorIntensity});
        }
    }

    return positions;
}

float AlienVegetationSystem::getGlowIntensity(const glm::vec3& position, float radius) const {
    float totalGlow = 0.0f;

    for (const auto& plant : allInstances) {
        float dist = glm::length(plant.position - position);
        if (dist < radius && plant.glowIntensity > 0.0f) {
            float effect = 1.0f - (dist / radius);
            totalGlow += calculateGlowValue(plant, simulationTime) * effect;
        }
    }

    return totalGlow;
}

glm::vec3 AlienVegetationSystem::getAmbientAlienColor(const glm::vec3& position, float radius) const {
    glm::vec3 totalColor(0.0f);
    float totalWeight = 0.0f;

    for (const auto& plant : allInstances) {
        float dist = glm::length(plant.position - position);
        if (dist < radius && plant.glowIntensity > 0.0f) {
            float weight = plant.glowIntensity * (1.0f - dist / radius);
            totalColor += calculateGlowColorAnimated(plant, simulationTime) * weight;
            totalWeight += weight;
        }
    }

    return totalWeight > 0.0f ? totalColor / totalWeight : glm::vec3(0.0f);
}

std::vector<std::pair<glm::vec3, float>> AlienVegetationSystem::getSoundEmitters() const {
    std::vector<std::pair<glm::vec3, float>> emitters;

    for (const auto& plant : allInstances) {
        if (plant.emitsSound) {
            emitters.push_back({plant.position, plant.soundFrequency});
        }
    }

    return emitters;
}

AlienVegetationSystem::AlienVegetationStats AlienVegetationSystem::getStats() const {
    AlienVegetationStats stats;
    stats.totalPlants = static_cast<int>(allInstances.size());
    stats.glowingPlants = 0;
    stats.dangerousPlants = 0;
    stats.predatoryPlants = 0;
    stats.totalGlowOutput = 0.0f;
    stats.averageDangerLevel = 0.0f;
    stats.activeColonies = static_cast<int>(colonies.size());

    for (const auto& plant : allInstances) {
        if (plant.glowIntensity > 0.1f) {
            stats.glowingPlants++;
            stats.totalGlowOutput += plant.glowIntensity;
        }
        if (isAlienPlantDangerous(plant.type)) {
            stats.dangerousPlants++;
        }
        if (plant.isPredatory) {
            stats.predatoryPlants++;
        }
    }

    if (!colonies.empty()) {
        float totalDanger = 0.0f;
        for (const auto& colony : colonies) {
            totalDanger += colony.areaDanger;
        }
        stats.averageDangerLevel = totalDanger / colonies.size();
    }

    return stats;
}

// ===== Helper Functions =====

bool AlienVegetationSystem::isValidAlienPlantLocation(float x, float z) const {
    if (terrain && terrain->isInBounds(x, z)) {
        if (terrain->isWater(x, z)) return false;
    }
    return true;
}

float AlienVegetationSystem::getRadiationLevel(float x, float z) const {
    // Simulated radiation - higher near certain areas
    return 0.1f + std::sin(x * 0.05f) * std::cos(z * 0.05f) * 0.2f;
}

glm::vec3 AlienVegetationSystem::generateGlowColor(AlienPlantType type, unsigned int seed) const {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

    AlienPlantConfig config = getAlienPlantConfig(type);
    glm::vec3 baseColor = config.baseGlowColor;

    // Add variation
    float variation = config.colorVariation;
    baseColor.r += (dist01(rng) - 0.5f) * variation;
    baseColor.g += (dist01(rng) - 0.5f) * variation;
    baseColor.b += (dist01(rng) - 0.5f) * variation;

    return glm::clamp(baseColor, glm::vec3(0.0f), glm::vec3(1.0f));
}

float AlienVegetationSystem::calculateGlowValue(const AlienPlantInstance& plant, float time) const {
    float base = plant.glowIntensity;
    float phase = plant.glowPhase + time * plant.glowCycleSpeed;

    switch (plant.glowPattern) {
        case 0:  // Constant
            return base;
        case 1:  // Sine pulse
            return base * (0.5f + 0.5f * std::sin(phase));
        case 2:  // Heartbeat
            return base * (0.5f + 0.5f * std::sin(phase) * std::sin(phase * 0.5f));
        case 3:  // Color shift (intensity varies)
            return base * (0.7f + 0.3f * std::sin(phase * 2.0f));
        case 4:  // Strobe
            return base * (std::sin(phase * 5.0f) > 0.0f ? 1.0f : 0.2f);
        case 5:  // Twinkling
            return base * (0.6f + 0.4f * std::sin(phase * 3.0f) * std::cos(phase * 7.0f));
        default:
            return base * (0.5f + 0.5f * std::sin(phase));
    }
}

glm::vec3 AlienVegetationSystem::calculateGlowColorAnimated(const AlienPlantInstance& plant, float time) const {
    glm::vec3 baseColor = plant.glowColor;
    float phase = plant.glowPhase + time * plant.glowCycleSpeed;

    // Pattern 3 is color-shifting
    if (plant.glowPattern == 3) {
        float hueShift = std::sin(phase) * 0.3f;
        // Simple HSV-like shift
        baseColor.r += hueShift;
        baseColor.g -= hueShift * 0.5f;
        baseColor.b += hueShift * 0.3f;
        baseColor = glm::clamp(baseColor, glm::vec3(0.0f), glm::vec3(1.0f));
    }

    return baseColor;
}

void AlienVegetationSystem::createBuffers() {
    if (!dx12Device || allInstances.empty()) return;
}

void AlienVegetationSystem::updateInstanceBuffer() {
    // Update instance data for rendering
}
