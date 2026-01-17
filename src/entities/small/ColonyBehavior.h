#pragma once

#include "SmallCreatureType.h"
#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <atomic>
#include <unordered_map>

namespace small {

using namespace DirectX;

// Forward declarations
struct SmallCreature;
class SmallCreatureManager;

// Colony ID generator
class ColonyID {
public:
    static uint32_t generate() {
        return nextID.fetch_add(1, std::memory_order_relaxed);
    }
private:
    static std::atomic<uint32_t> nextID;
};

// Colony roles for social insects
enum class ColonyRole : uint8_t {
    QUEEN,          // Reproduces
    KING,           // For termites
    WORKER,         // General labor
    SOLDIER,        // Defense
    NURSE,          // Care for larvae
    FORAGER,        // Find food
    BUILDER,        // Construct nest
    SCOUT,          // Explore
    DRONE           // Male for mating
};

// Task assignment for workers
struct ColonyTask {
    enum class Type : uint8_t {
        IDLE,
        FORAGE,
        PATROL,
        BUILD,
        NURSE,
        DEFEND,
        FOLLOW_TRAIL,
        RETURN_HOME,
        RECRUIT,
        STORE_FOOD,
        REMOVE_WASTE
    };

    Type type;
    XMFLOAT3 targetPosition;
    float priority;
    float timeRemaining;
    uint32_t assignedTo;     // Creature ID
};

// Nest/Hive structure
struct NestChamber {
    enum class Type : uint8_t {
        ENTRANCE,
        QUEEN_CHAMBER,
        BROOD_CHAMBER,
        FOOD_STORAGE,
        WASTE_CHAMBER,
        TUNNEL,
        DEFENSE_POST
    };

    uint32_t id;
    Type type;
    XMFLOAT3 position;
    XMFLOAT3 size;
    float capacity;          // How much can be stored
    float currentOccupancy;
    std::vector<uint32_t> connectedTo;  // Other chamber IDs
};

// Colony state and resources
struct ColonyResources {
    float foodStored;
    float buildingMaterial;
    int larvae;
    int pupae;
    int eggs;
    float nestIntegrity;     // 0-100, degrades over time
};

// Main Colony class
class Colony {
public:
    Colony(SmallCreatureType baseType, const XMFLOAT3& nestPosition);
    ~Colony();

    // Update colony state
    void update(float deltaTime, SmallCreatureManager& manager);

    // Member management
    void addMember(SmallCreature* creature, ColonyRole role = ColonyRole::WORKER);
    void removeMember(uint32_t creatureID);
    void setQueen(SmallCreature* queen);
    SmallCreature* getQueen() const { return queen_; }

    // Task assignment
    ColonyTask* assignTask(SmallCreature* creature);
    void completeTask(uint32_t creatureID);

    // Resource management
    void addFood(float amount);
    float consumeFood(float amount);
    void addBuildingMaterial(float amount);
    float getBuildingMaterial() const { return resources_.buildingMaterial; }

    // Nest management
    NestChamber* createChamber(NestChamber::Type type, const XMFLOAT3& position);
    NestChamber* getNearestChamber(const XMFLOAT3& position, NestChamber::Type type = static_cast<NestChamber::Type>(-1));

    // Query
    uint32_t getID() const { return id_; }
    const XMFLOAT3& getNestPosition() const { return nestPosition_; }
    size_t getMemberCount() const { return members_.size(); }
    SmallCreatureType getBaseType() const { return baseType_; }
    const ColonyResources& getResources() const { return resources_; }

    // Colony state
    bool isUnderAttack() const { return underAttack_; }
    void setUnderAttack(bool attack) { underAttack_ = attack; }
    float getColonyHealth() const;

    // Get members by role
    std::vector<SmallCreature*> getMembersByRole(ColonyRole role) const;

private:
    // Internal updates
    void updateTaskAssignments(float deltaTime, SmallCreatureManager& manager);
    void updateReproduction(float deltaTime, SmallCreatureManager& manager);
    void updateNestMaintenance(float deltaTime);
    void assignRoles();

    // Decision making
    void makeColonyDecisions(float deltaTime, SmallCreatureManager& manager);
    ColonyTask::Type decideNextPriority();

    uint32_t id_;
    SmallCreatureType baseType_;
    XMFLOAT3 nestPosition_;

    SmallCreature* queen_;
    std::unordered_map<uint32_t, std::pair<SmallCreature*, ColonyRole>> members_;
    std::vector<ColonyTask> taskQueue_;
    std::vector<std::unique_ptr<NestChamber>> chambers_;

    ColonyResources resources_;

    bool underAttack_;
    float reproductionCooldown_;
    float decisionCooldown_;
};

// Pheromone types
enum class PheromoneType : uint8_t {
    FOOD_TRAIL,       // Path to food
    HOME_TRAIL,       // Path to nest
    ALARM,            // Danger signal
    RECRUITMENT,      // Call for help
    TERRITORY,        // Boundary marker
    MATING,           // Attraction signal
    QUEEN,            // Queen presence
    DEATH             // Dead colony member
};

// Pheromone point in 3D space
struct PheromonePoint {
    XMFLOAT3 position;
    float strength;       // Current strength (decays)
    float maxStrength;    // Initial strength
    uint32_t colonyID;    // Which colony produced it
    enum class Type : uint8_t {
        FOOD_TRAIL,
        HOME_TRAIL,
        ALARM,
        RECRUITMENT,
        TERRITORY,
        MATING
    } type;
    float age;            // Time since creation
};

// Pheromone system for trail following
class PheromoneSystem {
public:
    PheromoneSystem(float worldSize);
    ~PheromoneSystem();

    // Add pheromone at position
    void addPheromone(const XMFLOAT3& position, uint32_t colonyID,
                      PheromonePoint::Type type, float strength = 1.0f);

    // Update (decay pheromones)
    void update(float deltaTime);

    // Query pheromones
    std::vector<PheromonePoint*> queryNearby(const XMFLOAT3& position, float radius,
                                              uint32_t colonyID = 0,
                                              PheromonePoint::Type type = static_cast<PheromonePoint::Type>(-1));

    // Get strongest pheromone in direction
    PheromonePoint* getStrongestInDirection(const XMFLOAT3& position,
                                             const XMFLOAT3& direction,
                                             float coneAngle,
                                             uint32_t colonyID,
                                             PheromonePoint::Type type);

    // Get gradient direction (for trail following)
    XMFLOAT3 getGradientDirection(const XMFLOAT3& position, float radius,
                                   uint32_t colonyID, PheromonePoint::Type type);

    // Access all points (for rendering)
    std::vector<PheromonePoint>& getPoints() { return points_; }
    const std::vector<PheromonePoint>& getPoints() const { return points_; }

    // Clear all pheromones
    void clear();

    // Statistics
    size_t getPointCount() const { return points_.size(); }

private:
    void cleanup();  // Remove weak pheromones

    std::vector<PheromonePoint> points_;
    float worldSize_;

    static constexpr float DECAY_RATE = 0.1f;       // Per second
    static constexpr float MIN_STRENGTH = 0.01f;    // Cleanup threshold
    static constexpr size_t MAX_POINTS = 50000;     // Memory limit
    static constexpr float MERGE_DISTANCE = 0.1f;   // Merge nearby points
};

// Swarm behavior for non-colonial aggregations
class SwarmBehavior {
public:
    // Calculate swarm forces (separation, alignment, cohesion)
    static XMFLOAT3 calculateSwarmForce(SmallCreature* creature,
                                         const std::vector<SmallCreature*>& neighbors,
                                         float separationWeight = 1.5f,
                                         float alignmentWeight = 1.0f,
                                         float cohesionWeight = 1.0f);

    // Locust swarm specific behavior (destructive migration)
    static XMFLOAT3 calculateLocustSwarmForce(SmallCreature* creature,
                                               const std::vector<SmallCreature*>& neighbors,
                                               const XMFLOAT3& foodDirection);

    // Fly swarm (loose aggregation around point)
    static XMFLOAT3 calculateFlySwarmForce(SmallCreature* creature,
                                            const std::vector<SmallCreature*>& neighbors,
                                            const XMFLOAT3& swarmCenter);

    // Mosquito cloud behavior
    static XMFLOAT3 calculateMosquitoCloudForce(SmallCreature* creature,
                                                 const std::vector<SmallCreature*>& neighbors,
                                                 const XMFLOAT3& targetPosition);
};

// Ant-specific behaviors
class AntBehavior {
public:
    // Follow pheromone trail
    static XMFLOAT3 followTrail(SmallCreature* creature, PheromoneSystem& pheromones);

    // Lay pheromone trail
    static void layTrail(SmallCreature* creature, PheromoneSystem& pheromones,
                         PheromonePoint::Type type, float strength);

    // Recruit other ants (tandem running)
    static void recruitNearbyAnts(SmallCreature* leader, Colony& colony,
                                   SmallCreatureManager& manager);

    // Formation behavior for soldier ants
    static XMFLOAT3 defendFormation(SmallCreature* soldier,
                                     const std::vector<SmallCreature*>& enemies,
                                     const XMFLOAT3& nestEntrance);

    // Carrying behavior
    static bool canCarryFood(SmallCreature* ant, float foodWeight);
    static float getCarryCapacity(SmallCreature* ant);
};

// Bee-specific behaviors
class BeeBehavior {
public:
    // Waggle dance communication
    struct DanceInfo {
        XMFLOAT3 foodDirection;
        float distance;
        float quality;
    };

    static void performWaggleDance(SmallCreature* bee, const DanceInfo& info,
                                    PheromoneSystem& pheromones);
    static DanceInfo interpretWaggleDance(const XMFLOAT3& dancePosition,
                                           PheromoneSystem& pheromones);

    // Foraging patterns
    static XMFLOAT3 calculateForagingPath(SmallCreature* bee,
                                          const std::vector<XMFLOAT3>& flowerPositions);

    // Hive temperature regulation
    static void regulateTemperature(SmallCreature* bee, float currentTemp,
                                     float targetTemp);

    // Swarming behavior (colony splitting)
    static bool shouldSwarm(Colony& colony);
    static XMFLOAT3 findNewNestLocation(SmallCreature* scout,
                                         const XMFLOAT3& currentNest);
};

// Termite-specific behaviors
class TermiteBehavior {
public:
    // Mound construction
    static XMFLOAT3 getMoundBuildPosition(SmallCreature* termite,
                                          const XMFLOAT3& moundCenter,
                                          float currentHeight);

    // Tunnel navigation
    static XMFLOAT3 navigateTunnel(SmallCreature* termite,
                                    const std::vector<NestChamber*>& chambers);

    // Wood decomposition
    static float calculateDigestionRate(SmallCreature* termite);
};

} // namespace small
