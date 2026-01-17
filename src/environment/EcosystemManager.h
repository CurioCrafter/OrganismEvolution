#pragma once

#include "ProducerSystem.h"
#include "DecomposerSystem.h"
#include "SeasonManager.h"
#include "EcosystemMetrics.h"
#include "../entities/CreatureType.h"
#include "../entities/EcosystemBehaviors.h"
#include "../entities/SwimBehavior.h"
#include <memory>
#include <vector>
#include <map>
#include <array>

class Terrain;
class Creature;
class SpatialGrid;

// ============================================================================
// EcosystemSignals - Read-only scarcity and abundance indicators
// Exposed for behaviors to read without modifying ecosystem state
// ============================================================================
struct EcosystemSignals {
    // Food pressure indicators (0 = abundant, 1 = scarce)
    float plantFoodPressure = 0.0f;      // Scarcity of plant food (grass, fruit, etc.)
    float preyPressure = 0.0f;           // Scarcity of prey (herbivores for carnivores)
    float carrionDensity = 0.0f;         // Availability of carrion/detritus (for scavengers)

    // Resource availability (0 = depleted, 1 = abundant)
    float producerBiomass = 0.5f;        // Overall plant biomass availability
    float detritusLevel = 0.5f;          // Detritus/dead matter for decomposers
    float nutrientSaturation = 0.5f;     // Soil nutrient levels (affects producer growth)

    // Population pressure (0 = underpopulated, 1 = overpopulated)
    float herbivorePopulationPressure = 0.0f;
    float carnivorePopulationPressure = 0.0f;

    // Seasonal and environmental signals
    float seasonalBloomStrength = 1.0f;  // Current bloom multiplier (> 1 during blooms)
    int activeBloomType = 0;             // 0=none, 1=spring, 2=fungal, 3=plankton
    bool isWinter = false;               // Simple winter flag for dormancy behaviors
    float dayLengthFactor = 1.0f;        // Day length relative to baseline

    // Competition signals (for territorial/social behaviors)
    float localCompetition = 0.0f;       // Competition intensity in local area
    float predationRisk = 0.0f;          // Risk of predation in local area

    // Timestamp for cache validity
    float lastUpdateTime = 0.0f;
};

// ============================================================================
// AquaticSpawnZone - Defines a region where aquatic creatures can spawn
// ============================================================================
struct AquaticSpawnZone {
    glm::vec3 center;
    float radius;
    float minDepth;
    float maxDepth;
    DepthBand primaryBand;

    // Environmental factors affecting spawning
    float temperature = 20.0f;           // Water temperature
    float oxygenLevel = 1.0f;            // Dissolved oxygen (0-1)
    float foodDensity = 0.5f;            // Local food availability
    float shelterDensity = 0.3f;         // Kelp/coral shelter

    // Population tracking
    int currentPopulation = 0;
    int maxCapacity = 50;

    // Spawn weights by creature type
    float herbivoreWeight = 0.6f;
    float predatorWeight = 0.3f;
    float apexWeight = 0.1f;

    bool isValid() const { return radius > 0 && maxDepth > minDepth; }
};

// ============================================================================
// AquaticPopulationStats - Per-depth-band population tracking
// ============================================================================
struct AquaticPopulationStats {
    // Counts per depth band
    std::array<int, static_cast<size_t>(DepthBand::COUNT)> countByDepth{};

    // Counts per creature type
    int herbivoreCount = 0;
    int predatorCount = 0;
    int apexCount = 0;
    int amphibianCount = 0;
    int totalAquatic = 0;

    // Distribution metrics
    float avgDepth = 0.0f;
    float depthVariance = 0.0f;

    // Food web metrics
    float herbivorePreyRatio = 0.0f;     // Herbivores per unit of plant food
    float predatorPreyRatio = 0.0f;      // Predators per herbivore
    float apexPredatorRatio = 0.0f;      // Apex per predator

    void reset() {
        countByDepth.fill(0);
        herbivoreCount = predatorCount = apexCount = amphibianCount = totalAquatic = 0;
        avgDepth = depthVariance = 0.0f;
        herbivorePreyRatio = predatorPreyRatio = apexPredatorRatio = 0.0f;
    }
};

// Central manager for the multi-trophic ecosystem
// Coordinates producers, decomposers, seasons, and tracks ecosystem health
class EcosystemManager {
public:
    EcosystemManager(Terrain* terrain);
    ~EcosystemManager();

    void init(unsigned int seed);
    void update(float deltaTime, const std::vector<std::unique_ptr<Creature>>& creatures);

    // Access subsystems
    ProducerSystem* getProducers() { return producers.get(); }
    DecomposerSystem* getDecomposers() { return decomposers.get(); }
    SeasonManager* getSeasons() { return seasons.get(); }
    EcosystemMetrics* getMetrics() { return metrics.get(); }

    const ProducerSystem* getProducers() const { return producers.get(); }
    const DecomposerSystem* getDecomposers() const { return decomposers.get(); }
    const SeasonManager* getSeasons() const { return seasons.get(); }
    const EcosystemMetrics* getMetrics() const { return metrics.get(); }

    // Creature death notification (creates corpse)
    void onCreatureDeath(const Creature& creature);

    // Get food positions for different diets
    std::vector<glm::vec3> getFoodPositionsFor(CreatureType type) const;

    // Population tracking
    void updatePopulationCounts(const std::vector<std::unique_ptr<Creature>>& creatures);
    const PopulationCounts& getPopulations() const { return currentPopulations; }

    // Ecosystem state queries
    bool isEcosystemHealthy() const;
    float getEcosystemHealth() const;
    std::string getEcosystemStatus() const;

    // Population targets based on carrying capacity
    int getTargetPopulation(CreatureType type) const;
    bool isPopulationCritical(CreatureType type) const;

    // Spawning recommendations
    struct SpawnRecommendation {
        CreatureType type;
        int count;
        std::string reason;
    };
    std::vector<SpawnRecommendation> getSpawnRecommendations() const;

    // Get ecosystem state for creature (for use in EcosystemBehaviors)
    EcosystemState& getCreatureState(int creatureId);

    // =========================================================================
    // EcosystemSignals - Read-only access for behaviors
    // =========================================================================

    // Get current ecosystem signals (read-only for behavior queries)
    const EcosystemSignals& getSignals() const { return cachedSignals; }

    // Get localized signals for a specific position (more accurate but slower)
    EcosystemSignals getLocalSignals(const glm::vec3& position, float radius) const;

    // Query specific signals (convenience methods)
    float getFoodPressure(CreatureType forType) const;
    float getCarrionAvailability() const { return cachedSignals.carrionDensity; }
    float getPredationRisk(const glm::vec3& position) const;

    // =========================================================================
    // Aquatic Ecosystem Management
    // =========================================================================

    // Get all aquatic spawn zones
    const std::vector<AquaticSpawnZone>& getAquaticSpawnZones() const { return aquaticSpawnZones; }

    // Find best spawn zone for a creature type
    const AquaticSpawnZone* findBestSpawnZone(CreatureType type) const;

    // Get spawn position within a zone
    glm::vec3 getAquaticSpawnPosition(const AquaticSpawnZone& zone, CreatureType type) const;

    // Get population stats for aquatic creatures
    const AquaticPopulationStats& getAquaticPopulationStats() const { return aquaticStats; }

    // Get population in a specific depth band
    int getPopulationInDepthBand(DepthBand band) const;

    // Check if aquatic population is healthy
    bool isAquaticEcosystemHealthy() const;

    // Get aquatic ecosystem health (0-1)
    float getAquaticEcosystemHealth() const;

    // Get food web balance for aquatic creatures
    float getAquaticFoodWebBalance() const;

    // Get debug histogram string for depth bands
    std::string getDepthBandHistogram() const;

    // Update aquatic population stats
    void updateAquaticPopulation(const std::vector<std::unique_ptr<Creature>>& creatures);

private:
    Terrain* terrain;

    std::unique_ptr<ProducerSystem> producers;
    std::unique_ptr<DecomposerSystem> decomposers;
    std::unique_ptr<SeasonManager> seasons;
    std::unique_ptr<EcosystemMetrics> metrics;

    PopulationCounts currentPopulations;

    // Per-creature ecosystem state (parasites, territory, etc.)
    std::map<int, EcosystemState> creatureStates;

    // Cached ecosystem signals (updated periodically for efficiency)
    EcosystemSignals cachedSignals;
    float signalUpdateTimer = 0.0f;
    static constexpr float signalUpdateInterval = 0.5f;  // Update twice per second
    void updateEcosystemSignals();

    // Carrying capacities (adjusted by season)
    std::map<CreatureType, int> baseCarryingCapacity;
    void initializeCarryingCapacities();
    int getSeasonalCarryingCapacity(CreatureType type) const;

    // Auto-balancing parameters
    float timeSinceLastBalance;
    static constexpr float balanceCheckInterval = 5.0f;

    void checkPopulationBalance();
    void updateCreatureStates(const std::vector<std::unique_ptr<Creature>>& creatures);
    void cleanupDeadCreatureStates(const std::vector<std::unique_ptr<Creature>>& creatures);

    // =========================================================================
    // Aquatic Ecosystem Private Members
    // =========================================================================

    // Aquatic spawn zones (generated during init)
    std::vector<AquaticSpawnZone> aquaticSpawnZones;

    // Population statistics
    AquaticPopulationStats aquaticStats;

    // Generation and update
    void generateAquaticSpawnZones();
    void updateAquaticSpawnZones();
    float getWaterDepthAt(float x, float z) const;
};
