#pragma once

#include "SmallCreatureType.h"
#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <atomic>
#include <random>
#include <functional>

// Forward declarations
class Terrain;
class SpatialGrid;

namespace small {

// Forward declarations within namespace
class Colony;
class PheromoneSystem;
struct SmallCreatureGenome;

using namespace DirectX;

// Unique ID generator for small creatures
class SmallCreatureID {
public:
    static uint32_t generate() {
        return nextID.fetch_add(1, std::memory_order_relaxed);
    }
private:
    static std::atomic<uint32_t> nextID;
};

// Compact genome for small creatures (optimized for memory)
struct SmallCreatureGenome {
    // Physical traits (4 bytes each = 24 bytes)
    float size;              // Body size multiplier
    float speed;             // Movement speed multiplier
    float metabolism;        // Energy consumption rate
    float reproduction;      // Reproduction rate multiplier
    float lifespan;          // Maximum age multiplier
    float aggression;        // Hunting/defensive behavior

    // Sensory traits (4 bytes each = 24 bytes)
    float visionRange;       // How far can see
    float visionAngle;       // Field of view
    float smellRange;        // Pheromone/scent detection
    float hearingRange;      // Sound detection
    float touchSensitivity;  // Vibration detection
    float antennaeLength;    // Insect-specific sensing

    // Behavioral traits (4 bytes each = 20 bytes)
    float socialStrength;    // How strongly follows group
    float territoriality;    // Home range defense
    float curiosity;         // Exploration tendency
    float fearResponse;      // Flight vs fight threshold
    float nestingDrive;      // Building/burrowing tendency

    // Neural weights (simplified - 8 weights = 32 bytes)
    float neuralWeights[8];

    // Color/appearance (4 bytes each = 12 bytes)
    float colorR, colorG, colorB;

    // Flags (1 byte)
    uint8_t flags;           // Packed boolean traits

    // Total: ~113 bytes per genome

    // Default constructor with random initialization
    SmallCreatureGenome();

    // Crossover two genomes
    static SmallCreatureGenome crossover(const SmallCreatureGenome& a,
                                         const SmallCreatureGenome& b,
                                         std::mt19937& rng);

    // Mutate genome
    void mutate(float rate, std::mt19937& rng);

    // Initialize for specific creature type
    void initializeForType(SmallCreatureType type, std::mt19937& rng);
};

// Life stage for creatures with metamorphosis
enum class LifeStage : uint8_t {
    EGG,
    LARVA,
    PUPA,
    ADULT,
    // For non-metamorphic creatures
    JUVENILE,
    MATURE
};

// Compact small creature data (optimized for cache efficiency)
struct SmallCreature {
    // Identity (8 bytes)
    uint32_t id;
    SmallCreatureType type;
    LifeStage stage;
    uint8_t flags;           // Packed booleans

    // Position/motion (48 bytes)
    XMFLOAT3 position;
    XMFLOAT3 velocity;
    XMFLOAT3 targetPosition;
    float rotation;          // Y-axis rotation

    // State (24 bytes)
    float energy;
    float age;
    float health;
    float fear;
    float hunger;
    float matingUrge;

    // Colonial data (8 bytes)
    uint32_t colonyID;       // 0 if solitary
    uint32_t nestID;         // Home location ID

    // Animation (8 bytes)
    float animationTime;
    float animationSpeed;

    // Genome pointer (8 bytes on 64-bit)
    SmallCreatureGenome* genome;

    // Total: ~104 bytes + genome

    // Flags bit definitions
    static constexpr uint8_t FLAG_ALIVE = 0x01;
    static constexpr uint8_t FLAG_MALE = 0x02;
    static constexpr uint8_t FLAG_CARRYING_FOOD = 0x04;
    static constexpr uint8_t FLAG_IN_NEST = 0x08;
    static constexpr uint8_t FLAG_FLEEING = 0x10;
    static constexpr uint8_t FLAG_HUNTING = 0x20;
    static constexpr uint8_t FLAG_RESTING = 0x40;
    static constexpr uint8_t FLAG_MATING = 0x80;

    bool isAlive() const { return flags & FLAG_ALIVE; }
    bool isMale() const { return flags & FLAG_MALE; }
    bool isCarryingFood() const { return flags & FLAG_CARRYING_FOOD; }
    bool isInNest() const { return flags & FLAG_IN_NEST; }
    bool isFleeing() const { return flags & FLAG_FLEEING; }
    bool isHunting() const { return flags & FLAG_HUNTING; }
    bool isResting() const { return flags & FLAG_RESTING; }
    bool isMating() const { return flags & FLAG_MATING; }

    void setAlive(bool v) { if(v) flags |= FLAG_ALIVE; else flags &= ~FLAG_ALIVE; }
    void setMale(bool v) { if(v) flags |= FLAG_MALE; else flags &= ~FLAG_MALE; }
    void setCarryingFood(bool v) { if(v) flags |= FLAG_CARRYING_FOOD; else flags &= ~FLAG_CARRYING_FOOD; }
    void setInNest(bool v) { if(v) flags |= FLAG_IN_NEST; else flags &= ~FLAG_IN_NEST; }
    void setFleeing(bool v) { if(v) flags |= FLAG_FLEEING; else flags &= ~FLAG_FLEEING; }
    void setHunting(bool v) { if(v) flags |= FLAG_HUNTING; else flags &= ~FLAG_HUNTING; }
    void setResting(bool v) { if(v) flags |= FLAG_RESTING; else flags &= ~FLAG_RESTING; }
    void setMating(bool v) { if(v) flags |= FLAG_MATING; else flags &= ~FLAG_MATING; }
};

// Food source for small creatures
struct MicroFood {
    XMFLOAT3 position;
    float amount;
    enum class Type : uint8_t {
        PLANT_MATTER,    // Leaves, grass
        NECTAR,          // Flower nectar
        POLLEN,          // Flower pollen
        SEEDS,           // Seeds, nuts
        FUNGUS,          // Mushrooms, mold
        CARRION,         // Dead creatures
        INSECT,          // Dead insects
        WOOD,            // Rotting wood (termites)
        BLOOD,           // For parasites
        SOIL_ORGANIC     // For earthworms
    } type;
    uint8_t padding[3];  // Alignment
};

// Pheromone trail point
struct PheromonePoint {
    XMFLOAT3 position;
    float strength;       // Decays over time
    uint32_t colonyID;    // Which colony produced it
    enum class Type : uint8_t {
        FOOD_TRAIL,       // Path to food
        HOME_TRAIL,       // Path to nest
        ALARM,            // Danger signal
        RECRUITMENT,      // Come here
        TERRITORY,        // Boundary marker
        MATING            // Attraction signal
    } type;
    uint8_t padding[3];
};

// Spatial hash for small creatures (separate from main SpatialGrid for different scale)
class MicroSpatialGrid {
public:
    MicroSpatialGrid(float worldSize, float cellSize = 1.0f);
    ~MicroSpatialGrid();

    void clear();
    void insert(SmallCreature* creature);
    void insertFood(MicroFood* food);
    void insertPheromone(PheromonePoint* pheromone);

    // Queries
    std::vector<SmallCreature*> queryCreatures(const XMFLOAT3& pos, float radius) const;
    std::vector<SmallCreature*> queryByType(const XMFLOAT3& pos, float radius, SmallCreatureType type) const;
    std::vector<MicroFood*> queryFood(const XMFLOAT3& pos, float radius) const;
    std::vector<PheromonePoint*> queryPheromones(const XMFLOAT3& pos, float radius,
                                                  PheromonePoint::Type type = static_cast<PheromonePoint::Type>(-1)) const;

    SmallCreature* findNearest(const XMFLOAT3& pos, float maxRadius,
                               std::function<bool(SmallCreature*)> filter = nullptr) const;

    int countInRadius(const XMFLOAT3& pos, float radius) const;

private:
    struct Cell {
        std::vector<SmallCreature*> creatures;
        std::vector<MicroFood*> food;
        std::vector<PheromonePoint*> pheromones;
    };

    int getCellIndex(float x, float z) const;
    void getCellRange(const XMFLOAT3& pos, float radius, int& minX, int& maxX, int& minZ, int& maxZ) const;

    std::vector<Cell> cells_;
    float worldSize_;
    float cellSize_;
    int gridDimension_;
};

// Main manager for all small creatures
class SmallCreatureManager {
public:
    SmallCreatureManager(float worldSize);
    ~SmallCreatureManager();

    // Initialization
    void initialize(Terrain* terrain, int initialPopulation = 1000);
    void setLargeCreatureSpatialGrid(SpatialGrid* grid) { largeCreatureGrid_ = grid; }

    // Spawning
    SmallCreature* spawn(SmallCreatureType type, const XMFLOAT3& position);
    SmallCreature* spawnWithGenome(SmallCreatureType type, const XMFLOAT3& position,
                                    const SmallCreatureGenome& genome);
    void spawnColony(SmallCreatureType type, const XMFLOAT3& position, int size);
    void spawnSwarm(SmallCreatureType type, const XMFLOAT3& center, float radius, int count);

    // Update
    void update(float deltaTime, Terrain* terrain);

    // Queries
    size_t getCreatureCount() const { return creatures_.size(); }
    size_t getAliveCount() const;
    const std::vector<SmallCreature>& getCreatures() const { return creatures_; }
    std::vector<SmallCreature>& getCreatures() { return creatures_; }

    // Colony access
    Colony* getColony(uint32_t id);
    const std::vector<std::unique_ptr<Colony>>& getColonies() const { return colonies_; }

    // Pheromone system access
    PheromoneSystem* getPheromoneSystem() { return pheromoneSystem_.get(); }

    // Food management
    void addFood(const XMFLOAT3& position, float amount, MicroFood::Type type);
    const std::vector<MicroFood>& getFoodSources() const { return foodSources_; }

    // Integration with large creatures
    void registerAsPreyForLargeCreatures(class EcosystemManager* ecosystem);

    // Statistics
    struct Stats {
        size_t totalCreatures;
        size_t aliveCreatures;
        size_t deadCreatures;
        size_t colonyCount;
        size_t insectCount;
        size_t arachnidCount;
        size_t mammalCount;
        size_t reptileCount;
        size_t amphibianCount;
        float averageEnergy;
        float averageAge;
    };
    Stats getStats() const;

    // Spatial grid access (for rendering)
    MicroSpatialGrid* getSpatialGrid() { return spatialGrid_.get(); }

private:
    // Update functions by category
    void updateInsects(float deltaTime, Terrain* terrain);
    void updateArachnids(float deltaTime, Terrain* terrain);
    void updateSmallMammals(float deltaTime, Terrain* terrain);
    void updateReptiles(float deltaTime, Terrain* terrain);
    void updateAmphibians(float deltaTime, Terrain* terrain);

    // Behavior updates
    void updateAntBehavior(SmallCreature& creature, float deltaTime);
    void updateBeeBehavior(SmallCreature& creature, float deltaTime);
    void updateSpiderBehavior(SmallCreature& creature, float deltaTime);
    void updateMouseBehavior(SmallCreature& creature, float deltaTime);
    void updateSquirrelBehavior(SmallCreature& creature, float deltaTime);
    void updateFrogBehavior(SmallCreature& creature, float deltaTime);

    // Life cycle
    void updateMetamorphosis(SmallCreature& creature, float deltaTime);
    void updateReproduction(SmallCreature& creature, float deltaTime);
    void checkDeath(SmallCreature& creature);

    // Cleanup dead creatures
    void cleanupDead();

    // Data
    std::vector<SmallCreature> creatures_;
    std::vector<SmallCreatureGenome> genomes_;  // Stored separately for cache efficiency
    std::vector<MicroFood> foodSources_;
    std::vector<std::unique_ptr<Colony>> colonies_;

    std::unique_ptr<MicroSpatialGrid> spatialGrid_;
    std::unique_ptr<PheromoneSystem> pheromoneSystem_;

    SpatialGrid* largeCreatureGrid_;  // For predator awareness
    Terrain* terrain_;

    float worldSize_;
    std::mt19937 rng_;

    // Pool management
    std::vector<size_t> deadIndices_;  // Reuse slots
    size_t maxCreatures_ = 50000;      // Hard limit
};

} // namespace small
