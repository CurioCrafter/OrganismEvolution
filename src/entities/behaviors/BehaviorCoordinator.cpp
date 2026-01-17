#include "BehaviorCoordinator.h"
#include "../Creature.h"
#include "../../core/CreatureManager.h"
#include "../../core/FoodChainManager.h"
#include "../../utils/SpatialGrid.h"
#include "../../environment/SeasonManager.h"
#include "../../environment/BiomeSystem.h"
#include "../../environment/Terrain.h"
#include <glm/gtc/constants.hpp>
#include <sstream>

BehaviorCoordinator::BehaviorCoordinator() {
    // Default weights
    m_weights.territorial = 1.0f;
    m_weights.social = 0.8f;
    m_weights.hunting = 1.5f;
    m_weights.migration = 2.0f;
    m_weights.parental = 1.2f;
    m_weights.fleeFromPredator = 3.0f;
}

void BehaviorCoordinator::init(CreatureManager* creatureManager,
                               SpatialGrid* spatialGrid,
                               FoodChainManager* foodChain,
                               SeasonManager* seasonManager,
                               BiomeSystem* biomeSystem,
                               Terrain* terrain) {
    m_creatureManager = creatureManager;
    m_spatialGrid = spatialGrid;
    m_foodChain = foodChain;
    m_seasonManager = seasonManager;
    m_biomeSystem = biomeSystem;
    m_terrain = terrain;

    // Initialize migration behavior with its dependencies
    m_migration.init(seasonManager, biomeSystem, terrain);

    // Initialize variety behavior system
    m_varietyBehaviors.init(creatureManager, spatialGrid);

    m_initialized = true;
}

void BehaviorCoordinator::reset() {
    // Reset all subsystems to initial state
    m_territorialBehavior = TerritorialBehavior();
    m_socialGroups = SocialGroupManager();
    m_packHunting = PackHuntingBehavior();
    m_migration = MigrationBehavior();
    m_parentalCare = ParentalCareBehavior();

    // Re-initialize migration with dependencies
    if (m_seasonManager && m_biomeSystem && m_terrain) {
        m_migration.init(m_seasonManager, m_biomeSystem, m_terrain);
    }

    // Reset variety behaviors
    m_varietyBehaviors.reset();
    if (m_creatureManager && m_spatialGrid) {
        m_varietyBehaviors.init(m_creatureManager, m_spatialGrid);
    }

    m_currentTime = 0.0f;
}

void BehaviorCoordinator::update(float deltaTime) {
    if (!m_initialized || !m_creatureManager || !m_spatialGrid) {
        return;
    }

    m_currentTime += deltaTime;

    // Update each behavior system
    // Order matters: territorial first, then social (which may depend on territories),
    // then hunting (depends on social groups), then migration/parental

    if (m_territorialEnabled) {
        m_territorialBehavior.update(deltaTime, *m_creatureManager, *m_spatialGrid);
    }

    if (m_socialEnabled) {
        m_socialGroups.update(deltaTime, *m_creatureManager, *m_spatialGrid);
    }

    if (m_huntingEnabled && m_foodChain) {
        m_packHunting.update(deltaTime, *m_creatureManager, m_socialGroups,
                            *m_spatialGrid, *m_foodChain);
    }

    if (m_migrationEnabled) {
        m_migration.update(deltaTime, *m_creatureManager);
    }

    if (m_parentalEnabled) {
        m_parentalCare.update(deltaTime, *m_creatureManager);
    }

    if (m_varietyEnabled) {
        m_varietyBehaviors.update(deltaTime, m_currentTime);
    }
}

glm::vec3 BehaviorCoordinator::calculateBehaviorForces(Creature* creature) {
    if (!creature || !creature->isAlive() || !m_initialized) {
        return glm::vec3(0.0f);
    }

    glm::vec3 territorialForce(0.0f);
    glm::vec3 socialForce(0.0f);
    glm::vec3 huntingForce(0.0f);
    glm::vec3 migrationForce(0.0f);
    glm::vec3 parentalForce(0.0f);
    glm::vec3 fleeForce(0.0f);
    glm::vec3 varietyForce(0.0f);

    // Calculate individual forces from each system
    if (m_territorialEnabled) {
        territorialForce = m_territorialBehavior.calculateForce(creature, *m_spatialGrid);
    }

    if (m_socialEnabled) {
        socialForce = m_socialGroups.calculateForce(creature);
    }

    if (m_huntingEnabled) {
        huntingForce = m_packHunting.calculateForce(creature);
    }

    if (m_migrationEnabled) {
        migrationForce = m_migration.calculateForce(creature);
    }

    if (m_parentalEnabled && m_creatureManager) {
        parentalForce = m_parentalCare.calculateForce(creature, *m_creatureManager);
    }

    // Calculate variety behavior forces (curiosity, mating display, scavenging, etc.)
    if (m_varietyEnabled) {
        varietyForce = m_varietyBehaviors.calculateBehaviorForce(creature, m_currentTime);
    }

    // Always calculate flee force for prey animals
    if (isHerbivore(creature->getType())) {
        fleeForce = calculateFleeForce(creature);
    }

    // Resolve conflicts and combine forces (variety force added to result)
    glm::vec3 result = resolveConflicts(creature, territorialForce, socialForce, huntingForce,
                                        migrationForce, parentalForce, fleeForce);

    // Add variety force with moderate weight (lower priority than survival behaviors)
    float varietyMag = glm::length(varietyForce);
    float fleeMag = glm::length(fleeForce);
    float huntingMag = glm::length(huntingForce);

    // Only add variety force if not actively fleeing or hunting
    if (fleeMag < 0.1f && huntingMag < 0.1f && varietyMag > 0.01f) {
        result += varietyForce * 0.6f;  // Moderate influence
    }

    return result;
}

void BehaviorCoordinator::registerBirth(Creature* parent, Creature* child) {
    if (m_parentalEnabled) {
        m_parentalCare.registerBirth(parent, child);
    }
}

bool BehaviorCoordinator::tryEstablishTerritory(Creature* creature, float resourceQuality) {
    if (!m_territorialEnabled) return false;
    return m_territorialBehavior.tryEstablishTerritory(creature, resourceQuality);
}

bool BehaviorCoordinator::isInActiveBehavior(uint32_t creatureID) const {
    if (m_territorialEnabled && m_territorialBehavior.hasTerritory(creatureID)) {
        return true;
    }
    if (m_socialEnabled && m_socialGroups.getGroupID(creatureID) != 0) {
        return true;
    }
    if (m_huntingEnabled && m_packHunting.isHunting(creatureID)) {
        return true;
    }
    if (m_migrationEnabled && m_migration.isMigrating(creatureID)) {
        return true;
    }
    if (m_parentalEnabled &&
        (m_parentalCare.isParent(creatureID) || m_parentalCare.isDependent(creatureID))) {
        return true;
    }
    if (m_varietyEnabled) {
        const auto* state = m_varietyBehaviors.getBehaviorState(creatureID);
        if (state && state->currentBehavior != VarietyBehaviorType::NONE &&
            state->currentBehavior != VarietyBehaviorType::WANDERING &&
            state->currentBehavior != VarietyBehaviorType::IDLE) {
            return true;
        }
    }
    return false;
}

std::string BehaviorCoordinator::getBehaviorState(uint32_t creatureID) const {
    std::stringstream ss;
    bool hasBehavior = false;

    // Check hunting first (most active)
    if (m_huntingEnabled && m_packHunting.isHunting(creatureID)) {
        auto* hunt = m_packHunting.getHunt(creatureID);
        if (hunt) {
            ss << "Hunting";
            auto role = m_packHunting.getRole(creatureID);
            switch (role) {
                case PackHuntingBehavior::HuntRole::LEADER: ss << " (Leader)"; break;
                case PackHuntingBehavior::HuntRole::FLANKER: ss << " (Flanker)"; break;
                case PackHuntingBehavior::HuntRole::CHASER: ss << " (Chaser)"; break;
                case PackHuntingBehavior::HuntRole::BLOCKER: ss << " (Blocker)"; break;
                default: break;
            }
            hasBehavior = true;
        }
    }

    // Migration
    if (m_migrationEnabled && m_migration.isMigrating(creatureID)) {
        if (hasBehavior) ss << ", ";
        auto* migration = m_migration.getMigration(creatureID);
        if (migration) {
            ss << "Migrating (" << static_cast<int>(migration->progress * 100) << "%)";
        } else {
            ss << "Migrating";
        }
        hasBehavior = true;
    }

    // Territorial
    if (m_territorialEnabled && m_territorialBehavior.hasTerritory(creatureID)) {
        if (hasBehavior) ss << ", ";
        auto* territory = m_territorialBehavior.getTerritory(creatureID);
        if (territory) {
            ss << "Defending Territory (Str: " << static_cast<int>(territory->strength * 100) << "%)";
        } else {
            ss << "Territorial";
        }
        hasBehavior = true;
    }

    // Social
    if (m_socialEnabled && m_socialGroups.getGroupID(creatureID) != 0) {
        if (hasBehavior) ss << ", ";
        if (m_socialGroups.isLeader(creatureID)) {
            ss << "Group Leader";
        } else {
            ss << "In Group";
        }
        hasBehavior = true;
    }

    // Parental
    if (m_parentalEnabled) {
        if (m_parentalCare.isParent(creatureID)) {
            if (hasBehavior) ss << ", ";
            auto children = m_parentalCare.getChildrenIDs(creatureID);
            ss << "Caring for " << children.size() << " offspring";
            hasBehavior = true;
        } else if (m_parentalCare.isDependent(creatureID)) {
            if (hasBehavior) ss << ", ";
            ss << "Following Parent";
            hasBehavior = true;
        }
    }

    // Being hunted
    if (m_huntingEnabled && m_packHunting.isBeingHunted(creatureID)) {
        if (hasBehavior) ss << ", ";
        ss << "Being Hunted!";
        hasBehavior = true;
    }

    // Variety behaviors (curiosity, mating display, scavenging, etc.)
    if (m_varietyEnabled) {
        const auto* state = m_varietyBehaviors.getBehaviorState(creatureID);
        if (state && state->currentBehavior != VarietyBehaviorType::NONE &&
            state->currentBehavior != VarietyBehaviorType::WANDERING &&
            state->currentBehavior != VarietyBehaviorType::IDLE) {
            if (hasBehavior) ss << ", ";
            ss << state->getStateName();
            hasBehavior = true;
        }
    }

    if (!hasBehavior) {
        ss << "Wandering";
    }

    return ss.str();
}

BehaviorCoordinator::BehaviorStats BehaviorCoordinator::getStats() const {
    BehaviorStats stats;

    // Territorial stats
    stats.territoryCount = m_territorialBehavior.getTerritoryCount();
    stats.avgTerritoryStrength = m_territorialBehavior.getAverageStrength();
    stats.totalIntrusions = m_territorialBehavior.getTotalIntrusions();

    // Social stats
    stats.groupCount = m_socialGroups.getGroupCount();
    stats.avgGroupSize = m_socialGroups.getAverageGroupSize();
    stats.largestGroup = m_socialGroups.getLargestGroupSize();

    // Hunting stats
    stats.activeHunts = m_packHunting.getActiveHuntCount();
    stats.successfulHunts = m_packHunting.getSuccessfulHunts();
    stats.failedHunts = m_packHunting.getFailedHunts();

    // Migration stats
    stats.activeMigrations = m_migration.getActiveMigrationCount();
    stats.completedMigrations = m_migration.getCompletedMigrations();

    // Parental stats
    stats.parentChildBonds = m_parentalCare.getActiveBondCount();
    stats.avgBondStrength = m_parentalCare.getAverageBondStrength();
    stats.totalEnergyShared = m_parentalCare.getTotalEnergyShared();

    // Variety behavior stats
    auto varietyStats = m_varietyBehaviors.getStats();
    stats.curiosityBehaviors = varietyStats.curiosityBehaviors;
    stats.matingDisplays = varietyStats.matingDisplays;
    stats.scavengingBehaviors = varietyStats.scavengingBehaviors;
    stats.playBehaviors = varietyStats.playBehaviors;
    stats.varietyTransitions = varietyStats.totalTransitions;

    return stats;
}

glm::vec3 BehaviorCoordinator::calculateFleeForce(Creature* creature) {
    if (!creature || !m_spatialGrid) {
        return glm::vec3(0.0f);
    }

    glm::vec3 fleeForce(0.0f);
    glm::vec3 pos = creature->getPosition();
    float fleeRadius = 30.0f;

    // Query nearby creatures
    auto& nearby = m_spatialGrid->query(pos, fleeRadius);

    for (Creature* other : nearby) {
        if (!other || !other->isAlive() || other == creature) continue;

        // Check if other is a predator
        if (!isPredator(other->getType())) continue;

        // Check if this creature could be prey
        if (!canBeHuntedBy(creature->getType(), other->getType(), creature->getGenome().size)) {
            continue;
        }

        glm::vec3 away = pos - other->getPosition();
        float dist = glm::length(away);

        if (dist > 0.1f && dist < fleeRadius) {
            // Flee force inversely proportional to distance
            float strength = (1.0f - dist / fleeRadius);
            fleeForce += glm::normalize(away) * strength * strength;  // Quadratic falloff
        }
    }

    return fleeForce;
}

glm::vec3 BehaviorCoordinator::resolveConflicts(Creature* creature, const glm::vec3& territorial,
                                                const glm::vec3& social, const glm::vec3& hunting,
                                                const glm::vec3& migration, const glm::vec3& parental,
                                                const glm::vec3& flee) {
    glm::vec3 result(0.0f);

    // Get magnitudes
    float fleeMag = glm::length(flee);
    float huntingMag = glm::length(hunting);
    float migrationMag = glm::length(migration);
    float parentalMag = glm::length(parental);
    float territorialMag = glm::length(territorial);
    float socialMag = glm::length(social);

    // Flee is always highest priority for prey
    if (fleeMag > 0.1f) {
        // Strong flee response overrides most other behaviors
        result += flee * m_weights.fleeFromPredator;

        // Reduce other forces when fleeing
        float fleeFactor = glm::clamp(fleeMag, 0.0f, 1.0f);
        result += social * m_weights.social * (1.0f - fleeFactor * 0.8f);

        // Parental care stays somewhat active even when fleeing
        if (m_parentalCare.isParent(creature->getID())) {
            result += parental * m_weights.parental * 0.5f;
        }

        return result;
    }

    // Hunting is high priority for predators (when active)
    if (huntingMag > 0.1f) {
        result += hunting * m_weights.hunting;

        // Hunting somewhat overrides territorial and social
        result += territorial * m_weights.territorial * 0.3f;
        result += social * m_weights.social * 0.5f;

        return result;
    }

    // Migration is high priority when active
    if (migrationMag > 0.1f) {
        result += migration * m_weights.migration;

        // Migration overrides territorial behavior (leave territory)
        // But social behavior (staying with flock) is still relevant
        result += social * m_weights.social * 0.7f;
        result += parental * m_weights.parental * 0.5f;

        return result;
    }

    // Parental care
    if (parentalMag > 0.1f) {
        result += parental * m_weights.parental;

        // Parents with young have reduced territorial and social forces
        result += territorial * m_weights.territorial * 0.5f;
        result += social * m_weights.social * 0.3f;

        return result;
    }

    // Default: standard weighted combination
    result += territorial * m_weights.territorial;
    result += social * m_weights.social;

    // Limit maximum force
    float maxForce = 5.0f;
    float mag = glm::length(result);
    if (mag > maxForce) {
        result = glm::normalize(result) * maxForce;
    }

    return result;
}

void BehaviorCoordinator::onCreatureDeath(uint32_t creatureId, const glm::vec3& deathPos) {
    if (m_varietyEnabled) {
        m_varietyBehaviors.onCreatureDeath(creatureId, deathPos, m_currentTime);
    }
}
