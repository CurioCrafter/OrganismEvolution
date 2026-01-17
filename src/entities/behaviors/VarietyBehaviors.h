#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <string>

class Creature;
class SpatialGrid;
namespace Forge { class CreatureManager; }
using Forge::CreatureManager;

// ============================================================================
// Creature Memory System - Lightweight short-term memory for behavior decisions
// ============================================================================

struct CreatureMemory {
    // Food memory
    glm::vec3 lastFoodLocation = glm::vec3(0.0f);
    float lastFoodTime = -1000.0f;
    bool hasValidFoodMemory = false;

    // Threat memory
    glm::vec3 lastThreatLocation = glm::vec3(0.0f);
    float lastThreatTime = -1000.0f;
    uint32_t lastThreatId = 0;
    bool hasValidThreatMemory = false;

    // Social memory
    glm::vec3 lastMateLocation = glm::vec3(0.0f);
    uint32_t lastMateId = 0;
    float lastMateTime = -1000.0f;
    bool hasMateMemory = false;

    // Exploration memory - locations visited recently
    std::vector<glm::vec3> recentLocations;
    static constexpr int MAX_RECENT_LOCATIONS = 10;

    // Carcass/decay zone memory for scavengers
    glm::vec3 lastCarcassLocation = glm::vec3(0.0f);
    float lastCarcassTime = -1000.0f;
    bool hasCarcassMemory = false;

    // Memory decay rates (seconds)
    static constexpr float FOOD_MEMORY_DECAY = 30.0f;
    static constexpr float THREAT_MEMORY_DECAY = 15.0f;
    static constexpr float MATE_MEMORY_DECAY = 60.0f;
    static constexpr float CARCASS_MEMORY_DECAY = 45.0f;

    void update(float currentTime);
    void rememberFood(const glm::vec3& pos, float time);
    void rememberThreat(const glm::vec3& pos, uint32_t threatId, float time);
    void rememberMate(const glm::vec3& pos, uint32_t mateId, float time);
    void rememberCarcass(const glm::vec3& pos, float time);
    void addVisitedLocation(const glm::vec3& pos);
    float getNoveltyScore(const glm::vec3& pos) const;
    void clear();
};

// ============================================================================
// Behavior Personality - Per-creature traits affecting behavior selection
// ============================================================================

struct BehaviorPersonality {
    // Core personality traits (0-1 scale)
    float curiosity = 0.5f;       // Tendency to explore novel stimuli
    float aggression = 0.5f;      // Willingness to engage in conflict
    float sociability = 0.5f;     // Preference for group activities
    float boldness = 0.5f;        // Risk tolerance (flee threshold)
    float patience = 0.5f;        // Affects mating display duration, waiting

    // Derived from genome traits
    void initFromGenome(float genomeAggression, float genomeSize, float genomeSpeed);

    // Adds small per-instance variation
    void addRandomVariation(uint32_t seed);
};

// ============================================================================
// Behavior State Machine - Tracks active behavior and transitions
// ============================================================================

enum class VarietyBehaviorType {
    NONE,
    IDLE,
    WANDERING,
    CURIOSITY_APPROACH,    // Approaching novel stimulus
    CURIOSITY_INSPECT,     // Inspecting novel object/creature
    MATING_DISPLAY,        // Performing mating ritual
    MATING_APPROACH,       // Moving toward potential mate
    SCAVENGING_SEEK,       // Searching for carcass
    SCAVENGING_FEED,       // Feeding on carcass
    RESTING,               // Energy conservation
    GROOMING,              // Self-maintenance behavior
    PLAYING,               // Juvenile behavior - social play
};

struct BehaviorState {
    VarietyBehaviorType currentBehavior = VarietyBehaviorType::NONE;
    VarietyBehaviorType previousBehavior = VarietyBehaviorType::NONE;
    float behaviorStartTime = 0.0f;
    float behaviorDuration = 0.0f;
    float cooldownEndTime = 0.0f;

    // Target for current behavior
    glm::vec3 targetPosition = glm::vec3(0.0f);
    uint32_t targetCreatureId = 0;

    // Mating display progress
    float displayProgress = 0.0f;
    float displayEnergyCost = 0.0f;

    // State flags
    bool transitionRequested = false;

    std::string getStateName() const;
};

// ============================================================================
// Behavior Priority System
// ============================================================================

struct BehaviorPriority {
    VarietyBehaviorType behavior;
    float priority;
    float urgency;

    bool operator<(const BehaviorPriority& other) const {
        return (priority * urgency) > (other.priority * other.urgency);
    }
};

// ============================================================================
// Variety Behavior Manager - Coordinates new behaviors per creature
// ============================================================================

class VarietyBehaviorManager {
public:
    // Priority levels for conflict resolution
    static constexpr float PRIORITY_SURVIVAL = 10.0f;    // Flee, avoid danger
    static constexpr float PRIORITY_MATING = 6.0f;       // Reproduction
    static constexpr float PRIORITY_HUNGER = 5.0f;       // Scavenging when hungry
    static constexpr float PRIORITY_CURIOSITY = 3.0f;    // Exploration
    static constexpr float PRIORITY_SOCIAL = 2.0f;       // Play, grooming
    static constexpr float PRIORITY_IDLE = 1.0f;         // Default behaviors

    // Cooldown times (seconds)
    static constexpr float CURIOSITY_COOLDOWN = 8.0f;
    static constexpr float MATING_DISPLAY_COOLDOWN = 20.0f;
    static constexpr float SCAVENGING_COOLDOWN = 10.0f;
    static constexpr float PLAY_COOLDOWN = 15.0f;

    VarietyBehaviorManager();
    ~VarietyBehaviorManager() = default;

    void init(CreatureManager* creatureManager, SpatialGrid* spatialGrid);
    void reset();

    // Per-creature state management
    void registerCreature(uint32_t creatureId, const BehaviorPersonality& personality);
    void unregisterCreature(uint32_t creatureId);

    // Main update
    void update(float deltaTime, float currentTime);

    // Calculate behavior force for a creature
    glm::vec3 calculateBehaviorForce(Creature* creature, float currentTime);

    // External event triggers
    void onFoodFound(uint32_t creatureId, const glm::vec3& foodPos, float time);
    void onThreatDetected(uint32_t creatureId, const glm::vec3& threatPos, uint32_t threatId, float time);
    void onPotentialMateDetected(uint32_t creatureId, const glm::vec3& matePos, uint32_t mateId, float time);
    void onCarcassFound(uint32_t creatureId, const glm::vec3& carcassPos, float time);
    void onCreatureDeath(uint32_t creatureId, const glm::vec3& deathPos, float time);

    // State queries
    const BehaviorState* getBehaviorState(uint32_t creatureId) const;
    const CreatureMemory* getMemory(uint32_t creatureId) const;
    std::string getBehaviorDescription(uint32_t creatureId) const;

    // Personality access
    BehaviorPersonality* getPersonality(uint32_t creatureId);
    const BehaviorPersonality* getPersonality(uint32_t creatureId) const;

    // Statistics
    struct Stats {
        int curiosityBehaviors = 0;
        int matingDisplays = 0;
        int scavengingBehaviors = 0;
        int playBehaviors = 0;
        int totalTransitions = 0;
    };
    Stats getStats() const { return m_stats; }

    // Debug
    void setDebugLogging(bool enabled) { m_debugLogging = enabled; }

private:
    // Per-creature data
    struct CreatureData {
        BehaviorState state;
        BehaviorPersonality personality;
        CreatureMemory memory;
    };

    std::unordered_map<uint32_t, CreatureData> m_creatureData;

    // System references
    CreatureManager* m_creatureManager = nullptr;
    SpatialGrid* m_spatialGrid = nullptr;

    // Global carcass tracking for scavenging
    struct CarcassInfo {
        glm::vec3 position;
        float spawnTime;
        float remainingFood;
        bool claimed;
    };
    std::vector<CarcassInfo> m_carcasses;

    // Stats
    Stats m_stats;
    bool m_debugLogging = false;

    // Behavior calculation methods
    glm::vec3 calculateCuriosityForce(Creature* creature, CreatureData& data, float currentTime);
    glm::vec3 calculateMatingDisplayForce(Creature* creature, CreatureData& data, float currentTime);
    glm::vec3 calculateScavengingForce(Creature* creature, CreatureData& data, float currentTime);
    glm::vec3 calculatePlayForce(Creature* creature, CreatureData& data, float currentTime);
    glm::vec3 calculateRestingForce(Creature* creature, CreatureData& data, float currentTime);

    // Behavior selection
    std::vector<BehaviorPriority> evaluateBehaviors(Creature* creature, CreatureData& data, float currentTime);
    void selectBehavior(Creature* creature, CreatureData& data, float currentTime);
    bool canTransitionTo(VarietyBehaviorType newBehavior, const CreatureData& data, float currentTime);
    void transitionBehavior(CreatureData& data, VarietyBehaviorType newBehavior, float currentTime);

    // Trigger detection
    bool detectNovelStimulus(Creature* creature, CreatureData& data, glm::vec3& outStimulusPos);
    bool detectPotentialMate(Creature* creature, CreatureData& data, glm::vec3& outMatePos, uint32_t& outMateId);
    bool detectCarcassNearby(Creature* creature, CreatureData& data, glm::vec3& outCarcassPos);

    // Carcass management
    void updateCarcasses(float deltaTime);
    void addCarcass(const glm::vec3& position, float time);

    // Utilities
    float calculateBehaviorWeight(VarietyBehaviorType behavior, const BehaviorPersonality& personality);
    void logBehaviorTransition(uint32_t creatureId, VarietyBehaviorType from, VarietyBehaviorType to);
};

// ============================================================================
// Aquatic Group Dynamics Extensions for FishSchooling
// ============================================================================

namespace aquatic {

// Extended school state for variety behaviors
enum class SchoolBehaviorState {
    CRUISING,           // Normal movement
    FEEDING_FRENZY,     // Aggressive feeding when food found
    PANIC_SCATTER,      // Split due to predator
    REFORMING,          // Rejoining after scatter
    LEADER_FOLLOWING,   // Following designated leader
    DEPTH_MIGRATION,    // Vertical movement for temperature/food
};

struct SchoolDynamics {
    SchoolBehaviorState state = SchoolBehaviorState::CRUISING;
    float stateStartTime = 0.0f;

    // Leader system
    uint32_t leaderId = 0;
    float leaderScore = 0.0f;      // Based on experience/size
    bool hasDesignatedLeader = false;

    // Split/rejoin mechanics
    std::vector<glm::vec3> splitPositions;  // Where groups split to
    float rejoinTimer = 0.0f;
    float splitDistance = 20.0f;

    // Panic wave
    glm::vec3 panicOrigin = glm::vec3(0.0f);
    float panicWaveRadius = 0.0f;
    float panicWaveSpeed = 15.0f;

    // Schooling intensity modulation
    float intensityMultiplier = 1.0f;  // 0.5 = loose, 2.0 = tight

    void update(float deltaTime);
    void triggerPanicWave(const glm::vec3& origin, float time);
    void requestSplit(int numGroups);
    void requestRejoin();
};

// Helper functions for extended schooling
glm::vec3 calculateLeaderFollowForce(const glm::vec3& fishPos, const glm::vec3& leaderPos,
                                     const glm::vec3& leaderVel, float followDistance);

glm::vec3 calculatePanicWaveForce(const glm::vec3& fishPos, const glm::vec3& panicOrigin,
                                  float waveRadius, float waveIntensity);

float calculateLeaderScore(float age, float size, float energy, float survivalTime);

} // namespace aquatic
