#include "AquaticCreatures.h"
#include <algorithm>
#include <cmath>

namespace aquatic {

// ============================================================================
// Utility Functions Implementation
// ============================================================================

const char* getSpeciesName(AquaticSpecies species) {
    switch (species) {
        case AquaticSpecies::SMALL_FISH: return "Small Fish";
        case AquaticSpecies::TROPICAL_FISH: return "Tropical Fish";
        case AquaticSpecies::MINNOW: return "Minnow";
        case AquaticSpecies::MEDIUM_FISH: return "Medium Fish";
        case AquaticSpecies::REEF_FISH: return "Reef Fish";
        case AquaticSpecies::BARRACUDA: return "Barracuda";
        case AquaticSpecies::TUNA: return "Tuna";
        case AquaticSpecies::SWORDFISH: return "Swordfish";
        case AquaticSpecies::SHARK_REEF: return "Reef Shark";
        case AquaticSpecies::SHARK_HAMMERHEAD: return "Hammerhead Shark";
        case AquaticSpecies::SHARK_GREAT_WHITE: return "Great White Shark";
        case AquaticSpecies::ORCA: return "Orca";
        case AquaticSpecies::CRAB: return "Crab";
        case AquaticSpecies::LOBSTER: return "Lobster";
        case AquaticSpecies::STARFISH: return "Starfish";
        case AquaticSpecies::SEA_URCHIN: return "Sea Urchin";
        case AquaticSpecies::FLOUNDER: return "Flounder";
        case AquaticSpecies::RAY: return "Ray";
        case AquaticSpecies::JELLYFISH_SMALL: return "Small Jellyfish";
        case AquaticSpecies::JELLYFISH_LARGE: return "Large Jellyfish";
        case AquaticSpecies::JELLYFISH_BOX: return "Box Jellyfish";
        case AquaticSpecies::PORTUGUESE_MAN_O_WAR: return "Portuguese Man O' War";
        case AquaticSpecies::SEA_ANEMONE: return "Sea Anemone";
        case AquaticSpecies::MORAY_EEL: return "Moray Eel";
        case AquaticSpecies::ELECTRIC_EEL: return "Electric Eel";
        case AquaticSpecies::SEA_SNAKE: return "Sea Snake";
        case AquaticSpecies::DOLPHIN: return "Dolphin";
        case AquaticSpecies::WHALE_HUMPBACK: return "Humpback Whale";
        case AquaticSpecies::WHALE_SPERM: return "Sperm Whale";
        case AquaticSpecies::SEAL: return "Seal";
        case AquaticSpecies::OCTOPUS: return "Octopus";
        case AquaticSpecies::SQUID: return "Squid";
        case AquaticSpecies::GIANT_SQUID: return "Giant Squid";
        case AquaticSpecies::CUTTLEFISH: return "Cuttlefish";
        case AquaticSpecies::PLANKTON: return "Plankton";
        case AquaticSpecies::KRILL: return "Krill";
        case AquaticSpecies::WHALE_SHARK: return "Whale Shark";
        case AquaticSpecies::MANTA_RAY: return "Manta Ray";
        case AquaticSpecies::ANGLERFISH: return "Anglerfish";
        case AquaticSpecies::PUFFERFISH: return "Pufferfish";
        case AquaticSpecies::SEAHORSE: return "Seahorse";
        case AquaticSpecies::NAUTILUS: return "Nautilus";
        default: return "Unknown";
    }
}

WaterZone getDepthZone(float depth) {
    depth = std::abs(depth); // Ensure positive depth value

    if (depth < 5.0f) return WaterZone::SURFACE;
    if (depth < 50.0f) return WaterZone::EPIPELAGIC;
    if (depth < 200.0f) return WaterZone::MESOPELAGIC;
    if (depth < 1000.0f) return WaterZone::BATHYPELAGIC;
    return WaterZone::ABYSSOPELAGIC;
}

float getZoneMinDepth(WaterZone zone) {
    switch (zone) {
        case WaterZone::SURFACE: return 0.0f;
        case WaterZone::EPIPELAGIC: return 5.0f;
        case WaterZone::MESOPELAGIC: return 50.0f;
        case WaterZone::BATHYPELAGIC: return 200.0f;
        case WaterZone::ABYSSOPELAGIC: return 1000.0f;
        case WaterZone::BENTHIC: return 0.0f;  // Any depth
        case WaterZone::REEF: return 0.0f;
        case WaterZone::COASTAL: return 0.0f;
        case WaterZone::OPEN_OCEAN: return 0.0f;
        default: return 0.0f;
    }
}

float getZoneMaxDepth(WaterZone zone) {
    switch (zone) {
        case WaterZone::SURFACE: return 5.0f;
        case WaterZone::EPIPELAGIC: return 50.0f;
        case WaterZone::MESOPELAGIC: return 200.0f;
        case WaterZone::BATHYPELAGIC: return 1000.0f;
        case WaterZone::ABYSSOPELAGIC: return 11000.0f;  // Mariana Trench depth
        case WaterZone::BENTHIC: return 11000.0f;
        case WaterZone::REEF: return 50.0f;
        case WaterZone::COASTAL: return 30.0f;
        case WaterZone::OPEN_OCEAN: return 1000.0f;
        default: return 100.0f;
    }
}

bool isSpeciesCompatibleWithZone(AquaticSpecies species, WaterZone zone) {
    auto config = AquaticSpeciesFactory::createSpeciesConfig(species);
    for (auto validZone : config.validZones) {
        if (validZone == zone) return true;
    }
    return config.primaryZone == zone;
}

bool canSpeciesEat(AquaticSpecies predator, AquaticSpecies prey) {
    auto predConfig = AquaticSpeciesFactory::createSpeciesConfig(predator);
    for (auto preySpecies : predConfig.preySpecies) {
        if (preySpecies == prey) return true;
    }
    return false;
}

float getSpeciesBaseSize(AquaticSpecies species) {
    switch (species) {
        case AquaticSpecies::PLANKTON: return 0.01f;
        case AquaticSpecies::KRILL: return 0.05f;
        case AquaticSpecies::MINNOW: return 0.1f;
        case AquaticSpecies::SMALL_FISH: return 0.2f;
        case AquaticSpecies::TROPICAL_FISH: return 0.15f;
        case AquaticSpecies::SEAHORSE: return 0.15f;
        case AquaticSpecies::JELLYFISH_SMALL: return 0.2f;
        case AquaticSpecies::CRAB: return 0.3f;
        case AquaticSpecies::LOBSTER: return 0.5f;
        case AquaticSpecies::STARFISH: return 0.3f;
        case AquaticSpecies::SEA_URCHIN: return 0.2f;
        case AquaticSpecies::PUFFERFISH: return 0.3f;
        case AquaticSpecies::REEF_FISH: return 0.3f;
        case AquaticSpecies::MEDIUM_FISH: return 0.5f;
        case AquaticSpecies::SQUID: return 0.5f;
        case AquaticSpecies::CUTTLEFISH: return 0.4f;
        case AquaticSpecies::OCTOPUS: return 0.6f;
        case AquaticSpecies::FLOUNDER: return 0.5f;
        case AquaticSpecies::MORAY_EEL: return 1.5f;
        case AquaticSpecies::RAY: return 1.0f;
        case AquaticSpecies::JELLYFISH_LARGE: return 1.0f;
        case AquaticSpecies::JELLYFISH_BOX: return 0.3f;
        case AquaticSpecies::BARRACUDA: return 1.5f;
        case AquaticSpecies::SEAL: return 1.8f;
        case AquaticSpecies::TUNA: return 2.0f;
        case AquaticSpecies::SWORDFISH: return 3.0f;
        case AquaticSpecies::SHARK_REEF: return 2.0f;
        case AquaticSpecies::ELECTRIC_EEL: return 2.0f;
        case AquaticSpecies::DOLPHIN: return 2.5f;
        case AquaticSpecies::SHARK_HAMMERHEAD: return 4.0f;
        case AquaticSpecies::MANTA_RAY: return 5.0f;
        case AquaticSpecies::SHARK_GREAT_WHITE: return 5.0f;
        case AquaticSpecies::GIANT_SQUID: return 10.0f;
        case AquaticSpecies::ORCA: return 8.0f;
        case AquaticSpecies::WHALE_SHARK: return 12.0f;
        case AquaticSpecies::WHALE_SPERM: return 15.0f;
        case AquaticSpecies::WHALE_HUMPBACK: return 15.0f;
        case AquaticSpecies::ANGLERFISH: return 0.3f;
        case AquaticSpecies::NAUTILUS: return 0.3f;
        case AquaticSpecies::SEA_SNAKE: return 1.0f;
        case AquaticSpecies::PORTUGUESE_MAN_O_WAR: return 0.3f;
        case AquaticSpecies::SEA_ANEMONE: return 0.2f;
        default: return 0.5f;
    }
}

glm::vec3 getSpeciesBaseColor(AquaticSpecies species) {
    switch (species) {
        case AquaticSpecies::SMALL_FISH: return glm::vec3(0.6f, 0.65f, 0.7f);      // Silver
        case AquaticSpecies::TROPICAL_FISH: return glm::vec3(1.0f, 0.5f, 0.0f);    // Orange
        case AquaticSpecies::MINNOW: return glm::vec3(0.7f, 0.75f, 0.8f);          // Silver-white
        case AquaticSpecies::BARRACUDA: return glm::vec3(0.5f, 0.55f, 0.6f);       // Dark silver
        case AquaticSpecies::TUNA: return glm::vec3(0.2f, 0.3f, 0.5f);             // Blue-gray
        case AquaticSpecies::SWORDFISH: return glm::vec3(0.3f, 0.35f, 0.5f);       // Blue-silver
        case AquaticSpecies::SHARK_REEF: return glm::vec3(0.4f, 0.45f, 0.5f);      // Gray
        case AquaticSpecies::SHARK_HAMMERHEAD: return glm::vec3(0.45f, 0.5f, 0.55f);
        case AquaticSpecies::SHARK_GREAT_WHITE: return glm::vec3(0.5f, 0.52f, 0.55f);
        case AquaticSpecies::ORCA: return glm::vec3(0.1f, 0.1f, 0.15f);            // Black
        case AquaticSpecies::CRAB: return glm::vec3(0.8f, 0.3f, 0.2f);             // Red-orange
        case AquaticSpecies::LOBSTER: return glm::vec3(0.6f, 0.2f, 0.15f);         // Deep red
        case AquaticSpecies::STARFISH: return glm::vec3(0.9f, 0.4f, 0.2f);         // Orange
        case AquaticSpecies::RAY: return glm::vec3(0.5f, 0.45f, 0.4f);             // Brown-gray
        case AquaticSpecies::JELLYFISH_SMALL: return glm::vec3(0.8f, 0.85f, 0.95f); // Translucent
        case AquaticSpecies::JELLYFISH_LARGE: return glm::vec3(0.7f, 0.3f, 0.4f);  // Reddish
        case AquaticSpecies::JELLYFISH_BOX: return glm::vec3(0.85f, 0.9f, 0.95f);  // Clear
        case AquaticSpecies::MORAY_EEL: return glm::vec3(0.3f, 0.35f, 0.25f);      // Green-brown
        case AquaticSpecies::ELECTRIC_EEL: return glm::vec3(0.25f, 0.3f, 0.35f);   // Dark gray
        case AquaticSpecies::DOLPHIN: return glm::vec3(0.5f, 0.55f, 0.6f);         // Gray
        case AquaticSpecies::WHALE_HUMPBACK: return glm::vec3(0.25f, 0.28f, 0.32f);
        case AquaticSpecies::OCTOPUS: return glm::vec3(0.6f, 0.4f, 0.35f);         // Reddish-brown
        case AquaticSpecies::SQUID: return glm::vec3(0.75f, 0.7f, 0.8f);           // Pale pink
        case AquaticSpecies::PLANKTON: return glm::vec3(0.3f, 0.8f, 0.4f);         // Green
        case AquaticSpecies::KRILL: return glm::vec3(0.9f, 0.5f, 0.4f);            // Pink-red
        case AquaticSpecies::WHALE_SHARK: return glm::vec3(0.3f, 0.35f, 0.45f);    // Blue-gray
        case AquaticSpecies::MANTA_RAY: return glm::vec3(0.15f, 0.18f, 0.22f);     // Dark
        case AquaticSpecies::ANGLERFISH: return glm::vec3(0.2f, 0.15f, 0.12f);     // Dark brown
        case AquaticSpecies::PUFFERFISH: return glm::vec3(0.9f, 0.85f, 0.5f);      // Yellow
        case AquaticSpecies::SEAHORSE: return glm::vec3(0.8f, 0.6f, 0.3f);         // Golden
        default: return glm::vec3(0.5f, 0.55f, 0.6f);
    }
}

// ============================================================================
// Species Factory Implementation
// ============================================================================

AquaticSpeciesConfig AquaticSpeciesFactory::createSpeciesConfig(AquaticSpecies species) {
    switch (species) {
        case AquaticSpecies::SMALL_FISH: return createSmallFish();
        case AquaticSpecies::TROPICAL_FISH: return createTropicalFish();
        case AquaticSpecies::MINNOW: return createMinnow();
        case AquaticSpecies::BARRACUDA: return createBarracuda();
        case AquaticSpecies::TUNA: return createTuna();
        case AquaticSpecies::SWORDFISH: return createSwordfish();
        case AquaticSpecies::SHARK_REEF: return createReefShark();
        case AquaticSpecies::SHARK_HAMMERHEAD: return createHammerhead();
        case AquaticSpecies::SHARK_GREAT_WHITE: return createGreatWhite();
        case AquaticSpecies::ORCA: return createOrca();
        case AquaticSpecies::CRAB: return createCrab();
        case AquaticSpecies::LOBSTER: return createLobster();
        case AquaticSpecies::STARFISH: return createStarfish();
        case AquaticSpecies::RAY: return createRay();
        case AquaticSpecies::JELLYFISH_SMALL: return createSmallJellyfish();
        case AquaticSpecies::JELLYFISH_LARGE: return createLargeJellyfish();
        case AquaticSpecies::JELLYFISH_BOX: return createBoxJellyfish();
        case AquaticSpecies::MORAY_EEL: return createMorayEel();
        case AquaticSpecies::ELECTRIC_EEL: return createElectricEel();
        case AquaticSpecies::DOLPHIN: return createDolphin();
        case AquaticSpecies::WHALE_HUMPBACK: return createHumpbackWhale();
        case AquaticSpecies::WHALE_SPERM: return createSpermWhale();
        case AquaticSpecies::OCTOPUS: return createOctopus();
        case AquaticSpecies::SQUID: return createSquid();
        case AquaticSpecies::GIANT_SQUID: return createGiantSquid();
        case AquaticSpecies::PLANKTON: return createPlankton();
        case AquaticSpecies::KRILL: return createKrill();
        case AquaticSpecies::WHALE_SHARK: return createWhaleShark();
        case AquaticSpecies::MANTA_RAY: return createMantaRay();
        case AquaticSpecies::ANGLERFISH: return createAnglerfish();
        case AquaticSpecies::PUFFERFISH: return createPufferfish();
        case AquaticSpecies::SEAHORSE: return createSeahorse();
        default: return createSmallFish();
    }
}

// ============================================================================
// Small Fish Species
// ============================================================================

AquaticSpeciesConfig AquaticSpeciesFactory::createSmallFish() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::SMALL_FISH;
    config.name = "Small Fish";
    config.trophicLevel = 2;
    config.primaryZone = WaterZone::EPIPELAGIC;
    config.validZones = {WaterZone::SURFACE, WaterZone::EPIPELAGIC, WaterZone::COASTAL};

    // Physiology
    config.physiology.bodyLength = 0.2f;
    config.physiology.bodyWidth = 0.25f;
    config.physiology.streamlining = 0.8f;
    config.physiology.locomotion = SwimLocomotion::CARANGIFORM;
    config.physiology.maxSpeed = 8.0f;
    config.physiology.burstSpeedMultiplier = 3.0f;
    config.physiology.turnRate = 4.0f;
    config.physiology.swimbladderEfficiency = 0.9f;
    config.physiology.maxDepth = 50.0f;
    config.physiology.preferredDepth = 10.0f;
    config.physiology.mass = 0.1f;

    // Senses
    config.senses.visionRange = 20.0f;
    config.senses.lateralLineRange = 15.0f;
    config.senses.lateralLineSensitivity = 0.9f;

    // Behavior - strong schooling
    config.behavior.schoolingTendency = 0.95f;
    config.behavior.preferredSchoolSize = 50.0f;
    config.behavior.schoolingDistance = 1.5f;
    config.behavior.flightResponse = 0.9f;
    config.behavior.panicThreshold = 0.2f;
    config.behavior.scatteringTendency = 0.7f;

    // Diet - filter feeder / plankton eater
    config.isFilterFeeder = true;
    config.preySpecies = {AquaticSpecies::PLANKTON, AquaticSpecies::KRILL};
    config.predatorSpecies = {AquaticSpecies::BARRACUDA, AquaticSpecies::TUNA,
                              AquaticSpecies::SHARK_REEF, AquaticSpecies::DOLPHIN};

    // Reproduction
    config.reproductionThreshold = 120.0f;
    config.reproductionCost = 40.0f;
    config.offspringCount = 100;
    config.offspringSurvivalRate = 0.05f;

    // Energy
    config.baseEnergyDrain = 0.3f;
    config.maxEnergy = 100.0f;
    config.foodValue = 20.0f;

    // Visuals
    config.baseColor = glm::vec3(0.6f, 0.65f, 0.7f);
    config.colorVariation = 0.1f;

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createTropicalFish() {
    AquaticSpeciesConfig config = createSmallFish();
    config.species = AquaticSpecies::TROPICAL_FISH;
    config.name = "Tropical Fish";
    config.primaryZone = WaterZone::REEF;
    config.validZones = {WaterZone::REEF, WaterZone::COASTAL};

    config.physiology.locomotion = SwimLocomotion::LABRIFORM;
    config.physiology.maxSpeed = 5.0f;
    config.physiology.turnRate = 6.0f;

    config.behavior.schoolingTendency = 0.6f;
    config.behavior.preferredSchoolSize = 10.0f;
    config.behavior.territoriality = 0.4f;

    config.baseColor = glm::vec3(1.0f, 0.5f, 0.0f);
    config.colorVariation = 0.4f;
    config.hasStripes = true;
    config.hasPatterns = true;

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createMinnow() {
    AquaticSpeciesConfig config = createSmallFish();
    config.species = AquaticSpecies::MINNOW;
    config.name = "Minnow";

    config.physiology.bodyLength = 0.08f;
    config.physiology.mass = 0.01f;
    config.physiology.maxSpeed = 6.0f;

    config.reproductionThreshold = 80.0f;
    config.reproductionCost = 20.0f;
    config.offspringCount = 200;
    config.offspringSurvivalRate = 0.02f;

    config.baseEnergyDrain = 0.2f;
    config.maxEnergy = 60.0f;
    config.foodValue = 10.0f;

    return config;
}

// ============================================================================
// Predatory Fish
// ============================================================================

AquaticSpeciesConfig AquaticSpeciesFactory::createBarracuda() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::BARRACUDA;
    config.name = "Barracuda";
    config.trophicLevel = 3;
    config.primaryZone = WaterZone::REEF;
    config.validZones = {WaterZone::REEF, WaterZone::COASTAL, WaterZone::EPIPELAGIC};

    config.physiology.bodyLength = 1.5f;
    config.physiology.bodyWidth = 0.15f;
    config.physiology.streamlining = 0.95f;
    config.physiology.locomotion = SwimLocomotion::CARANGIFORM;
    config.physiology.maxSpeed = 15.0f;
    config.physiology.burstSpeedMultiplier = 3.0f;
    config.physiology.acceleration = 12.0f;
    config.physiology.mass = 20.0f;

    config.senses.visionRange = 40.0f;
    config.senses.lowLightVision = 0.6f;

    config.behavior.schoolingTendency = 0.3f;
    config.behavior.aggression = 0.8f;
    config.behavior.huntingStyle = 0.2f; // Ambush
    config.behavior.stalkingPatience = 0.8f;

    config.preySpecies = {AquaticSpecies::SMALL_FISH, AquaticSpecies::TROPICAL_FISH,
                          AquaticSpecies::MINNOW, AquaticSpecies::SQUID};
    config.predatorSpecies = {AquaticSpecies::SHARK_GREAT_WHITE};

    config.reproductionThreshold = 180.0f;
    config.reproductionCost = 80.0f;
    config.offspringCount = 5;
    config.offspringSurvivalRate = 0.3f;

    config.baseEnergyDrain = 0.6f;
    config.maxEnergy = 250.0f;
    config.foodValue = 80.0f;

    config.baseColor = glm::vec3(0.5f, 0.55f, 0.6f);

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createTuna() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::TUNA;
    config.name = "Tuna";
    config.trophicLevel = 3;
    config.primaryZone = WaterZone::OPEN_OCEAN;
    config.validZones = {WaterZone::OPEN_OCEAN, WaterZone::EPIPELAGIC, WaterZone::MESOPELAGIC};

    config.physiology.bodyLength = 2.0f;
    config.physiology.bodyWidth = 0.35f;
    config.physiology.streamlining = 0.98f;
    config.physiology.locomotion = SwimLocomotion::THUNNIFORM;
    config.physiology.maxSpeed = 20.0f; // One of the fastest fish
    config.physiology.burstSpeedMultiplier = 2.0f;
    config.physiology.swimbladderEfficiency = 0.0f;
    config.physiology.ramVentilation = true;
    config.physiology.isEndothermic = true; // Warm-blooded
    config.physiology.metabolicRate = 1.5f;
    config.physiology.mass = 200.0f;
    config.physiology.maxDepth = 500.0f;

    config.senses.visionRange = 60.0f;

    config.behavior.schoolingTendency = 0.5f;
    config.behavior.aggression = 0.7f;
    config.behavior.huntingStyle = 1.0f; // Pursuit
    config.behavior.migrationTendency = 0.9f;

    config.preySpecies = {AquaticSpecies::SMALL_FISH, AquaticSpecies::SQUID,
                          AquaticSpecies::MINNOW, AquaticSpecies::KRILL};
    config.predatorSpecies = {AquaticSpecies::SHARK_GREAT_WHITE, AquaticSpecies::ORCA};

    config.reproductionThreshold = 200.0f;
    config.reproductionCost = 100.0f;
    config.offspringCount = 10;
    config.offspringSurvivalRate = 0.1f;

    config.baseEnergyDrain = 1.0f;
    config.maxEnergy = 400.0f;
    config.foodValue = 150.0f;

    config.baseColor = glm::vec3(0.2f, 0.3f, 0.5f);

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createSwordfish() {
    AquaticSpeciesConfig config = createTuna();
    config.species = AquaticSpecies::SWORDFISH;
    config.name = "Swordfish";

    config.physiology.bodyLength = 3.0f;
    config.physiology.maxSpeed = 25.0f; // Fastest fish
    config.physiology.mass = 450.0f;

    config.behavior.schoolingTendency = 0.0f; // Solitary
    config.behavior.territoriality = 0.3f;

    config.foodValue = 200.0f;
    config.baseColor = glm::vec3(0.3f, 0.35f, 0.5f);

    return config;
}

// ============================================================================
// Sharks (Apex Predators)
// ============================================================================

AquaticSpeciesConfig AquaticSpeciesFactory::createReefShark() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::SHARK_REEF;
    config.name = "Reef Shark";
    config.trophicLevel = 4;
    config.primaryZone = WaterZone::REEF;
    config.validZones = {WaterZone::REEF, WaterZone::COASTAL, WaterZone::EPIPELAGIC};

    config.physiology.bodyLength = 2.0f;
    config.physiology.bodyWidth = 0.25f;
    config.physiology.streamlining = 0.9f;
    config.physiology.locomotion = SwimLocomotion::THUNNIFORM;
    config.physiology.maxSpeed = 12.0f;
    config.physiology.swimbladderEfficiency = 0.0f;
    config.physiology.oilLiverBuoyancy = 0.8f;
    config.physiology.ramVentilation = true;
    config.physiology.scaleArmor = 0.7f; // Dermal denticles
    config.physiology.mass = 80.0f;

    config.senses.visionRange = 50.0f;
    config.senses.lowLightVision = 0.8f;
    config.senses.electroreceptionRange = 20.0f;
    config.senses.electroreceptionSensitivity = 0.9f;
    config.senses.smellRange = 200.0f;
    config.senses.smellSensitivity = 0.95f; // Can smell blood

    config.behavior.aggression = 0.7f;
    config.behavior.huntingStyle = 0.5f;
    config.behavior.territoriality = 0.6f;

    config.preySpecies = {AquaticSpecies::SMALL_FISH, AquaticSpecies::TROPICAL_FISH,
                          AquaticSpecies::SQUID, AquaticSpecies::OCTOPUS,
                          AquaticSpecies::CRAB, AquaticSpecies::RAY};
    config.predatorSpecies = {AquaticSpecies::SHARK_GREAT_WHITE, AquaticSpecies::ORCA};

    config.reproductionThreshold = 250.0f;
    config.reproductionCost = 120.0f;
    config.offspringCount = 3;
    config.offspringSurvivalRate = 0.5f;

    config.baseEnergyDrain = 0.7f;
    config.maxEnergy = 350.0f;
    config.foodValue = 100.0f;

    config.baseColor = glm::vec3(0.4f, 0.45f, 0.5f);

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createHammerhead() {
    AquaticSpeciesConfig config = createReefShark();
    config.species = AquaticSpecies::SHARK_HAMMERHEAD;
    config.name = "Hammerhead Shark";

    config.physiology.bodyLength = 4.0f;
    config.physiology.mass = 300.0f;

    // Enhanced senses due to wide head
    config.senses.visionRange = 70.0f;  // Wide field of view
    config.senses.electroreceptionRange = 40.0f;
    config.senses.electroreceptionSensitivity = 0.98f;

    config.behavior.schoolingTendency = 0.4f; // Sometimes school

    config.foodValue = 150.0f;
    config.maxEnergy = 450.0f;

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createGreatWhite() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::SHARK_GREAT_WHITE;
    config.name = "Great White Shark";
    config.trophicLevel = 4;
    config.primaryZone = WaterZone::OPEN_OCEAN;
    config.validZones = {WaterZone::OPEN_OCEAN, WaterZone::COASTAL, WaterZone::EPIPELAGIC,
                         WaterZone::MESOPELAGIC};

    config.physiology.bodyLength = 5.0f;
    config.physiology.bodyWidth = 0.3f;
    config.physiology.streamlining = 0.92f;
    config.physiology.locomotion = SwimLocomotion::THUNNIFORM;
    config.physiology.maxSpeed = 16.0f;
    config.physiology.burstSpeedMultiplier = 2.5f;
    config.physiology.swimbladderEfficiency = 0.0f;
    config.physiology.oilLiverBuoyancy = 0.85f;
    config.physiology.ramVentilation = true;
    config.physiology.isEndothermic = true;
    config.physiology.scaleArmor = 0.8f;
    config.physiology.mass = 2000.0f;
    config.physiology.maxDepth = 1200.0f;

    config.senses.visionRange = 80.0f;
    config.senses.lowLightVision = 0.85f;
    config.senses.electroreceptionRange = 30.0f;
    config.senses.electroreceptionSensitivity = 0.95f;
    config.senses.smellRange = 500.0f;
    config.senses.smellSensitivity = 0.98f;
    config.senses.lateralLineRange = 50.0f;

    config.behavior.aggression = 0.9f;
    config.behavior.huntingStyle = 0.3f; // Ambush from below
    config.behavior.territoriality = 0.5f;
    config.behavior.migrationTendency = 0.7f;

    config.preySpecies = {AquaticSpecies::SEAL, AquaticSpecies::TUNA,
                          AquaticSpecies::RAY, AquaticSpecies::DOLPHIN,
                          AquaticSpecies::SHARK_REEF, AquaticSpecies::SQUID};
    // No natural predators (except orca)
    config.predatorSpecies = {AquaticSpecies::ORCA};

    config.reproductionThreshold = 400.0f;
    config.reproductionCost = 200.0f;
    config.offspringCount = 2;
    config.offspringSurvivalRate = 0.7f;

    config.baseEnergyDrain = 1.2f;
    config.maxEnergy = 600.0f;
    config.foodValue = 300.0f;

    config.baseColor = glm::vec3(0.5f, 0.52f, 0.55f);

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createOrca() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::ORCA;
    config.name = "Orca";
    config.trophicLevel = 4;
    config.primaryZone = WaterZone::OPEN_OCEAN;
    config.validZones = {WaterZone::OPEN_OCEAN, WaterZone::COASTAL, WaterZone::EPIPELAGIC,
                         WaterZone::SURFACE};

    config.physiology.bodyLength = 8.0f;
    config.physiology.bodyWidth = 0.2f;
    config.physiology.locomotion = SwimLocomotion::CETACEAN;
    config.physiology.maxSpeed = 14.0f;
    config.physiology.burstSpeedMultiplier = 2.0f;
    config.physiology.canBreathAir = true;
    config.physiology.breathHoldDuration = 900.0f; // 15 minutes
    config.physiology.isEndothermic = true;
    config.physiology.mass = 5000.0f;
    config.physiology.maxDepth = 300.0f;

    config.senses.visionRange = 100.0f;
    config.senses.echolocationRange = 200.0f;
    config.senses.echolocationPrecision = 0.95f;
    config.senses.smellRange = 100.0f;

    config.behavior.schoolingTendency = 0.9f; // Pod behavior
    config.behavior.preferredSchoolSize = 8.0f;
    config.behavior.aggression = 0.85f;
    config.behavior.huntingStyle = 1.0f; // Pursuit and coordination
    config.behavior.packHuntingCoordination = 0.95f;

    // Apex predator - hunts almost everything
    config.preySpecies = {AquaticSpecies::SEAL, AquaticSpecies::DOLPHIN,
                          AquaticSpecies::SHARK_GREAT_WHITE, AquaticSpecies::TUNA,
                          AquaticSpecies::WHALE_HUMPBACK, AquaticSpecies::RAY,
                          AquaticSpecies::SQUID, AquaticSpecies::OCTOPUS};
    // No natural predators

    config.reproductionThreshold = 500.0f;
    config.reproductionCost = 300.0f;
    config.offspringCount = 1;
    config.offspringSurvivalRate = 0.8f;

    config.baseEnergyDrain = 2.0f;
    config.maxEnergy = 800.0f;
    config.foodValue = 500.0f;

    config.baseColor = glm::vec3(0.1f, 0.1f, 0.15f);
    config.hasPatterns = true; // White patches

    return config;
}

// ============================================================================
// Bottom Dwellers
// ============================================================================

AquaticSpeciesConfig AquaticSpeciesFactory::createCrab() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::CRAB;
    config.name = "Crab";
    config.trophicLevel = 2;
    config.primaryZone = WaterZone::BENTHIC;
    config.validZones = {WaterZone::BENTHIC, WaterZone::REEF, WaterZone::COASTAL};

    config.physiology.bodyLength = 0.3f;
    config.physiology.bodyWidth = 1.2f; // Wide body
    config.physiology.locomotion = SwimLocomotion::WALKING;
    config.physiology.maxSpeed = 2.0f;
    config.physiology.turnRate = 5.0f;
    config.physiology.shellArmor = 0.9f;
    config.physiology.mass = 1.0f;
    config.physiology.maxDepth = 200.0f;

    config.senses.visionRange = 10.0f;
    config.senses.visionAngle = 5.5f; // Nearly 360 degree
    config.senses.smellRange = 30.0f;

    config.behavior.aggression = 0.4f;
    config.behavior.territoriality = 0.6f;
    config.behavior.flightResponse = 0.7f;

    config.isDetritivore = true;
    config.preySpecies = {AquaticSpecies::PLANKTON};
    config.predatorSpecies = {AquaticSpecies::OCTOPUS, AquaticSpecies::RAY,
                              AquaticSpecies::SHARK_REEF};

    config.reproductionThreshold = 100.0f;
    config.reproductionCost = 30.0f;
    config.offspringCount = 500;
    config.offspringSurvivalRate = 0.01f;

    config.baseEnergyDrain = 0.2f;
    config.maxEnergy = 80.0f;
    config.foodValue = 30.0f;

    config.baseColor = glm::vec3(0.8f, 0.3f, 0.2f);

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createLobster() {
    AquaticSpeciesConfig config = createCrab();
    config.species = AquaticSpecies::LOBSTER;
    config.name = "Lobster";

    config.physiology.bodyLength = 0.5f;
    config.physiology.bodyWidth = 0.5f;
    config.physiology.mass = 3.0f;
    config.physiology.maxDepth = 500.0f;

    config.behavior.territoriality = 0.8f;
    config.behavior.nocturnalTendency = 0.9f;

    config.foodValue = 50.0f;
    config.maxEnergy = 120.0f;

    config.baseColor = glm::vec3(0.6f, 0.2f, 0.15f);

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createStarfish() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::STARFISH;
    config.name = "Starfish";
    config.trophicLevel = 2;
    config.primaryZone = WaterZone::BENTHIC;
    config.validZones = {WaterZone::BENTHIC, WaterZone::REEF};

    config.physiology.bodyLength = 0.3f;
    config.physiology.locomotion = SwimLocomotion::WALKING;
    config.physiology.maxSpeed = 0.1f; // Very slow
    config.physiology.mass = 0.5f;

    config.senses.visionRange = 2.0f;
    config.senses.smellRange = 20.0f;

    config.behavior.aggression = 0.3f;
    config.behavior.flightResponse = 0.1f;

    config.isDetritivore = true;
    config.predatorSpecies = {AquaticSpecies::SHARK_REEF};

    config.reproductionThreshold = 80.0f;
    config.reproductionCost = 20.0f;
    config.offspringCount = 1000;
    config.offspringSurvivalRate = 0.001f;

    config.baseEnergyDrain = 0.1f;
    config.maxEnergy = 60.0f;
    config.foodValue = 15.0f;

    config.baseColor = glm::vec3(0.9f, 0.4f, 0.2f);

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createRay() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::RAY;
    config.name = "Stingray";
    config.trophicLevel = 3;
    config.primaryZone = WaterZone::BENTHIC;
    config.validZones = {WaterZone::BENTHIC, WaterZone::COASTAL, WaterZone::REEF};

    config.physiology.bodyLength = 1.0f;
    config.physiology.bodyWidth = 1.5f;
    config.physiology.locomotion = SwimLocomotion::RAJIFORM;
    config.physiology.maxSpeed = 6.0f;
    config.physiology.turnRate = 3.0f;
    config.physiology.toxicity = 0.7f; // Venomous tail
    config.physiology.mass = 50.0f;

    config.senses.visionRange = 30.0f;
    config.senses.electroreceptionRange = 15.0f;
    config.senses.electroreceptionSensitivity = 0.8f;

    config.behavior.aggression = 0.3f;
    config.behavior.huntingStyle = 0.0f; // Ambush/foraging
    config.behavior.flightResponse = 0.6f;

    config.preySpecies = {AquaticSpecies::CRAB, AquaticSpecies::SMALL_FISH,
                          AquaticSpecies::MINNOW};
    config.predatorSpecies = {AquaticSpecies::SHARK_HAMMERHEAD, AquaticSpecies::SHARK_GREAT_WHITE};

    config.reproductionThreshold = 150.0f;
    config.reproductionCost = 70.0f;
    config.offspringCount = 4;
    config.offspringSurvivalRate = 0.4f;

    config.baseEnergyDrain = 0.4f;
    config.maxEnergy = 200.0f;
    config.foodValue = 70.0f;

    config.baseColor = glm::vec3(0.5f, 0.45f, 0.4f);

    return config;
}

// ============================================================================
// Jellyfish
// ============================================================================

AquaticSpeciesConfig AquaticSpeciesFactory::createSmallJellyfish() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::JELLYFISH_SMALL;
    config.name = "Moon Jellyfish";
    config.trophicLevel = 2;
    config.primaryZone = WaterZone::EPIPELAGIC;
    config.validZones = {WaterZone::SURFACE, WaterZone::EPIPELAGIC, WaterZone::COASTAL};

    config.physiology.bodyLength = 0.2f;
    config.physiology.bodyWidth = 1.0f; // Bell shape
    config.physiology.locomotion = SwimLocomotion::MEDUSOID;
    config.physiology.maxSpeed = 1.0f;
    config.physiology.swimFrequency = 1.0f;
    config.physiology.toxicity = 0.2f; // Mild sting
    config.physiology.mass = 0.5f;

    config.senses.visionRange = 5.0f; // Light detection only
    config.senses.lateralLineSensitivity = 0.3f;

    config.behavior.schoolingTendency = 0.6f; // Bloom behavior
    config.behavior.preferredSchoolSize = 100.0f;

    config.isFilterFeeder = true;
    config.preySpecies = {AquaticSpecies::PLANKTON};
    config.predatorSpecies = {AquaticSpecies::TUNA, AquaticSpecies::DOLPHIN};

    config.reproductionThreshold = 60.0f;
    config.reproductionCost = 15.0f;
    config.offspringCount = 1000;
    config.offspringSurvivalRate = 0.01f;

    config.baseEnergyDrain = 0.1f;
    config.maxEnergy = 50.0f;
    config.foodValue = 5.0f;

    config.baseColor = glm::vec3(0.8f, 0.85f, 0.95f);

    // Bioluminescence
    config.bioluminescence.enabled = true;
    config.bioluminescence.color = glm::vec3(0.0f, 0.6f, 1.0f);
    config.bioluminescence.intensity = 0.3f;
    config.bioluminescence.pattern = Bioluminescence::Pattern::PULSING;
    config.bioluminescence.finEdgeGlow = true;

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createLargeJellyfish() {
    AquaticSpeciesConfig config = createSmallJellyfish();
    config.species = AquaticSpecies::JELLYFISH_LARGE;
    config.name = "Lion's Mane Jellyfish";

    config.physiology.bodyLength = 1.0f;
    config.physiology.mass = 20.0f;
    config.physiology.toxicity = 0.6f;

    config.behavior.schoolingTendency = 0.2f;

    config.foodValue = 20.0f;
    config.maxEnergy = 100.0f;

    config.baseColor = glm::vec3(0.7f, 0.3f, 0.4f);

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createBoxJellyfish() {
    AquaticSpeciesConfig config = createSmallJellyfish();
    config.species = AquaticSpecies::JELLYFISH_BOX;
    config.name = "Box Jellyfish";

    config.physiology.bodyLength = 0.3f;
    config.physiology.maxSpeed = 2.0f;
    config.physiology.toxicity = 1.0f; // Extremely venomous

    config.senses.visionRange = 15.0f; // Has actual eyes

    config.behavior.schoolingTendency = 0.0f;
    config.behavior.aggression = 0.5f;

    config.baseColor = glm::vec3(0.85f, 0.9f, 0.95f);

    return config;
}

// ============================================================================
// Eels
// ============================================================================

AquaticSpeciesConfig AquaticSpeciesFactory::createMorayEel() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::MORAY_EEL;
    config.name = "Moray Eel";
    config.trophicLevel = 3;
    config.primaryZone = WaterZone::REEF;
    config.validZones = {WaterZone::REEF, WaterZone::BENTHIC};

    config.physiology.bodyLength = 1.5f;
    config.physiology.bodyWidth = 0.1f;
    config.physiology.locomotion = SwimLocomotion::ANGUILLIFORM;
    config.physiology.maxSpeed = 5.0f;
    config.physiology.swimAmplitude = 0.4f;
    config.physiology.mass = 15.0f;

    config.senses.visionRange = 20.0f;
    config.senses.lowLightVision = 0.7f;
    config.senses.smellRange = 80.0f;
    config.senses.smellSensitivity = 0.9f;

    config.behavior.aggression = 0.6f;
    config.behavior.huntingStyle = 0.0f; // Ambush
    config.behavior.territoriality = 0.9f;
    config.behavior.nocturnalTendency = 0.8f;

    config.preySpecies = {AquaticSpecies::SMALL_FISH, AquaticSpecies::CRAB,
                          AquaticSpecies::OCTOPUS, AquaticSpecies::SQUID};
    config.predatorSpecies = {AquaticSpecies::SHARK_REEF, AquaticSpecies::BARRACUDA};

    config.reproductionThreshold = 160.0f;
    config.reproductionCost = 60.0f;
    config.offspringCount = 10000;
    config.offspringSurvivalRate = 0.001f;

    config.baseEnergyDrain = 0.3f;
    config.maxEnergy = 180.0f;
    config.foodValue = 60.0f;

    config.baseColor = glm::vec3(0.3f, 0.35f, 0.25f);
    config.hasPatterns = true;

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createElectricEel() {
    AquaticSpeciesConfig config = createMorayEel();
    config.species = AquaticSpecies::ELECTRIC_EEL;
    config.name = "Electric Eel";
    config.primaryZone = WaterZone::COASTAL; // Freshwater

    config.physiology.bodyLength = 2.0f;
    config.physiology.mass = 20.0f;
    config.physiology.electricDischarge = 1.0f; // 600+ volts

    config.senses.electroreceptionRange = 10.0f;
    config.senses.electroreceptionSensitivity = 0.95f;

    config.behavior.territoriality = 0.7f;

    config.baseColor = glm::vec3(0.25f, 0.3f, 0.35f);

    return config;
}

// ============================================================================
// Marine Mammals
// ============================================================================

AquaticSpeciesConfig AquaticSpeciesFactory::createDolphin() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::DOLPHIN;
    config.name = "Dolphin";
    config.trophicLevel = 3;
    config.primaryZone = WaterZone::EPIPELAGIC;
    config.validZones = {WaterZone::SURFACE, WaterZone::EPIPELAGIC, WaterZone::COASTAL,
                         WaterZone::OPEN_OCEAN};

    config.physiology.bodyLength = 2.5f;
    config.physiology.bodyWidth = 0.25f;
    config.physiology.streamlining = 0.95f;
    config.physiology.locomotion = SwimLocomotion::CETACEAN;
    config.physiology.maxSpeed = 12.0f;
    config.physiology.burstSpeedMultiplier = 2.0f;
    config.physiology.canBreathAir = true;
    config.physiology.breathHoldDuration = 480.0f; // 8 minutes
    config.physiology.isEndothermic = true;
    config.physiology.mass = 200.0f;

    config.senses.visionRange = 60.0f;
    config.senses.echolocationRange = 150.0f;
    config.senses.echolocationPrecision = 0.9f;

    config.behavior.schoolingTendency = 0.9f;
    config.behavior.preferredSchoolSize = 15.0f;
    config.behavior.aggression = 0.5f;
    config.behavior.huntingStyle = 1.0f;
    config.behavior.packHuntingCoordination = 0.8f;

    config.preySpecies = {AquaticSpecies::SMALL_FISH, AquaticSpecies::SQUID,
                          AquaticSpecies::MINNOW};
    config.predatorSpecies = {AquaticSpecies::SHARK_GREAT_WHITE, AquaticSpecies::ORCA};

    config.reproductionThreshold = 300.0f;
    config.reproductionCost = 150.0f;
    config.offspringCount = 1;
    config.offspringSurvivalRate = 0.7f;

    config.baseEnergyDrain = 1.5f;
    config.maxEnergy = 500.0f;
    config.foodValue = 200.0f;

    config.baseColor = glm::vec3(0.5f, 0.55f, 0.6f);

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createHumpbackWhale() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::WHALE_HUMPBACK;
    config.name = "Humpback Whale";
    config.trophicLevel = 2; // Filter feeder
    config.primaryZone = WaterZone::OPEN_OCEAN;
    config.validZones = {WaterZone::SURFACE, WaterZone::EPIPELAGIC, WaterZone::OPEN_OCEAN};

    config.physiology.bodyLength = 15.0f;
    config.physiology.bodyWidth = 0.2f;
    config.physiology.locomotion = SwimLocomotion::CETACEAN;
    config.physiology.maxSpeed = 8.0f;
    config.physiology.canBreathAir = true;
    config.physiology.breathHoldDuration = 1200.0f; // 20 minutes
    config.physiology.isEndothermic = true;
    config.physiology.mass = 30000.0f;
    config.physiology.maxDepth = 200.0f;

    config.senses.visionRange = 100.0f;
    config.senses.echolocationRange = 500.0f;

    config.behavior.schoolingTendency = 0.5f;
    config.behavior.preferredSchoolSize = 3.0f;
    config.behavior.migrationTendency = 1.0f;
    config.behavior.matingDisplayIntensity = 0.9f; // Whale songs

    config.isFilterFeeder = true;
    config.preySpecies = {AquaticSpecies::KRILL, AquaticSpecies::PLANKTON,
                          AquaticSpecies::SMALL_FISH};
    config.predatorSpecies = {AquaticSpecies::ORCA};

    config.reproductionThreshold = 800.0f;
    config.reproductionCost = 500.0f;
    config.offspringCount = 1;
    config.offspringSurvivalRate = 0.8f;

    config.baseEnergyDrain = 5.0f;
    config.maxEnergy = 2000.0f;
    config.foodValue = 1000.0f;

    config.baseColor = glm::vec3(0.25f, 0.28f, 0.32f);
    config.hasPatterns = true;

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createSpermWhale() {
    AquaticSpeciesConfig config = createHumpbackWhale();
    config.species = AquaticSpecies::WHALE_SPERM;
    config.name = "Sperm Whale";
    config.trophicLevel = 4; // Apex predator

    config.physiology.bodyLength = 15.0f;
    config.physiology.breathHoldDuration = 5400.0f; // 90 minutes!
    config.physiology.maxDepth = 2000.0f; // Deep diver
    config.physiology.pressureResistance = 1.5f;
    config.physiology.mass = 40000.0f;

    config.senses.echolocationRange = 1000.0f;
    config.senses.echolocationPrecision = 0.95f;
    config.senses.lowLightVision = 0.9f;

    config.behavior.aggression = 0.6f;
    config.behavior.huntingStyle = 1.0f;

    config.isFilterFeeder = false;
    config.preySpecies = {AquaticSpecies::GIANT_SQUID, AquaticSpecies::SQUID,
                          AquaticSpecies::OCTOPUS};

    config.baseColor = glm::vec3(0.3f, 0.32f, 0.35f);

    return config;
}

// ============================================================================
// Cephalopods
// ============================================================================

AquaticSpeciesConfig AquaticSpeciesFactory::createOctopus() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::OCTOPUS;
    config.name = "Octopus";
    config.trophicLevel = 3;
    config.primaryZone = WaterZone::REEF;
    config.validZones = {WaterZone::REEF, WaterZone::BENTHIC, WaterZone::COASTAL};

    config.physiology.bodyLength = 0.6f;
    config.physiology.locomotion = SwimLocomotion::JET_PROPULSION;
    config.physiology.maxSpeed = 8.0f;
    config.physiology.burstSpeedMultiplier = 3.0f;
    config.physiology.hasCamoflage = true;
    config.physiology.mass = 5.0f;

    config.senses.visionRange = 40.0f;
    config.senses.polarizedVision = 0.9f;
    config.senses.smellRange = 30.0f;

    config.behavior.aggression = 0.6f;
    config.behavior.huntingStyle = 0.3f;
    config.behavior.territoriality = 0.7f;
    config.behavior.nocturnalTendency = 0.7f;
    config.behavior.mimicryAbility = 0.8f;

    config.preySpecies = {AquaticSpecies::CRAB, AquaticSpecies::LOBSTER,
                          AquaticSpecies::SMALL_FISH, AquaticSpecies::MINNOW};
    config.predatorSpecies = {AquaticSpecies::MORAY_EEL, AquaticSpecies::SHARK_REEF,
                              AquaticSpecies::DOLPHIN};

    config.reproductionThreshold = 150.0f;
    config.reproductionCost = 100.0f; // Dies after reproduction
    config.offspringCount = 50000;
    config.offspringSurvivalRate = 0.001f;

    config.baseEnergyDrain = 0.5f;
    config.maxEnergy = 200.0f;
    config.foodValue = 40.0f;

    config.baseColor = glm::vec3(0.6f, 0.4f, 0.35f);
    config.colorVariation = 0.5f; // High color change

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createSquid() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::SQUID;
    config.name = "Squid";
    config.trophicLevel = 2;
    config.primaryZone = WaterZone::EPIPELAGIC;
    config.validZones = {WaterZone::EPIPELAGIC, WaterZone::MESOPELAGIC, WaterZone::OPEN_OCEAN};

    config.physiology.bodyLength = 0.5f;
    config.physiology.locomotion = SwimLocomotion::JET_PROPULSION;
    config.physiology.maxSpeed = 10.0f;
    config.physiology.burstSpeedMultiplier = 4.0f;
    config.physiology.mass = 2.0f;

    config.senses.visionRange = 50.0f;
    config.senses.lowLightVision = 0.8f;

    config.behavior.schoolingTendency = 0.7f;
    config.behavior.preferredSchoolSize = 20.0f;
    config.behavior.flightResponse = 0.8f;

    config.preySpecies = {AquaticSpecies::SMALL_FISH, AquaticSpecies::MINNOW,
                          AquaticSpecies::KRILL};
    config.predatorSpecies = {AquaticSpecies::TUNA, AquaticSpecies::DOLPHIN,
                              AquaticSpecies::SHARK_REEF, AquaticSpecies::WHALE_SPERM};

    config.reproductionThreshold = 120.0f;
    config.reproductionCost = 60.0f;
    config.offspringCount = 10000;
    config.offspringSurvivalRate = 0.005f;

    config.baseEnergyDrain = 0.6f;
    config.maxEnergy = 150.0f;
    config.foodValue = 30.0f;

    config.baseColor = glm::vec3(0.75f, 0.7f, 0.8f);

    // Bioluminescence
    config.bioluminescence.enabled = true;
    config.bioluminescence.color = glm::vec3(0.2f, 0.8f, 0.6f);
    config.bioluminescence.pattern = Bioluminescence::Pattern::COUNTER_ILLUMINATION;
    config.bioluminescence.bellyGlow = true;

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createGiantSquid() {
    AquaticSpeciesConfig config = createSquid();
    config.species = AquaticSpecies::GIANT_SQUID;
    config.name = "Giant Squid";
    config.trophicLevel = 4;
    config.primaryZone = WaterZone::BATHYPELAGIC;
    config.validZones = {WaterZone::MESOPELAGIC, WaterZone::BATHYPELAGIC};

    config.physiology.bodyLength = 10.0f;
    config.physiology.mass = 250.0f;
    config.physiology.maxDepth = 1500.0f;
    config.physiology.pressureResistance = 1.3f;

    config.senses.visionRange = 100.0f;
    config.senses.lowLightVision = 0.95f;

    config.behavior.schoolingTendency = 0.0f;
    config.behavior.aggression = 0.8f;

    config.preySpecies = {AquaticSpecies::SQUID, AquaticSpecies::MEDIUM_FISH};
    config.predatorSpecies = {AquaticSpecies::WHALE_SPERM};

    config.foodValue = 150.0f;
    config.maxEnergy = 400.0f;

    // Bioluminescence
    config.bioluminescence.enabled = true;
    config.bioluminescence.spotPattern = true;

    return config;
}

// ============================================================================
// Plankton & Filter Feeders
// ============================================================================

AquaticSpeciesConfig AquaticSpeciesFactory::createPlankton() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::PLANKTON;
    config.name = "Plankton";
    config.trophicLevel = 1; // Producer
    config.primaryZone = WaterZone::SURFACE;
    config.validZones = {WaterZone::SURFACE, WaterZone::EPIPELAGIC};

    config.physiology.bodyLength = 0.01f;
    config.physiology.locomotion = SwimLocomotion::DRIFTING;
    config.physiology.maxSpeed = 0.1f;
    config.physiology.mass = 0.0001f;

    config.behavior.schoolingTendency = 0.0f; // Passive

    // No prey - photosynthetic or detritivore

    config.reproductionThreshold = 10.0f;
    config.reproductionCost = 2.0f;
    config.offspringCount = 1000000;
    config.offspringSurvivalRate = 0.0001f;

    config.baseEnergyDrain = 0.01f;
    config.maxEnergy = 10.0f;
    config.foodValue = 0.1f;

    config.baseColor = glm::vec3(0.3f, 0.8f, 0.4f);

    // Some plankton are bioluminescent
    config.bioluminescence.enabled = true;
    config.bioluminescence.color = glm::vec3(0.3f, 0.9f, 0.8f);
    config.bioluminescence.pattern = Bioluminescence::Pattern::FLASHING;
    config.bioluminescence.intensity = 0.1f;

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createKrill() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::KRILL;
    config.name = "Krill";
    config.trophicLevel = 2;
    config.primaryZone = WaterZone::EPIPELAGIC;
    config.validZones = {WaterZone::SURFACE, WaterZone::EPIPELAGIC, WaterZone::OPEN_OCEAN};

    config.physiology.bodyLength = 0.05f;
    config.physiology.locomotion = SwimLocomotion::CARANGIFORM;
    config.physiology.maxSpeed = 2.0f;
    config.physiology.mass = 0.002f;

    config.behavior.schoolingTendency = 0.99f; // Massive swarms
    config.behavior.preferredSchoolSize = 10000.0f;
    config.behavior.flightResponse = 0.95f;

    config.isFilterFeeder = true;
    config.preySpecies = {AquaticSpecies::PLANKTON};
    // Everything eats krill

    config.reproductionThreshold = 20.0f;
    config.reproductionCost = 5.0f;
    config.offspringCount = 10000;
    config.offspringSurvivalRate = 0.01f;

    config.baseEnergyDrain = 0.05f;
    config.maxEnergy = 20.0f;
    config.foodValue = 0.5f;

    config.baseColor = glm::vec3(0.9f, 0.5f, 0.4f);

    // Bioluminescence
    config.bioluminescence.enabled = true;
    config.bioluminescence.color = glm::vec3(0.2f, 0.6f, 1.0f);
    config.bioluminescence.pattern = Bioluminescence::Pattern::CONSTANT_GLOW;

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createWhaleShark() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::WHALE_SHARK;
    config.name = "Whale Shark";
    config.trophicLevel = 2; // Filter feeder
    config.primaryZone = WaterZone::OPEN_OCEAN;
    config.validZones = {WaterZone::SURFACE, WaterZone::EPIPELAGIC, WaterZone::OPEN_OCEAN};

    config.physiology.bodyLength = 12.0f;
    config.physiology.bodyWidth = 0.2f;
    config.physiology.locomotion = SwimLocomotion::THUNNIFORM;
    config.physiology.maxSpeed = 5.0f;
    config.physiology.swimbladderEfficiency = 0.0f;
    config.physiology.oilLiverBuoyancy = 0.9f;
    config.physiology.ramVentilation = true;
    config.physiology.mass = 18000.0f;
    config.physiology.scaleArmor = 0.6f;

    config.senses.visionRange = 50.0f;
    config.senses.lateralLineRange = 80.0f;

    config.behavior.aggression = 0.0f; // Docile
    config.behavior.migrationTendency = 0.8f;

    config.isFilterFeeder = true;
    config.preySpecies = {AquaticSpecies::PLANKTON, AquaticSpecies::KRILL,
                          AquaticSpecies::SMALL_FISH};
    // No predators

    config.reproductionThreshold = 600.0f;
    config.reproductionCost = 400.0f;
    config.offspringCount = 300;
    config.offspringSurvivalRate = 0.1f;

    config.baseEnergyDrain = 3.0f;
    config.maxEnergy = 1500.0f;
    config.foodValue = 800.0f;

    config.baseColor = glm::vec3(0.3f, 0.35f, 0.45f);
    config.hasSpots = true;

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createMantaRay() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::MANTA_RAY;
    config.name = "Manta Ray";
    config.trophicLevel = 2;
    config.primaryZone = WaterZone::OPEN_OCEAN;
    config.validZones = {WaterZone::SURFACE, WaterZone::EPIPELAGIC, WaterZone::REEF};

    config.physiology.bodyLength = 5.0f;
    config.physiology.bodyWidth = 1.5f;
    config.physiology.locomotion = SwimLocomotion::RAJIFORM;
    config.physiology.maxSpeed = 6.0f;
    config.physiology.mass = 1350.0f;

    config.senses.visionRange = 60.0f;

    config.behavior.aggression = 0.0f;
    config.behavior.schoolingTendency = 0.3f;
    config.behavior.migrationTendency = 0.6f;
    config.behavior.cleaningBehavior = 0.5f; // Visits cleaning stations

    config.isFilterFeeder = true;
    config.preySpecies = {AquaticSpecies::PLANKTON, AquaticSpecies::KRILL};
    config.predatorSpecies = {AquaticSpecies::ORCA, AquaticSpecies::SHARK_GREAT_WHITE};

    config.reproductionThreshold = 350.0f;
    config.reproductionCost = 200.0f;
    config.offspringCount = 1;
    config.offspringSurvivalRate = 0.6f;

    config.baseEnergyDrain = 1.5f;
    config.maxEnergy = 600.0f;
    config.foodValue = 300.0f;

    config.baseColor = glm::vec3(0.15f, 0.18f, 0.22f);

    return config;
}

// ============================================================================
// Special Species
// ============================================================================

AquaticSpeciesConfig AquaticSpeciesFactory::createAnglerfish() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::ANGLERFISH;
    config.name = "Anglerfish";
    config.trophicLevel = 3;
    config.primaryZone = WaterZone::BATHYPELAGIC;
    config.validZones = {WaterZone::MESOPELAGIC, WaterZone::BATHYPELAGIC, WaterZone::ABYSSOPELAGIC};

    config.physiology.bodyLength = 0.3f;
    config.physiology.locomotion = SwimLocomotion::CARANGIFORM;
    config.physiology.maxSpeed = 2.0f;
    config.physiology.mass = 2.0f;
    config.physiology.minDepth = 200.0f;
    config.physiology.maxDepth = 4000.0f;
    config.physiology.preferredDepth = 1000.0f;
    config.physiology.pressureResistance = 1.5f;

    config.senses.visionRange = 20.0f;
    config.senses.lowLightVision = 0.95f;
    config.senses.smellRange = 100.0f;

    config.behavior.aggression = 0.7f;
    config.behavior.huntingStyle = 0.0f; // Ambush with lure
    config.behavior.stalkingPatience = 1.0f;

    config.preySpecies = {AquaticSpecies::SMALL_FISH, AquaticSpecies::SQUID};

    config.reproductionThreshold = 140.0f;
    config.reproductionCost = 50.0f;
    config.offspringCount = 100000;
    config.offspringSurvivalRate = 0.0001f;

    config.baseEnergyDrain = 0.2f;
    config.maxEnergy = 120.0f;
    config.foodValue = 20.0f;

    config.baseColor = glm::vec3(0.2f, 0.15f, 0.12f);

    // Bioluminescent lure
    config.bioluminescence.enabled = true;
    config.bioluminescence.color = glm::vec3(0.3f, 0.9f, 1.0f);
    config.bioluminescence.intensity = 0.8f;
    config.bioluminescence.pattern = Bioluminescence::Pattern::LURE;
    config.bioluminescence.lureAppendage = true;

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createPufferfish() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::PUFFERFISH;
    config.name = "Pufferfish";
    config.trophicLevel = 2;
    config.primaryZone = WaterZone::REEF;
    config.validZones = {WaterZone::REEF, WaterZone::COASTAL};

    config.physiology.bodyLength = 0.3f;
    config.physiology.locomotion = SwimLocomotion::LABRIFORM;
    config.physiology.maxSpeed = 3.0f;
    config.physiology.turnRate = 5.0f;
    config.physiology.toxicity = 0.95f; // Tetrodotoxin
    config.physiology.hasSpines = true;
    config.physiology.mass = 1.0f;

    config.senses.visionRange = 25.0f;

    config.behavior.aggression = 0.2f;
    config.behavior.flightResponse = 0.8f;

    config.isHerbivore = true;
    config.preySpecies = {AquaticSpecies::SEA_URCHIN}; // Also eats algae
    config.predatorSpecies = {AquaticSpecies::SHARK_REEF}; // Few predators due to toxin

    config.reproductionThreshold = 100.0f;
    config.reproductionCost = 40.0f;
    config.offspringCount = 200;
    config.offspringSurvivalRate = 0.05f;

    config.baseEnergyDrain = 0.3f;
    config.maxEnergy = 100.0f;
    config.foodValue = 10.0f; // Low due to toxin

    config.baseColor = glm::vec3(0.9f, 0.85f, 0.5f);
    config.hasSpots = true;

    return config;
}

AquaticSpeciesConfig AquaticSpeciesFactory::createSeahorse() {
    AquaticSpeciesConfig config;
    config.species = AquaticSpecies::SEAHORSE;
    config.name = "Seahorse";
    config.trophicLevel = 2;
    config.primaryZone = WaterZone::REEF;
    config.validZones = {WaterZone::REEF, WaterZone::COASTAL};

    config.physiology.bodyLength = 0.15f;
    config.physiology.locomotion = SwimLocomotion::OSTRACIFORM;
    config.physiology.maxSpeed = 0.5f; // Very slow
    config.physiology.turnRate = 6.0f;
    config.physiology.hasCamoflage = true;
    config.physiology.mass = 0.01f;

    config.senses.visionRange = 15.0f;

    config.behavior.territoriality = 0.6f;
    config.behavior.matingDisplayIntensity = 0.9f;
    config.behavior.parentalCare = 1.0f; // Males carry eggs

    config.preySpecies = {AquaticSpecies::PLANKTON, AquaticSpecies::KRILL};
    config.predatorSpecies = {AquaticSpecies::CRAB};

    config.reproductionThreshold = 80.0f;
    config.reproductionCost = 30.0f;
    config.offspringCount = 100;
    config.offspringSurvivalRate = 0.01f;

    config.baseEnergyDrain = 0.15f;
    config.maxEnergy = 60.0f;
    config.foodValue = 5.0f;

    config.baseColor = glm::vec3(0.8f, 0.6f, 0.3f);
    config.colorVariation = 0.4f;

    return config;
}

} // namespace aquatic
