#include "SocialGroups.h"
#include "../Creature.h"
#include "../../core/CreatureManager.h"
#include "../../utils/SpatialGrid.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

// Static helper functions
SocialGroupManager::GroupType SocialGroupManager::getGroupTypeForCreature(CreatureType type) {
    switch (type) {
        case CreatureType::GRAZER:
        case CreatureType::BROWSER:
        case CreatureType::FRUGIVORE:
            return GroupType::HERD;

        case CreatureType::SMALL_PREDATOR:
        case CreatureType::APEX_PREDATOR:
            return GroupType::PACK;

        case CreatureType::FLYING_BIRD:
        case CreatureType::FLYING_INSECT:
        case CreatureType::AERIAL_PREDATOR:
            return GroupType::FLOCK;

        case CreatureType::AQUATIC_HERBIVORE:
        case CreatureType::AQUATIC_PREDATOR:
            return GroupType::SCHOOL;

        case CreatureType::OMNIVORE:
        case CreatureType::SCAVENGER:
        case CreatureType::AQUATIC_APEX:
        case CreatureType::AMPHIBIAN:
        case CreatureType::PARASITE:
        case CreatureType::CLEANER:
        default:
            return GroupType::SOLITARY;
    }
}

bool SocialGroupManager::isSocialType(CreatureType type) {
    return getGroupTypeForCreature(type) != GroupType::SOLITARY;
}

void SocialGroupManager::update(float deltaTime, CreatureManager& creatures, const SpatialGrid& grid) {
    m_currentTime += deltaTime;

    // Update existing groups first
    updateExistingGroups(deltaTime, creatures);

    // Try to form new groups
    formNewGroups(creatures, grid);

    // Merge nearby groups
    mergeNearbyGroups(creatures);

    // Split oversized groups
    splitOversizedGroups(creatures);

    // Clean up empty groups
    for (uint32_t id : m_groupsToRemove) {
        m_groups.erase(id);
    }
    m_groupsToRemove.clear();
}

glm::vec3 SocialGroupManager::calculateForce(Creature* creature) {
    if (!creature || !creature->isAlive()) {
        return glm::vec3(0.0f);
    }

    uint32_t creatureID = creature->getID();
    auto it = m_creatureToGroup.find(creatureID);

    if (it == m_creatureToGroup.end()) {
        return glm::vec3(0.0f);  // Not in a group
    }

    auto groupIt = m_groups.find(it->second);
    if (groupIt == m_groups.end()) {
        return glm::vec3(0.0f);
    }

    const Group& group = groupIt->second;
    glm::vec3 totalForce(0.0f);

    // Get creature manager for group member lookups
    // Note: We can't directly access CreatureManager here, so we use cached data

    totalForce += calculateCohesionForce(creature, group) * m_config.cohesionForce;
    totalForce += calculateAlignmentForce(creature, group) * m_config.alignmentForce;

    // Leader has different behavior - leads the group
    if (creature->getID() == group.leaderID) {
        // Leader is less constrained by cohesion
        totalForce *= 0.3f;
    }

    return totalForce;
}

bool SocialGroupManager::isLeader(uint32_t creatureID) const {
    auto it = m_creatureToGroup.find(creatureID);
    if (it == m_creatureToGroup.end()) return false;

    auto groupIt = m_groups.find(it->second);
    if (groupIt == m_groups.end()) return false;

    return groupIt->second.leaderID == creatureID;
}

const SocialGroupManager::Group* SocialGroupManager::getCreatureGroup(uint32_t creatureID) const {
    auto it = m_creatureToGroup.find(creatureID);
    if (it == m_creatureToGroup.end()) return nullptr;

    auto groupIt = m_groups.find(it->second);
    if (groupIt == m_groups.end()) return nullptr;

    return &groupIt->second;
}

SocialGroupManager::Group* SocialGroupManager::getCreatureGroup(uint32_t creatureID) {
    auto it = m_creatureToGroup.find(creatureID);
    if (it == m_creatureToGroup.end()) return nullptr;

    auto groupIt = m_groups.find(it->second);
    if (groupIt == m_groups.end()) return nullptr;

    return &groupIt->second;
}

uint32_t SocialGroupManager::getGroupID(uint32_t creatureID) const {
    auto it = m_creatureToGroup.find(creatureID);
    if (it == m_creatureToGroup.end()) return 0;
    return it->second;
}

void SocialGroupManager::leaveGroup(uint32_t creatureID) {
    removeFromGroup(creatureID);
}

size_t SocialGroupManager::getAverageGroupSize() const {
    if (m_groups.empty()) return 0;

    size_t total = 0;
    for (const auto& [id, group] : m_groups) {
        total += group.members.size();
    }
    return total / m_groups.size();
}

size_t SocialGroupManager::getLargestGroupSize() const {
    size_t largest = 0;
    for (const auto& [id, group] : m_groups) {
        largest = std::max(largest, group.members.size());
    }
    return largest;
}

void SocialGroupManager::formNewGroups(CreatureManager& creatures, const SpatialGrid& grid) {
    // Find ungrouped social creatures
    std::vector<Creature*> ungrouped;

    creatures.forEach([&](Creature& c, size_t) {
        if (!c.isAlive()) return;
        if (!isSocialType(c.getType())) return;
        if (m_creatureToGroup.count(c.getID()) > 0) return;

        ungrouped.push_back(&c);
    });

    // Try to form groups from clusters of ungrouped creatures
    std::unordered_set<uint32_t> processed;

    for (Creature* creature : ungrouped) {
        if (processed.count(creature->getID()) > 0) continue;

        // Find nearby ungrouped creatures of same type
        auto& nearby = grid.query(creature->getPosition(), m_config.groupFormDistance);
        std::vector<Creature*> potentialMembers;

        for (Creature* other : nearby) {
            if (!other || !other->isAlive()) continue;
            if (other->getType() != creature->getType()) continue;
            if (m_creatureToGroup.count(other->getID()) > 0) continue;
            if (processed.count(other->getID()) > 0) continue;

            potentialMembers.push_back(other);
        }

        // Need at least minGroupSize to form a group
        if (potentialMembers.size() >= static_cast<size_t>(m_config.minGroupSize)) {
            // Create new group
            uint32_t groupID = m_nextGroupID++;

            Group newGroup;
            newGroup.groupID = groupID;
            newGroup.creatureType = creature->getType();
            newGroup.groupType = getGroupTypeForCreature(creature->getType());
            newGroup.cohesion = 1.0f;
            newGroup.formationRadius = m_config.groupFormDistance;
            newGroup.age = 0.0f;

            // Add members (up to maxGroupSize)
            size_t count = 0;
            for (Creature* member : potentialMembers) {
                if (count >= static_cast<size_t>(m_config.maxGroupSize)) break;

                GroupMember gm;
                gm.creatureID = member->getID();
                gm.joinTime = m_currentTime;
                gm.loyalty = 0.3f;
                gm.isLeader = false;

                newGroup.members.push_back(gm);
                m_creatureToGroup[member->getID()] = groupID;
                processed.insert(member->getID());
                count++;
            }

            // Elect initial leader
            m_groups[groupID] = newGroup;
            electLeader(m_groups[groupID], creatures);
            updateGroupStats(m_groups[groupID], creatures);
        }
    }
}

void SocialGroupManager::updateExistingGroups(float deltaTime, CreatureManager& creatures) {
    for (auto& [groupID, group] : m_groups) {
        group.age += deltaTime;

        // Remove dead members and update loyalty
        std::vector<size_t> toRemove;
        for (size_t i = 0; i < group.members.size(); i++) {
            GroupMember& member = group.members[i];
            Creature* creature = creatures.getCreatureByID(member.creatureID);

            if (!creature || !creature->isAlive()) {
                toRemove.push_back(i);
                m_creatureToGroup.erase(member.creatureID);
                continue;
            }

            // Check distance from group centroid
            float dist = glm::distance(creature->getPosition(), group.centroid);
            if (dist > m_config.groupBreakDistance) {
                // Too far - decrease loyalty
                member.loyalty -= m_config.loyaltyDecayRate * deltaTime * 2.0f;

                if (member.loyalty <= 0.0f) {
                    toRemove.push_back(i);
                    m_creatureToGroup.erase(member.creatureID);
                }
            } else {
                // In range - increase loyalty
                member.loyalty = glm::min(1.0f, member.loyalty + m_config.loyaltyGainRate * deltaTime);
            }
        }

        // Remove departed members (reverse order to preserve indices)
        for (auto it = toRemove.rbegin(); it != toRemove.rend(); ++it) {
            group.members.erase(group.members.begin() + *it);
        }

        // Check if group is too small
        if (group.members.size() < static_cast<size_t>(m_config.minGroupSize)) {
            // Disband group
            for (const auto& member : group.members) {
                m_creatureToGroup.erase(member.creatureID);
            }
            m_groupsToRemove.insert(groupID);
            continue;
        }

        // Check if leader is still valid
        bool leaderValid = false;
        for (const auto& member : group.members) {
            if (member.creatureID == group.leaderID) {
                leaderValid = true;
                break;
            }
        }

        if (!leaderValid) {
            electLeader(group, creatures);
        }

        // Update group statistics
        updateGroupStats(group, creatures);

        // Periodically challenge leader (every ~30 seconds)
        if (std::fmod(group.age, 30.0f) < deltaTime) {
            electLeader(group, creatures);
        }
    }
}

void SocialGroupManager::mergeNearbyGroups(CreatureManager& creatures) {
    std::vector<std::pair<uint32_t, uint32_t>> toMerge;

    for (auto& [id1, group1] : m_groups) {
        for (auto& [id2, group2] : m_groups) {
            if (id1 >= id2) continue;  // Avoid duplicate checks
            if (group1.creatureType != group2.creatureType) continue;

            float dist = glm::distance(group1.centroid, group2.centroid);
            float mergeThreshold = (group1.formationRadius + group2.formationRadius) * 0.5f;

            // Check if groups are close and combined size is acceptable
            if (dist < mergeThreshold &&
                group1.members.size() + group2.members.size() <= static_cast<size_t>(m_config.maxGroupSize)) {
                toMerge.emplace_back(id1, id2);
            }
        }
    }

    // Perform merges
    for (const auto& [id1, id2] : toMerge) {
        auto it1 = m_groups.find(id1);
        auto it2 = m_groups.find(id2);
        if (it1 == m_groups.end() || it2 == m_groups.end()) continue;

        Group& target = it1->second;
        Group& source = it2->second;

        // Move members from source to target
        for (auto& member : source.members) {
            member.loyalty *= 0.5f;  // Reduce loyalty during merge
            target.members.push_back(member);
            m_creatureToGroup[member.creatureID] = id1;
        }

        // Re-elect leader
        electLeader(target, creatures);
        updateGroupStats(target, creatures);

        // Mark source for removal
        m_groupsToRemove.insert(id2);
    }
}

void SocialGroupManager::splitOversizedGroups(CreatureManager& creatures) {
    std::vector<uint32_t> toSplit;

    for (const auto& [id, group] : m_groups) {
        if (group.members.size() > static_cast<size_t>(m_config.maxGroupSize) * 1.5f) {
            toSplit.push_back(id);
        }
    }

    for (uint32_t groupID : toSplit) {
        auto it = m_groups.find(groupID);
        if (it == m_groups.end()) continue;

        Group& original = it->second;
        size_t splitPoint = original.members.size() / 2;

        // Create new group with half the members
        uint32_t newGroupID = m_nextGroupID++;
        Group newGroup;
        newGroup.groupID = newGroupID;
        newGroup.creatureType = original.creatureType;
        newGroup.groupType = original.groupType;
        newGroup.formationRadius = original.formationRadius;

        // Move half the members to new group
        for (size_t i = splitPoint; i < original.members.size(); i++) {
            newGroup.members.push_back(original.members[i]);
            m_creatureToGroup[original.members[i].creatureID] = newGroupID;
        }

        // Remove moved members from original
        original.members.resize(splitPoint);

        // Update both groups
        electLeader(original, creatures);
        updateGroupStats(original, creatures);

        m_groups[newGroupID] = newGroup;
        electLeader(m_groups[newGroupID], creatures);
        updateGroupStats(m_groups[newGroupID], creatures);
    }
}

void SocialGroupManager::updateGroupStats(Group& group, CreatureManager& creatures) {
    if (group.members.empty()) return;

    glm::vec3 centroidSum(0.0f);
    glm::vec3 velocitySum(0.0f);
    int count = 0;

    for (const auto& member : group.members) {
        Creature* creature = creatures.getCreatureByID(member.creatureID);
        if (creature && creature->isAlive()) {
            centroidSum += creature->getPosition();
            velocitySum += creature->getVelocity();
            count++;
        }
    }

    if (count > 0) {
        group.centroid = centroidSum / static_cast<float>(count);
        group.averageVelocity = velocitySum / static_cast<float>(count);
    }

    // Update cohesion based on spread
    float maxDist = 0.0f;
    for (const auto& member : group.members) {
        Creature* creature = creatures.getCreatureByID(member.creatureID);
        if (creature && creature->isAlive()) {
            float dist = glm::distance(creature->getPosition(), group.centroid);
            maxDist = std::max(maxDist, dist);
        }
    }

    group.cohesion = 1.0f - glm::clamp(maxDist / group.formationRadius, 0.0f, 1.0f);
}

void SocialGroupManager::electLeader(Group& group, CreatureManager& creatures) {
    if (group.members.empty()) return;

    // Find fittest member to be leader
    Creature* bestLeader = nullptr;
    float bestFitness = -1.0f;
    uint32_t bestID = 0;

    for (auto& member : group.members) {
        member.isLeader = false;

        Creature* creature = creatures.getCreatureByID(member.creatureID);
        if (!creature || !creature->isAlive()) continue;

        // Leadership score based on: fitness, energy, size, loyalty
        float score = creature->getFitness() * 0.3f +
                      creature->getEnergy() / 200.0f * 0.3f +
                      creature->getGenome().size * 0.2f +
                      member.loyalty * 0.2f;

        if (score > bestFitness) {
            bestFitness = score;
            bestLeader = creature;
            bestID = member.creatureID;
        }
    }

    if (bestLeader) {
        group.leaderID = bestID;

        // Mark leader
        for (auto& member : group.members) {
            if (member.creatureID == bestID) {
                member.isLeader = true;
                break;
            }
        }
    }
}

void SocialGroupManager::updateFormation(Group& group, CreatureManager& creatures) {
    if (group.members.empty()) return;

    Creature* leader = creatures.getCreatureByID(group.leaderID);
    if (!leader || !leader->isAlive()) return;

    glm::vec3 leaderPos = leader->getPosition();
    glm::vec3 leaderDir = glm::normalize(leader->getVelocity());
    if (glm::length(leader->getVelocity()) < 0.1f) {
        leaderDir = glm::vec3(1, 0, 0);  // Default direction
    }

    // Calculate formation positions based on group type
    size_t memberIdx = 0;
    for (auto& member : group.members) {
        if (member.creatureID == group.leaderID) {
            member.targetOffset = glm::vec3(0.0f);
            continue;
        }

        float angle, dist;
        switch (group.groupType) {
            case GroupType::FLOCK:
                // V-formation
                {
                    int side = (memberIdx % 2 == 0) ? 1 : -1;
                    int row = static_cast<int>(memberIdx) / 2 + 1;
                    angle = side * 0.5f * row;  // Radians from leader direction
                    dist = 3.0f * row;  // Distance behind leader
                }
                break;

            case GroupType::PACK:
                // Semi-circle behind leader
                {
                    int count = static_cast<int>(group.members.size()) - 1;
                    float spread = glm::pi<float>() * 0.8f;
                    angle = -spread / 2.0f + spread * memberIdx / std::max(1, count - 1);
                    dist = 4.0f + 2.0f * (memberIdx / 3);
                }
                break;

            case GroupType::SCHOOL:
                // Dense 3D school
                {
                    int layer = static_cast<int>(memberIdx) / 8;
                    int posInLayer = memberIdx % 8;
                    angle = posInLayer * glm::pi<float>() * 0.25f;
                    dist = 2.0f + layer * 2.0f;
                }
                break;

            case GroupType::HERD:
            default:
                // Loose cluster
                {
                    angle = memberIdx * 2.4f;  // Golden angle-ish
                    dist = 3.0f + 2.0f * std::sqrt(static_cast<float>(memberIdx));
                }
                break;
        }

        // Convert angle/distance to offset
        glm::vec3 perpendicular(-leaderDir.z, 0, leaderDir.x);
        member.targetOffset = -leaderDir * dist + perpendicular * std::sin(angle) * dist * 0.5f;

        memberIdx++;
    }
}

glm::vec3 SocialGroupManager::calculateCohesionForce(Creature* creature, const Group& group) {
    // Steer toward group centroid
    glm::vec3 toCentroid = group.centroid - creature->getPosition();
    float dist = glm::length(toCentroid);

    if (dist < 0.1f) return glm::vec3(0.0f);

    // Stronger force when farther from centroid
    float strength = glm::clamp(dist / group.formationRadius, 0.0f, 1.0f);
    return glm::normalize(toCentroid) * strength * group.cohesion;
}

glm::vec3 SocialGroupManager::calculateSeparationForce(Creature* creature, const Group& group,
                                                       CreatureManager& creatures) {
    glm::vec3 force(0.0f);
    float personalSpace = 2.5f;

    for (const auto& member : group.members) {
        if (member.creatureID == creature->getID()) continue;

        Creature* other = creatures.getCreatureByID(member.creatureID);
        if (!other || !other->isAlive()) continue;

        glm::vec3 away = creature->getPosition() - other->getPosition();
        float dist = glm::length(away);

        if (dist < personalSpace && dist > 0.01f) {
            force += glm::normalize(away) * (1.0f - dist / personalSpace);
        }
    }

    return force;
}

glm::vec3 SocialGroupManager::calculateAlignmentForce(Creature* creature, const Group& group) {
    // Match group's average velocity
    glm::vec3 desiredVel = group.averageVelocity;
    glm::vec3 currentVel = creature->getVelocity();

    if (glm::length(desiredVel) < 0.1f) return glm::vec3(0.0f);

    return (desiredVel - currentVel) * 0.1f;
}

glm::vec3 SocialGroupManager::calculateFormationForce(Creature* creature, const Group& group,
                                                      CreatureManager& creatures) {
    Creature* leader = creatures.getCreatureByID(group.leaderID);
    if (!leader || !leader->isAlive()) return glm::vec3(0.0f);

    // Find this creature's target offset
    for (const auto& member : group.members) {
        if (member.creatureID == creature->getID()) {
            glm::vec3 targetPos = leader->getPosition() + member.targetOffset;
            glm::vec3 toTarget = targetPos - creature->getPosition();
            float dist = glm::length(toTarget);

            if (dist > 0.5f) {
                return glm::normalize(toTarget) * glm::min(dist * 0.1f, 1.0f);
            }
            break;
        }
    }

    return glm::vec3(0.0f);
}

void SocialGroupManager::addToGroup(uint32_t groupID, uint32_t creatureID) {
    auto it = m_groups.find(groupID);
    if (it == m_groups.end()) return;

    // Check if already in a group
    if (m_creatureToGroup.count(creatureID) > 0) {
        removeFromGroup(creatureID);
    }

    GroupMember member;
    member.creatureID = creatureID;
    member.joinTime = m_currentTime;
    member.loyalty = 0.3f;
    member.isLeader = false;

    it->second.members.push_back(member);
    m_creatureToGroup[creatureID] = groupID;
}

void SocialGroupManager::removeFromGroup(uint32_t creatureID) {
    auto it = m_creatureToGroup.find(creatureID);
    if (it == m_creatureToGroup.end()) return;

    uint32_t groupID = it->second;
    m_creatureToGroup.erase(it);

    auto groupIt = m_groups.find(groupID);
    if (groupIt != m_groups.end()) {
        auto& members = groupIt->second.members;
        members.erase(
            std::remove_if(members.begin(), members.end(),
                          [creatureID](const GroupMember& m) {
                              return m.creatureID == creatureID;
                          }),
            members.end());
    }
}
