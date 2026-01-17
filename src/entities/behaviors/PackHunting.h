#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include "../CreatureType.h"

// Forward declarations
class Creature;
class SpatialGrid;
class SocialGroupManager;
namespace Forge {
    class CreatureManager;
    class FoodChainManager;
}
using Forge::CreatureManager;
using Forge::FoodChainManager;

/**
 * @brief Manages coordinated pack hunting behavior for predators
 *
 * Pack hunts involve multiple predators coordinating to take down prey.
 * Different roles (leader, flanker, chaser, blocker) create emergent tactics.
 * Hunts progress through phases: stalking, flanking, chase, takedown.
 */
class PackHuntingBehavior {
public:
    enum class HuntPhase {
        NONE,           // Not hunting
        STALKING,       // Moving slowly toward prey
        FLANKING,       // Spreading out to surround
        CHASE,          // Active pursuit
        TAKEDOWN,       // Closing in for kill
        COMPLETED,      // Hunt finished (success or failure)
        ABANDONED       // Hunt called off
    };

    enum class HuntRole {
        NONE,           // Not assigned
        LEADER,         // Directs hunt, often makes killing blow
        FLANKER,        // Moves to side positions
        CHASER,         // Directly pursues prey
        BLOCKER,        // Cuts off escape routes
        AMBUSHER        // Waits ahead of prey's path
    };

    struct Hunter {
        uint32_t creatureID = 0;
        HuntRole role = HuntRole::NONE;
        glm::vec3 assignedPosition{0.0f};   // Target position for role
        float fatigue = 0.0f;               // Accumulates during chase
        bool inPosition = false;            // Ready for phase transition
    };

    struct Hunt {
        uint32_t huntID = 0;
        uint32_t targetID = 0;              // Prey creature ID
        std::vector<Hunter> hunters;
        glm::vec3 targetLastKnownPos{0.0f}; // Updated each frame
        glm::vec3 targetPredictedPos{0.0f}; // Where prey will be
        HuntPhase phase = HuntPhase::NONE;
        float startTime = 0.0f;
        float phaseStartTime = 0.0f;
        float phaseDuration = 0.0f;         // Time in current phase
        int failedAttempts = 0;             // Attacks that missed
        float encirclementScore = 0.0f;     // 0-1, how surrounded prey is
    };

    struct HuntingConfig {
        float minPackSize = 2;              // Minimum hunters to start hunt
        float maxPackSize = 8;              // Maximum hunters per hunt
        float huntRange = 50.0f;            // Range to detect prey
        float stalkSpeed = 0.3f;            // Speed multiplier during stalk
        float flankingDistance = 15.0f;     // Distance to spread for flanking
        float chaseSpeed = 1.2f;            // Speed boost during chase
        float attackRange = 3.0f;           // Range for takedown
        float fatigueRate = 0.1f;           // Fatigue gain per second
        float maxFatigue = 1.0f;            // Fatigue level that forces rest
        float stalkDuration = 10.0f;        // Max time in stalk phase
        float flankDuration = 8.0f;         // Max time in flank phase
        float chaseDuration = 20.0f;        // Max time before abandoning chase
        float cooldownAfterHunt = 15.0f;    // Rest time after hunt
        float successBonus = 50.0f;         // Energy gain on successful hunt
    };

    PackHuntingBehavior() = default;
    ~PackHuntingBehavior() = default;

    /**
     * @brief Update all active hunts - called once per frame
     */
    void update(float deltaTime, CreatureManager& creatures,
                const SocialGroupManager& groups, const SpatialGrid& grid,
                const FoodChainManager& foodChain);

    /**
     * @brief Calculate hunting steering force for a creature
     */
    glm::vec3 calculateForce(Creature* hunter);

    /**
     * @brief Check if a creature is currently hunting
     */
    bool isHunting(uint32_t creatureID) const;

    /**
     * @brief Get the hunt a creature is participating in
     */
    const Hunt* getHunt(uint32_t creatureID) const;

    /**
     * @brief Get a creature's role in a hunt
     */
    HuntRole getRole(uint32_t creatureID) const;

    /**
     * @brief Check if a creature is being hunted
     */
    bool isBeingHunted(uint32_t creatureID) const;

    /**
     * @brief Get all active hunts for visualization
     */
    const std::unordered_map<uint32_t, Hunt>& getActiveHunts() const { return m_activeHunts; }

    /**
     * @brief Configuration access
     */
    HuntingConfig& getConfig() { return m_config; }
    const HuntingConfig& getConfig() const { return m_config; }

    /**
     * @brief Statistics
     */
    size_t getActiveHuntCount() const { return m_activeHunts.size(); }
    int getSuccessfulHunts() const { return m_successfulHunts; }
    int getFailedHunts() const { return m_failedHunts; }

private:
    // Try to initiate new hunts for pack groups
    void initiateNewHunts(CreatureManager& creatures, const SocialGroupManager& groups,
                          const SpatialGrid& grid, const FoodChainManager& foodChain);

    // Update existing hunts
    void updateActiveHunts(float deltaTime, CreatureManager& creatures, const SpatialGrid& grid);

    // Assign roles to hunters based on position
    void assignRoles(Hunt& hunt, CreatureManager& creatures);

    // Update target tracking
    void updateTargetTracking(Hunt& hunt, CreatureManager& creatures);

    // Calculate assigned positions for each role
    void calculateRolePositions(Hunt& hunt, CreatureManager& creatures);

    // Check phase transition conditions
    bool shouldAdvancePhase(const Hunt& hunt, CreatureManager& creatures);

    // Advance to next hunt phase
    void advancePhase(Hunt& hunt);

    // Check if hunt should be abandoned
    bool shouldAbandonHunt(const Hunt& hunt, CreatureManager& creatures);

    // Process hunt completion (success or failure)
    void completeHunt(Hunt& hunt, bool success, CreatureManager& creatures);

    // Calculate force for stalking phase
    glm::vec3 calculateStalkingForce(const Hunter& hunter, const Hunt& hunt, Creature* creature);

    // Calculate force for flanking phase
    glm::vec3 calculateFlankingForce(const Hunter& hunter, const Hunt& hunt, Creature* creature);

    // Calculate force for chase phase
    glm::vec3 calculateChaseForce(const Hunter& hunter, const Hunt& hunt, Creature* creature);

    // Calculate force for takedown phase
    glm::vec3 calculateTakedownForce(const Hunter& hunter, const Hunt& hunt, Creature* creature);

    // Calculate encirclement score (how surrounded prey is)
    float calculateEncirclement(const Hunt& hunt, CreatureManager& creatures);

    // Predict where prey will be in the near future
    glm::vec3 predictPreyPosition(const Creature* prey, float timeAhead);

    std::unordered_map<uint32_t, Hunt> m_activeHunts;          // huntID -> Hunt
    std::unordered_map<uint32_t, uint32_t> m_creatureToHunt;   // creatureID -> huntID
    std::unordered_map<uint32_t, HuntRole> m_hunterRoles;      // creatureID -> role
    std::unordered_set<uint32_t> m_targetsBeingHunted;         // prey IDs
    std::unordered_map<uint32_t, float> m_huntCooldowns;       // creatureID -> cooldown remaining
    std::unordered_set<uint32_t> m_huntsToRemove;

    uint32_t m_nextHuntID = 1;
    HuntingConfig m_config;
    float m_currentTime = 0.0f;
    int m_successfulHunts = 0;
    int m_failedHunts = 0;
};
