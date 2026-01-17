#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "../CreatureType.h"

// Forward declarations
class Creature;
namespace Forge { class CreatureManager; }
using Forge::CreatureManager;

/**
 * @brief Manages parental care behavior where parents protect and feed offspring
 *
 * Parent-offspring bonds form at birth and last until offspring mature.
 * Parents provide protection, food sharing, and teaching behaviors.
 * Offspring follow parents and learn foraging locations.
 */
class ParentalCareBehavior {
public:
    enum class CareStage {
        NONE,           // No parental care
        NESTING,        // Parent stays with eggs/newborns
        NURSING,        // Active feeding of offspring
        GUARDING,       // Protecting mobile offspring
        TEACHING,       // Teaching foraging/hunting
        WEANING         // Gradual independence
    };

    struct ParentChild {
        uint32_t parentID = 0;
        uint32_t childID = 0;
        float bondStartTime = 0.0f;     // When bond formed
        float bondStrength = 1.0f;      // 0-1, weakens over time
        CareStage stage = CareStage::NURSING;
        float energyShared = 0.0f;      // Total energy transferred
        float timeNearParent = 0.0f;    // Time offspring spent near parent
        bool isDependent = true;        // Still needs care
    };

    struct NestSite {
        uint32_t parentID = 0;
        glm::vec3 location{0.0f};
        float established = 0.0f;
        std::vector<uint32_t> childrenIDs;
        float safety = 1.0f;            // 0-1, decreases with predator activity
    };

    struct ParentalConfig {
        float careDuration = 60.0f;         // Base duration of parental care
        float careRadiusMultiplier = 3.0f;  // How far offspring can stray
        float followDistance = 5.0f;         // Target distance for following
        float protectionRange = 15.0f;       // Range for defense behavior
        float energyShareRate = 0.5f;        // Energy per second shared
        float energyShareThreshold = 0.6f;   // Parent energy % to start sharing
        float childEnergyThreshold = 0.4f;   // Child energy % to receive
        float bondDecayRate = 0.01f;         // Bond weakening per second
        float weaningBondThreshold = 0.3f;   // Bond strength to start weaning
        float independenceAge = 45.0f;       // Age at which offspring independent
        float protectionForce = 2.0f;        // Force when defending offspring
        float followForce = 1.0f;            // Force for offspring following
    };

    ParentalCareBehavior() = default;
    ~ParentalCareBehavior() = default;

    /**
     * @brief Register a birth event - creates parent-child bond
     */
    void registerBirth(Creature* parent, Creature* child);

    /**
     * @brief Update all parent-child relationships - called once per frame
     */
    void update(float deltaTime, CreatureManager& creatures);

    /**
     * @brief Calculate parental care steering force for a creature
     * @return Force vector - positive for parent (protect), or for child (follow)
     */
    glm::vec3 calculateForce(Creature* creature, CreatureManager& creatures);

    /**
     * @brief Check if a creature is a parent with dependents
     */
    bool isParent(uint32_t creatureID) const;

    /**
     * @brief Check if a creature is a dependent offspring
     */
    bool isDependent(uint32_t creatureID) const;

    /**
     * @brief Get the parent ID for an offspring
     */
    uint32_t getParentID(uint32_t childID) const;

    /**
     * @brief Get all children IDs for a parent
     */
    std::vector<uint32_t> getChildrenIDs(uint32_t parentID) const;

    /**
     * @brief Get the parent-child bond info
     */
    const ParentChild* getBond(uint32_t childID) const;

    /**
     * @brief Force a child to become independent
     */
    void forceIndependence(uint32_t childID);

    /**
     * @brief Check if creature type provides parental care
     */
    static bool providesParentalCare(CreatureType type);

    /**
     * @brief Get care duration multiplier for creature type
     */
    static float getCareDurationMultiplier(CreatureType type);

    /**
     * @brief Get all parent-child pairs for visualization
     */
    const std::vector<ParentChild>& getAllBonds() const { return m_parentChildPairs; }

    /**
     * @brief Get all nest sites for visualization
     */
    const std::vector<NestSite>& getNestSites() const { return m_nestSites; }

    /**
     * @brief Configuration access
     */
    ParentalConfig& getConfig() { return m_config; }
    const ParentalConfig& getConfig() const { return m_config; }

    /**
     * @brief Statistics
     */
    size_t getActiveBondCount() const;
    float getAverageBondStrength() const;
    float getTotalEnergyShared() const { return m_totalEnergyShared; }

private:
    // Update a single parent-child bond
    void updateBond(ParentChild& bond, float deltaTime, CreatureManager& creatures);

    // Update care stage based on bond state
    void updateCareStage(ParentChild& bond, Creature* parent, Creature* child);

    // Process energy sharing from parent to child
    void processEnergySharing(ParentChild& bond, Creature* parent, Creature* child, float deltaTime);

    // Calculate protection force for parent
    glm::vec3 calculateParentProtectionForce(const ParentChild& bond, Creature* parent,
                                             Creature* child, CreatureManager& creatures);

    // Calculate following force for child
    glm::vec3 calculateChildFollowForce(const ParentChild& bond, Creature* child, Creature* parent);

    // Check if offspring should become independent
    bool shouldBecomeIndependent(const ParentChild& bond, Creature* child);

    // Remove dead creatures from bonds
    void cleanupDeadCreatures(CreatureManager& creatures);

    // Register a nest site for a parent
    void registerNest(uint32_t parentID, const glm::vec3& location);

    std::vector<ParentChild> m_parentChildPairs;
    std::unordered_map<uint32_t, size_t> m_childToParent;   // childID -> parent index in pairs
    std::unordered_map<uint32_t, std::vector<size_t>> m_parentToChildren;  // parentID -> indices
    std::vector<NestSite> m_nestSites;
    std::vector<size_t> m_bondsToRemove;  // Indices of bonds to remove

    ParentalConfig m_config;
    float m_currentTime = 0.0f;
    float m_totalEnergyShared = 0.0f;
};
