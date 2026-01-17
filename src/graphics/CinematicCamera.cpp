#include "CinematicCamera.h"
#include "../entities/Creature.h"
#include "../environment/Terrain.h"
#include <algorithm>
#include <cmath>

namespace Forge {

// ============================================================================
// CameraStateSnapshot
// ============================================================================

CameraStateSnapshot CameraStateSnapshot::lerp(const CameraStateSnapshot& a,
                                               const CameraStateSnapshot& b,
                                               float t) {
    CameraStateSnapshot result;
    result.position = glm::mix(a.position, b.position, t);
    result.target = glm::mix(a.target, b.target, t);
    result.fov = glm::mix(a.fov, b.fov, t);
    result.roll = glm::mix(a.roll, b.roll, t);
    return result;
}

// ============================================================================
// Constructor
// ============================================================================

CinematicCamera::CinematicCamera() {
    std::random_device rd;
    m_rng = std::mt19937(rd());

    m_currentState.fov = m_config.defaultFOV;
    m_targetState.fov = m_config.defaultFOV;
}

// ============================================================================
// Initialization
// ============================================================================

void CinematicCamera::init(Camera* camera, CameraController* controller) {
    m_camera = camera;
    m_controller = controller;

    if (m_camera) {
        m_currentState.position = m_camera->Position;
        m_currentState.target = m_camera->Position + m_camera->Front * 50.0f;
        m_currentState.fov = m_camera->Zoom;
        m_currentState.roll = 0.0f;

        m_targetState = m_currentState;
    }
}

// ============================================================================
// Mode Control
// ============================================================================

void CinematicCamera::setMode(CinematicMode mode, bool smooth) {
    if (mode == m_mode) return;

    m_previousMode = m_mode;
    m_mode = mode;

    // Reset mode-specific state
    switch (mode) {
        case CinematicMode::SLOW_ORBIT:
            if (m_currentTarget) {
                m_orbitCenter = m_currentTarget->getPosition();
            }
            m_orbitAngle = 0.0f;
            break;

        case CinematicMode::GLIDE:
            m_currentWaypointIndex = 0;
            m_waypointProgress = 0.0f;
            if (m_glideWaypoints.empty()) {
                // Generate default path if none set
                generateAutoGlidePath(500.0f, 500.0f, 6);
            }
            break;

        case CinematicMode::FOLLOW_TARGET:
            if (!m_currentTarget && !m_potentialTargets.empty()) {
                m_currentTarget = selectTarget();
            }
            break;

        case CinematicMode::AUTO_DIRECTOR:
            m_shotTimer = 0.0f;
            m_currentShotDuration = m_config.minShotDuration +
                static_cast<float>(m_rng() % 1000) / 1000.0f *
                (m_config.maxShotDuration - m_config.minShotDuration);
            break;

        default:
            break;
    }

    // Start smooth transition if requested
    if (smooth && mode != CinematicMode::DISABLED) {
        CameraStateSnapshot target = m_targetState;
        startTransition(target, m_config.transitionDuration);
    }

    // Notify callback
    if (m_modeChangedCallback) {
        m_modeChangedCallback(mode);
    }
}

// ============================================================================
// Target Selection
// ============================================================================

void CinematicCamera::setTargetHeuristic(TargetSelectionHeuristic heuristic) {
    m_heuristic = heuristic;

    // If not locked, select new target
    if (!m_targetLocked) {
        const Creature* newTarget = selectTarget();
        if (newTarget != m_currentTarget) {
            m_currentTarget = newTarget;
            if (m_targetChangedCallback) {
                m_targetChangedCallback(newTarget);
            }
        }
    }
}

void CinematicCamera::lockTarget(const Creature* creature) {
    m_currentTarget = creature;
    m_targetLocked = true;
    m_heuristic = TargetSelectionHeuristic::USER_SELECTED;

    if (m_targetChangedCallback) {
        m_targetChangedCallback(creature);
    }
}

void CinematicCamera::unlockTarget() {
    m_targetLocked = false;
    // Will select new target on next update
}

void CinematicCamera::updateCreatureList(const std::vector<const Creature*>& creatures) {
    m_potentialTargets.clear();
    m_potentialTargets.reserve(creatures.size());

    for (const Creature* creature : creatures) {
        if (!creature || !creature->isActive()) continue;

        CinematicTarget target;
        target.creature = creature;
        target.position = creature->getPosition();
        target.size = creature->getSize();
        target.interestScore = calculateInterestScore(creature);
        target.isActive = true;

        m_potentialTargets.push_back(target);
    }

    // Sort by interest score
    std::sort(m_potentialTargets.begin(), m_potentialTargets.end(),
              [](const CinematicTarget& a, const CinematicTarget& b) {
                  return a.interestScore > b.interestScore;
              });

    // Validate current target
    if (m_currentTarget && !m_currentTarget->isActive()) {
        m_currentTarget = nullptr;
        m_targetLocked = false;
    }
}

const Creature* CinematicCamera::selectTarget() {
    if (m_potentialTargets.empty()) return nullptr;

    switch (m_heuristic) {
        case TargetSelectionHeuristic::LARGEST_CREATURE:
            return selectLargestCreature();
        case TargetSelectionHeuristic::NEAREST_ACTION:
            return selectNearestAction();
        case TargetSelectionHeuristic::RANDOM_FOCUS:
            return selectRandomCreature();
        case TargetSelectionHeuristic::MOST_OFFSPRING:
            return selectMostOffspring();
        case TargetSelectionHeuristic::OLDEST_LIVING:
            return selectOldestLiving();
        case TargetSelectionHeuristic::USER_SELECTED:
            return m_currentTarget;  // Keep current
        default:
            return selectLargestCreature();
    }
}

const Creature* CinematicCamera::selectLargestCreature() {
    const Creature* largest = nullptr;
    float maxSize = 0.0f;

    for (const auto& target : m_potentialTargets) {
        if (target.creature && target.creature->isActive() && target.size > maxSize) {
            maxSize = target.size;
            largest = target.creature;
        }
    }

    return largest;
}

const Creature* CinematicCamera::selectNearestAction() {
    // Find creature with highest velocity (most activity)
    const Creature* mostActive = nullptr;
    float maxActivity = 0.0f;

    for (const auto& target : m_potentialTargets) {
        if (!target.creature || !target.creature->isActive()) continue;

        float activity = glm::length(target.creature->getVelocity());

        // Bonus for having nearby creatures (action!)
        for (const auto& other : m_potentialTargets) {
            if (other.creature == target.creature) continue;
            float dist = glm::length(target.position - other.position);
            if (dist < 30.0f) {
                activity += (30.0f - dist) / 30.0f * 2.0f;
            }
        }

        if (activity > maxActivity) {
            maxActivity = activity;
            mostActive = target.creature;
        }
    }

    return mostActive;
}

const Creature* CinematicCamera::selectRandomCreature() {
    if (m_potentialTargets.empty()) return nullptr;

    std::uniform_int_distribution<size_t> dist(0, m_potentialTargets.size() - 1);
    size_t attempts = 0;
    size_t maxAttempts = m_potentialTargets.size();

    while (attempts < maxAttempts) {
        size_t idx = dist(m_rng);
        if (m_potentialTargets[idx].creature &&
            m_potentialTargets[idx].creature->isActive()) {
            return m_potentialTargets[idx].creature;
        }
        attempts++;
    }

    return nullptr;
}

const Creature* CinematicCamera::selectMostOffspring() {
    // Creatures with highest fitness/generation are most successful
    const Creature* best = nullptr;
    float maxFitness = -1.0f;

    for (const auto& target : m_potentialTargets) {
        if (!target.creature || !target.creature->isActive()) continue;

        float fitness = target.creature->getFitness();
        if (fitness > maxFitness) {
            maxFitness = fitness;
            best = target.creature;
        }
    }

    return best;
}

const Creature* CinematicCamera::selectOldestLiving() {
    const Creature* oldest = nullptr;
    float maxAge = 0.0f;

    for (const auto& target : m_potentialTargets) {
        if (!target.creature || !target.creature->isActive()) continue;

        float age = target.creature->getAge();
        if (age > maxAge) {
            maxAge = age;
            oldest = target.creature;
        }
    }

    return oldest;
}

float CinematicCamera::calculateInterestScore(const Creature* creature) {
    if (!creature) return 0.0f;

    float score = 0.0f;

    // Size contributes to visual interest
    score += creature->getSize() * 2.0f;

    // Activity (velocity) is interesting
    score += glm::length(creature->getVelocity()) * 0.5f;

    // Health/energy shows thriving creatures
    score += creature->getEnergy() * 0.01f;

    // Fitness shows evolutionary success
    score += creature->getFitness() * 0.5f;

    return score;
}

// ============================================================================
// Glide Configuration
// ============================================================================

void CinematicCamera::setGlidePath(const std::vector<GlideWaypoint>& waypoints) {
    m_glideWaypoints = waypoints;
    m_currentWaypointIndex = 0;
    m_waypointProgress = 0.0f;
}

void CinematicCamera::addGlideWaypoint(const GlideWaypoint& waypoint) {
    m_glideWaypoints.push_back(waypoint);
}

void CinematicCamera::clearGlidePath() {
    m_glideWaypoints.clear();
    m_currentWaypointIndex = 0;
    m_waypointProgress = 0.0f;
}

void CinematicCamera::generateAutoGlidePath(float worldWidth, float worldHeight, int numWaypoints) {
    m_glideWaypoints.clear();

    float halfW = worldWidth * 0.4f;
    float halfH = worldHeight * 0.4f;
    float baseHeight = 40.0f;
    float centerHeight = 80.0f;

    for (int i = 0; i < numWaypoints; ++i) {
        float angle = static_cast<float>(i) / numWaypoints * glm::two_pi<float>();
        float nextAngle = static_cast<float>(i + 1) / numWaypoints * glm::two_pi<float>();

        // Vary the radius slightly for organic feel
        float radiusVariation = 0.8f + 0.4f * std::sin(angle * 3.0f);

        GlideWaypoint wp;
        wp.position = glm::vec3(
            std::cos(angle) * halfW * radiusVariation,
            baseHeight + std::sin(angle * 2.0f) * 15.0f,
            std::sin(angle) * halfH * radiusVariation
        );

        // Look ahead on the path
        wp.lookTarget = glm::vec3(
            std::cos(nextAngle) * halfW * 0.3f,
            5.0f,
            std::sin(nextAngle) * halfH * 0.3f
        );

        // Vary FOV for cinematic effect
        wp.fov = m_config.defaultFOV + std::sin(angle * 2.0f) * 10.0f;
        wp.duration = 6.0f + std::sin(angle) * 2.0f;

        m_glideWaypoints.push_back(wp);
    }
}

// ============================================================================
// Orbit Configuration
// ============================================================================

void CinematicCamera::setOrbitCenter(const glm::vec3& center) {
    m_orbitCenter = center;
}

// ============================================================================
// Photo Mode
// ============================================================================

void CinematicCamera::enablePhotoMode(bool enable) {
    m_photoModeEnabled = enable;

    if (enable) {
        // Capture current camera state for manual control
        m_photoYaw = m_camera ? m_camera->Yaw : 0.0f;
        m_photoPitch = m_camera ? m_camera->Pitch : 0.0f;
        m_photoZoom = 1.0f;
    }
}

void CinematicCamera::photoModeRotate(float yaw, float pitch) {
    if (!m_photoModeEnabled) return;

    m_photoYaw += yaw * 0.2f;
    m_photoPitch = glm::clamp(m_photoPitch + pitch * 0.2f, -85.0f, 85.0f);
}

void CinematicCamera::photoModeZoom(float delta) {
    if (!m_photoModeEnabled) return;

    m_photoZoom = glm::clamp(m_photoZoom + delta * 0.1f, 0.5f, 2.0f);
}

void CinematicCamera::photoModePan(float x, float y) {
    if (!m_photoModeEnabled || !m_camera) return;

    glm::vec3 right = m_camera->Right * x * 0.5f;
    glm::vec3 up = m_camera->Up * y * 0.5f;

    m_currentState.position += right + up;
    m_currentState.target += right + up;
}

// ============================================================================
// Main Update
// ============================================================================

void CinematicCamera::update(float deltaTime) {
    if (!m_camera) return;

    // Handle photo mode separately
    if (m_photoModeEnabled) {
        updatePhotoMode(deltaTime);
        applyStateToCamera();
        return;
    }

    // Handle transition
    if (m_inTransition) {
        updateTransition(deltaTime);
        applyStateToCamera();
        return;
    }

    // Update based on mode
    switch (m_mode) {
        case CinematicMode::DISABLED:
            // Let CameraController handle it
            return;

        case CinematicMode::SLOW_ORBIT:
            updateSlowOrbit(deltaTime);
            break;

        case CinematicMode::GLIDE:
            updateGlide(deltaTime);
            break;

        case CinematicMode::FOLLOW_TARGET:
            updateFollowTarget(deltaTime);
            break;

        case CinematicMode::AUTO_DIRECTOR:
            updateAutoDirector(deltaTime);
            break;
    }

    // Apply smoothing to reach target state
    m_currentState.position = smoothDampVec3(m_currentState.position, m_targetState.position,
                                              m_positionVelocity, m_config.positionSmoothTime, deltaTime);
    m_currentState.target = smoothDampVec3(m_currentState.target, m_targetState.target,
                                            m_targetVelocity, m_config.rotationSmoothTime, deltaTime);
    m_currentState.fov = smoothDampFloat(m_currentState.fov, m_targetState.fov,
                                          m_fovVelocity, m_config.fovSmoothTime, deltaTime);
    m_currentState.roll = smoothDampFloat(m_currentState.roll, m_targetState.roll,
                                           m_rollVelocity, m_config.positionSmoothTime, deltaTime);

    // Apply collision avoidance
    m_currentState.position = applyCollisionAvoidance(m_currentState.position);

    // Apply to camera
    applyStateToCamera();
}

// ============================================================================
// Mode Update Methods
// ============================================================================

void CinematicCamera::updateSlowOrbit(float deltaTime) {
    // Update orbit center if following a target
    if (m_currentTarget && m_currentTarget->isActive()) {
        glm::vec3 targetPos = m_currentTarget->getPosition();
        m_orbitCenter = glm::mix(m_orbitCenter, targetPos, deltaTime * 2.0f);
    }

    // Slowly advance orbit angle
    m_orbitAngle += m_config.orbitSpeed * deltaTime;
    if (m_orbitAngle > glm::two_pi<float>()) {
        m_orbitAngle -= glm::two_pi<float>();
    }

    // Calculate orbit position
    float x = std::cos(m_orbitAngle) * m_orbitRadius;
    float z = std::sin(m_orbitAngle) * m_orbitRadius;

    m_targetState.position = m_orbitCenter + glm::vec3(x, m_orbitHeight, z);
    m_targetState.target = m_orbitCenter + glm::vec3(0.0f, 3.0f, 0.0f);

    // Subtle roll based on orbit direction
    m_targetState.roll = std::sin(m_orbitAngle) * m_config.maxRollAngle;

    // Vary FOV slightly
    m_targetState.fov = m_config.defaultFOV + std::sin(m_orbitAngle * 0.5f) * 5.0f;
}

void CinematicCamera::updateGlide(float deltaTime) {
    if (m_glideWaypoints.empty()) return;

    // Get current and next waypoints
    const GlideWaypoint& current = m_glideWaypoints[m_currentWaypointIndex];
    const GlideWaypoint& next = m_glideWaypoints[(m_currentWaypointIndex + 1) % m_glideWaypoints.size()];

    // Advance progress
    m_waypointProgress += deltaTime / current.duration;

    if (m_waypointProgress >= 1.0f) {
        m_waypointProgress -= 1.0f;
        m_currentWaypointIndex = (m_currentWaypointIndex + 1) % m_glideWaypoints.size();
    }

    // Smooth interpolation with easing
    float t = easeInOutSine(m_waypointProgress);

    m_targetState.position = glm::mix(current.position, next.position, t);
    m_targetState.target = glm::mix(current.lookTarget, next.lookTarget, t);
    m_targetState.fov = glm::mix(current.fov, next.fov, t);

    // Add subtle roll based on turn direction
    glm::vec3 toNext = next.position - current.position;
    float turnAngle = std::atan2(toNext.x, toNext.z);
    m_targetState.roll = std::sin(turnAngle + m_waypointProgress * glm::pi<float>()) * m_config.maxRollAngle;
}

void CinematicCamera::updateFollowTarget(float deltaTime) {
    if (!m_currentTarget || !m_currentTarget->isActive()) {
        // Select new target
        if (!m_targetLocked) {
            m_currentTarget = selectTarget();
            if (m_targetChangedCallback && m_currentTarget) {
                m_targetChangedCallback(m_currentTarget);
            }
        }

        if (!m_currentTarget) {
            // No valid target, fall back to glide
            setMode(CinematicMode::GLIDE, true);
            return;
        }
    }

    glm::vec3 creaturePos = m_currentTarget->getPosition();
    glm::vec3 creatureVel = m_currentTarget->getVelocity();

    // Get creature facing direction
    glm::vec3 creatureDir;
    if (glm::length(creatureVel) > 0.5f) {
        creatureDir = glm::normalize(creatureVel);
    } else {
        // Use camera's current relative position
        creatureDir = glm::normalize(m_currentState.position - creaturePos);
        creatureDir.y = 0.0f;
        if (glm::length(creatureDir) < 0.1f) {
            creatureDir = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        creatureDir = glm::normalize(creatureDir);
    }

    // Position camera behind and above creature
    glm::vec3 behindOffset = -creatureDir * m_config.followDistance;
    behindOffset.y = m_config.followHeight;

    m_targetState.position = creaturePos + behindOffset;
    m_targetState.target = creaturePos + glm::vec3(0.0f, m_currentTarget->getSize() * 0.5f, 0.0f);

    // Adjust FOV based on creature speed
    float speed = glm::length(creatureVel);
    m_targetState.fov = m_config.defaultFOV - std::min(speed * 0.5f, 10.0f);

    // Subtle roll when creature turns
    float turnRate = creatureVel.x * creatureDir.z - creatureVel.z * creatureDir.x;
    m_targetState.roll = glm::clamp(turnRate * 2.0f, -m_config.maxRollAngle, m_config.maxRollAngle);
}

void CinematicCamera::updateAutoDirector(float deltaTime) {
    m_shotTimer += deltaTime;

    // Check if it's time to switch shots
    if (m_shotTimer >= m_currentShotDuration) {
        m_shotTimer = 0.0f;

        // Randomize next shot duration
        std::uniform_real_distribution<float> dist(m_config.minShotDuration, m_config.maxShotDuration);
        m_currentShotDuration = dist(m_rng);

        // Pick a random mode (excluding DISABLED and AUTO_DIRECTOR)
        std::uniform_int_distribution<int> modeDist(0, 2);
        int modeChoice = modeDist(m_rng);

        switch (modeChoice) {
            case 0:
                // Switch to slow orbit
                if (m_currentTarget && m_currentTarget->isActive()) {
                    m_orbitCenter = m_currentTarget->getPosition();
                }
                updateSlowOrbit(deltaTime);
                break;

            case 1:
                // Switch target for follow mode
                if (!m_targetLocked) {
                    // Pick a different target
                    const Creature* newTarget = selectRandomCreature();
                    if (newTarget && newTarget != m_currentTarget) {
                        m_currentTarget = newTarget;
                        if (m_targetChangedCallback) {
                            m_targetChangedCallback(newTarget);
                        }
                    }
                }
                updateFollowTarget(deltaTime);
                break;

            case 2:
            default:
                updateGlide(deltaTime);
                break;
        }

        // Start transition to new shot
        startTransition(m_targetState, m_config.transitionDuration);
    } else {
        // Continue current shot type based on what we're doing
        if (m_currentTarget && m_currentTarget->isActive()) {
            updateFollowTarget(deltaTime);
        } else {
            updateSlowOrbit(deltaTime);
        }
    }
}

void CinematicCamera::updateTransition(float deltaTime) {
    m_transitionProgress += deltaTime / m_config.transitionDuration;

    if (m_transitionProgress >= 1.0f) {
        m_transitionProgress = 1.0f;
        m_inTransition = false;
        m_currentState = m_transitionEnd;
    } else {
        float t = easeInOutCubic(m_transitionProgress);
        m_currentState = CameraStateSnapshot::lerp(m_transitionStart, m_transitionEnd, t);
    }
}

void CinematicCamera::updatePhotoMode(float deltaTime) {
    // In photo mode, camera is frozen but can be manually adjusted
    // Build view direction from yaw/pitch
    glm::vec3 direction;
    direction.x = std::cos(glm::radians(m_photoPitch)) * std::cos(glm::radians(m_photoYaw));
    direction.y = std::sin(glm::radians(m_photoPitch));
    direction.z = std::cos(glm::radians(m_photoPitch)) * std::sin(glm::radians(m_photoYaw));
    direction = glm::normalize(direction);

    m_currentState.target = m_currentState.position + direction * 50.0f;
    m_currentState.fov = m_config.defaultFOV / m_photoZoom;
}

// ============================================================================
// Collision Avoidance
// ============================================================================

glm::vec3 CinematicCamera::applyCollisionAvoidance(const glm::vec3& desiredPosition) {
    glm::vec3 safePosition = desiredPosition;

    // Get terrain height at this position
    float terrainHeight = getTerrainHeight(desiredPosition.x, desiredPosition.z);

    // Ensure minimum clearance above terrain
    float minHeight = terrainHeight + m_config.minTerrainClearance;
    if (safePosition.y < minHeight) {
        safePosition.y = minHeight;
    }

    // Ensure minimum clearance above water
    float minWaterHeight = m_waterLevel + m_config.minWaterClearance;
    if (safePosition.y < minWaterHeight) {
        safePosition.y = minWaterHeight;
    }

    return safePosition;
}

float CinematicCamera::getTerrainHeight(float x, float z) const {
    if (m_terrain) {
        return m_terrain->getHeight(x, z);
    }
    return 0.0f;
}

bool CinematicCamera::isPositionSafe(const glm::vec3& position) const {
    float terrainHeight = getTerrainHeight(position.x, position.z);
    return position.y >= terrainHeight + m_config.minTerrainClearance &&
           position.y >= m_waterLevel + m_config.minWaterClearance;
}

// ============================================================================
// Smoothing Helpers (Critically Damped Spring)
// ============================================================================

float CinematicCamera::smoothDampFloat(float current, float target, float& velocity,
                                         float smoothTime, float deltaTime) {
    // Based on Unity's SmoothDamp implementation
    smoothTime = std::max(0.0001f, smoothTime);
    float omega = 2.0f / smoothTime;
    float x = omega * deltaTime;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

    float change = current - target;
    float temp = (velocity + omega * change) * deltaTime;
    velocity = (velocity - omega * temp) * exp;

    float result = target + (change + temp) * exp;

    // Prevent overshooting
    if ((target - current > 0.0f) == (result > target)) {
        result = target;
        velocity = 0.0f;
    }

    return result;
}

glm::vec3 CinematicCamera::smoothDampVec3(const glm::vec3& current, const glm::vec3& target,
                                            glm::vec3& velocity, float smoothTime, float deltaTime) {
    return glm::vec3(
        smoothDampFloat(current.x, target.x, velocity.x, smoothTime, deltaTime),
        smoothDampFloat(current.y, target.y, velocity.y, smoothTime, deltaTime),
        smoothDampFloat(current.z, target.z, velocity.z, smoothTime, deltaTime)
    );
}

// ============================================================================
// Easing Functions
// ============================================================================

float CinematicCamera::easeInOutCubic(float t) {
    return t < 0.5f
        ? 4.0f * t * t * t
        : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

float CinematicCamera::easeOutQuad(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

float CinematicCamera::easeInOutSine(float t) {
    return -(std::cos(glm::pi<float>() * t) - 1.0f) / 2.0f;
}

// ============================================================================
// State Application
// ============================================================================

void CinematicCamera::applyStateToCamera() {
    if (!m_camera) return;

    m_camera->Position = m_currentState.position;
    m_camera->Zoom = m_currentState.fov;

    // Calculate view direction
    glm::vec3 lookDir = glm::normalize(m_currentState.target - m_currentState.position);

    // Apply roll to the up vector
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(lookDir, worldUp));

    // Rotate up vector around look direction by roll amount
    float rollRad = glm::radians(m_currentState.roll);
    glm::vec3 rolledUp = worldUp * std::cos(rollRad) + right * std::sin(rollRad);

    m_camera->Front = lookDir;
    m_camera->Right = glm::normalize(glm::cross(lookDir, rolledUp));
    m_camera->Up = glm::normalize(glm::cross(m_camera->Right, lookDir));

    // Update yaw/pitch for consistency
    m_camera->Yaw = glm::degrees(std::atan2(lookDir.z, lookDir.x));
    m_camera->Pitch = glm::degrees(std::asin(glm::clamp(lookDir.y, -1.0f, 1.0f)));
}

void CinematicCamera::startTransition(const CameraStateSnapshot& target, float duration) {
    m_transitionStart = m_currentState;
    m_transitionEnd = target;
    m_transitionProgress = 0.0f;
    m_inTransition = true;
}

} // namespace Forge
