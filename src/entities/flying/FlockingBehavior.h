#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cstdint>

class Creature;

// =============================================================================
// FLOCKING BEHAVIOR SYSTEM
// =============================================================================
// Implements various flocking algorithms for flying creatures including:
// - Reynolds boids (separation, alignment, cohesion)
// - V-formation for migratory birds
// - Murmurations (starling-style topological neighbor flocking)
// - Thermal soaring circles
// - Hunting coordination for raptors

// Types of flocking behavior
enum class FlockType {
    NONE,               // Solitary
    BOIDS,              // Classic Reynolds boids
    V_FORMATION,        // Migratory geese formation
    MURMURATION,        // Starling-style swarm
    THERMAL_CIRCLE,     // Soaring birds in thermals
    HUNTING_PACK,       // Coordinated predator group
    BREEDING_DISPLAY,   // Lek mating display
    ROOSTING           // Evening roost gathering
};

// Formation position in V-formation
enum class FormationRole {
    LEADER,             // Front of formation
    LEFT_WING,          // Left side of V
    RIGHT_WING,         // Right side of V
    FOLLOWER,           // Any position
    UNASSIGNED
};

// Individual bird's state within a flock
struct FlockMember {
    uint32_t creatureId;
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 targetVelocity;   // Desired velocity from flocking rules

    // Formation state
    FormationRole role = FormationRole::UNASSIGNED;
    int formationIndex = -1;     // Position in formation
    uint32_t leaderId = 0;       // Who this bird is following

    // Murmuration state
    std::vector<uint32_t> topologicalNeighbors;  // Fixed number of nearest neighbors
    float phaseOffset = 0.0f;    // For synchronized maneuvers

    // Fatigue for leader rotation
    float leaderFatigue = 0.0f;
    float timeSinceLastTurn = 0.0f;

    // Visual properties
    float wingPhase = 0.0f;      // For synchronized wing beats
    float bankAngle = 0.0f;

    FlockMember() = default;
    FlockMember(uint32_t id) : creatureId(id), position(0.0f), velocity(0.0f),
                               targetVelocity(0.0f) {}
};

// Configuration for flocking behavior
struct FlockingConfig {
    FlockType type = FlockType::BOIDS;

    // Reynolds boids parameters
    float separationRadius = 2.0f;      // Minimum distance between birds
    float alignmentRadius = 10.0f;      // Distance for velocity matching
    float cohesionRadius = 15.0f;       // Distance for group cohesion

    float separationWeight = 1.5f;      // Importance of separation
    float alignmentWeight = 1.0f;       // Importance of alignment
    float cohesionWeight = 1.0f;        // Importance of cohesion

    // V-Formation parameters
    float formationSpacing = 3.0f;      // Distance between birds in V
    float vAngle = 70.0f;               // Angle of V in degrees (120 = wide V)
    float followDistance = 2.5f;        // How far back followers stay
    float leaderRotationTime = 60.0f;   // Seconds before leader rotation

    // Murmuration parameters
    int topologicalNeighbors = 7;       // Number of neighbors to track
    float synchronizationStrength = 0.5f;
    float waveSpeed = 2.0f;             // Speed of waves through flock
    float maxTurnAngle = 120.0f;        // Max degrees/second turn

    // Thermal soaring parameters
    float thermalCircleRadius = 15.0f;
    float thermalSpacing = 5.0f;
    float climbRate = 2.0f;             // m/s climb rate

    // General parameters
    float maxSpeed = 15.0f;
    float minSpeed = 8.0f;
    float maxAcceleration = 5.0f;
    float obstacleAvoidanceRadius = 5.0f;
    float goalStrength = 0.3f;          // Weight of goal-seeking
};

// A flock is a collection of birds with shared behavior
struct Flock {
    uint32_t flockId;
    FlockType type;
    FlockingConfig config;

    std::vector<FlockMember> members;
    std::unordered_map<uint32_t, size_t> memberIndex;  // creatureId -> index

    // Flock state
    glm::vec3 centroid;                 // Center of flock
    glm::vec3 averageVelocity;          // Average velocity
    glm::vec3 goalPosition;             // Where flock is heading
    glm::vec3 flightDirection;          // Overall flock direction

    // V-Formation state
    uint32_t leaderId = 0;
    float timeSinceLeaderChange = 0.0f;

    // Murmuration state
    glm::vec3 waveOrigin;               // Origin of current wave
    float wavePhase = 0.0f;
    bool inManeuver = false;

    // Thermal state
    glm::vec3 thermalCenter;
    float currentAltitude = 0.0f;

    Flock() = default;
    Flock(uint32_t id, FlockType t) : flockId(id), type(t), centroid(0.0f),
          averageVelocity(0.0f), goalPosition(0.0f), flightDirection(0.0f, 0.0f, -1.0f),
          waveOrigin(0.0f), thermalCenter(0.0f) {}
};

// =============================================================================
// FLOCKING BEHAVIOR CLASS
// =============================================================================

class FlockingBehavior {
public:
    FlockingBehavior();

    // Create and manage flocks
    uint32_t createFlock(FlockType type, const FlockingConfig& config = FlockingConfig());
    void disbandFlock(uint32_t flockId);
    Flock* getFlock(uint32_t flockId);

    // Member management
    void addMember(uint32_t flockId, uint32_t creatureId, const glm::vec3& position,
                   const glm::vec3& velocity);
    void removeMember(uint32_t flockId, uint32_t creatureId);
    FlockMember* getMember(uint32_t flockId, uint32_t creatureId);

    // Update all flocks
    void update(float deltaTime);

    // Update individual flock
    void updateFlock(Flock& flock, float deltaTime);

    // Get calculated velocity for a creature
    glm::vec3 getTargetVelocity(uint32_t flockId, uint32_t creatureId) const;

    // Set flock goal (migration destination, etc.)
    void setFlockGoal(uint32_t flockId, const glm::vec3& goal);

    // Formation control
    void assignFormationRole(uint32_t flockId, uint32_t creatureId, FormationRole role);
    void rotateLeader(uint32_t flockId);

    // Murmuration control
    void triggerManeuver(uint32_t flockId, const glm::vec3& direction);
    void setWaveOrigin(uint32_t flockId, const glm::vec3& origin);

    // Thermal soaring
    void setThermalCenter(uint32_t flockId, const glm::vec3& center);

    // Query functions
    std::vector<uint32_t> getNearbyFlocks(const glm::vec3& position, float radius) const;
    uint32_t findFlockForCreature(uint32_t creatureId) const;
    bool isCreatureInFlock(uint32_t creatureId) const;

    // Auto-flock management (automatically group nearby creatures)
    void autoFormFlocks(const std::vector<Creature*>& creatures, float groupingRadius,
                        FlockType defaultType);
    void mergeCloseFlocks(float mergeDistance);
    void splitLargeFlocks(int maxFlockSize);

    // Statistics
    struct FlockStats {
        int totalFlocks;
        int totalMembers;
        float averageFlockSize;
        float averageSpeed;
        glm::vec3 overallCentroid;
    };
    FlockStats getStats() const;

private:
    std::unordered_map<uint32_t, Flock> m_flocks;
    std::unordered_map<uint32_t, uint32_t> m_creatureToFlock;  // creatureId -> flockId
    uint32_t m_nextFlockId = 1;

    // Update functions for different flock types
    void updateBoidsFlock(Flock& flock, float deltaTime);
    void updateVFormation(Flock& flock, float deltaTime);
    void updateMurmuration(Flock& flock, float deltaTime);
    void updateThermalCircle(Flock& flock, float deltaTime);
    void updateHuntingPack(Flock& flock, float deltaTime);

    // Reynolds boids rules
    glm::vec3 calculateSeparation(const FlockMember& member, const Flock& flock);
    glm::vec3 calculateAlignment(const FlockMember& member, const Flock& flock);
    glm::vec3 calculateCohesion(const FlockMember& member, const Flock& flock);
    glm::vec3 calculateGoalSeeking(const FlockMember& member, const Flock& flock);
    glm::vec3 calculateObstacleAvoidance(const FlockMember& member);

    // V-Formation helpers
    glm::vec3 calculateFormationPosition(const Flock& flock, int formationIndex);
    void assignFormationPositions(Flock& flock);
    uint32_t selectNextLeader(const Flock& flock);

    // Murmuration helpers
    void updateTopologicalNeighbors(Flock& flock);
    glm::vec3 calculateMurmurationVelocity(const FlockMember& member, const Flock& flock);
    void propagateWave(Flock& flock, float deltaTime);

    // Thermal helpers
    glm::vec3 calculateThermalPosition(const Flock& flock, int memberIndex);
    float calculateThermalLift(const glm::vec3& position, const glm::vec3& thermalCenter,
                               float radius);

    // Utility
    void updateFlockCentroid(Flock& flock);
    void limitVelocity(glm::vec3& velocity, float minSpeed, float maxSpeed);
    void smoothVelocityChange(FlockMember& member, float maxAccel, float deltaTime);
};

// =============================================================================
// MIGRATION SYSTEM
// =============================================================================

struct MigrationRoute {
    uint32_t routeId;
    std::vector<glm::vec3> waypoints;
    int currentWaypoint = 0;
    float waypointRadius = 50.0f;  // How close to get before next waypoint
    bool isCircular = false;      // Loop back to start

    glm::vec3 getCurrentWaypoint() const {
        if (waypoints.empty()) return glm::vec3(0.0f);
        return waypoints[currentWaypoint % waypoints.size()];
    }

    void advanceWaypoint() {
        currentWaypoint++;
        if (isCircular) {
            currentWaypoint %= waypoints.size();
        } else {
            currentWaypoint = std::min(currentWaypoint,
                                       static_cast<int>(waypoints.size()) - 1);
        }
    }

    bool isComplete() const {
        return !isCircular && currentWaypoint >= static_cast<int>(waypoints.size()) - 1;
    }
};

class MigrationManager {
public:
    MigrationManager(FlockingBehavior* flockingSystem);

    // Create migration routes
    uint32_t createRoute(const std::vector<glm::vec3>& waypoints, bool circular = false);
    void deleteRoute(uint32_t routeId);

    // Assign flocks to routes
    void assignFlockToRoute(uint32_t flockId, uint32_t routeId);
    void removeFlockFromRoute(uint32_t flockId);

    // Update migration progress
    void update(float deltaTime);

    // Seasonal migration triggers
    void triggerSpringMigration();
    void triggerFallMigration();

    // Get migration status
    bool isFlockMigrating(uint32_t flockId) const;
    float getMigrationProgress(uint32_t flockId) const;

private:
    FlockingBehavior* m_flockingSystem;
    std::unordered_map<uint32_t, MigrationRoute> m_routes;
    std::unordered_map<uint32_t, uint32_t> m_flockToRoute;  // flockId -> routeId
    uint32_t m_nextRouteId = 1;
};

// =============================================================================
// PREDEFINED FLOCK CONFIGURATIONS
// =============================================================================

namespace FlockPresets {
    // Small songbird flock (sparrows, finches)
    FlockingConfig smallBirdFlock();

    // Goose V-formation for migration
    FlockingConfig geeseMigration();

    // Starling murmuration
    FlockingConfig starlingMurmuration();

    // Vulture/hawk thermal soaring
    FlockingConfig vultureThermal();

    // Crow murder (medium group behavior)
    FlockingConfig crowMurder();

    // Seabird following fishing boats
    FlockingConfig seabirdGathering();

    // Swallow hunting swarm
    FlockingConfig swallowHunt();

    // Pelican fishing formation
    FlockingConfig pelicanFishing();
}
