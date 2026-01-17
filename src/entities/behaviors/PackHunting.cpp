#include "PackHunting.h"
#include "SocialGroups.h"
#include "../Creature.h"
#include "../../core/CreatureManager.h"
#include "../../core/FoodChainManager.h"
#include "../../utils/SpatialGrid.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

// Helper to determine if a predator type can hunt a prey type
static bool isValidPreyType(CreatureType predator, CreatureType prey) {
    // Use predefined isPredator helper from CreatureType.h
    if (!isPredator(predator)) return false;

    // Apex predators hunt herbivores and small predators
    if (predator == CreatureType::APEX_PREDATOR || predator == CreatureType::CARNIVORE) {
        return prey == CreatureType::GRAZER || prey == CreatureType::BROWSER ||
               prey == CreatureType::FRUGIVORE || prey == CreatureType::SMALL_PREDATOR ||
               prey == CreatureType::AMPHIBIAN || prey == CreatureType::HERBIVORE;
    }
    // Small predators hunt frugivores
    if (predator == CreatureType::SMALL_PREDATOR) {
        return prey == CreatureType::FRUGIVORE;
    }
    // Aerial predators dive on ground prey
    if (predator == CreatureType::AERIAL_PREDATOR) {
        return prey == CreatureType::GRAZER || prey == CreatureType::FRUGIVORE ||
               prey == CreatureType::HERBIVORE;
    }
    // Aquatic predators hunt aquatic herbivores
    if (predator == CreatureType::AQUATIC_PREDATOR || predator == CreatureType::AQUATIC_APEX) {
        return prey == CreatureType::AQUATIC_HERBIVORE || prey == CreatureType::AQUATIC;
    }
    // Flying creatures (generic) can hunt small ground creatures
    if (isFlying(predator)) {
        return prey == CreatureType::HERBIVORE || prey == CreatureType::GRAZER ||
               prey == CreatureType::FRUGIVORE;
    }
    return false;
}

void PackHuntingBehavior::update(float deltaTime, CreatureManager& creatures,
                                  const SocialGroupManager& groups, const SpatialGrid& grid,
                                  const FoodChainManager& foodChain) {
    m_currentTime += deltaTime;

    // Update cooldowns
    for (auto it = m_huntCooldowns.begin(); it != m_huntCooldowns.end(); ) {
        it->second -= deltaTime;
        if (it->second <= 0.0f) {
            it = m_huntCooldowns.erase(it);
        } else {
            ++it;
        }
    }

    // Update existing hunts
    updateActiveHunts(deltaTime, creatures, grid);

    // Try to start new hunts
    initiateNewHunts(creatures, groups, grid, foodChain);

    // Clean up completed hunts
    for (uint32_t id : m_huntsToRemove) {
        auto& hunt = m_activeHunts[id];
        for (const auto& hunter : hunt.hunters) {
            m_creatureToHunt.erase(hunter.creatureID);
            m_hunterRoles.erase(hunter.creatureID);
        }
        m_targetsBeingHunted.erase(hunt.targetID);
        m_activeHunts.erase(id);
    }
    m_huntsToRemove.clear();
}

glm::vec3 PackHuntingBehavior::calculateForce(Creature* hunter) {
    if (!hunter || !hunter->isAlive()) {
        return glm::vec3(0.0f);
    }

    auto it = m_creatureToHunt.find(hunter->getID());
    if (it == m_creatureToHunt.end()) {
        return glm::vec3(0.0f);
    }

    auto huntIt = m_activeHunts.find(it->second);
    if (huntIt == m_activeHunts.end()) {
        return glm::vec3(0.0f);
    }

    const Hunt& hunt = huntIt->second;

    // Find this hunter in the hunt
    const Hunter* thisHunter = nullptr;
    for (const auto& h : hunt.hunters) {
        if (h.creatureID == hunter->getID()) {
            thisHunter = &h;
            break;
        }
    }

    if (!thisHunter) return glm::vec3(0.0f);

    // Calculate force based on current phase
    switch (hunt.phase) {
        case HuntPhase::STALKING:
            return calculateStalkingForce(*thisHunter, hunt, hunter);
        case HuntPhase::FLANKING:
            return calculateFlankingForce(*thisHunter, hunt, hunter);
        case HuntPhase::CHASE:
            return calculateChaseForce(*thisHunter, hunt, hunter);
        case HuntPhase::TAKEDOWN:
            return calculateTakedownForce(*thisHunter, hunt, hunter);
        default:
            return glm::vec3(0.0f);
    }
}

bool PackHuntingBehavior::isHunting(uint32_t creatureID) const {
    return m_creatureToHunt.count(creatureID) > 0;
}

const PackHuntingBehavior::Hunt* PackHuntingBehavior::getHunt(uint32_t creatureID) const {
    auto it = m_creatureToHunt.find(creatureID);
    if (it == m_creatureToHunt.end()) return nullptr;

    auto huntIt = m_activeHunts.find(it->second);
    if (huntIt == m_activeHunts.end()) return nullptr;

    return &huntIt->second;
}

PackHuntingBehavior::HuntRole PackHuntingBehavior::getRole(uint32_t creatureID) const {
    auto it = m_hunterRoles.find(creatureID);
    if (it == m_hunterRoles.end()) return HuntRole::NONE;
    return it->second;
}

bool PackHuntingBehavior::isBeingHunted(uint32_t creatureID) const {
    return m_targetsBeingHunted.count(creatureID) > 0;
}

void PackHuntingBehavior::initiateNewHunts(CreatureManager& creatures,
                                           const SocialGroupManager& groups,
                                           const SpatialGrid& grid,
                                           const FoodChainManager& foodChain) {
    // Check each predator pack for potential hunts
    for (const auto& [groupID, group] : groups.getGroups()) {
        // Only packs can hunt
        if (group.groupType != SocialGroupManager::GroupType::PACK) continue;

        // Check if pack already hunting
        bool alreadyHunting = false;
        for (const auto& member : group.members) {
            if (m_creatureToHunt.count(member.creatureID) > 0) {
                alreadyHunting = true;
                break;
            }
        }
        if (alreadyHunting) continue;

        // Check if enough members are ready (not on cooldown)
        std::vector<uint32_t> availableHunters;
        for (const auto& member : group.members) {
            if (m_huntCooldowns.count(member.creatureID) == 0) {
                availableHunters.push_back(member.creatureID);
            }
        }

        if (availableHunters.size() < static_cast<size_t>(m_config.minPackSize)) continue;

        // Find prey near the group
        auto& nearbyCreatures = grid.query(group.centroid, m_config.huntRange);

        Creature* bestPrey = nullptr;
        float bestScore = -1.0f;

        for (Creature* potential : nearbyCreatures) {
            if (!potential || !potential->isAlive()) continue;

            // Check if already being hunted
            if (m_targetsBeingHunted.count(potential->getID()) > 0) continue;

            // Check if valid prey for this predator type (predators hunt herbivores/grazers)
            if (!isValidPreyType(group.creatureType, potential->getType())) continue;

            // Score prey (prefer isolated, well-fed prey)
            float dist = glm::distance(potential->getPosition(), group.centroid);
            float distScore = 1.0f - (dist / m_config.huntRange);
            float energyScore = potential->getEnergy() / 200.0f;  // Well-fed prey = more reward

            // Count nearby same-type creatures (prefer isolated prey)
            int defenders = 0;
            auto& nearPrey = grid.query(potential->getPosition(), 15.0f);
            for (auto* nearby : nearPrey) {
                if (nearby && nearby->isAlive() && nearby->getType() == potential->getType()) {
                    defenders++;
                }
            }
            float isolationScore = 1.0f / (1.0f + defenders * 0.3f);

            float score = distScore * 0.4f + energyScore * 0.3f + isolationScore * 0.3f;

            if (score > bestScore) {
                bestScore = score;
                bestPrey = potential;
            }
        }

        if (bestPrey && bestScore > 0.3f) {
            // Initiate hunt
            Hunt newHunt;
            newHunt.huntID = m_nextHuntID++;
            newHunt.targetID = bestPrey->getID();
            newHunt.targetLastKnownPos = bestPrey->getPosition();
            newHunt.targetPredictedPos = predictPreyPosition(bestPrey, 2.0f);
            newHunt.phase = HuntPhase::STALKING;
            newHunt.startTime = m_currentTime;
            newHunt.phaseStartTime = m_currentTime;

            // Add hunters (up to max)
            size_t count = 0;
            for (uint32_t hunterID : availableHunters) {
                if (count >= static_cast<size_t>(m_config.maxPackSize)) break;

                Hunter hunter;
                hunter.creatureID = hunterID;
                hunter.fatigue = 0.0f;
                hunter.inPosition = false;
                newHunt.hunters.push_back(hunter);
                count++;
            }

            // Assign roles
            assignRoles(newHunt, creatures);

            // Register hunt
            for (const auto& hunter : newHunt.hunters) {
                m_creatureToHunt[hunter.creatureID] = newHunt.huntID;
                m_hunterRoles[hunter.creatureID] = hunter.role;
            }
            m_targetsBeingHunted.insert(newHunt.targetID);
            m_activeHunts[newHunt.huntID] = newHunt;
        }
    }
}

void PackHuntingBehavior::updateActiveHunts(float deltaTime, CreatureManager& creatures,
                                            const SpatialGrid& grid) {
    for (auto& [huntID, hunt] : m_activeHunts) {
        // Update phase duration
        hunt.phaseDuration = m_currentTime - hunt.phaseStartTime;

        // Update target tracking
        updateTargetTracking(hunt, creatures);

        // Check if target is still valid
        Creature* target = creatures.getCreatureByID(hunt.targetID);
        if (!target || !target->isAlive()) {
            completeHunt(hunt, false, creatures);
            m_huntsToRemove.insert(huntID);
            continue;
        }

        // Check if hunt should be abandoned
        if (shouldAbandonHunt(hunt, creatures)) {
            completeHunt(hunt, false, creatures);
            m_huntsToRemove.insert(huntID);
            continue;
        }

        // Remove dead hunters
        hunt.hunters.erase(
            std::remove_if(hunt.hunters.begin(), hunt.hunters.end(),
                          [&creatures](const Hunter& h) {
                              Creature* c = creatures.getCreatureByID(h.creatureID);
                              return !c || !c->isAlive();
                          }),
            hunt.hunters.end());

        // Check minimum hunters
        if (hunt.hunters.size() < static_cast<size_t>(m_config.minPackSize)) {
            completeHunt(hunt, false, creatures);
            m_huntsToRemove.insert(huntID);
            continue;
        }

        // Update fatigue
        if (hunt.phase == HuntPhase::CHASE || hunt.phase == HuntPhase::TAKEDOWN) {
            for (auto& hunter : hunt.hunters) {
                hunter.fatigue += m_config.fatigueRate * deltaTime;
            }
        }

        // Calculate role positions
        calculateRolePositions(hunt, creatures);

        // Calculate encirclement
        hunt.encirclementScore = calculateEncirclement(hunt, creatures);

        // Check for phase transition
        if (shouldAdvancePhase(hunt, creatures)) {
            advancePhase(hunt);
        }

        // Check for successful takedown
        if (hunt.phase == HuntPhase::TAKEDOWN) {
            for (const auto& hunter : hunt.hunters) {
                Creature* hunterCreature = creatures.getCreatureByID(hunter.creatureID);
                if (!hunterCreature) continue;

                float dist = glm::distance(hunterCreature->getPosition(), target->getPosition());
                if (dist < m_config.attackRange) {
                    // Attack!
                    hunterCreature->attack(target, deltaTime);

                    if (!target->isAlive()) {
                        completeHunt(hunt, true, creatures);
                        m_huntsToRemove.insert(huntID);
                        break;
                    }
                }
            }
        }
    }
}

void PackHuntingBehavior::assignRoles(Hunt& hunt, CreatureManager& creatures) {
    if (hunt.hunters.empty()) return;

    Creature* target = creatures.getCreatureByID(hunt.targetID);
    if (!target) return;

    glm::vec3 targetPos = target->getPosition();

    // Find the creature closest to target - they're the leader
    float minDist = FLT_MAX;
    size_t leaderIdx = 0;

    for (size_t i = 0; i < hunt.hunters.size(); i++) {
        Creature* hunter = creatures.getCreatureByID(hunt.hunters[i].creatureID);
        if (!hunter) continue;

        float dist = glm::distance(hunter->getPosition(), targetPos);
        if (dist < minDist) {
            minDist = dist;
            leaderIdx = i;
        }
    }

    // Assign leader
    hunt.hunters[leaderIdx].role = HuntRole::LEADER;

    // Assign other roles based on position relative to target
    for (size_t i = 0; i < hunt.hunters.size(); i++) {
        if (i == leaderIdx) continue;

        Creature* hunter = creatures.getCreatureByID(hunt.hunters[i].creatureID);
        if (!hunter) continue;

        glm::vec3 hunterPos = hunter->getPosition();
        glm::vec3 toTarget = targetPos - hunterPos;
        glm::vec3 targetVel = target->getVelocity();

        // Determine role based on position and prey movement
        float angle = 0.0f;
        if (glm::length(targetVel) > 0.1f) {
            glm::vec3 preyDir = glm::normalize(targetVel);
            glm::vec3 hunterDir = glm::length(toTarget) > 0.1f ? glm::normalize(toTarget) : glm::vec3(1, 0, 0);
            angle = std::acos(glm::clamp(glm::dot(preyDir, hunterDir), -1.0f, 1.0f));
        }

        // Position ahead of prey movement -> blocker
        // Position to side -> flanker
        // Position behind -> chaser
        if (angle < glm::pi<float>() * 0.25f) {
            hunt.hunters[i].role = HuntRole::BLOCKER;
        } else if (angle > glm::pi<float>() * 0.75f) {
            hunt.hunters[i].role = HuntRole::CHASER;
        } else {
            hunt.hunters[i].role = HuntRole::FLANKER;
        }
    }
}

void PackHuntingBehavior::updateTargetTracking(Hunt& hunt, CreatureManager& creatures) {
    Creature* target = creatures.getCreatureByID(hunt.targetID);
    if (!target || !target->isAlive()) return;

    hunt.targetLastKnownPos = target->getPosition();
    hunt.targetPredictedPos = predictPreyPosition(target, 2.0f);
}

void PackHuntingBehavior::calculateRolePositions(Hunt& hunt, CreatureManager& creatures) {
    Creature* target = creatures.getCreatureByID(hunt.targetID);
    if (!target) return;

    glm::vec3 targetPos = hunt.targetLastKnownPos;
    glm::vec3 targetVel = target->getVelocity();
    glm::vec3 targetDir = glm::length(targetVel) > 0.1f ? glm::normalize(targetVel) : glm::vec3(1, 0, 0);
    glm::vec3 perpendicular(-targetDir.z, 0, targetDir.x);

    int flankerCount = 0;
    for (auto& hunter : hunt.hunters) {
        switch (hunter.role) {
            case HuntRole::LEADER:
                // Leader stays closest to target
                hunter.assignedPosition = targetPos - targetDir * m_config.flankingDistance * 0.5f;
                break;

            case HuntRole::FLANKER:
                // Flankers spread to sides
                {
                    float side = (flankerCount % 2 == 0) ? 1.0f : -1.0f;
                    hunter.assignedPosition = targetPos + perpendicular * side * m_config.flankingDistance;
                    flankerCount++;
                }
                break;

            case HuntRole::CHASER:
                // Chasers pursue from behind
                hunter.assignedPosition = targetPos - targetDir * m_config.flankingDistance;
                break;

            case HuntRole::BLOCKER:
                // Blockers get ahead of prey
                hunter.assignedPosition = hunt.targetPredictedPos + targetDir * m_config.flankingDistance;
                break;

            case HuntRole::AMBUSHER:
                // Ambushers wait far ahead
                hunter.assignedPosition = hunt.targetPredictedPos + targetDir * m_config.flankingDistance * 2.0f;
                break;

            default:
                hunter.assignedPosition = targetPos;
                break;
        }

        // Check if in position
        Creature* hunterCreature = creatures.getCreatureByID(hunter.creatureID);
        if (hunterCreature) {
            float dist = glm::distance(hunterCreature->getPosition(), hunter.assignedPosition);
            hunter.inPosition = (dist < 5.0f);
        }
    }
}

bool PackHuntingBehavior::shouldAdvancePhase(const Hunt& hunt, CreatureManager& creatures) {
    switch (hunt.phase) {
        case HuntPhase::STALKING:
            // Advance when all hunters are close enough or time limit reached
            if (hunt.phaseDuration > m_config.stalkDuration) return true;
            for (const auto& hunter : hunt.hunters) {
                if (!hunter.inPosition) return false;
            }
            return true;

        case HuntPhase::FLANKING:
            // Advance when encirclement is good or time limit
            if (hunt.phaseDuration > m_config.flankDuration) return true;
            return hunt.encirclementScore > 0.6f;

        case HuntPhase::CHASE:
            // Advance to takedown when close enough
            {
                Creature* target = creatures.getCreatureByID(hunt.targetID);
                if (!target) return false;

                for (const auto& hunter : hunt.hunters) {
                    Creature* h = creatures.getCreatureByID(hunter.creatureID);
                    if (h && glm::distance(h->getPosition(), target->getPosition()) < m_config.attackRange * 2.0f) {
                        return true;
                    }
                }
            }
            return false;

        default:
            return false;
    }
}

void PackHuntingBehavior::advancePhase(Hunt& hunt) {
    hunt.phaseStartTime = m_currentTime;

    switch (hunt.phase) {
        case HuntPhase::STALKING:
            hunt.phase = HuntPhase::FLANKING;
            break;
        case HuntPhase::FLANKING:
            hunt.phase = HuntPhase::CHASE;
            break;
        case HuntPhase::CHASE:
            hunt.phase = HuntPhase::TAKEDOWN;
            break;
        case HuntPhase::TAKEDOWN:
            hunt.phase = HuntPhase::COMPLETED;
            break;
        default:
            break;
    }
}

bool PackHuntingBehavior::shouldAbandonHunt(const Hunt& hunt, CreatureManager& creatures) {
    // Too long in chase phase
    if (hunt.phase == HuntPhase::CHASE && hunt.phaseDuration > m_config.chaseDuration) {
        return true;
    }

    // All hunters fatigued
    bool allFatigued = true;
    for (const auto& hunter : hunt.hunters) {
        if (hunter.fatigue < m_config.maxFatigue) {
            allFatigued = false;
            break;
        }
    }
    if (allFatigued) return true;

    // Target too far from all hunters
    Creature* target = creatures.getCreatureByID(hunt.targetID);
    if (target) {
        bool anyClose = false;
        for (const auto& hunter : hunt.hunters) {
            Creature* h = creatures.getCreatureByID(hunter.creatureID);
            if (h && glm::distance(h->getPosition(), target->getPosition()) < m_config.huntRange * 1.5f) {
                anyClose = true;
                break;
            }
        }
        if (!anyClose) return true;
    }

    // Too many failed attempts
    if (hunt.failedAttempts > 5) return true;

    return false;
}

void PackHuntingBehavior::completeHunt(Hunt& hunt, bool success, CreatureManager& creatures) {
    if (success) {
        m_successfulHunts++;
        // Hunters share energy bonus
        float sharePerHunter = m_config.successBonus / static_cast<float>(hunt.hunters.size());
        for (const auto& hunter : hunt.hunters) {
            Creature* h = creatures.getCreatureByID(hunter.creatureID);
            if (h) {
                h->consumeFood(sharePerHunter);
            }
        }
    } else {
        m_failedHunts++;
    }

    // Put hunters on cooldown
    for (const auto& hunter : hunt.hunters) {
        m_huntCooldowns[hunter.creatureID] = m_config.cooldownAfterHunt;
    }

    hunt.phase = success ? HuntPhase::COMPLETED : HuntPhase::ABANDONED;
}

glm::vec3 PackHuntingBehavior::calculateStalkingForce(const Hunter& hunter, const Hunt& hunt,
                                                       Creature* creature) {
    // Move slowly toward target
    glm::vec3 toTarget = hunt.targetLastKnownPos - creature->getPosition();
    float dist = glm::length(toTarget);

    if (dist < 0.1f) return glm::vec3(0.0f);

    // Slow approach
    return glm::normalize(toTarget) * m_config.stalkSpeed;
}

glm::vec3 PackHuntingBehavior::calculateFlankingForce(const Hunter& hunter, const Hunt& hunt,
                                                       Creature* creature) {
    // Move to assigned flanking position
    glm::vec3 toPosition = hunter.assignedPosition - creature->getPosition();
    float dist = glm::length(toPosition);

    if (dist < 1.0f) {
        // In position - face target
        glm::vec3 toTarget = hunt.targetLastKnownPos - creature->getPosition();
        if (glm::length(toTarget) > 0.1f) {
            return glm::normalize(toTarget) * 0.1f;
        }
        return glm::vec3(0.0f);
    }

    return glm::normalize(toPosition) * glm::min(dist * 0.3f, 1.0f);
}

glm::vec3 PackHuntingBehavior::calculateChaseForce(const Hunter& hunter, const Hunt& hunt,
                                                    Creature* creature) {
    glm::vec3 targetPos;

    // Different roles pursue different positions
    switch (hunter.role) {
        case HuntRole::BLOCKER:
        case HuntRole::AMBUSHER:
            // Get ahead of prey
            targetPos = hunt.targetPredictedPos;
            break;
        case HuntRole::FLANKER:
            // Maintain flanking position
            targetPos = hunter.assignedPosition;
            break;
        default:
            // Direct pursuit
            targetPos = hunt.targetLastKnownPos;
            break;
    }

    glm::vec3 toTarget = targetPos - creature->getPosition();
    float dist = glm::length(toTarget);

    if (dist < 0.1f) return glm::vec3(0.0f);

    return glm::normalize(toTarget) * m_config.chaseSpeed;
}

glm::vec3 PackHuntingBehavior::calculateTakedownForce(const Hunter& hunter, const Hunt& hunt,
                                                       Creature* creature) {
    // All-out pursuit
    glm::vec3 toTarget = hunt.targetLastKnownPos - creature->getPosition();
    float dist = glm::length(toTarget);

    if (dist < 0.1f) return glm::vec3(0.0f);

    return glm::normalize(toTarget) * m_config.chaseSpeed * 1.2f;
}

float PackHuntingBehavior::calculateEncirclement(const Hunt& hunt, CreatureManager& creatures) {
    Creature* target = creatures.getCreatureByID(hunt.targetID);
    if (!target || hunt.hunters.size() < 2) return 0.0f;

    glm::vec3 targetPos = target->getPosition();

    // Calculate angle coverage around target
    std::vector<float> angles;
    for (const auto& hunter : hunt.hunters) {
        Creature* h = creatures.getCreatureByID(hunter.creatureID);
        if (!h) continue;

        glm::vec3 toHunter = h->getPosition() - targetPos;
        float angle = std::atan2(toHunter.z, toHunter.x);
        angles.push_back(angle);
    }

    if (angles.size() < 2) return 0.0f;

    // Sort angles and calculate coverage
    std::sort(angles.begin(), angles.end());

    float maxGap = 0.0f;
    for (size_t i = 0; i < angles.size(); i++) {
        float gap = angles[(i + 1) % angles.size()] - angles[i];
        if (i == angles.size() - 1) {
            gap = (glm::two_pi<float>() - angles.back()) + angles.front();
        }
        maxGap = std::max(maxGap, gap);
    }

    // Encirclement = 1 - (largest gap / full circle)
    float coverage = 1.0f - (maxGap / glm::two_pi<float>());
    return glm::clamp(coverage, 0.0f, 1.0f);
}

glm::vec3 PackHuntingBehavior::predictPreyPosition(const Creature* prey, float timeAhead) {
    if (!prey) return glm::vec3(0.0f);

    glm::vec3 pos = prey->getPosition();
    glm::vec3 vel = prey->getVelocity();

    return pos + vel * timeAhead;
}
