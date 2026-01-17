#include "FlockingBehavior.h"
#include "../Creature.h"
#include <algorithm>
#include <cmath>
#include <limits>

// =============================================================================
// FLOCKING BEHAVIOR IMPLEMENTATION
// =============================================================================

FlockingBehavior::FlockingBehavior() {}

// =============================================================================
// FLOCK MANAGEMENT
// =============================================================================

uint32_t FlockingBehavior::createFlock(FlockType type, const FlockingConfig& config) {
    uint32_t id = m_nextFlockId++;
    Flock flock(id, type);
    flock.config = config;
    flock.config.type = type;
    m_flocks[id] = flock;
    return id;
}

void FlockingBehavior::disbandFlock(uint32_t flockId) {
    auto it = m_flocks.find(flockId);
    if (it != m_flocks.end()) {
        // Remove all creature mappings
        for (const auto& member : it->second.members) {
            m_creatureToFlock.erase(member.creatureId);
        }
        m_flocks.erase(it);
    }
}

Flock* FlockingBehavior::getFlock(uint32_t flockId) {
    auto it = m_flocks.find(flockId);
    return (it != m_flocks.end()) ? &it->second : nullptr;
}

void FlockingBehavior::addMember(uint32_t flockId, uint32_t creatureId,
                                  const glm::vec3& position, const glm::vec3& velocity) {
    auto it = m_flocks.find(flockId);
    if (it == m_flocks.end()) return;

    // Remove from any existing flock first
    auto existingFlock = m_creatureToFlock.find(creatureId);
    if (existingFlock != m_creatureToFlock.end()) {
        removeMember(existingFlock->second, creatureId);
    }

    FlockMember member(creatureId);
    member.position = position;
    member.velocity = velocity;
    member.targetVelocity = velocity;
    member.wingPhase = static_cast<float>(creatureId) * 0.1f;  // Slight phase offset

    it->second.members.push_back(member);
    it->second.memberIndex[creatureId] = it->second.members.size() - 1;
    m_creatureToFlock[creatureId] = flockId;

    // Reassign formation if V-formation
    if (it->second.type == FlockType::V_FORMATION) {
        assignFormationPositions(it->second);
    }
}

void FlockingBehavior::removeMember(uint32_t flockId, uint32_t creatureId) {
    auto it = m_flocks.find(flockId);
    if (it == m_flocks.end()) return;

    auto indexIt = it->second.memberIndex.find(creatureId);
    if (indexIt == it->second.memberIndex.end()) return;

    size_t index = indexIt->second;

    // Swap with last and remove
    if (index < it->second.members.size() - 1) {
        std::swap(it->second.members[index], it->second.members.back());
        // Update index of swapped member
        it->second.memberIndex[it->second.members[index].creatureId] = index;
    }

    it->second.members.pop_back();
    it->second.memberIndex.erase(indexIt);
    m_creatureToFlock.erase(creatureId);

    // Handle leader removal in V-formation
    if (it->second.type == FlockType::V_FORMATION && it->second.leaderId == creatureId) {
        rotateLeader(flockId);
    }
}

FlockMember* FlockingBehavior::getMember(uint32_t flockId, uint32_t creatureId) {
    auto it = m_flocks.find(flockId);
    if (it == m_flocks.end()) return nullptr;

    auto indexIt = it->second.memberIndex.find(creatureId);
    if (indexIt == it->second.memberIndex.end()) return nullptr;

    return &it->second.members[indexIt->second];
}

// =============================================================================
// UPDATE LOOP
// =============================================================================

void FlockingBehavior::update(float deltaTime) {
    for (auto& pair : m_flocks) {
        updateFlock(pair.second, deltaTime);
    }
}

void FlockingBehavior::updateFlock(Flock& flock, float deltaTime) {
    if (flock.members.empty()) return;

    // Update centroid and average velocity
    updateFlockCentroid(flock);

    // Update based on flock type
    switch (flock.type) {
        case FlockType::BOIDS:
            updateBoidsFlock(flock, deltaTime);
            break;
        case FlockType::V_FORMATION:
            updateVFormation(flock, deltaTime);
            break;
        case FlockType::MURMURATION:
            updateMurmuration(flock, deltaTime);
            break;
        case FlockType::THERMAL_CIRCLE:
            updateThermalCircle(flock, deltaTime);
            break;
        case FlockType::HUNTING_PACK:
            updateHuntingPack(flock, deltaTime);
            break;
        default:
            updateBoidsFlock(flock, deltaTime);
            break;
    }

    // Apply velocity smoothing to all members
    for (auto& member : flock.members) {
        smoothVelocityChange(member, flock.config.maxAcceleration, deltaTime);

        // Update wing phase for synchronized flapping
        member.wingPhase += deltaTime * 2.0f * 3.14159f;
        if (member.wingPhase > 2.0f * 3.14159f) {
            member.wingPhase -= 2.0f * 3.14159f;
        }

        // Calculate bank angle from turn rate
        glm::vec3 velocityDir = glm::normalize(member.velocity);
        glm::vec3 targetDir = glm::normalize(member.targetVelocity);
        float turnAngle = std::acos(std::clamp(glm::dot(velocityDir, targetDir), -1.0f, 1.0f));
        member.bankAngle = turnAngle * 30.0f;  // Bank proportional to turn
    }
}

// =============================================================================
// REYNOLDS BOIDS
// =============================================================================

void FlockingBehavior::updateBoidsFlock(Flock& flock, float deltaTime) {
    for (auto& member : flock.members) {
        glm::vec3 separation = calculateSeparation(member, flock);
        glm::vec3 alignment = calculateAlignment(member, flock);
        glm::vec3 cohesion = calculateCohesion(member, flock);
        glm::vec3 goal = calculateGoalSeeking(member, flock);
        glm::vec3 avoidance = calculateObstacleAvoidance(member);

        // Combine forces with weights
        member.targetVelocity = member.velocity +
            separation * flock.config.separationWeight +
            alignment * flock.config.alignmentWeight +
            cohesion * flock.config.cohesionWeight +
            goal * flock.config.goalStrength +
            avoidance * 2.0f;  // High weight for obstacle avoidance

        limitVelocity(member.targetVelocity, flock.config.minSpeed, flock.config.maxSpeed);
    }
}

glm::vec3 FlockingBehavior::calculateSeparation(const FlockMember& member, const Flock& flock) {
    glm::vec3 steering(0.0f);
    int count = 0;

    for (const auto& other : flock.members) {
        if (other.creatureId == member.creatureId) continue;

        float distance = glm::length(other.position - member.position);
        if (distance < flock.config.separationRadius && distance > 0.001f) {
            // Steer away from neighbor
            glm::vec3 diff = member.position - other.position;
            diff = glm::normalize(diff) / distance;  // Weight by distance
            steering += diff;
            count++;
        }
    }

    if (count > 0) {
        steering /= static_cast<float>(count);
    }

    return steering;
}

glm::vec3 FlockingBehavior::calculateAlignment(const FlockMember& member, const Flock& flock) {
    glm::vec3 averageVelocity(0.0f);
    int count = 0;

    for (const auto& other : flock.members) {
        if (other.creatureId == member.creatureId) continue;

        float distance = glm::length(other.position - member.position);
        if (distance < flock.config.alignmentRadius) {
            averageVelocity += other.velocity;
            count++;
        }
    }

    if (count > 0) {
        averageVelocity /= static_cast<float>(count);
        return (averageVelocity - member.velocity) * 0.1f;  // Gradually match
    }

    return glm::vec3(0.0f);
}

glm::vec3 FlockingBehavior::calculateCohesion(const FlockMember& member, const Flock& flock) {
    glm::vec3 centerOfMass(0.0f);
    int count = 0;

    for (const auto& other : flock.members) {
        if (other.creatureId == member.creatureId) continue;

        float distance = glm::length(other.position - member.position);
        if (distance < flock.config.cohesionRadius) {
            centerOfMass += other.position;
            count++;
        }
    }

    if (count > 0) {
        centerOfMass /= static_cast<float>(count);
        glm::vec3 direction = centerOfMass - member.position;
        return glm::normalize(direction) * 0.5f;  // Move toward center
    }

    return glm::vec3(0.0f);
}

glm::vec3 FlockingBehavior::calculateGoalSeeking(const FlockMember& member, const Flock& flock) {
    if (glm::length(flock.goalPosition) < 0.001f) {
        return glm::vec3(0.0f);
    }

    glm::vec3 toGoal = flock.goalPosition - member.position;
    float distance = glm::length(toGoal);

    if (distance > 1.0f) {
        return glm::normalize(toGoal);
    }

    return glm::vec3(0.0f);
}

glm::vec3 FlockingBehavior::calculateObstacleAvoidance(const FlockMember& member) {
    // Ground avoidance
    glm::vec3 avoidance(0.0f);

    if (member.position.y < 10.0f) {
        avoidance.y += (10.0f - member.position.y) * 0.5f;
    }

    // Could add terrain/obstacle detection here

    return avoidance;
}

// =============================================================================
// V-FORMATION
// =============================================================================

void FlockingBehavior::updateVFormation(Flock& flock, float deltaTime) {
    // Update leader fatigue
    flock.timeSinceLeaderChange += deltaTime;

    // Rotate leader if tired
    if (flock.timeSinceLeaderChange > flock.config.leaderRotationTime) {
        rotateLeader(flock.flockId);
    }

    // Find leader
    FlockMember* leader = nullptr;
    for (auto& member : flock.members) {
        if (member.creatureId == flock.leaderId) {
            leader = &member;
            break;
        }
    }

    if (!leader) {
        // No leader, assign one
        if (!flock.members.empty()) {
            flock.leaderId = flock.members[0].creatureId;
            flock.members[0].role = FormationRole::LEADER;
            leader = &flock.members[0];
            assignFormationPositions(flock);
        } else {
            return;
        }
    }

    // Leader follows goal
    leader->targetVelocity = calculateGoalSeeking(*leader, flock) * flock.config.maxSpeed;
    if (glm::length(leader->targetVelocity) < flock.config.minSpeed) {
        leader->targetVelocity = flock.flightDirection * flock.config.maxSpeed;
    }
    limitVelocity(leader->targetVelocity, flock.config.minSpeed, flock.config.maxSpeed);

    // Update flock flight direction based on leader
    flock.flightDirection = glm::normalize(leader->velocity);

    // Other members follow their formation position
    for (auto& member : flock.members) {
        if (member.creatureId == flock.leaderId) continue;

        glm::vec3 targetPos = calculateFormationPosition(flock, member.formationIndex);
        glm::vec3 toTarget = targetPos - member.position;

        // Basic PD controller to reach formation position
        float distance = glm::length(toTarget);
        if (distance > 0.1f) {
            member.targetVelocity = glm::normalize(toTarget) * std::min(distance * 2.0f,
                                                                        flock.config.maxSpeed);
            // Add leader's velocity to maintain relative position
            member.targetVelocity += leader->velocity * 0.8f;
        } else {
            member.targetVelocity = leader->velocity;
        }

        limitVelocity(member.targetVelocity, flock.config.minSpeed, flock.config.maxSpeed);
    }
}

glm::vec3 FlockingBehavior::calculateFormationPosition(const Flock& flock, int formationIndex) {
    // Find leader position and direction
    const FlockMember* leader = nullptr;
    for (const auto& member : flock.members) {
        if (member.creatureId == flock.leaderId) {
            leader = &member;
            break;
        }
    }

    if (!leader) return glm::vec3(0.0f);

    glm::vec3 leaderPos = leader->position;
    glm::vec3 direction = glm::normalize(leader->velocity);

    // Calculate perpendicular vector for V spread
    glm::vec3 up(0, 1, 0);
    glm::vec3 right = glm::normalize(glm::cross(direction, up));

    // V-formation geometry
    float vAngleRad = glm::radians(flock.config.vAngle * 0.5f);
    float spacing = flock.config.formationSpacing;

    // Alternate left and right
    int side = (formationIndex % 2 == 0) ? 1 : -1;
    int depth = (formationIndex + 1) / 2;

    float backOffset = depth * spacing * std::cos(vAngleRad);
    float sideOffset = depth * spacing * std::sin(vAngleRad) * side;

    glm::vec3 position = leaderPos
                         - direction * backOffset
                         + right * sideOffset
                         - up * depth * 0.5f;  // Slight altitude stagger

    return position;
}

void FlockingBehavior::assignFormationPositions(Flock& flock) {
    int index = 0;
    for (auto& member : flock.members) {
        if (member.creatureId == flock.leaderId) {
            member.role = FormationRole::LEADER;
            member.formationIndex = 0;
        } else {
            member.role = (index % 2 == 0) ? FormationRole::LEFT_WING : FormationRole::RIGHT_WING;
            member.formationIndex = index + 1;
            index++;
        }
        member.leaderId = flock.leaderId;
    }
}

void FlockingBehavior::rotateLeader(uint32_t flockId) {
    auto it = m_flocks.find(flockId);
    if (it == m_flocks.end()) return;

    Flock& flock = it->second;
    if (flock.members.size() < 2) return;

    // Find current leader and mark as follower
    for (auto& member : flock.members) {
        if (member.creatureId == flock.leaderId) {
            member.leaderFatigue = 0.0f;  // Reset fatigue
            member.role = FormationRole::FOLLOWER;
            break;
        }
    }

    // Select next leader (lowest fatigue among followers)
    uint32_t newLeaderId = selectNextLeader(flock);
    flock.leaderId = newLeaderId;
    flock.timeSinceLeaderChange = 0.0f;

    // Mark new leader
    for (auto& member : flock.members) {
        if (member.creatureId == newLeaderId) {
            member.role = FormationRole::LEADER;
            break;
        }
    }

    // Reassign formation positions
    assignFormationPositions(flock);
}

uint32_t FlockingBehavior::selectNextLeader(const Flock& flock) {
    float lowestFatigue = std::numeric_limits<float>::max();
    uint32_t bestCandidate = flock.members[0].creatureId;

    for (const auto& member : flock.members) {
        if (member.creatureId == flock.leaderId) continue;  // Skip current leader

        if (member.leaderFatigue < lowestFatigue) {
            lowestFatigue = member.leaderFatigue;
            bestCandidate = member.creatureId;
        }
    }

    return bestCandidate;
}

// =============================================================================
// MURMURATION
// =============================================================================

void FlockingBehavior::updateMurmuration(Flock& flock, float deltaTime) {
    // Update topological neighbors periodically
    static float neighborUpdateTimer = 0.0f;
    neighborUpdateTimer += deltaTime;
    if (neighborUpdateTimer > 0.5f) {  // Update every 0.5 seconds
        updateTopologicalNeighbors(flock);
        neighborUpdateTimer = 0.0f;
    }

    // Propagate wave if in maneuver
    if (flock.inManeuver) {
        propagateWave(flock, deltaTime);
    }

    // Calculate velocity for each member based on topological neighbors
    for (auto& member : flock.members) {
        member.targetVelocity = calculateMurmurationVelocity(member, flock);
        limitVelocity(member.targetVelocity, flock.config.minSpeed, flock.config.maxSpeed);
    }
}

void FlockingBehavior::updateTopologicalNeighbors(Flock& flock) {
    int k = flock.config.topologicalNeighbors;

    for (auto& member : flock.members) {
        // Find k nearest neighbors
        std::vector<std::pair<float, uint32_t>> distances;

        for (const auto& other : flock.members) {
            if (other.creatureId == member.creatureId) continue;

            float dist = glm::length(other.position - member.position);
            distances.push_back({dist, other.creatureId});
        }

        // Sort by distance
        std::sort(distances.begin(), distances.end());

        // Take k nearest
        member.topologicalNeighbors.clear();
        for (int i = 0; i < std::min(k, static_cast<int>(distances.size())); ++i) {
            member.topologicalNeighbors.push_back(distances[i].second);
        }
    }
}

glm::vec3 FlockingBehavior::calculateMurmurationVelocity(const FlockMember& member,
                                                          const Flock& flock) {
    if (member.topologicalNeighbors.empty()) {
        return member.velocity;
    }

    glm::vec3 avgPosition(0.0f);
    glm::vec3 avgVelocity(0.0f);
    glm::vec3 separation(0.0f);
    int count = 0;

    // Calculate averages from topological neighbors only
    for (uint32_t neighborId : member.topologicalNeighbors) {
        auto indexIt = flock.memberIndex.find(neighborId);
        if (indexIt == flock.memberIndex.end()) continue;

        const FlockMember& neighbor = flock.members[indexIt->second];

        avgPosition += neighbor.position;
        avgVelocity += neighbor.velocity;

        // Separation from very close neighbors
        float dist = glm::length(neighbor.position - member.position);
        if (dist < flock.config.separationRadius && dist > 0.001f) {
            separation += glm::normalize(member.position - neighbor.position) / dist;
        }

        count++;
    }

    if (count == 0) return member.velocity;

    avgPosition /= static_cast<float>(count);
    avgVelocity /= static_cast<float>(count);

    // Combine forces
    glm::vec3 toCenter = (avgPosition - member.position) * 0.05f;  // Cohesion
    glm::vec3 alignment = (avgVelocity - member.velocity) * 0.1f;

    // Wave-based turning (if in maneuver)
    glm::vec3 waveTurn(0.0f);
    if (flock.inManeuver) {
        float distFromWave = glm::length(member.position - flock.waveOrigin);
        float waveInfluence = std::exp(-distFromWave * 0.1f) *
                              std::sin(flock.wavePhase - distFromWave * 0.2f);
        waveTurn = glm::cross(glm::vec3(0, 1, 0), glm::normalize(member.velocity)) * waveInfluence;
    }

    return member.velocity + toCenter + alignment + separation * 1.5f + waveTurn;
}

void FlockingBehavior::propagateWave(Flock& flock, float deltaTime) {
    flock.wavePhase += deltaTime * flock.config.waveSpeed;

    // End maneuver after wave has propagated
    if (flock.wavePhase > 4.0f * 3.14159f) {
        flock.inManeuver = false;
        flock.wavePhase = 0.0f;
    }
}

void FlockingBehavior::triggerManeuver(uint32_t flockId, const glm::vec3& direction) {
    auto it = m_flocks.find(flockId);
    if (it == m_flocks.end()) return;

    Flock& flock = it->second;
    flock.inManeuver = true;
    flock.wavePhase = 0.0f;

    // Pick edge of flock in given direction as wave origin
    float maxDot = -1.0f;
    for (const auto& member : flock.members) {
        glm::vec3 fromCentroid = member.position - flock.centroid;
        float dot = glm::dot(glm::normalize(fromCentroid), direction);
        if (dot > maxDot) {
            maxDot = dot;
            flock.waveOrigin = member.position;
        }
    }
}

// =============================================================================
// THERMAL SOARING
// =============================================================================

void FlockingBehavior::updateThermalCircle(Flock& flock, float deltaTime) {
    if (glm::length(flock.thermalCenter) < 0.001f) {
        // No thermal set, use boids behavior
        updateBoidsFlock(flock, deltaTime);
        return;
    }

    int index = 0;
    for (auto& member : flock.members) {
        // Calculate position in thermal circle
        glm::vec3 targetPos = calculateThermalPosition(flock, index);

        // Seek circle position
        glm::vec3 toTarget = targetPos - member.position;
        float distance = glm::length(toTarget);

        // Calculate lift
        float lift = calculateThermalLift(member.position, flock.thermalCenter,
                                          flock.config.thermalCircleRadius);

        if (distance > 1.0f) {
            member.targetVelocity = glm::normalize(toTarget) * flock.config.maxSpeed * 0.5f;
        }

        // Add circular motion
        glm::vec3 toCenter = flock.thermalCenter - member.position;
        toCenter.y = 0;  // Horizontal only
        glm::vec3 tangent = glm::normalize(glm::cross(glm::vec3(0, 1, 0), toCenter));

        member.targetVelocity += tangent * flock.config.maxSpeed;
        member.targetVelocity.y += lift;  // Add lift

        // Maintain separation in thermal
        glm::vec3 sep = calculateSeparation(member, flock);
        member.targetVelocity += sep * flock.config.separationWeight;

        limitVelocity(member.targetVelocity, flock.config.minSpeed, flock.config.maxSpeed);
        index++;
    }

    // Update flock altitude
    flock.currentAltitude = flock.centroid.y;
}

glm::vec3 FlockingBehavior::calculateThermalPosition(const Flock& flock, int memberIndex) {
    int totalMembers = flock.members.size();
    float angle = (static_cast<float>(memberIndex) / totalMembers) * 2.0f * 3.14159f;
    float radius = flock.config.thermalCircleRadius;

    // Stagger altitude slightly
    float altitudeOffset = memberIndex * flock.config.thermalSpacing * 0.5f;

    return flock.thermalCenter +
           glm::vec3(std::cos(angle) * radius, altitudeOffset, std::sin(angle) * radius);
}

float FlockingBehavior::calculateThermalLift(const glm::vec3& position,
                                              const glm::vec3& thermalCenter,
                                              float radius) {
    float horizontalDist = glm::length(glm::vec2(position.x - thermalCenter.x,
                                                  position.z - thermalCenter.z));

    // Thermal has strong core and weaker edges
    float normalizedDist = horizontalDist / radius;
    if (normalizedDist > 1.5f) return 0.0f;

    // Bell curve lift profile
    float lift = std::exp(-normalizedDist * normalizedDist) * 3.0f;
    return lift;
}

// =============================================================================
// HUNTING PACK
// =============================================================================

void FlockingBehavior::updateHuntingPack(Flock& flock, float deltaTime) {
    // Hunting packs coordinate to surround prey
    // This is a simplified version - spread around goal position

    if (flock.members.empty()) return;

    int index = 0;
    float packRadius = 20.0f;

    for (auto& member : flock.members) {
        // Position around goal
        float angle = (static_cast<float>(index) / flock.members.size()) * 2.0f * 3.14159f;
        glm::vec3 targetPos = flock.goalPosition +
                              glm::vec3(std::cos(angle) * packRadius, 0, std::sin(angle) * packRadius);

        glm::vec3 toTarget = targetPos - member.position;

        // Move toward position while maintaining formation
        member.targetVelocity = glm::normalize(toTarget) *
                                std::min(glm::length(toTarget), flock.config.maxSpeed);

        // Avoid other pack members
        glm::vec3 sep = calculateSeparation(member, flock);
        member.targetVelocity += sep * flock.config.separationWeight * 2.0f;

        limitVelocity(member.targetVelocity, flock.config.minSpeed, flock.config.maxSpeed);
        index++;
    }
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void FlockingBehavior::updateFlockCentroid(Flock& flock) {
    if (flock.members.empty()) {
        flock.centroid = glm::vec3(0.0f);
        flock.averageVelocity = glm::vec3(0.0f);
        return;
    }

    flock.centroid = glm::vec3(0.0f);
    flock.averageVelocity = glm::vec3(0.0f);

    for (const auto& member : flock.members) {
        flock.centroid += member.position;
        flock.averageVelocity += member.velocity;
    }

    float count = static_cast<float>(flock.members.size());
    flock.centroid /= count;
    flock.averageVelocity /= count;
}

void FlockingBehavior::limitVelocity(glm::vec3& velocity, float minSpeed, float maxSpeed) {
    float speed = glm::length(velocity);
    if (speed < 0.001f) {
        velocity = glm::vec3(1, 0, 0) * minSpeed;  // Default direction
    } else if (speed < minSpeed) {
        velocity = glm::normalize(velocity) * minSpeed;
    } else if (speed > maxSpeed) {
        velocity = glm::normalize(velocity) * maxSpeed;
    }
}

void FlockingBehavior::smoothVelocityChange(FlockMember& member, float maxAccel, float deltaTime) {
    glm::vec3 desiredChange = member.targetVelocity - member.velocity;
    float changeLength = glm::length(desiredChange);

    float maxChange = maxAccel * deltaTime;
    if (changeLength > maxChange) {
        desiredChange = glm::normalize(desiredChange) * maxChange;
    }

    member.velocity += desiredChange;
}

glm::vec3 FlockingBehavior::getTargetVelocity(uint32_t flockId, uint32_t creatureId) const {
    auto it = m_flocks.find(flockId);
    if (it == m_flocks.end()) return glm::vec3(0.0f);

    auto indexIt = it->second.memberIndex.find(creatureId);
    if (indexIt == it->second.memberIndex.end()) return glm::vec3(0.0f);

    return it->second.members[indexIt->second].targetVelocity;
}

void FlockingBehavior::setFlockGoal(uint32_t flockId, const glm::vec3& goal) {
    auto it = m_flocks.find(flockId);
    if (it != m_flocks.end()) {
        it->second.goalPosition = goal;
    }
}

void FlockingBehavior::setThermalCenter(uint32_t flockId, const glm::vec3& center) {
    auto it = m_flocks.find(flockId);
    if (it != m_flocks.end()) {
        it->second.thermalCenter = center;
    }
}

bool FlockingBehavior::isCreatureInFlock(uint32_t creatureId) const {
    return m_creatureToFlock.find(creatureId) != m_creatureToFlock.end();
}

uint32_t FlockingBehavior::findFlockForCreature(uint32_t creatureId) const {
    auto it = m_creatureToFlock.find(creatureId);
    return (it != m_creatureToFlock.end()) ? it->second : 0;
}

FlockingBehavior::FlockStats FlockingBehavior::getStats() const {
    FlockStats stats;
    stats.totalFlocks = m_flocks.size();
    stats.totalMembers = 0;
    stats.averageSpeed = 0.0f;
    stats.overallCentroid = glm::vec3(0.0f);

    for (const auto& pair : m_flocks) {
        stats.totalMembers += pair.second.members.size();
        stats.overallCentroid += pair.second.centroid;
        stats.averageSpeed += glm::length(pair.second.averageVelocity);
    }

    if (stats.totalFlocks > 0) {
        stats.overallCentroid /= static_cast<float>(stats.totalFlocks);
        stats.averageSpeed /= static_cast<float>(stats.totalFlocks);
        stats.averageFlockSize = static_cast<float>(stats.totalMembers) / stats.totalFlocks;
    } else {
        stats.averageFlockSize = 0.0f;
    }

    return stats;
}

// =============================================================================
// MIGRATION MANAGER
// =============================================================================

MigrationManager::MigrationManager(FlockingBehavior* flockingSystem)
    : m_flockingSystem(flockingSystem) {}

uint32_t MigrationManager::createRoute(const std::vector<glm::vec3>& waypoints, bool circular) {
    uint32_t id = m_nextRouteId++;
    MigrationRoute route;
    route.routeId = id;
    route.waypoints = waypoints;
    route.isCircular = circular;
    m_routes[id] = route;
    return id;
}

void MigrationManager::assignFlockToRoute(uint32_t flockId, uint32_t routeId) {
    if (m_routes.find(routeId) == m_routes.end()) return;

    m_flockToRoute[flockId] = routeId;

    // Set initial goal
    Flock* flock = m_flockingSystem->getFlock(flockId);
    if (flock) {
        m_flockingSystem->setFlockGoal(flockId, m_routes[routeId].getCurrentWaypoint());
    }
}

void MigrationManager::update(float deltaTime) {
    for (auto& pair : m_flockToRoute) {
        uint32_t flockId = pair.first;
        uint32_t routeId = pair.second;

        auto routeIt = m_routes.find(routeId);
        if (routeIt == m_routes.end()) continue;

        MigrationRoute& route = routeIt->second;
        Flock* flock = m_flockingSystem->getFlock(flockId);
        if (!flock) continue;

        // Check if flock reached current waypoint
        float distToWaypoint = glm::length(flock->centroid - route.getCurrentWaypoint());
        if (distToWaypoint < route.waypointRadius) {
            route.advanceWaypoint();
            m_flockingSystem->setFlockGoal(flockId, route.getCurrentWaypoint());
        }
    }
}

bool MigrationManager::isFlockMigrating(uint32_t flockId) const {
    return m_flockToRoute.find(flockId) != m_flockToRoute.end();
}

float MigrationManager::getMigrationProgress(uint32_t flockId) const {
    auto it = m_flockToRoute.find(flockId);
    if (it == m_flockToRoute.end()) return 0.0f;

    auto routeIt = m_routes.find(it->second);
    if (routeIt == m_routes.end()) return 0.0f;

    const MigrationRoute& route = routeIt->second;
    if (route.waypoints.empty()) return 0.0f;

    return static_cast<float>(route.currentWaypoint) / route.waypoints.size();
}

// =============================================================================
// FLOCK PRESETS
// =============================================================================

namespace FlockPresets {

FlockingConfig smallBirdFlock() {
    FlockingConfig config;
    config.type = FlockType::BOIDS;
    config.separationRadius = 1.5f;
    config.alignmentRadius = 8.0f;
    config.cohesionRadius = 12.0f;
    config.separationWeight = 1.5f;
    config.alignmentWeight = 1.2f;
    config.cohesionWeight = 1.0f;
    config.maxSpeed = 12.0f;
    config.minSpeed = 6.0f;
    config.maxAcceleration = 8.0f;
    return config;
}

FlockingConfig geeseMigration() {
    FlockingConfig config;
    config.type = FlockType::V_FORMATION;
    config.formationSpacing = 4.0f;
    config.vAngle = 70.0f;
    config.followDistance = 3.5f;
    config.leaderRotationTime = 45.0f;
    config.maxSpeed = 18.0f;
    config.minSpeed = 12.0f;
    config.maxAcceleration = 3.0f;
    config.goalStrength = 0.5f;
    return config;
}

FlockingConfig starlingMurmuration() {
    FlockingConfig config;
    config.type = FlockType::MURMURATION;
    config.topologicalNeighbors = 7;
    config.synchronizationStrength = 0.6f;
    config.waveSpeed = 3.0f;
    config.maxTurnAngle = 150.0f;
    config.separationRadius = 1.0f;
    config.maxSpeed = 15.0f;
    config.minSpeed = 8.0f;
    config.maxAcceleration = 12.0f;  // High for tight turns
    return config;
}

FlockingConfig vultureThermal() {
    FlockingConfig config;
    config.type = FlockType::THERMAL_CIRCLE;
    config.thermalCircleRadius = 20.0f;
    config.thermalSpacing = 8.0f;
    config.climbRate = 3.0f;
    config.separationRadius = 5.0f;
    config.separationWeight = 2.0f;
    config.maxSpeed = 20.0f;
    config.minSpeed = 10.0f;
    config.maxAcceleration = 2.0f;  // Gentle turns for soaring
    return config;
}

FlockingConfig crowMurder() {
    FlockingConfig config;
    config.type = FlockType::BOIDS;
    config.separationRadius = 2.0f;
    config.alignmentRadius = 10.0f;
    config.cohesionRadius = 20.0f;
    config.separationWeight = 1.3f;
    config.alignmentWeight = 0.8f;
    config.cohesionWeight = 1.2f;
    config.maxSpeed = 14.0f;
    config.minSpeed = 7.0f;
    config.maxAcceleration = 6.0f;
    return config;
}

FlockingConfig seabirdGathering() {
    FlockingConfig config;
    config.type = FlockType::BOIDS;
    config.separationRadius = 3.0f;
    config.alignmentRadius = 15.0f;
    config.cohesionRadius = 25.0f;
    config.separationWeight = 1.0f;
    config.alignmentWeight = 0.6f;
    config.cohesionWeight = 1.5f;
    config.maxSpeed = 16.0f;
    config.minSpeed = 10.0f;
    config.maxAcceleration = 4.0f;
    config.goalStrength = 0.8f;  // Strong attraction to food
    return config;
}

FlockingConfig swallowHunt() {
    FlockingConfig config;
    config.type = FlockType::BOIDS;
    config.separationRadius = 1.0f;
    config.alignmentRadius = 6.0f;
    config.cohesionRadius = 10.0f;
    config.separationWeight = 2.0f;
    config.alignmentWeight = 0.5f;
    config.cohesionWeight = 0.8f;
    config.maxSpeed = 20.0f;
    config.minSpeed = 12.0f;
    config.maxAcceleration = 15.0f;  // Very maneuverable
    return config;
}

FlockingConfig pelicanFishing() {
    FlockingConfig config;
    config.type = FlockType::V_FORMATION;  // Line formation for fishing
    config.formationSpacing = 3.0f;
    config.vAngle = 170.0f;  // Almost a line
    config.followDistance = 2.0f;
    config.leaderRotationTime = 120.0f;
    config.maxSpeed = 12.0f;
    config.minSpeed = 6.0f;
    config.maxAcceleration = 3.0f;
    return config;
}

} // namespace FlockPresets
