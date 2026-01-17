#pragma once

#include "CreatureType.h"
#include <glm/glm.hpp>
#include <vector>

class Creature;
class ProducerSystem;
class DecomposerSystem;
class SpatialGrid;
class EcosystemMetrics;

// Food source information for creature decision making
struct FoodSource {
    glm::vec3 position;
    FoodSourceType type;
    float energyValue;
    float distance;
};

// Ecosystem-aware behavior system that enhances creature AI
class EcosystemBehaviors {
public:
    // Get appropriate food sources for a creature type
    static std::vector<FoodSource> getAvailableFood(
        CreatureType creatureType,
        const glm::vec3& position,
        float visionRange,
        const ProducerSystem* producers,
        const DecomposerSystem* decomposers);

    // Get list of valid prey for a predator
    static std::vector<Creature*> getValidPrey(
        CreatureType predatorType,
        float predatorSize,
        const glm::vec3& position,
        float visionRange,
        const std::vector<Creature*>& creatures);

    // Get list of threats for a creature
    static std::vector<Creature*> getThreats(
        CreatureType creatureType,
        float creatureSize,
        const glm::vec3& position,
        float visionRange,
        const std::vector<Creature*>& creatures);

    // Consume food from producer system and return energy gained
    static float consumeProducerFood(
        CreatureType creatureType,
        const glm::vec3& position,
        float consumeRate,
        ProducerSystem* producers,
        EcosystemMetrics* metrics);

    // Consume carrion and return energy gained
    static float consumeCarrion(
        const glm::vec3& position,
        float consumeRate,
        DecomposerSystem* decomposers,
        EcosystemMetrics* metrics);

    // Get reproduction requirements for creature type
    static void getReproductionRequirements(
        CreatureType type,
        float& energyThreshold,
        float& energyCost,
        int& requiredKills);

    // Get metabolism rate for creature type (energy per second)
    static float getMetabolismRate(CreatureType type, float size, float efficiency);

    // Check if creature type is prey for another type
    static bool isPreyFor(CreatureType prey, CreatureType predator);

    // Get social behavior parameters
    static bool shouldFormHerd(CreatureType type);
    static bool shouldFormPack(CreatureType type);
    static float getTerritoryRadius(CreatureType type, float size);

    // Omnivore diet switching logic
    static bool shouldHuntInsteadOfGraze(
        CreatureType type,
        float energy,
        float maxEnergy,
        int nearbyPrey,
        int nearbyPlants);

    // Parasite behavior
    static Creature* findBestHost(
        const glm::vec3& position,
        float visionRange,
        const std::vector<Creature*>& creatures);

    static float drainHost(Creature* host, float drainRate, float deltaTime);

    // Cleaner symbiosis
    static Creature* findParasitizedHost(
        const glm::vec3& position,
        float visionRange,
        const std::vector<Creature*>& creatures);

    static float cleanHost(Creature* host, float cleanRate, float deltaTime);

private:
    // Helper to get preferred food type for creature
    static FoodSourceType getPreferredFoodType(CreatureType type);
};

// Parasite state tracking (attached to creatures)
struct ParasiteInfection {
    int parasiteId;         // ID of parasite creature
    float infectionTime;    // When infection started
    float energyDrained;    // Total energy drained
    float severity;         // 0-1, affects drain rate

    ParasiteInfection(int id)
        : parasiteId(id)
        , infectionTime(0.0f)
        , energyDrained(0.0f)
        , severity(0.5f)
    {}
};

// Extended creature state for ecosystem features
struct EcosystemState {
    CreatureTraits traits;

    // Parasite/host relationships
    std::vector<ParasiteInfection> parasites;
    int hostId = -1;  // If parasite, ID of host creature
    bool isBeingCleaned = false;

    // Omnivore state
    bool inHuntingMode = false;
    float timeSinceLastModeSwitch = 0.0f;

    // Territory
    glm::vec3 territoryCenter;
    bool hasTerritory = false;

    // Pack/herd membership
    int packId = -1;

    // Statistics
    float totalEnergyFromPlants = 0.0f;
    float totalEnergyFromPrey = 0.0f;
    float totalEnergyFromCarrion = 0.0f;

    EcosystemState() : territoryCenter(0.0f) {}

    bool hasParasites() const { return !parasites.empty(); }
    float getParasiteBurden() const {
        float burden = 0.0f;
        for (const auto& p : parasites) {
            burden += p.severity;
        }
        return burden;
    }
};
