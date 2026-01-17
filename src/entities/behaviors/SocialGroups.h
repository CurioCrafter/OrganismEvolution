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
namespace Forge { class CreatureManager; }
using Forge::CreatureManager;

/**
 * @brief Manages pack/herd/flock social groupings for creatures
 *
 * Social groups form naturally based on proximity and creature type.
 * Groups have leaders, maintain cohesion, and move together.
 * Supports herds (herbivores), packs (predators), and flocks (flying).
 */
class SocialGroupManager {
public:
    enum class GroupType {
        HERD,       // Herbivores - safety in numbers
        PACK,       // Predators - coordinated hunting
        FLOCK,      // Flying creatures - V-formations, murmuration
        SCHOOL,     // Aquatic creatures - synchronized swimming
        SOLITARY    // No social grouping
    };

    struct GroupMember {
        uint32_t creatureID = 0;
        float joinTime = 0.0f;
        float loyalty = 0.0f;           // 0-1, affects how likely to stay in group
        glm::vec3 targetOffset{0.0f};   // Desired position relative to leader
        bool isLeader = false;
    };

    struct Group {
        uint32_t groupID = 0;
        uint32_t leaderID = 0;
        std::vector<GroupMember> members;
        glm::vec3 centroid{0.0f};
        glm::vec3 averageVelocity{0.0f};
        CreatureType creatureType = CreatureType::GRAZER;
        GroupType groupType = GroupType::HERD;
        float cohesion = 1.0f;          // Group tightness (0-1)
        float formationRadius = 20.0f;  // Max spread of group
        float age = 0.0f;               // Time since group formed
        bool isHunting = false;         // Pack hunting state
        glm::vec3 huntTarget{0.0f};     // Target position when hunting
    };

    struct SocialConfig {
        float minGroupSize = 2;                 // Minimum creatures to form group
        float maxGroupSize = 20;                // Maximum creatures per group
        float groupFormDistance = 25.0f;        // Distance to detect potential members
        float groupBreakDistance = 50.0f;       // Distance before member leaves
        float loyaltyGainRate = 0.05f;          // Loyalty increase per second in group
        float loyaltyDecayRate = 0.02f;         // Loyalty decay when separated
        float leaderChallengeThreshold = 0.8f;  // Fitness ratio to challenge leader
        float cohesionForce = 1.0f;             // Force pulling toward centroid
        float separationForce = 1.2f;           // Force preventing overcrowding
        float alignmentForce = 0.8f;            // Force matching group velocity
        float formationForce = 0.5f;            // Force maintaining formation
    };

    SocialGroupManager() = default;
    ~SocialGroupManager() = default;

    /**
     * @brief Update all social groups - called once per frame
     */
    void update(float deltaTime, CreatureManager& creatures, const SpatialGrid& grid);

    /**
     * @brief Calculate social steering force for a creature
     */
    glm::vec3 calculateForce(Creature* creature);

    /**
     * @brief Check if a creature is a group leader
     */
    bool isLeader(uint32_t creatureID) const;

    /**
     * @brief Get the group a creature belongs to
     * @return Pointer to group, or nullptr if not in a group
     */
    const Group* getCreatureGroup(uint32_t creatureID) const;
    Group* getCreatureGroup(uint32_t creatureID);

    /**
     * @brief Get group ID for a creature
     * @return Group ID, or 0 if not in a group
     */
    uint32_t getGroupID(uint32_t creatureID) const;

    /**
     * @brief Force a creature to leave its group
     */
    void leaveGroup(uint32_t creatureID);

    /**
     * @brief Get all groups for visualization
     */
    const std::unordered_map<uint32_t, Group>& getGroups() const { return m_groups; }

    /**
     * @brief Configuration access
     */
    SocialConfig& getConfig() { return m_config; }
    const SocialConfig& getConfig() const { return m_config; }

    /**
     * @brief Statistics
     */
    size_t getGroupCount() const { return m_groups.size(); }
    size_t getAverageGroupSize() const;
    size_t getLargestGroupSize() const;

    /**
     * @brief Determine what type of social group a creature type forms
     */
    static GroupType getGroupTypeForCreature(CreatureType type);

    /**
     * @brief Check if a creature type is naturally social
     */
    static bool isSocialType(CreatureType type);

private:
    // Try to form new groups from ungrouped creatures
    void formNewGroups(CreatureManager& creatures, const SpatialGrid& grid);

    // Update existing groups (remove dead, check distances)
    void updateExistingGroups(float deltaTime, CreatureManager& creatures);

    // Merge nearby groups of same type
    void mergeNearbyGroups(CreatureManager& creatures);

    // Split oversized groups
    void splitOversizedGroups(CreatureManager& creatures);

    // Update group centroid and average velocity
    void updateGroupStats(Group& group, CreatureManager& creatures);

    // Select best leader for a group
    void electLeader(Group& group, CreatureManager& creatures);

    // Calculate formation positions for members
    void updateFormation(Group& group, CreatureManager& creatures);

    // Calculate force to maintain group cohesion
    glm::vec3 calculateCohesionForce(Creature* creature, const Group& group);

    // Calculate force for separation (personal space)
    glm::vec3 calculateSeparationForce(Creature* creature, const Group& group, CreatureManager& creatures);

    // Calculate force for velocity alignment
    glm::vec3 calculateAlignmentForce(Creature* creature, const Group& group);

    // Calculate force to maintain formation position
    glm::vec3 calculateFormationForce(Creature* creature, const Group& group, CreatureManager& creatures);

    // Add creature to a group
    void addToGroup(uint32_t groupID, uint32_t creatureID);

    // Remove creature from its group
    void removeFromGroup(uint32_t creatureID);

    std::unordered_map<uint32_t, Group> m_groups;
    std::unordered_map<uint32_t, uint32_t> m_creatureToGroup;  // creatureID -> groupID
    std::unordered_set<uint32_t> m_groupsToRemove;
    uint32_t m_nextGroupID = 1;
    SocialConfig m_config;
    float m_currentTime = 0.0f;
};
