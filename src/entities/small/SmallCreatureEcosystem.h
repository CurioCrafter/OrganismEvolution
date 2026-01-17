#pragma once

#include "SmallCreatures.h"
#include "ColonyBehavior.h"
#include "TreeDwellers.h"
#include "../Creature.h"
#include "../EcosystemBehaviors.h"
#include "../../environment/Terrain.h"
#include "../../utils/SpatialGrid.h"
#include <memory>

namespace small {

// Integration layer between small creatures and the main ecosystem
class SmallCreatureEcosystem {
public:
    SmallCreatureEcosystem(float worldSize);
    ~SmallCreatureEcosystem();

    // Initialize the small creature ecosystem
    void initialize(Terrain* terrain,
                    SpatialGrid* largeCreatureGrid,
                    class VegetationManager* vegManager,
                    class ProducerSystem* producers,
                    class DecomposerSystem* decomposers);

    // Main update loop
    void update(float deltaTime,
                std::vector<Creature>& largeCreatures,
                Terrain* terrain);

    // Spawn initial populations
    void spawnInitialPopulations(int totalCount = 5000);

    // Add food sources from ecosystem
    void syncFoodFromEcosystem(class ProducerSystem* producers,
                                class DecomposerSystem* decomposers);

    // Register carrion when large creatures die
    void addCarrion(const XMFLOAT3& position, float amount, CreatureType type);

    // Check if small creature is prey for large creature
    bool isPreyFor(SmallCreature* small, Creature* large);

    // Handle large creature eating small creature
    void consumeSmallCreature(Creature* predator, SmallCreature* prey);

    // Query functions for large creatures
    SmallCreature* findSmallPreyNear(const XMFLOAT3& position, float radius,
                                      CreatureType predatorType);
    std::vector<SmallCreature*> findSmallCreaturesNear(const XMFLOAT3& position, float radius);

    // Access sub-systems
    SmallCreatureManager& getManager() { return *manager_; }
    TreeDwellerSystem& getTreeSystem() { return *treeSystem_; }
    PheromoneSystem& getPheromoneSystem() { return *manager_->getPheromoneSystem(); }

    // Statistics
    struct EcosystemStats {
        // Population counts
        size_t totalSmallCreatures;
        size_t aliveSmallCreatures;
        size_t insectCount;
        size_t arachnidCount;
        size_t mammalCount;
        size_t reptileCount;
        size_t amphibianCount;

        // Colony stats
        size_t colonyCount;
        size_t totalColonyMembers;
        float averageColonySize;

        // Interaction stats
        size_t smallEatenByLarge;
        size_t smallKilledBySmall;
        size_t foodConsumed;

        // Tree dwelling
        size_t nestCount;
        size_t creaturesInTrees;
    };
    EcosystemStats getStats() const;

private:
    // Update interactions with large creatures
    void updatePredatorInteractions(std::vector<Creature>& largeCreatures);

    // Update decomposer activity
    void updateDecomposers(float deltaTime);

    // Update pollination
    void updatePollinators(float deltaTime, class ProducerSystem* producers);

    // Spawn creatures based on biome
    void spawnForBiome(const XMFLOAT3& position, int biomeType, int count);

    // Get spawn height based on habitat type
    float getSpawnHeightForHabitat(const XMFLOAT3& basePos, HabitatType habitat, std::mt19937& rng);

    // Get valid small prey types for large predator
    std::vector<SmallCreatureType> getValidPreyTypes(CreatureType predatorType);

    // Sub-systems
    std::unique_ptr<SmallCreatureManager> manager_;
    std::unique_ptr<TreeDwellerSystem> treeSystem_;

    // External references (not owned)
    Terrain* terrain_;
    SpatialGrid* largeCreatureGrid_;
    class VegetationManager* vegManager_;
    class ProducerSystem* producers_;
    class DecomposerSystem* decomposers_;

    float worldSize_;

    // Statistics tracking
    size_t smallEatenByLarge_ = 0;
    size_t smallKilledBySmall_ = 0;
    size_t foodConsumed_ = 0;
};

// Utility functions for ecosystem integration
namespace EcosystemIntegration {

// Convert large creature type to small predator threat level
float getThreatLevel(CreatureType largeType);

// Get attraction level of small creature for large predator
float getAttractionLevel(SmallCreatureType smallType, CreatureType largeType);

// Check if biome is suitable for small creature type
bool isSuitableBiome(SmallCreatureType type, int biomeType);

// Get spawn probability for small creature in biome
float getSpawnProbability(SmallCreatureType type, int biomeType);

// Calculate energy value of small creature for large predator
float getEnergyValue(SmallCreatureType type, float size);

// Determine if small creature should flee from large creature
bool shouldFlee(SmallCreature* small, Creature* large, float distance);

// Calculate flee direction
XMFLOAT3 calculateFleeDirection(SmallCreature* small, Creature* large);

}  // namespace EcosystemIntegration

}  // namespace small
