#include "CameraController.h"
#include "../entities/Creature.h"
#include "../environment/Terrain.h"
#include <cmath>
#include <algorithm>
#include <chrono>

namespace Forge {

// ============================================================================
// Constructor
// ============================================================================

CameraController::CameraController() {
    m_transition.active = false;
    m_transition.easing = easeInOutCubic;

    // Initialize random number generator with time-based seed
    auto seed = static_cast<unsigned int>(
        std::chrono::steady_clock::now().time_since_epoch().count()
    );
    m_rng.seed(seed);

    // Initialize cinematic FOV
    m_cinematicFOV = m_cinematicConfig.baseFOV;
    m_targetCinematicFOV = m_cinematicConfig.baseFOV;
}

void CameraController::init(Camera* camera, float worldWidth, float worldHeight) {
    m_camera = camera;
    m_worldWidth = worldWidth;
    m_worldHeight = worldHeight;

    // Set default bounds based on world size
    float halfWidth = worldWidth * 0.5f;
    float halfHeight = worldHeight * 0.5f;
    m_boundsMin = glm::vec3(-halfWidth, 0.0f, -halfHeight);
    m_boundsMax = glm::vec3(halfWidth, 200.0f, halfHeight);
}

// ============================================================================
// Mode Control
// ============================================================================

void CameraController::setMode(CameraMode mode, bool smooth) {
    if (mode == m_mode) return;

    CameraMode oldMode = m_mode;
    m_mode = mode;

    if (!m_camera) return;

    // Exit photo mode if switching to another mode
    if (oldMode == CameraMode::PHOTO_MODE && mode != CameraMode::PHOTO_MODE) {
        m_photoMode.active = false;
    }

    // Set up initial state for new mode
    glm::vec3 targetPos = m_camera->Position;
    glm::vec3 targetLook = m_camera->Position + m_camera->Front * 10.0f;

    switch (mode) {
        case CameraMode::FOLLOW:
            if (m_followTarget) {
                glm::vec3 creaturePos = m_followTarget->getPosition();
                glm::vec3 creatureDir = m_followTarget->getVelocity();
                if (glm::length(creatureDir) < 0.1f) {
                    creatureDir = glm::vec3(0.0f, 0.0f, 1.0f);
                }
                creatureDir = glm::normalize(creatureDir);

                targetPos = creaturePos - creatureDir * m_followDistance +
                           glm::vec3(0.0f, m_followHeight, 0.0f);
                targetLook = creaturePos;
            }
            break;

        case CameraMode::ORBIT:
            targetPos = m_orbitCenter +
                       glm::vec3(cos(m_orbitAngle) * m_orbitRadius,
                                m_orbitHeight,
                                sin(m_orbitAngle) * m_orbitRadius);
            targetLook = m_orbitCenter;
            break;

        case CameraMode::OVERVIEW:
            targetPos = glm::vec3(0.0f, m_flyingHeight * 2.0f, 0.0f);
            targetLook = glm::vec3(0.0f, 0.0f, 0.0f);
            break;

        case CameraMode::FLYING:
            targetPos.y = std::max(targetPos.y, m_flyingHeight);
            break;

        // Phase 7: New cinematic modes
        case CameraMode::CINEMATIC_SLOW_ORBIT:
            // Will be properly set up by startSlowOrbit()
            m_targetCinematicFOV = m_cinematicConfig.cinematicFOV;
            break;

        case CameraMode::CINEMATIC_GLIDE:
            // Will be properly set up by startGlide()
            m_glideProgress = 0.0f;
            m_targetCinematicFOV = m_cinematicConfig.cinematicFOV;
            break;

        case CameraMode::CINEMATIC_FOLLOW_TARGET:
            // Will be properly set up by startFollowTarget()
            m_targetCinematicFOV = m_cinematicConfig.baseFOV;
            break;

        case CameraMode::PHOTO_MODE:
            // Will be properly set up by enterPhotoMode()
            break;

        default:
            // Reset to base FOV for non-cinematic modes
            m_targetCinematicFOV = m_cinematicConfig.baseFOV;
            break;
    }

    if (smooth && oldMode != CameraMode::FREE) {
        transitionTo(targetPos, targetLook, 1.0f);
    } else {
        m_camera->Position = targetPos;
        m_targetPoint = targetLook;
    }
}

void CameraController::setFollowTarget(const Creature* creature) {
    m_followTarget = creature;
    if (creature && m_mode == CameraMode::FOLLOW) {
        // Smoothly transition to new target
        glm::vec3 creaturePos = creature->getPosition();
        m_targetPoint = creaturePos;
    }
}

void CameraController::setOrbitTarget(const glm::vec3& target) {
    m_orbitCenter = target;
    m_targetPoint = target;
}

void CameraController::setCinematicPath(const std::vector<CameraKeyframe>& keyframes) {
    m_cinematicKeyframes = keyframes;
    m_cinematicTime = 0.0f;
}

// ============================================================================
// Update
// ============================================================================

void CameraController::update(float deltaTime) {
    if (!m_camera) return;

    // Phase 7: Update target selection for cinematic modes
    if (m_mode == CameraMode::CINEMATIC_FOLLOW_TARGET ||
        m_mode == CameraMode::CINEMATIC_SLOW_ORBIT) {
        updateTargetSelection(deltaTime);
    }

    // Update transition first
    if (m_transition.active) {
        updateTransition(deltaTime);
    } else {
        // Update based on current mode
        switch (m_mode) {
            case CameraMode::FREE:
                updateFreeMode(deltaTime);
                break;
            case CameraMode::FOLLOW:
                updateFollowMode(deltaTime);
                break;
            case CameraMode::ORBIT:
                updateOrbitMode(deltaTime);
                break;
            case CameraMode::UNDERWATER:
                updateUnderwaterMode(deltaTime);
                break;
            case CameraMode::FLYING:
                updateFlyingMode(deltaTime);
                break;
            case CameraMode::CINEMATIC:
                updateCinematicMode(deltaTime);
                break;
            case CameraMode::OVERVIEW:
                updateOverviewMode(deltaTime);
                break;
            case CameraMode::FIRST_PERSON:
                updateFirstPersonMode(deltaTime);
                break;
            // Phase 7: New cinematic presentation modes
            case CameraMode::CINEMATIC_SLOW_ORBIT:
                updateSlowOrbitMode(deltaTime);
                break;
            case CameraMode::CINEMATIC_GLIDE:
                updateGlideMode(deltaTime);
                break;
            case CameraMode::CINEMATIC_FOLLOW_TARGET:
                updateFollowTargetMode(deltaTime);
                break;
            case CameraMode::PHOTO_MODE:
                updatePhotoMode(deltaTime);
                break;
            // Phase 8: Inspect mode
            case CameraMode::INSPECT:
                updateInspectMode(deltaTime);
                break;
        }
    }

    // Phase 7: Update cinematic presentation effects
    updateCinematicEffects(deltaTime);

    // Apply shake
    updateShake(deltaTime);

    // Apply bounds
    if (m_boundsEnabled) {
        applyBounds();
    }

    // Auto-switch to underwater mode if below water
    if (m_mode != CameraMode::UNDERWATER && isUnderwater()) {
        // Don't auto-switch, but apply underwater effects
    }
}

bool CameraController::processKeyboard(CameraMovement direction, float deltaTime) {
    if (!m_camera) return false;

    switch (m_mode) {
        case CameraMode::FREE:
        case CameraMode::FLYING:
            m_camera->processKeyboard(direction, deltaTime);
            return true;

        case CameraMode::FOLLOW:
            // Adjust follow distance with forward/backward
            if (direction == FORWARD) {
                m_followDistance = std::max(5.0f, m_followDistance - 20.0f * deltaTime);
                return true;
            } else if (direction == BACKWARD) {
                m_followDistance = std::min(100.0f, m_followDistance + 20.0f * deltaTime);
                return true;
            }
            break;

        case CameraMode::ORBIT:
            // Adjust orbit radius
            if (direction == FORWARD) {
                m_orbitRadius = std::max(5.0f, m_orbitRadius - 20.0f * deltaTime);
                return true;
            } else if (direction == BACKWARD) {
                m_orbitRadius = std::min(200.0f, m_orbitRadius + 20.0f * deltaTime);
                return true;
            }
            break;

        default:
            break;
    }

    return false;
}

bool CameraController::processMouseMovement(float xOffset, float yOffset) {
    if (!m_camera) return false;

    switch (m_mode) {
        case CameraMode::FREE:
        case CameraMode::FLYING:
            m_camera->processMouseMovement(xOffset, yOffset);
            return true;

        case CameraMode::FOLLOW:
            // Rotate around target
            m_followYaw += xOffset * 0.1f;
            m_followHeight = glm::clamp(m_followHeight - yOffset * 0.2f, 2.0f, 50.0f);
            return true;

        case CameraMode::ORBIT:
            // Rotate orbit
            m_orbitAngle += xOffset * 0.01f;
            m_orbitHeight = glm::clamp(m_orbitHeight - yOffset * 0.2f, 2.0f, 100.0f);
            return true;

        default:
            break;
    }

    return false;
}

bool CameraController::processMouseScroll(float yOffset) {
    if (!m_camera) return false;

    switch (m_mode) {
        case CameraMode::FREE:
        case CameraMode::FLYING:
            m_camera->processMouseScroll(yOffset);
            return true;

        case CameraMode::FOLLOW:
            m_followDistance = glm::clamp(m_followDistance - yOffset * 5.0f, 5.0f, 100.0f);
            return true;

        case CameraMode::ORBIT:
            m_orbitRadius = glm::clamp(m_orbitRadius - yOffset * 5.0f, 5.0f, 200.0f);
            return true;

        case CameraMode::OVERVIEW:
            m_flyingHeight = glm::clamp(m_flyingHeight - yOffset * 10.0f, 20.0f, 500.0f);
            return true;

        default:
            break;
    }

    return false;
}

// ============================================================================
// Mode Updates
// ============================================================================

void CameraController::updateFreeMode(float deltaTime) {
    // Free mode is handled by the base camera
    m_targetPoint = m_camera->Position + m_camera->Front * 10.0f;
}

void CameraController::updateFollowMode(float deltaTime) {
    if (!m_followTarget || !m_followTarget->isActive()) {
        setMode(CameraMode::FREE, true);
        return;
    }

    glm::vec3 creaturePos = m_followTarget->getPosition();
    glm::vec3 creatureVel = m_followTarget->getVelocity();

    // Get direction creature is facing
    glm::vec3 creatureDir;
    if (glm::length(creatureVel) > 0.1f) {
        creatureDir = glm::normalize(creatureVel);
    } else {
        // Use camera's current direction relative to creature
        glm::vec3 toCamera = m_camera->Position - creaturePos;
        toCamera.y = 0.0f;
        if (glm::length(toCamera) > 0.1f) {
            creatureDir = -glm::normalize(toCamera);
        } else {
            creatureDir = glm::vec3(0.0f, 0.0f, 1.0f);
        }
    }

    // Apply user yaw offset
    float angle = atan2(creatureDir.z, creatureDir.x) + glm::radians(m_followYaw);
    creatureDir = glm::vec3(cos(angle), 0.0f, sin(angle));

    // Calculate desired camera position
    glm::vec3 desiredPos = creaturePos - creatureDir * m_followDistance +
                          glm::vec3(0.0f, m_followHeight, 0.0f);

    // Smooth interpolation
    static glm::vec3 velocity(0.0f);
    m_camera->Position = smoothDamp(m_camera->Position, desiredPos, velocity,
                                    1.0f / m_followSmoothing, deltaTime);

    // Look at creature
    m_targetPoint = creaturePos + glm::vec3(0.0f, 2.0f, 0.0f);  // Slightly above
    glm::vec3 lookDir = glm::normalize(m_targetPoint - m_camera->Position);

    // Update camera orientation
    m_camera->Front = lookDir;
    m_camera->Right = glm::normalize(glm::cross(m_camera->WorldUp, m_camera->Front));
    m_camera->Up = glm::normalize(glm::cross(m_camera->Right, m_camera->Front));

    // Update yaw/pitch for consistency
    m_camera->Yaw = glm::degrees(atan2(lookDir.z, lookDir.x));
    m_camera->Pitch = glm::degrees(asin(lookDir.y));
}

void CameraController::updateOrbitMode(float deltaTime) {
    // Auto-rotate
    m_orbitAngle += m_orbitSpeed * deltaTime;

    // Calculate position on orbit circle
    float x = cos(m_orbitAngle) * m_orbitRadius;
    float z = sin(m_orbitAngle) * m_orbitRadius;

    glm::vec3 desiredPos = m_orbitCenter + glm::vec3(x, m_orbitHeight, z);

    // Smooth transition
    static glm::vec3 velocity(0.0f);
    m_camera->Position = smoothDamp(m_camera->Position, desiredPos, velocity, 0.2f, deltaTime);

    // Always look at center
    m_targetPoint = m_orbitCenter;
    glm::vec3 lookDir = glm::normalize(m_targetPoint - m_camera->Position);

    m_camera->Front = lookDir;
    m_camera->Right = glm::normalize(glm::cross(m_camera->WorldUp, m_camera->Front));
    m_camera->Up = glm::normalize(glm::cross(m_camera->Right, m_camera->Front));

    m_camera->Yaw = glm::degrees(atan2(lookDir.z, lookDir.x));
    m_camera->Pitch = glm::degrees(asin(lookDir.y));
}

void CameraController::updateUnderwaterMode(float deltaTime) {
    // Underwater mode is like free mode but with restricted movement
    m_targetPoint = m_camera->Position + m_camera->Front * 10.0f;

    // Add slight bobbing motion
    static float bobTime = 0.0f;
    bobTime += deltaTime;
    float bobOffset = sin(bobTime * 2.0f) * 0.1f;
    m_camera->Position.y += bobOffset * deltaTime;

    // Keep below water level
    if (m_camera->Position.y > m_waterLevel - 1.0f) {
        m_camera->Position.y = m_waterLevel - 1.0f;
    }
}

void CameraController::updateFlyingMode(float deltaTime) {
    // Flying mode is like free mode but at higher altitude
    m_targetPoint = m_camera->Position + m_camera->Front * 10.0f;

    // Enforce minimum height
    if (m_camera->Position.y < m_flyingHeight) {
        m_camera->Position.y = glm::mix(m_camera->Position.y, m_flyingHeight,
                                        deltaTime * 2.0f);
    }
}

void CameraController::updateCinematicMode(float deltaTime) {
    if (!m_cinematicPlaying || m_cinematicPaused) return;
    if (m_cinematicKeyframes.empty()) return;

    m_cinematicTime += deltaTime;

    // Get interpolated keyframe
    CameraKeyframe keyframe = interpolateKeyframes(m_cinematicTime);

    // Apply
    m_camera->Position = keyframe.position;
    m_targetPoint = keyframe.target;
    m_camera->Zoom = keyframe.fov;

    // Update camera vectors
    glm::vec3 lookDir = glm::normalize(m_targetPoint - m_camera->Position);
    m_camera->Front = lookDir;
    m_camera->Right = glm::normalize(glm::cross(m_camera->WorldUp, m_camera->Front));
    m_camera->Up = glm::normalize(glm::cross(m_camera->Right, m_camera->Front));

    // Check if cinematic ended
    if (!m_cinematicKeyframes.empty() &&
        m_cinematicTime >= m_cinematicKeyframes.back().time) {
        m_cinematicPlaying = false;
        setMode(CameraMode::FREE, true);
    }
}

void CameraController::updateOverviewMode(float deltaTime) {
    // Look straight down at world center
    glm::vec3 desiredPos = glm::vec3(0.0f, m_flyingHeight * 2.0f, 0.0f);

    static glm::vec3 velocity(0.0f);
    m_camera->Position = smoothDamp(m_camera->Position, desiredPos, velocity, 0.5f, deltaTime);

    m_targetPoint = glm::vec3(0.0f, 0.0f, 0.0f);

    // Look down
    m_camera->Front = glm::vec3(0.0f, -1.0f, 0.01f);  // Slight forward to avoid gimbal lock
    m_camera->Front = glm::normalize(m_camera->Front);
    m_camera->Right = glm::normalize(glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), m_camera->Front));
    m_camera->Up = glm::normalize(glm::cross(m_camera->Right, m_camera->Front));

    m_camera->Pitch = -89.0f;
    m_camera->Yaw = 0.0f;
}

void CameraController::updateFirstPersonMode(float deltaTime) {
    if (!m_followTarget || !m_followTarget->isActive()) {
        setMode(CameraMode::FREE, true);
        return;
    }

    // Position at creature's "eye" level
    glm::vec3 creaturePos = m_followTarget->getPosition();
    float eyeHeight = m_followTarget->getSize() * 1.5f;

    m_camera->Position = creaturePos + glm::vec3(0.0f, eyeHeight, 0.0f);

    // Look in creature's movement direction
    glm::vec3 vel = m_followTarget->getVelocity();
    if (glm::length(vel) > 0.1f) {
        glm::vec3 lookDir = glm::normalize(vel);
        m_camera->Front = lookDir;
        m_camera->Right = glm::normalize(glm::cross(m_camera->WorldUp, m_camera->Front));
        m_camera->Up = glm::normalize(glm::cross(m_camera->Right, m_camera->Front));

        m_camera->Yaw = glm::degrees(atan2(lookDir.z, lookDir.x));
        m_camera->Pitch = glm::degrees(asin(lookDir.y));
    }

    m_targetPoint = m_camera->Position + m_camera->Front * 10.0f;
}

// ============================================================================
// Underwater
// ============================================================================

float CameraController::getUnderwaterDepth() const {
    if (!m_camera) return 0.0f;
    return std::max(0.0f, m_waterLevel - m_camera->Position.y);
}

// ============================================================================
// Cinematic
// ============================================================================

void CameraController::playCinematic() {
    if (m_cinematicKeyframes.empty()) return;

    m_cinematicPlaying = true;
    m_cinematicPaused = false;
    m_cinematicTime = 0.0f;
    m_mode = CameraMode::CINEMATIC;
}

void CameraController::pauseCinematic() {
    m_cinematicPaused = true;
}

void CameraController::stopCinematic() {
    m_cinematicPlaying = false;
    m_cinematicPaused = false;
    m_cinematicTime = 0.0f;
}

float CameraController::getCinematicProgress() const {
    if (m_cinematicKeyframes.empty()) return 0.0f;
    return m_cinematicTime / m_cinematicKeyframes.back().time;
}

// ============================================================================
// Transitions
// ============================================================================

void CameraController::transitionTo(const glm::vec3& position, const glm::vec3& target,
                                     float duration) {
    if (!m_camera) return;

    m_transition.startPosition = m_camera->Position;
    m_transition.endPosition = position;
    m_transition.startTarget = m_targetPoint;
    m_transition.endTarget = target;
    m_transition.duration = duration;
    m_transition.elapsed = 0.0f;
    m_transition.active = true;
    m_transition.easing = easeInOutCubic;
}

void CameraController::skipTransition() {
    if (!m_transition.active || !m_camera) return;

    m_camera->Position = m_transition.endPosition;
    m_targetPoint = m_transition.endTarget;

    // Update camera orientation
    glm::vec3 lookDir = glm::normalize(m_targetPoint - m_camera->Position);
    m_camera->Front = lookDir;
    m_camera->Right = glm::normalize(glm::cross(m_camera->WorldUp, m_camera->Front));
    m_camera->Up = glm::normalize(glm::cross(m_camera->Right, m_camera->Front));

    m_transition.active = false;
}

void CameraController::updateTransition(float deltaTime) {
    if (!m_transition.active || !m_camera) return;

    m_transition.elapsed += deltaTime;
    float t = std::min(1.0f, m_transition.elapsed / m_transition.duration);
    float eased = m_transition.easing(t);

    // Interpolate position and target
    m_camera->Position = glm::mix(m_transition.startPosition, m_transition.endPosition, eased);
    m_targetPoint = glm::mix(m_transition.startTarget, m_transition.endTarget, eased);

    // Update camera orientation
    glm::vec3 lookDir = glm::normalize(m_targetPoint - m_camera->Position);
    m_camera->Front = lookDir;
    m_camera->Right = glm::normalize(glm::cross(m_camera->WorldUp, m_camera->Front));
    m_camera->Up = glm::normalize(glm::cross(m_camera->Right, m_camera->Front));

    m_camera->Yaw = glm::degrees(atan2(lookDir.z, lookDir.x));
    m_camera->Pitch = glm::degrees(asin(lookDir.y));

    // Check if done
    if (t >= 1.0f) {
        m_transition.active = false;
    }
}

// ============================================================================
// Shake
// ============================================================================

void CameraController::addShake(float intensity, float duration) {
    m_shakeIntensity = std::max(m_shakeIntensity, intensity);
    m_shakeDuration = duration;
    m_shakeTimer = 0.0f;
}

void CameraController::updateShake(float deltaTime) {
    if (m_shakeIntensity <= 0.0f || !m_camera) return;

    m_shakeTimer += deltaTime;

    // Calculate shake falloff
    float falloff = 1.0f - (m_shakeTimer / m_shakeDuration);
    if (falloff <= 0.0f) {
        m_shakeIntensity = 0.0f;
        return;
    }

    // Random offset
    float offsetX = ((std::rand() % 1000) / 500.0f - 1.0f) * m_shakeIntensity * falloff;
    float offsetY = ((std::rand() % 1000) / 500.0f - 1.0f) * m_shakeIntensity * falloff;
    float offsetZ = ((std::rand() % 1000) / 500.0f - 1.0f) * m_shakeIntensity * falloff;

    m_camera->Position += glm::vec3(offsetX, offsetY, offsetZ);
}

// ============================================================================
// Bounds
// ============================================================================

void CameraController::setWorldBounds(const glm::vec3& min, const glm::vec3& max) {
    m_boundsMin = min;
    m_boundsMax = max;
}

void CameraController::applyBounds() {
    if (!m_camera) return;

    m_camera->Position.x = glm::clamp(m_camera->Position.x, m_boundsMin.x, m_boundsMax.x);
    m_camera->Position.y = std::max(m_camera->Position.y, m_minHeight);
    m_camera->Position.y = glm::clamp(m_camera->Position.y, m_boundsMin.y, m_boundsMax.y);
    m_camera->Position.z = glm::clamp(m_camera->Position.z, m_boundsMin.z, m_boundsMax.z);
}

// ============================================================================
// Accessors
// ============================================================================

float CameraController::getEffectiveFOV() const {
    if (!m_camera) return 45.0f;

    float fov = m_camera->Zoom;

    // Underwater: slightly wider FOV
    if (isUnderwater()) {
        fov *= 1.1f;
    }

    return fov;
}

// ============================================================================
// Helpers
// ============================================================================

glm::vec3 CameraController::smoothDamp(const glm::vec3& current, const glm::vec3& target,
                                        glm::vec3& velocity, float smoothTime,
                                        float deltaTime) {
    // Based on Unity's SmoothDamp
    float omega = 2.0f / smoothTime;
    float x = omega * deltaTime;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

    glm::vec3 change = current - target;
    glm::vec3 temp = (velocity + omega * change) * deltaTime;
    velocity = (velocity - omega * temp) * exp;

    glm::vec3 result = target + (change + temp) * exp;

    // Prevent overshooting
    if (glm::dot(target - current, result - target) > 0.0f) {
        result = target;
        velocity = glm::vec3(0.0f);
    }

    return result;
}

float CameraController::easeInOutCubic(float t) {
    return t < 0.5f
        ? 4.0f * t * t * t
        : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

float CameraController::easeOutQuad(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

CameraKeyframe CameraController::interpolateKeyframes(float time) const {
    if (m_cinematicKeyframes.empty()) {
        return CameraKeyframe{{0, 0, 0}, {0, 0, 1}, 45.0f, 0.0f};
    }

    if (time <= m_cinematicKeyframes.front().time) {
        return m_cinematicKeyframes.front();
    }

    if (time >= m_cinematicKeyframes.back().time) {
        return m_cinematicKeyframes.back();
    }

    // Find surrounding keyframes
    for (size_t i = 0; i < m_cinematicKeyframes.size() - 1; ++i) {
        const auto& kf1 = m_cinematicKeyframes[i];
        const auto& kf2 = m_cinematicKeyframes[i + 1];

        if (time >= kf1.time && time <= kf2.time) {
            float t = (time - kf1.time) / (kf2.time - kf1.time);
            t = easeInOutCubic(t);

            CameraKeyframe result;
            result.position = glm::mix(kf1.position, kf2.position, t);
            result.target = glm::mix(kf1.target, kf2.target, t);
            result.fov = glm::mix(kf1.fov, kf2.fov, t);
            result.time = time;
            return result;
        }
    }

    return m_cinematicKeyframes.back();
}

// ============================================================================
// Phase 7: Cinematic Presentation Modes
// ============================================================================

void CameraController::startSlowOrbit(const glm::vec3& center, float radius) {
    m_slowOrbitCenter = center;
    m_slowOrbitRadius = (radius < 0.0f) ? m_cinematicConfig.preferredDistance : radius;
    m_slowOrbitAngle = 0.0f;
    m_slowOrbitVerticalPhase = 0.0f;

    // Reset smoothing velocities
    m_positionVelocity = glm::vec3(0.0f);
    m_targetVelocity = glm::vec3(0.0f);

    setMode(CameraMode::CINEMATIC_SLOW_ORBIT, true);
}

void CameraController::startGlide(const glm::vec3& startPos, const glm::vec3& endPos, float duration) {
    m_glideStart = startPos;
    m_glideEnd = endPos;
    m_glideDuration = duration;
    m_glideProgress = 0.0f;

    // Apply collision avoidance to both start and end positions
    m_glideStart = applyCollisionAvoidance(m_glideStart);
    m_glideEnd = applyCollisionAvoidance(m_glideEnd);

    // Reset smoothing velocities
    m_positionVelocity = glm::vec3(0.0f);
    m_targetVelocity = glm::vec3(0.0f);

    setMode(CameraMode::CINEMATIC_GLIDE, true);
}

void CameraController::startFollowTarget(const Creature* creature) {
    m_currentCinematicTarget = creature;
    m_cinematicTargetVelocity = glm::vec3(0.0f);

    // Reset smoothing velocities
    m_positionVelocity = glm::vec3(0.0f);
    m_targetVelocity = glm::vec3(0.0f);

    setMode(CameraMode::CINEMATIC_FOLLOW_TARGET, true);
}

void CameraController::updateSlowOrbitMode(float deltaTime) {
    if (!m_camera) return;

    // Update orbit angle with slow, smooth movement
    m_slowOrbitAngle += m_cinematicConfig.orbitSpeed * deltaTime;
    m_slowOrbitVerticalPhase += m_cinematicConfig.orbitSpeed * 0.3f * deltaTime;

    // Calculate position with vertical variation for more dynamic movement
    float verticalOffset = std::sin(m_slowOrbitVerticalPhase) *
                          m_cinematicConfig.heightVariation * m_cinematicConfig.heightOffset;

    float x = std::cos(m_slowOrbitAngle) * m_slowOrbitRadius;
    float z = std::sin(m_slowOrbitAngle) * m_slowOrbitRadius;
    float y = m_cinematicConfig.heightOffset + verticalOffset;

    glm::vec3 desiredPos = m_slowOrbitCenter + glm::vec3(x, y, z);

    // Apply collision avoidance
    desiredPos = applyCollisionAvoidance(desiredPos);

    // Smooth interpolation using critically damped spring
    m_camera->Position = smoothDamp(m_camera->Position, desiredPos,
                                    m_positionVelocity,
                                    m_cinematicConfig.positionSmoothTime, deltaTime);

    // Smooth target tracking
    m_targetPoint = smoothDamp(m_targetPoint, m_slowOrbitCenter,
                               m_targetVelocity,
                               m_cinematicConfig.rotationSmoothTime, deltaTime);

    // Update camera orientation
    glm::vec3 lookDir = glm::normalize(m_targetPoint - m_camera->Position);
    m_camera->Front = lookDir;
    m_camera->Right = glm::normalize(glm::cross(m_camera->WorldUp, m_camera->Front));
    m_camera->Up = glm::normalize(glm::cross(m_camera->Right, m_camera->Front));

    m_camera->Yaw = glm::degrees(std::atan2(lookDir.z, lookDir.x));
    m_camera->Pitch = glm::degrees(std::asin(glm::clamp(lookDir.y, -1.0f, 1.0f)));
}

void CameraController::updateGlideMode(float deltaTime) {
    if (!m_camera) return;

    // Update progress
    m_glideProgress += deltaTime / m_glideDuration;

    if (m_glideProgress >= 1.0f) {
        // Glide complete, switch to slow orbit around end point
        startSlowOrbit(m_glideEnd, m_cinematicConfig.preferredDistance);
        return;
    }

    // Use smooth easing for glide motion
    float t = easeInOutSine(m_glideProgress);

    // Calculate desired position along glide path
    glm::vec3 desiredPos = glm::mix(m_glideStart, m_glideEnd, t);

    // Add subtle vertical wave motion
    float waveOffset = std::sin(m_glideProgress * 3.14159f * 2.0f) *
                      m_cinematicConfig.heightVariation * 5.0f;
    desiredPos.y += waveOffset;

    // Apply collision avoidance
    desiredPos = applyCollisionAvoidance(desiredPos);

    // Smooth interpolation
    m_camera->Position = smoothDamp(m_camera->Position, desiredPos,
                                    m_positionVelocity,
                                    m_cinematicConfig.positionSmoothTime * 0.5f, deltaTime);

    // Look ahead along the glide path
    glm::vec3 lookTarget = glm::mix(m_glideStart, m_glideEnd, std::min(1.0f, t + 0.2f));

    m_targetPoint = smoothDamp(m_targetPoint, lookTarget,
                               m_targetVelocity,
                               m_cinematicConfig.rotationSmoothTime, deltaTime);

    // Update camera orientation
    glm::vec3 lookDir = glm::normalize(m_targetPoint - m_camera->Position);
    if (glm::length(lookDir) > 0.001f) {
        m_camera->Front = lookDir;
        m_camera->Right = glm::normalize(glm::cross(m_camera->WorldUp, m_camera->Front));
        m_camera->Up = glm::normalize(glm::cross(m_camera->Right, m_camera->Front));

        m_camera->Yaw = glm::degrees(std::atan2(lookDir.z, lookDir.x));
        m_camera->Pitch = glm::degrees(std::asin(glm::clamp(lookDir.y, -1.0f, 1.0f)));
    }
}

void CameraController::updateFollowTargetMode(float deltaTime) {
    if (!m_camera) return;

    // Get effective target (override or selected)
    const Creature* target = m_targetOverrideActive ? m_overrideCreature : m_currentCinematicTarget;
    glm::vec3 targetPos;

    if (m_targetOverrideActive && m_usePositionOverride) {
        targetPos = m_overridePosition;
    } else if (target && target->isActive()) {
        targetPos = target->getPosition();
    } else {
        // No valid target, fall back to slow orbit at current position
        startSlowOrbit(m_targetPoint, m_cinematicConfig.preferredDistance);
        return;
    }

    // Calculate smart camera position based on creature movement
    glm::vec3 creatureVel = glm::vec3(0.0f);
    if (target) {
        creatureVel = target->getVelocity();
    }

    // Determine camera offset based on movement direction
    glm::vec3 offsetDir;
    if (glm::length(creatureVel) > 0.5f) {
        // Position camera to the side and behind, anticipating movement
        offsetDir = -glm::normalize(creatureVel);
        // Add some side offset for more dynamic framing
        glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), offsetDir));
        offsetDir = glm::normalize(offsetDir + right * 0.3f);
    } else {
        // Use current camera direction when stationary
        glm::vec3 toCamera = m_camera->Position - targetPos;
        toCamera.y = 0.0f;
        if (glm::length(toCamera) > 0.1f) {
            offsetDir = glm::normalize(toCamera);
        } else {
            offsetDir = glm::vec3(0.0f, 0.0f, 1.0f);
        }
    }

    // Calculate desired camera position
    float distance = m_cinematicConfig.preferredDistance;
    float height = m_cinematicConfig.heightOffset;

    // Adjust distance based on creature speed for more dynamic shots
    float speed = glm::length(creatureVel);
    distance += speed * 0.5f;  // Pull back slightly when moving fast
    distance = glm::clamp(distance, m_cinematicConfig.minDistance, m_cinematicConfig.maxDistance);

    glm::vec3 desiredPos = targetPos + offsetDir * distance + glm::vec3(0, height, 0);

    // Apply collision avoidance
    desiredPos = applyCollisionAvoidance(desiredPos);

    // Smooth interpolation with dynamic smoothing (smoother when far, responsive when close)
    float distToDesired = glm::length(desiredPos - m_camera->Position);
    float dynamicSmooth = m_cinematicConfig.positionSmoothTime *
                         (1.0f + distToDesired * 0.02f);

    m_camera->Position = smoothDamp(m_camera->Position, desiredPos,
                                    m_positionVelocity, dynamicSmooth, deltaTime);

    // Track target with slight lead for smooth following
    glm::vec3 leadTarget = targetPos + creatureVel * 0.3f;
    leadTarget.y += 2.0f;  // Look slightly above creature

    m_targetPoint = smoothDamp(m_targetPoint, leadTarget,
                               m_targetVelocity,
                               m_cinematicConfig.rotationSmoothTime, deltaTime);

    // Update camera orientation
    glm::vec3 lookDir = glm::normalize(m_targetPoint - m_camera->Position);
    m_camera->Front = lookDir;
    m_camera->Right = glm::normalize(glm::cross(m_camera->WorldUp, m_camera->Front));
    m_camera->Up = glm::normalize(glm::cross(m_camera->Right, m_camera->Front));

    m_camera->Yaw = glm::degrees(std::atan2(lookDir.z, lookDir.x));
    m_camera->Pitch = glm::degrees(std::asin(glm::clamp(lookDir.y, -1.0f, 1.0f)));

    // Adjust FOV based on speed for cinematic effect
    float targetFOV = m_cinematicConfig.baseFOV - speed * 0.5f;
    targetFOV = glm::clamp(targetFOV,
                           m_cinematicConfig.cinematicFOV,
                           m_cinematicConfig.baseFOV + m_cinematicConfig.maxFOVChange);
    m_targetCinematicFOV = targetFOV;
}

void CameraController::updatePhotoMode(float deltaTime) {
    if (!m_camera || !m_photoMode.active) return;

    // In photo mode, camera stays at frozen position unless manually nudged
    if (m_photoMode.freezeCamera) {
        m_camera->Position = m_photoMode.frozenPosition;
        m_targetPoint = m_photoMode.frozenTarget;
    }

    // Apply manual FOV and roll
    m_camera->Zoom = m_photoMode.manualFOV;
    m_cinematicRoll = m_photoMode.manualRoll;

    // Update camera orientation (needed for manual nudges)
    glm::vec3 lookDir = glm::normalize(m_targetPoint - m_camera->Position);
    m_camera->Front = lookDir;
    m_camera->Right = glm::normalize(glm::cross(m_camera->WorldUp, m_camera->Front));
    m_camera->Up = glm::normalize(glm::cross(m_camera->Right, m_camera->Front));
}

// ============================================================================
// Phase 7: Target Selection System
// ============================================================================

void CameraController::updateTargetSelection(float deltaTime) {
    if (m_targetConfig.lockTarget || !m_creaturePool || m_creaturePool->empty()) {
        return;
    }

    // Don't auto-switch if target override is active
    if (m_targetOverrideActive) {
        return;
    }

    m_targetSwitchTimer += deltaTime;

    // Check if it's time to potentially switch targets
    if (m_targetSwitchTimer >= m_targetConfig.switchInterval) {
        m_targetSwitchTimer = 0.0f;

        const Creature* newTarget = selectBestTarget();
        if (newTarget && newTarget != m_currentCinematicTarget) {
            // Smooth transition to new target
            m_currentCinematicTarget = newTarget;
        }
    }

    // Also switch if current target is no longer valid
    if (m_currentCinematicTarget && !m_currentCinematicTarget->isActive()) {
        m_currentCinematicTarget = selectBestTarget();
    }
}

const Creature* CameraController::selectBestTarget() const {
    if (!m_creaturePool || m_creaturePool->empty()) {
        return nullptr;
    }

    switch (m_targetConfig.mode) {
        case TargetSelectionMode::LARGEST_CREATURE:
            return findLargestCreature();
        case TargetSelectionMode::NEAREST_ACTION:
            return findMostActiveCreature();
        case TargetSelectionMode::RANDOM_FOCUS:
            return findRandomCreature();
        case TargetSelectionMode::PREDATOR_PRIORITY:
            return findPredatorCreature();
        case TargetSelectionMode::MOST_OFFSPRING:
            // Fall back to largest for now (requires lineage tracking)
            return findLargestCreature();
        case TargetSelectionMode::MANUAL:
        default:
            return m_currentCinematicTarget;
    }
}

const Creature* CameraController::findLargestCreature() const {
    if (!m_creaturePool || m_creaturePool->empty()) return nullptr;

    const Creature* largest = nullptr;
    float maxSize = 0.0f;

    for (const Creature* creature : *m_creaturePool) {
        if (creature && creature->isActive()) {
            float size = creature->getSize();
            if (size > maxSize) {
                maxSize = size;
                largest = creature;
            }
        }
    }

    return largest;
}

const Creature* CameraController::findMostActiveCreature() const {
    if (!m_creaturePool || m_creaturePool->empty()) return nullptr;

    const Creature* mostActive = nullptr;
    float maxActivity = m_targetConfig.actionThreshold;

    for (const Creature* creature : *m_creaturePool) {
        if (creature && creature->isActive()) {
            float speed = glm::length(creature->getVelocity());
            if (speed > maxActivity) {
                maxActivity = speed;
                mostActive = creature;
            }
        }
    }

    // If no creature exceeds threshold, return current or largest
    return mostActive ? mostActive : findLargestCreature();
}

const Creature* CameraController::findRandomCreature() const {
    if (!m_creaturePool || m_creaturePool->empty()) return nullptr;

    // Collect active creatures
    std::vector<const Creature*> active;
    for (const Creature* creature : *m_creaturePool) {
        if (creature && creature->isActive()) {
            active.push_back(creature);
        }
    }

    if (active.empty()) return nullptr;

    // Random selection
    std::uniform_int_distribution<size_t> dist(0, active.size() - 1);
    return active[dist(m_rng)];
}

const Creature* CameraController::findPredatorCreature() const {
    if (!m_creaturePool || m_creaturePool->empty()) return nullptr;

    // Prefer larger, faster creatures (proxy for predators)
    const Creature* best = nullptr;
    float bestScore = 0.0f;

    for (const Creature* creature : *m_creaturePool) {
        if (creature && creature->isActive()) {
            float size = creature->getSize();
            float speed = glm::length(creature->getVelocity());
            // Score: larger and faster = more likely predator
            float score = size * 2.0f + speed;
            if (score > bestScore) {
                bestScore = score;
                best = creature;
            }
        }
    }

    return best;
}

void CameraController::overrideTarget(const Creature* creature) {
    m_targetOverrideActive = true;
    m_overrideCreature = creature;
    m_usePositionOverride = false;
}

void CameraController::overrideTargetPosition(const glm::vec3& position) {
    m_targetOverrideActive = true;
    m_overridePosition = position;
    m_usePositionOverride = true;
}

void CameraController::clearTargetOverride() {
    m_targetOverrideActive = false;
    m_overrideCreature = nullptr;
    m_usePositionOverride = false;
}

glm::vec3 CameraController::getCurrentTargetPosition() const {
    if (m_targetOverrideActive && m_usePositionOverride) {
        return m_overridePosition;
    }
    if (m_targetOverrideActive && m_overrideCreature && m_overrideCreature->isActive()) {
        return m_overrideCreature->getPosition();
    }
    if (m_currentCinematicTarget && m_currentCinematicTarget->isActive()) {
        return m_currentCinematicTarget->getPosition();
    }
    return m_targetPoint;
}

// ============================================================================
// Phase 7: Collision and Terrain Avoidance
// ============================================================================

float CameraController::getTerrainHeight(float x, float z) const {
    if (!m_terrain) return 0.0f;
    return m_terrain->getHeight(x, z);
}

bool CameraController::isPositionUnderwater(const glm::vec3& pos) const {
    if (!m_terrain) return false;
    return pos.y < m_waterLevel;
}

glm::vec3 CameraController::correctForTerrain(const glm::vec3& pos) const {
    if (!m_terrain || !m_cinematicConfig.avoidTerrain) return pos;

    float terrainHeight = getTerrainHeight(pos.x, pos.z);
    float minHeight = terrainHeight + m_cinematicConfig.collisionPadding;

    if (pos.y < minHeight) {
        return glm::vec3(pos.x, minHeight, pos.z);
    }
    return pos;
}

glm::vec3 CameraController::correctForWater(const glm::vec3& pos) const {
    if (!m_cinematicConfig.avoidUnderwater) return pos;

    float minHeight = m_waterLevel + m_cinematicConfig.underwaterAvoidanceMargin;

    if (pos.y < minHeight) {
        return glm::vec3(pos.x, minHeight, pos.z);
    }
    return pos;
}

glm::vec3 CameraController::applyCollisionAvoidance(const glm::vec3& desiredPosition) const {
    glm::vec3 corrected = desiredPosition;

    // First correct for terrain
    corrected = correctForTerrain(corrected);

    // Then correct for water (if underwater is not desired)
    corrected = correctForWater(corrected);

    return corrected;
}

bool CameraController::wouldCollide(const glm::vec3& position) const {
    if (m_terrain && m_cinematicConfig.avoidTerrain) {
        float terrainHeight = getTerrainHeight(position.x, position.z);
        if (position.y < terrainHeight + m_cinematicConfig.collisionPadding) {
            return true;
        }
    }

    if (m_cinematicConfig.avoidUnderwater) {
        if (position.y < m_waterLevel + m_cinematicConfig.underwaterAvoidanceMargin) {
            return true;
        }
    }

    return false;
}

// ============================================================================
// Phase 7: Photo Mode
// ============================================================================

void CameraController::enterPhotoMode() {
    if (!m_camera) return;

    m_photoMode.active = true;
    m_photoMode.freezeCamera = true;
    m_photoMode.frozenPosition = m_camera->Position;
    m_photoMode.frozenTarget = m_targetPoint;
    m_photoMode.manualFOV = m_camera->Zoom;
    m_photoMode.manualRoll = 0.0f;

    setMode(CameraMode::PHOTO_MODE, false);

    // Notify Agent 10 if callback is registered
    if (onPhotoModeFreeze) {
        onPhotoModeFreeze(true);
    }
}

void CameraController::exitPhotoMode() {
    if (!m_photoMode.active) return;

    m_photoMode.active = false;

    // Restore FOV
    if (m_camera) {
        m_camera->Zoom = m_cinematicConfig.baseFOV;
    }
    m_cinematicRoll = 0.0f;

    // Notify Agent 10
    if (onPhotoModeFreeze) {
        onPhotoModeFreeze(false);
    }

    // Return to free mode
    setMode(CameraMode::FREE, false);
}

void CameraController::setPhotoModeFOV(float fov) {
    m_photoMode.manualFOV = glm::clamp(fov, 10.0f, 120.0f);
}

void CameraController::setPhotoModeRoll(float roll) {
    m_photoMode.manualRoll = glm::clamp(roll, -0.5f, 0.5f);  // Radians
}

void CameraController::setPhotoModeFocus(float distance, float strength) {
    m_photoMode.depthOfFieldFocus = std::max(1.0f, distance);
    m_photoMode.depthOfFieldStrength = glm::clamp(strength, 0.0f, 1.0f);
}

void CameraController::photoModeNudge(const glm::vec3& offset) {
    if (!m_photoMode.active || !m_camera) return;

    // Apply small position adjustment in camera-relative space
    glm::vec3 worldOffset = m_camera->Right * offset.x +
                           m_camera->Up * offset.y +
                           m_camera->Front * offset.z;

    m_photoMode.frozenPosition += worldOffset;
    m_photoMode.frozenTarget += worldOffset;

    // Apply collision avoidance
    m_photoMode.frozenPosition = applyCollisionAvoidance(m_photoMode.frozenPosition);
}

// ============================================================================
// Phase 7: Cinematic Presentation Effects
// ============================================================================

void CameraController::updateCinematicEffects(float deltaTime) {
    // Only apply cinematic effects in cinematic modes
    bool isCinematicMode = (m_mode == CameraMode::CINEMATIC_SLOW_ORBIT ||
                           m_mode == CameraMode::CINEMATIC_GLIDE ||
                           m_mode == CameraMode::CINEMATIC_FOLLOW_TARGET ||
                           m_mode == CameraMode::CINEMATIC);

    if (isCinematicMode && m_mode != CameraMode::PHOTO_MODE) {
        updateCinematicRoll(deltaTime);
        updateCinematicFOV(deltaTime);
    } else if (m_mode != CameraMode::PHOTO_MODE) {
        // Smoothly return to neutral
        m_cinematicRoll = smoothDampFloat(m_cinematicRoll, 0.0f,
                                         m_rollVelocity, 0.5f, deltaTime);
        m_cinematicFOV = smoothDampFloat(m_cinematicFOV, m_cinematicConfig.baseFOV,
                                        m_fovVelocity, 0.5f, deltaTime);
    }
}

void CameraController::updateCinematicRoll(float deltaTime) {
    // Subtle roll oscillation for cinematic feel
    m_cinematicRollPhase += m_cinematicConfig.rollSpeed * deltaTime;

    float targetRoll = std::sin(m_cinematicRollPhase) * m_cinematicConfig.rollIntensity;

    m_cinematicRoll = smoothDampFloat(m_cinematicRoll, targetRoll,
                                      m_rollVelocity,
                                      0.3f, deltaTime);
}

void CameraController::updateCinematicFOV(float deltaTime) {
    // Smooth FOV transitions
    m_cinematicFOV = smoothDampFloat(m_cinematicFOV, m_targetCinematicFOV,
                                     m_fovVelocity,
                                     m_cinematicConfig.fovSmoothTime, deltaTime);

    // Apply to camera
    if (m_camera) {
        m_camera->Zoom = m_cinematicFOV;
    }
}

float CameraController::smoothDampFloat(float current, float target, float& velocity,
                                         float smoothTime, float deltaTime) {
    // Based on Unity's SmoothDamp for floats
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

float CameraController::easeInOutSine(float t) {
    return -(std::cos(3.14159f * t) - 1.0f) / 2.0f;
}

// ============================================================================
// Phase 8: Inspect Mode Implementation
// ============================================================================

void CameraController::startInspect(const Creature* creature) {
    if (!creature || !creature->isActive()) return;

    m_inspectTarget = creature;
    m_inspectDistance = m_inspectConfig.distance;

    // Calculate initial orbit angle based on current camera position
    if (m_camera) {
        glm::vec3 toCamera = m_camera->Position - creature->getPosition();
        toCamera.y = 0.0f;
        if (glm::length(toCamera) > 0.1f) {
            m_inspectYaw = std::atan2(toCamera.z, toCamera.x);
        } else {
            m_inspectYaw = 0.0f;
        }
    }

    // Reset smoothing velocities for clean transition
    m_positionVelocity = glm::vec3(0.0f);
    m_targetVelocity = glm::vec3(0.0f);

    setMode(CameraMode::INSPECT, true);
}

void CameraController::exitInspect() {
    if (m_mode != CameraMode::INSPECT) return;

    m_inspectTarget = nullptr;
    setMode(CameraMode::FREE, true);
}

void CameraController::inspectZoom(float delta) {
    if (m_mode != CameraMode::INSPECT || !m_inspectConfig.allowZoom) return;

    m_inspectDistance = glm::clamp(
        m_inspectDistance - delta * 2.0f,
        m_inspectConfig.minDistance,
        m_inspectConfig.maxDistance
    );
}

void CameraController::inspectOrbit(float yaw, float pitch) {
    if (m_mode != CameraMode::INSPECT || !m_inspectConfig.allowOrbit) return;

    m_inspectYaw += yaw * 0.01f;
    m_inspectPitch = glm::clamp(
        m_inspectPitch - pitch * 0.01f,
        0.1f,   // Minimum pitch (almost horizontal)
        1.4f    // Maximum pitch (almost top-down)
    );
}

void CameraController::updateInspectMode(float deltaTime) {
    if (!m_camera) return;

    // Validate target is still alive
    if (!m_inspectTarget || !m_inspectTarget->isActive()) {
        // Target died or despawned - return to free mode
        m_inspectTarget = nullptr;
        setMode(CameraMode::FREE, true);
        return;
    }

    // Get creature position
    glm::vec3 creaturePos = m_inspectTarget->getPosition();

    // Apply optional slow auto-orbit
    if (m_inspectConfig.orbitSpeed != 0.0f) {
        m_inspectYaw += m_inspectConfig.orbitSpeed * deltaTime;
    }

    // Calculate desired camera position using spherical coordinates
    float cosP = std::cos(m_inspectPitch);
    float sinP = std::sin(m_inspectPitch);
    float cosY = std::cos(m_inspectYaw);
    float sinY = std::sin(m_inspectYaw);

    glm::vec3 offset(
        cosP * cosY * m_inspectDistance,
        sinP * m_inspectDistance + m_inspectConfig.height,
        cosP * sinY * m_inspectDistance
    );

    glm::vec3 desiredPos = creaturePos + offset;

    // Apply collision avoidance
    desiredPos = applyCollisionAvoidance(desiredPos);

    // Smooth interpolation to desired position
    m_camera->Position = smoothDamp(
        m_camera->Position,
        desiredPos,
        m_positionVelocity,
        m_inspectConfig.smoothTime,
        deltaTime
    );

    // Look at creature (slightly above center for better framing)
    glm::vec3 lookTarget = creaturePos + glm::vec3(0.0f, m_inspectTarget->getSize() * 0.5f, 0.0f);

    m_targetPoint = smoothDamp(
        m_targetPoint,
        lookTarget,
        m_targetVelocity,
        m_inspectConfig.smoothTime * 0.5f,
        deltaTime
    );

    // Update camera orientation
    glm::vec3 lookDir = glm::normalize(m_targetPoint - m_camera->Position);
    m_camera->Front = lookDir;
    m_camera->Right = glm::normalize(glm::cross(m_camera->WorldUp, m_camera->Front));
    m_camera->Up = glm::normalize(glm::cross(m_camera->Right, m_camera->Front));

    m_camera->Yaw = glm::degrees(std::atan2(lookDir.z, lookDir.x));
    m_camera->Pitch = glm::degrees(std::asin(glm::clamp(lookDir.y, -1.0f, 1.0f)));
}

} // namespace Forge
