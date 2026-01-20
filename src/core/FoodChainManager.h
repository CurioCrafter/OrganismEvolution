#pragma once

// FoodChainManager - Connects producers to consumers with energy flow tracking
// Manages the food web, predator-prey relationships, and population balance

#include "../entities/CreatureType.h"
#include "../environment/ProducerSystem.h"
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <unordered_map>
#include <functional>

// Forward declarations (global namespace)
class Terrain;
class Creature;
class EcosystemManager;

namespace Forge {

// Import global namespace classes into Forge namespace
using ::Creature;
using ::Terrain;
using ::EcosystemManager;

class CreatureManager;

// ============================================================================
// Energy Flow Statistics
// ============================================================================

struct EnergyFlowStats {
    // Energy entering the system (from producers)
    float producerEnergy = 0.0f;       // Total energy from plants
    float solarEnergy = 0.0f;          // Energy from sun (drives producers)

    // Energy at each trophic level
    float herbivoreEnergy = 0.0f;      // Energy in primary consumers
    float smallPredatorEnergy = 0.0f;  // Energy in secondary consumers
    float apexPredatorEnergy = 0.0f;   // Energy in tertiary consumers

    // Energy transfers (per frame/second)
    float plantToHerbivore = 0.0f;     // Grazing rate
    float herbivoreToSmallPred = 0.0f; // Small predation rate
    float herbivoreToApex = 0.0f;      // Apex predation on herbivores
    float smallPredToApex = 0.0f;      // Apex predation on small predators

    // Energy losses
    float respirationLoss = 0.0f;      // Energy lost to metabolism
    float deathDecay = 0.0f;           // Energy in corpses returning to system

    // Efficiency metrics
    float transferEfficiency = 0.0f;   // Average transfer efficiency (typically ~10%)
    float systemEfficiency = 0.0f;     // Overall ecosystem efficiency

    void reset() {
        producerEnergy = solarEnergy = 0.0f;
        herbivoreEnergy = smallPredatorEnergy = apexPredatorEnergy = 0.0f;
        plantToHerbivore = herbivoreToSmallPred = herbivoreToApex = smallPredToApex = 0.0f;
        respirationLoss = deathDecay = 0.0f;
        transferEfficiency = systemEfficiency = 0.0f;
    }
};

// ============================================================================
// Population Balance Parameters
// ============================================================================

struct PopulationBalance {
    // Carrying capacities by type
    std::unordered_map<CreatureType, int> carryingCapacity;

    // Current populations
    std::unordered_map<CreatureType, int> currentPopulation;

    // Birth/death rate modifiers based on food availability
    std::unordered_map<CreatureType, float> birthRateModifier;
    std::unordered_map<CreatureType, float> deathRateModifier;

    // Starvation tracking
    std::unordered_map<CreatureType, float> avgHunger;

    // Get pressure (0 = underpopulated, 1 = at capacity, >1 = overpopulated)
    float getPressure(CreatureType type) const {
        auto capIt = carryingCapacity.find(type);
        auto popIt = currentPopulation.find(type);
        if (capIt == carryingCapacity.end() || popIt == currentPopulation.end()) return 1.0f;
        if (capIt->second == 0) return 1.0f;
        return static_cast<float>(popIt->second) / static_cast<float>(capIt->second);
    }
};

// ============================================================================
// Feeding Event
// ============================================================================

struct FeedingEvent {
    CreatureType consumer;
    CreatureType consumed;  // GRAZER = plant, otherwise prey
    float energyTransferred;
    glm::vec3 location;
    float timestamp;
};

// ============================================================================
// Food Chain Manager
// ============================================================================

class FoodChainManager {
public:
    FoodChainManager();
    ~FoodChainManager() = default;

    // Initialization
    void init(CreatureManager* creatures, EcosystemManager* ecosystem, Terrain* terrain);
    void reset();

    // ========================================================================
    // Main Update
    // ========================================================================

    // Call every frame to process feeding, energy flow, and balance
    void update(float deltaTime);

    // ========================================================================
    // Feeding Operations
    // ========================================================================

    // Attempt to feed a creature (returns energy gained)
    float tryFeed(Creature& creature, float deltaTime);

    // Herbivore feeding (from producers)
    float feedOnPlant(Creature& herbivore, float deltaTime);

    // Aquatic herbivore feeding (from aquatic producers)
    float feedOnAquaticPlant(Creature& herbivore, float deltaTime);

    // Predator feeding (from prey)
    float feedOnPrey(Creature& predator, Creature& prey);

    // Scavenger feeding (from corpses)
    float feedOnCorpse(Creature& scavenger, float deltaTime);

    // ========================================================================
    // Food Finding
    // ========================================================================

    // Find nearest food for a creature (returns position or zero if none)
    glm::vec3 findNearestFood(const Creature& creature, float maxRange) const;

    // Find nearest plant for herbivore
    glm::vec3 findNearestPlant(const glm::vec3& position, FoodSourceType preferredType,
                                float maxRange) const;

    // Find nearest prey for predator
    Creature* findNearestPrey(const Creature& predator, float maxRange) const;

    // Find nearest corpse for scavenger
    glm::vec3 findNearestCorpse(const glm::vec3& position, float maxRange) const;

    // ========================================================================
    // Population Control
    // ========================================================================

    // Get spawn recommendations based on ecosystem balance
    struct SpawnRecommendation {
        CreatureType type;
        int count;
        std::string reason;
        float priority;  // 0-1, higher = more urgent
    };

    std::vector<SpawnRecommendation> getSpawnRecommendations() const;

    // Check if a type should be spawned (soft cap)
    bool shouldSpawn(CreatureType type) const;

    // Get culling recommendations (if overpopulated)
    std::vector<SpawnRecommendation> getCullingRecommendations() const;

    // ========================================================================
    // Carrying Capacity
    // ========================================================================

    // Update carrying capacity based on producer biomass
    void updateCarryingCapacity();

    // Get current carrying capacity for a type
    int getCarryingCapacity(CreatureType type) const;

    // Set base carrying capacity (before modifiers)
    void setBaseCarryingCapacity(CreatureType type, int capacity);

    // ========================================================================
    // Statistics
    // ========================================================================

    const EnergyFlowStats& getEnergyStats() const { return m_energyStats; }
    const PopulationBalance& getPopulationBalance() const { return m_balance; }

    // Get recent feeding events (for visualization)
    const std::vector<FeedingEvent>& getRecentFeedingEvents() const { return m_recentEvents; }

    // Get food availability score (0-1) for visualization
    float getFoodAvailability(const glm::vec3& position, CreatureType forType) const;

    // ========================================================================
    // Configuration
    // ========================================================================

    // Energy transfer efficiency (default 10% - realistic)
    void setTransferEfficiency(float efficiency) { m_transferEfficiency = efficiency; }
    float getTransferEfficiency() const { return m_transferEfficiency; }

    // Hunting success rate modifier
    void setHuntingSuccessModifier(float modifier) { m_huntingSuccessModifier = modifier; }

    // Grazing rate (energy per second when feeding on plants)
    void setGrazingRate(float rate) { m_grazingRate = rate; }

private:
    // References to other managers
    CreatureManager* m_creatures = nullptr;
    EcosystemManager* m_ecosystem = nullptr;
    Terrain* m_terrain = nullptr;

    // Statistics
    EnergyFlowStats m_energyStats;
    PopulationBalance m_balance;

    // Recent feeding events (for visualization, capped at 100)
    std::vector<FeedingEvent> m_recentEvents;
    static constexpr size_t MAX_EVENTS = 100;

    // Configuration
    float m_transferEfficiency = 0.10f;     // 10% energy transfer
    float m_huntingSuccessModifier = 1.0f;  // Base hunting success
    float m_grazingRate = 5.0f;             // Energy/second when grazing
    float m_scavengeRate = 5.0f;            // Biomass/second when scavenging

    // Base carrying capacities
    std::unordered_map<CreatureType, int> m_baseCapacity;

    // Simulation time
    float m_simulationTime = 0.0f;

    // Update intervals
    float m_timeSinceCapacityUpdate = 0.0f;
    static constexpr float CAPACITY_UPDATE_INTERVAL = 5.0f;

    // Helper methods
    void initializeBaseCapacities();
    void updateEnergyStats();
    void updatePopulationBalance();
    void recordFeedingEvent(CreatureType consumer, CreatureType consumed,
                            float energy, const glm::vec3& location);
    float calculateHuntingSuccess(const Creature& predator, const Creature& prey) const;
    bool isValidPrey(const Creature& predator, const Creature& prey) const;
};

} // namespace Forge
