#pragma once

// CreatureManager - Unified creature pool with spatial partitioning by domain
// Manages all creature types efficiently with domain-specific optimizations

#include "../entities/Creature.h"
#include "../entities/CreatureType.h"
#include "../utils/SpatialGrid.h"
#include "../physics/Metamorphosis.h"  // PHASE 7 - Agent 3: Amphibious transition
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <functional>
#include <array>
#include <unordered_map>

class Terrain;
class EcosystemManager;

namespace Forge {

// Import global namespace classes into Forge namespace
using ::Creature;
using ::Terrain;
using ::EcosystemManager;

// ============================================================================
// Domain Types for Spatial Partitioning
// ============================================================================

enum class CreatureDomain {
    LAND,       // Ground-dwelling creatures
    WATER,      // Aquatic creatures
    AIR,        // Flying creatures
    AMPHIBIOUS, // Can exist in multiple domains
    COUNT
};

// Get domain for a creature type
inline CreatureDomain getDomain(CreatureType type) {
    if (isFlying(type)) return CreatureDomain::AIR;
    if (isAquatic(type) && type != CreatureType::AMPHIBIAN) return CreatureDomain::WATER;
    if (type == CreatureType::AMPHIBIAN) return CreatureDomain::AMPHIBIOUS;
    return CreatureDomain::LAND;
}

// ============================================================================
// Creature Handle - Lightweight reference to a creature
// ============================================================================

struct CreatureHandle {
    uint32_t index = 0;
    uint32_t generation = 0;

    bool isValid() const { return generation != 0; }
    static CreatureHandle invalid() { return CreatureHandle{0, 0}; }
};

// ============================================================================
// Population Statistics
// ============================================================================

struct PopulationStats {
    std::array<int, static_cast<size_t>(CreatureType::AMPHIBIAN) + 1> byType{};
    std::array<int, static_cast<size_t>(CreatureDomain::COUNT)> byDomain{};
    int total = 0;
    int alive = 0;
    int births = 0;
    int deaths = 0;
    float avgEnergy = 0.0f;
    float avgAge = 0.0f;
    float avgFitness = 0.0f;

    // Evolution tracking
    float bestFitness = 0.0f;
    float minFitness = 0.0f;
    float fitnessStdDev = 0.0f;
    int currentGeneration = 0;
    float avgBrainComplexity = 0.0f;

    // PHASE 7 - Agent 3: Amphibious transition tracking
    std::array<int, 4> byAmphibiousStage{};  // Count per AmphibiousStage
    int totalTransitions = 0;                 // Total stage transitions this session
    int transitionsThisFrame = 0;             // Transitions in current update
    float avgTransitionProgress = 0.0f;       // Average progress across all transitioning creatures

    void reset() {
        byType.fill(0);
        byDomain.fill(0);
        byAmphibiousStage.fill(0);
        total = alive = births = deaths = 0;
        avgEnergy = avgAge = avgFitness = 0.0f;
        bestFitness = minFitness = fitnessStdDev = 0.0f;
        avgBrainComplexity = 0.0f;
        totalTransitions = transitionsThisFrame = 0;
        avgTransitionProgress = 0.0f;
    }
};

// ============================================================================
// Creature Manager
// ============================================================================

class CreatureManager {
public:
    // Configuration
    static constexpr size_t MAX_CREATURES = 65536;
    static constexpr size_t INITIAL_POOL_SIZE = 4096;
    static constexpr float WORLD_SIZE = 500.0f;
    static constexpr int GRID_RESOLUTION = 25;

    CreatureManager(float worldWidth = WORLD_SIZE, float worldDepth = WORLD_SIZE);
    ~CreatureManager();

    // Initialization
    void init(Terrain* terrain, EcosystemManager* ecosystem, unsigned int seed = 0);
    void clear();

    // ========================================================================
    // Creature Lifecycle
    // ========================================================================

    // Spawn a new creature, returns handle
    CreatureHandle spawn(CreatureType type, const glm::vec3& position,
                         const Genome* parentGenome = nullptr);

    // Spawn with specific genome (for loading saves)
    CreatureHandle spawnWithGenome(const glm::vec3& position, const Genome& genome);

    // Kill a creature (marks for death, handled in update)
    void kill(CreatureHandle handle, const std::string& cause = "");

    // Check if handle is valid and creature is alive
    bool isAlive(CreatureHandle handle) const;

    // Get creature by handle (nullptr if invalid)
    Creature* get(CreatureHandle handle);
    const Creature* get(CreatureHandle handle) const;

    // Get creature by unique ID (slower than handle - use sparingly)
    Creature* getCreatureByID(uint32_t id);
    const Creature* getCreatureByID(uint32_t id) const;

    // Access spatial grid (for behavior systems)
    SpatialGrid* getGlobalGrid() { return m_globalGrid.get(); }
    const SpatialGrid* getGlobalGrid() const { return m_globalGrid.get(); }

    // ========================================================================
    // Batch Operations
    // ========================================================================

    // Iterate all living creatures
    template<typename Func>
    void forEach(Func&& func) {
        for (size_t i = 0; i < m_creatures.size(); ++i) {
            if (m_creatures[i] && m_creatures[i]->isActive()) {
                func(*m_creatures[i], i);
            }
        }
    }

    // Iterate creatures by domain
    template<typename Func>
    void forEachInDomain(CreatureDomain domain, Func&& func) {
        for (auto* creature : m_domainLists[static_cast<size_t>(domain)]) {
            if (creature && creature->isActive()) {
                func(*creature);
            }
        }
    }

    // Iterate creatures by type
    template<typename Func>
    void forEachOfType(CreatureType type, Func&& func) {
        for (auto& creature : m_creatures) {
            if (creature && creature->isActive() && creature->getType() == type) {
                func(*creature);
            }
        }
    }

    // ========================================================================
    // Spatial Queries
    // ========================================================================

    // Query nearby creatures (all domains)
    const std::vector<Creature*>& queryNearby(const glm::vec3& position, float radius);

    // Query nearby creatures in specific domain
    const std::vector<Creature*>& queryNearbyInDomain(const glm::vec3& position,
                                                        float radius,
                                                        CreatureDomain domain);

    // Query by type within radius
    const std::vector<Creature*>& queryNearbyByType(const glm::vec3& position,
                                                     float radius,
                                                     CreatureType type);

    // Find nearest creature
    Creature* findNearest(const glm::vec3& position, float maxRadius,
                          CreatureType typeFilter = CreatureType::GRAZER);

    // Find nearest prey for a predator
    Creature* findNearestPrey(const Creature& predator, float maxRadius);

    // Find nearest predator threatening a creature
    Creature* findNearestThreat(const Creature& prey, float maxRadius);

    // ========================================================================
    // Selection and Camera Following
    // ========================================================================

    // Select a creature to follow
    void select(CreatureHandle handle);
    void selectNearest(const glm::vec3& position);
    void selectRandom();
    void selectNextOfType(CreatureType type);
    void clearSelection();

    CreatureHandle getSelected() const { return m_selectedCreature; }
    Creature* getSelectedCreature();
    const Creature* getSelectedCreature() const;

    // Auto-follow best creature of a type
    void followFittest(CreatureType type);
    void followOldest(CreatureType type);

    // ========================================================================
    // Update & Maintenance
    // ========================================================================

    // Main update (call once per frame)
    void update(float deltaTime);

    // Rebuild spatial grids (call after position updates)
    void rebuildSpatialGrids();

    // Remove dead creatures and compact pools
    void cleanup();

    // Population culling when limits exceeded
    void cullToLimit(size_t maxCount);
    void cullWeakest(CreatureType type, size_t targetCount);

    // ========================================================================
    // Statistics
    // ========================================================================

    const PopulationStats& getStats() const { return m_stats; }
    int getPopulation(CreatureType type) const;
    int getPopulation(CreatureDomain domain) const;
    int getTotalPopulation() const { return m_stats.alive; }

    // Get all creatures (for rendering, saving, etc.)
    const std::vector<std::unique_ptr<Creature>>& getAllCreatures() const { return m_creatures; }
    std::vector<std::unique_ptr<Creature>>& getAllCreatures() { return m_creatures; }

    // ========================================================================
    // Reproduction
    // ========================================================================

    // Handle reproduction between two creatures
    CreatureHandle reproduce(CreatureHandle parent1, CreatureHandle parent2);

    // Asexual reproduction (mutation only)
    CreatureHandle reproduceAsexual(CreatureHandle parent);

    // Check if creature can reproduce
    bool canReproduce(CreatureHandle handle) const;

    // ========================================================================
    // Amphibious Transition (PHASE 7 - Agent 3)
    // ========================================================================

    // Update amphibious transitions for all creatures
    // Call this after main update to process environment-driven transitions
    void updateAmphibiousTransitions(float deltaTime, float waterLevel);

    // Get amphibious transition controller for a creature (nullptr if not applicable)
    AmphibiousTransitionController* getTransitionController(CreatureHandle handle);
    const AmphibiousTransitionController* getTransitionController(CreatureHandle handle) const;

    // Force a creature to begin transitioning (for testing/scenarios)
    void forceTransitionStage(CreatureHandle handle, AmphibiousStage stage);

    // Get count of creatures at each amphibious stage
    int getAmphibiousStageCount(AmphibiousStage stage) const;

    // Enable/disable amphibious transition debug logging
    void setAmphibiousDebugLogging(bool enabled) { m_amphibiousDebugEnabled = enabled; }
    bool isAmphibiousDebugLoggingEnabled() const { return m_amphibiousDebugEnabled; }

    // Configuration for mass transition prevention
    void setMaxTransitionsPerFrame(int max) { m_maxTransitionsPerFrame = max; }
    void setTransitionCooldown(float seconds) { m_globalTransitionCooldown = seconds; }

private:
    // Terrain for height sampling
    Terrain* m_terrain = nullptr;
    EcosystemManager* m_ecosystem = nullptr;

    // Main creature storage (pooled)
    std::vector<std::unique_ptr<Creature>> m_creatures;
    std::vector<uint32_t> m_freeIndices;
    std::vector<uint32_t> m_generations;  // For handle validation

    // Domain-specific lists (for efficient iteration)
    std::array<std::vector<Creature*>, static_cast<size_t>(CreatureDomain::COUNT)> m_domainLists;

    // Spatial grids per domain
    std::unique_ptr<SpatialGrid> m_landGrid;
    std::unique_ptr<SpatialGrid> m_waterGrid;
    std::unique_ptr<SpatialGrid> m_airGrid;

    // Combined grid for cross-domain queries
    std::unique_ptr<SpatialGrid> m_globalGrid;

    // Query result buffer (reused to avoid allocations)
    mutable std::vector<Creature*> m_queryBuffer;

    // Statistics
    PopulationStats m_stats;

    // Selection
    CreatureHandle m_selectedCreature;

    // World parameters
    float m_worldWidth;
    float m_worldDepth;
    unsigned int m_seed;

    // Pending deaths (processed in update)
    std::vector<std::pair<size_t, std::string>> m_pendingDeaths;

    // Helper methods
    size_t allocateSlot();
    void releaseSlot(size_t index);
    void updateStats();
    void rebuildDomainLists();
    float getTerrainHeight(const glm::vec3& position) const;
    glm::vec3 clampToWorld(const glm::vec3& position) const;
    bool isValidPosition(const glm::vec3& position, CreatureType type) const;

    // PHASE 7 - Agent 3: Amphibious transition private members
    std::unordered_map<uint32_t, AmphibiousTransitionController> m_transitionControllers;
    bool m_amphibiousDebugEnabled = false;
    int m_maxTransitionsPerFrame = 5;       // Prevent mass conversion
    float m_globalTransitionCooldown = 1.0f; // Seconds between global transition checks
    float m_transitionCooldownTimer = 0.0f;

    // Helper to determine environment zone for a position
    EnvironmentZone determineEnvironmentZone(const glm::vec3& position, float waterLevel) const;

    // Initialize transition controller for aquatic creatures
    void initializeTransitionController(uint32_t creatureIndex, CreatureType type);
};

} // namespace Forge
