#include "SecondaryMotion.h"
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <random>

namespace animation {

// =============================================================================
// TAIL DYNAMICS IMPLEMENTATION
// =============================================================================

void TailDynamics::initialize(const TailConfig& config) {
    m_config = config;

    int segmentCount = static_cast<int>(config.boneIndices.size());
    if (segmentCount == 0) return;

    m_elements.resize(segmentCount);
    m_rotations.resize(segmentCount, glm::quat(1, 0, 0, 0));
    m_positions.resize(segmentCount, glm::vec3(0));

    // Initialize each segment
    float totalLength = 1.0f;  // Will be set from actual bone lengths
    float segmentLength = totalLength / segmentCount;

    for (int i = 0; i < segmentCount; ++i) {
        ChainElement& elem = m_elements[i];
        elem.length = segmentLength;
        elem.mass = getInterpolatedMass(i);
        elem.stiffness = getInterpolatedStiffness(i);
        elem.damping = getInterpolatedDamping(i);
        elem.maxBendAngle = config.maxBendPerSegment;
        elem.position = glm::vec3(0, 0, -i * segmentLength);
        elem.velocity = glm::vec3(0);
        elem.rotation = glm::quat(1, 0, 0, 0);
        elem.angularVelocity = glm::vec3(0);
    }
}

void TailDynamics::update(
    float deltaTime,
    const glm::vec3& rootPosition,
    const glm::quat& rootRotation,
    const glm::vec3& bodyVelocity,
    const glm::vec3& bodyAngularVelocity
) {
    if (m_elements.empty()) return;

    deltaTime = std::min(deltaTime, 0.033f);  // Cap for stability

    glm::vec3 gravity(0, -9.81f * m_config.gravityScale, 0);

    // Update wag phase
    m_wagPhase += m_wagFrequency * deltaTime * 6.28318f;
    if (m_wagPhase > 6.28318f) m_wagPhase -= 6.28318f;

    // Process each segment
    glm::vec3 parentPos = rootPosition;
    glm::quat parentRot = rootRotation;

    // Apply raise angle to root
    glm::quat raiseRot = glm::angleAxis(m_raiseAngle, glm::vec3(1, 0, 0));
    parentRot = parentRot * raiseRot;

    for (size_t i = 0; i < m_elements.size(); ++i) {
        ChainElement& elem = m_elements[i];

        // Calculate target position (where element wants to be)
        glm::vec3 localDir = glm::vec3(0, 0, -1);  // Tail extends backward

        // Add wag motion
        if (m_wagAmplitude > 0.001f && m_wagFrequency > 0.001f) {
            float wagAngle = std::sin(m_wagPhase + i * 0.3f) * m_wagAmplitude;
            localDir = glm::angleAxis(wagAngle, glm::vec3(0, 1, 0)) * localDir;
        }

        // Add curl
        if (std::abs(m_curlAmount) > 0.001f) {
            float curlAngle = m_curlAmount * (static_cast<float>(i) / m_elements.size());
            localDir = glm::angleAxis(curlAngle, glm::vec3(1, 0, 0)) * localDir;
        }

        // Add animation wave overlay
        if (m_config.waveAmplitude > 0.001f && m_config.waveFrequency > 0.001f) {
            float wavePhase = m_wagPhase * m_config.waveFrequency + i * 0.5f;
            float waveAngle = std::sin(wavePhase) * m_config.waveAmplitude;
            localDir = glm::angleAxis(waveAngle, glm::vec3(0, 1, 0)) * localDir;
        }

        glm::vec3 targetDir = parentRot * localDir;
        glm::vec3 targetPos = parentPos + targetDir * elem.length;

        // Calculate forces
        glm::vec3 force(0);

        // Spring force toward target
        glm::vec3 toTarget = targetPos - elem.position;
        force += toTarget * elem.stiffness;

        // Damping
        force -= elem.velocity * elem.damping;

        // Gravity
        force += gravity * elem.mass;

        // Inertia from body motion
        glm::vec3 inertialForce = -bodyVelocity * m_config.inertiaScale * 2.0f;
        force += inertialForce * elem.mass;

        // Angular momentum from body rotation
        glm::vec3 tangentialVel = glm::cross(bodyAngularVelocity, elem.position - rootPosition);
        force -= tangentialVel * elem.mass * m_config.inertiaScale;

        // Drag
        float speed = glm::length(elem.velocity);
        if (speed > 0.001f) {
            glm::vec3 dragForce = -glm::normalize(elem.velocity) * speed * speed * elem.drag;
            force += dragForce;
        }

        // Integrate
        glm::vec3 acceleration = force / elem.mass;
        elem.velocity += acceleration * deltaTime;
        elem.position += elem.velocity * deltaTime;

        // Constrain to segment length
        glm::vec3 dir = elem.position - parentPos;
        float dist = glm::length(dir);
        if (dist > 0.001f) {
            dir /= dist;
            elem.position = parentPos + dir * elem.length;
        }

        // Calculate rotation from direction
        glm::vec3 parentForward = parentRot * glm::vec3(0, 0, -1);
        glm::vec3 currentDir = glm::normalize(elem.position - parentPos);

        // Limit bend angle
        float dotProduct = glm::dot(parentForward, currentDir);
        dotProduct = std::max(dotProduct, std::cos(elem.maxBendAngle));
        if (dotProduct < 0.999f) {
            glm::vec3 axis = glm::cross(parentForward, currentDir);
            float axisLen = glm::length(axis);
            if (axisLen > 0.001f) {
                axis /= axisLen;
                float angle = std::acos(glm::clamp(dotProduct, -1.0f, 1.0f));
                angle = std::min(angle, elem.maxBendAngle);
                m_rotations[i] = glm::angleAxis(angle, axis);
            }
        } else {
            m_rotations[i] = glm::quat(1, 0, 0, 0);
        }

        m_positions[i] = elem.position;

        // Update parent for next segment
        parentPos = elem.position;
        parentRot = parentRot * m_rotations[i];
    }
}

void TailDynamics::reset() {
    for (auto& elem : m_elements) {
        elem.velocity = glm::vec3(0);
        elem.angularVelocity = glm::vec3(0);
    }
    m_wagPhase = 0.0f;
    m_curlAmount = 0.0f;
    m_raiseAngle = 0.0f;
}

void TailDynamics::setWag(float amplitude, float frequency) {
    m_wagAmplitude = amplitude;
    m_wagFrequency = frequency;
}

void TailDynamics::setCurl(float amount) {
    m_curlAmount = amount;
}

void TailDynamics::setRaise(float angle) {
    m_raiseAngle = angle;
}

float TailDynamics::getInterpolatedStiffness(int index) const {
    if (m_elements.empty()) return m_config.baseStiffness;
    float t = static_cast<float>(index) / (m_elements.size() - 1);
    return glm::mix(m_config.baseStiffness, m_config.tipStiffness, t);
}

float TailDynamics::getInterpolatedDamping(int index) const {
    if (m_elements.empty()) return m_config.baseDamping;
    float t = static_cast<float>(index) / (m_elements.size() - 1);
    return glm::mix(m_config.baseDamping, m_config.tipDamping, t);
}

float TailDynamics::getInterpolatedMass(int index) const {
    if (m_elements.empty()) return m_config.baseMass;
    float t = static_cast<float>(index) / (m_elements.size() - 1);
    return glm::mix(m_config.baseMass, m_config.tipMass, t);
}

// =============================================================================
// EAR DYNAMICS IMPLEMENTATION
// =============================================================================

void EarDynamics::initialize(const EarConfig& leftConfig, const EarConfig& rightConfig) {
    m_leftConfig = leftConfig;
    m_rightConfig = rightConfig;
    m_leftRotation = glm::quat(1, 0, 0, 0);
    m_rightRotation = glm::quat(1, 0, 0, 0);
    m_leftVelocity = glm::vec3(0);
    m_rightVelocity = glm::vec3(0);
}

void EarDynamics::update(
    float deltaTime,
    const glm::vec3& headVelocity,
    const glm::vec3& headAngularVelocity,
    float alertness
) {
    deltaTime = std::min(deltaTime, 0.033f);

    // Adjust stiffness based on alertness
    float alertStiffness = glm::mix(0.5f, 1.5f, alertness);

    // Calculate target angles based on mood
    float leftTarget = glm::mix(m_leftConfig.relaxedAngle, m_leftConfig.alertAngle, m_alertness);
    float rightTarget = glm::mix(m_rightConfig.relaxedAngle, m_rightConfig.alertAngle, m_alertness);

    // Apply expression overrides
    leftTarget = glm::mix(leftTarget, m_targetLeftAngle, 0.5f);
    rightTarget = glm::mix(rightTarget, m_targetRightAngle, 0.5f);

    // Spring-damper for left ear
    {
        glm::vec3 currentEuler = glm::eulerAngles(m_leftRotation);
        float currentAngle = currentEuler.z;

        float springForce = (leftTarget - currentAngle) * m_leftConfig.stiffness * alertStiffness;
        float dampingForce = -m_leftVelocity.z * m_leftConfig.damping;

        // Inertial force from head motion
        float inertialForce = -headVelocity.x * 2.0f - headAngularVelocity.y * 0.5f;

        float acceleration = (springForce + dampingForce + inertialForce) / m_leftConfig.mass;
        m_leftVelocity.z += acceleration * deltaTime;
        currentAngle += m_leftVelocity.z * deltaTime;

        // Clamp
        currentAngle = glm::clamp(currentAngle, -m_leftConfig.rotateRange, m_leftConfig.rotateRange);

        // Add flop (gravity effect for floppy ears)
        float flopAngle = m_leftConfig.flopFactor * -0.3f;

        m_leftRotation = glm::angleAxis(currentAngle, glm::vec3(0, 0, 1)) *
                         glm::angleAxis(flopAngle, glm::vec3(1, 0, 0));
    }

    // Spring-damper for right ear
    {
        glm::vec3 currentEuler = glm::eulerAngles(m_rightRotation);
        float currentAngle = currentEuler.z;

        float springForce = (rightTarget - currentAngle) * m_rightConfig.stiffness * alertStiffness;
        float dampingForce = -m_rightVelocity.z * m_rightConfig.damping;

        float inertialForce = headVelocity.x * 2.0f + headAngularVelocity.y * 0.5f;

        float acceleration = (springForce + dampingForce + inertialForce) / m_rightConfig.mass;
        m_rightVelocity.z += acceleration * deltaTime;
        currentAngle += m_rightVelocity.z * deltaTime;

        currentAngle = glm::clamp(currentAngle, -m_rightConfig.rotateRange, m_rightConfig.rotateRange);

        float flopAngle = m_rightConfig.flopFactor * -0.3f;

        m_rightRotation = glm::angleAxis(currentAngle, glm::vec3(0, 0, 1)) *
                         glm::angleAxis(flopAngle, glm::vec3(1, 0, 0));
    }
}

void EarDynamics::setMood(float alertness, float happiness, float fear) {
    m_alertness = alertness;

    // Fear makes ears go back
    if (fear > 0.5f) {
        m_targetLeftAngle = m_leftConfig.backAngle;
        m_targetRightAngle = m_rightConfig.backAngle;
    } else if (alertness > 0.7f) {
        m_targetLeftAngle = m_leftConfig.alertAngle;
        m_targetRightAngle = m_rightConfig.alertAngle;
    } else {
        m_targetLeftAngle = m_leftConfig.relaxedAngle;
        m_targetRightAngle = m_rightConfig.relaxedAngle;
    }
}

void EarDynamics::pointAt(const glm::vec3& direction) {
    // Both ears rotate toward sound source
    float angle = std::atan2(direction.x, direction.z);
    m_targetLeftAngle = glm::clamp(angle, -m_leftConfig.rotateRange, m_leftConfig.rotateRange);
    m_targetRightAngle = glm::clamp(angle, -m_rightConfig.rotateRange, m_rightConfig.rotateRange);
}

// =============================================================================
// ANTENNA DYNAMICS IMPLEMENTATION
// =============================================================================

void AntennaDynamics::initialize(const AntennaConfig& leftConfig, const AntennaConfig& rightConfig) {
    m_leftConfig = leftConfig;
    m_rightConfig = rightConfig;

    int leftSegments = static_cast<int>(leftConfig.boneIndices.size());
    int rightSegments = static_cast<int>(rightConfig.boneIndices.size());

    m_leftElements.resize(leftSegments);
    m_rightElements.resize(rightSegments);
    m_leftRotations.resize(leftSegments, glm::quat(1, 0, 0, 0));
    m_rightRotations.resize(rightSegments, glm::quat(1, 0, 0, 0));

    float segmentLength = leftConfig.length / std::max(1, leftSegments);
    for (int i = 0; i < leftSegments; ++i) {
        m_leftElements[i].length = segmentLength;
        m_leftElements[i].mass = leftConfig.mass / leftSegments;
        m_leftElements[i].stiffness = leftConfig.stiffness;
        m_leftElements[i].damping = leftConfig.damping;
        m_leftElements[i].drag = leftConfig.airDragScale;
    }

    segmentLength = rightConfig.length / std::max(1, rightSegments);
    for (int i = 0; i < rightSegments; ++i) {
        m_rightElements[i].length = segmentLength;
        m_rightElements[i].mass = rightConfig.mass / rightSegments;
        m_rightElements[i].stiffness = rightConfig.stiffness;
        m_rightElements[i].damping = rightConfig.damping;
        m_rightElements[i].drag = rightConfig.airDragScale;
    }
}

void AntennaDynamics::update(
    float deltaTime,
    const glm::vec3& headVelocity,
    const glm::vec3& headAngularVelocity
) {
    deltaTime = std::min(deltaTime, 0.033f);

    // Update search motion
    if (m_searching) {
        m_searchPhase += deltaTime * m_leftConfig.searchFrequency * 6.28318f;
        if (m_searchPhase > 6.28318f) m_searchPhase -= 6.28318f;
    }

    // Simple chain physics for antennae
    auto updateChain = [&](
        std::vector<ChainElement>& elements,
        std::vector<glm::quat>& rotations,
        const AntennaConfig& config,
        float sideOffset
    ) {
        glm::vec3 gravity(0, -9.81f * config.gravityScale, 0);

        for (size_t i = 0; i < elements.size(); ++i) {
            ChainElement& elem = elements[i];

            // Target direction (forward and slightly up)
            glm::vec3 targetDir(sideOffset * 0.3f, 0.3f, 1.0f);
            targetDir = glm::normalize(targetDir);

            // Add search motion
            if (m_searching) {
                float searchAngle = std::sin(m_searchPhase + i * 0.5f) * config.searchAmplitude;
                float searchAngle2 = std::cos(m_searchPhase * 0.7f + i * 0.3f) * config.searchAmplitude * 0.5f;
                targetDir = glm::angleAxis(searchAngle, glm::vec3(0, 1, 0)) *
                           glm::angleAxis(searchAngle2, glm::vec3(1, 0, 0)) * targetDir;
            }

            // Spring toward target
            glm::vec3 currentDir = elem.rotation * glm::vec3(0, 0, 1);
            glm::vec3 force = (targetDir - currentDir) * elem.stiffness;

            // Damping
            force -= elem.angularVelocity * elem.damping;

            // Gravity effect
            force += gravity * 0.1f;

            // Inertia from head motion
            force -= headVelocity * 0.5f;

            // Integrate
            elem.angularVelocity += force * deltaTime;
            elem.angularVelocity *= (1.0f - elem.drag * deltaTime);

            // Convert angular velocity to rotation
            float angVelMag = glm::length(elem.angularVelocity);
            if (angVelMag > 0.001f) {
                glm::vec3 axis = elem.angularVelocity / angVelMag;
                elem.rotation = glm::angleAxis(angVelMag * deltaTime, axis) * elem.rotation;
            }

            // Store rotation
            rotations[i] = elem.rotation;
        }
    };

    updateChain(m_leftElements, m_leftRotations, m_leftConfig, -1.0f);
    updateChain(m_rightElements, m_rightRotations, m_rightConfig, 1.0f);
}

void AntennaDynamics::setSearching(bool active) {
    m_searching = active;
}

void AntennaDynamics::pointAt(const glm::vec3& direction) {
    // TODO: Override search to point both antennae at target
}

// =============================================================================
// SOFT BODY DYNAMICS IMPLEMENTATION
// =============================================================================

void SoftBodyDynamics::initialize(const std::vector<SoftBodyConfig>& regions) {
    m_regions = regions;
    m_displacements.resize(regions.size(), glm::vec3(0));
    m_velocities.resize(regions.size(), glm::vec3(0));
}

void SoftBodyDynamics::update(
    float deltaTime,
    const glm::vec3& bodyVelocity,
    const glm::vec3& bodyAcceleration
) {
    deltaTime = std::min(deltaTime, 0.033f);

    for (size_t i = 0; i < m_regions.size(); ++i) {
        const SoftBodyConfig& config = m_regions[i];
        glm::vec3& disp = m_displacements[i];
        glm::vec3& vel = m_velocities[i];

        // Force from acceleration (inertia)
        glm::vec3 inertialForce = -bodyAcceleration * config.mass;

        // Bias toward primary axis
        if (config.axialBias > 0.0f) {
            glm::vec3 axialComponent = glm::dot(inertialForce, config.jiggleAxis) * config.jiggleAxis;
            glm::vec3 perpComponent = inertialForce - axialComponent;
            inertialForce = axialComponent + perpComponent * (1.0f - config.axialBias);
        }

        // Spring force back to rest
        glm::vec3 springForce = -disp * config.stiffness;

        // Damping
        glm::vec3 dampingForce = -vel * config.damping;

        // Total force
        glm::vec3 totalForce = inertialForce + springForce + dampingForce;

        // Integrate
        glm::vec3 acceleration = totalForce / config.mass;
        vel += acceleration * deltaTime;
        disp += vel * deltaTime;

        // Clamp displacement
        float dispMag = glm::length(disp);
        if (dispMag > config.maxDisplacement) {
            disp = glm::normalize(disp) * config.maxDisplacement;
            // Also dampen velocity in that direction
            vel *= 0.5f;
        }
    }
}

glm::vec3 SoftBodyDynamics::getDisplacementAt(const glm::vec3& localPosition) const {
    glm::vec3 totalDisp(0);
    float totalWeight = 0.0f;

    for (size_t i = 0; i < m_regions.size(); ++i) {
        const SoftBodyConfig& config = m_regions[i];
        float dist = glm::length(localPosition - config.center);
        if (dist < config.radius) {
            float weight = 1.0f - (dist / config.radius);
            weight = weight * weight;  // Smooth falloff
            totalDisp += m_displacements[i] * weight;
            totalWeight += weight;
        }
    }

    if (totalWeight > 0.001f) {
        return totalDisp / totalWeight;
    }
    return glm::vec3(0);
}

// =============================================================================
// TENTACLE DYNAMICS IMPLEMENTATION
// =============================================================================

void TentacleDynamics::initialize(const TentacleConfig& config) {
    m_config = config;

    int segmentCount = static_cast<int>(config.boneIndices.size());
    if (segmentCount == 0) segmentCount = 8;  // Default

    m_elements.resize(segmentCount);
    m_rotations.resize(segmentCount, glm::quat(1, 0, 0, 0));
    m_positions.resize(segmentCount, glm::vec3(0));

    float segmentLength = config.totalLength / segmentCount;

    for (int i = 0; i < segmentCount; ++i) {
        ChainElement& elem = m_elements[i];
        float t = static_cast<float>(i) / (segmentCount - 1);

        elem.length = segmentLength;
        elem.mass = config.massPerSegment;
        elem.stiffness = glm::mix(config.baseStiffness, config.tipStiffness, t);
        elem.damping = glm::mix(config.baseDamping, config.tipDamping, t);
        elem.maxBendAngle = 0.8f;  // Tentacles are very flexible
        elem.position = glm::vec3(0, 0, -i * segmentLength);
        elem.velocity = glm::vec3(0);
    }
}

void TentacleDynamics::update(
    float deltaTime,
    const glm::vec3& basePosition,
    const glm::quat& baseRotation,
    const glm::vec3& bodyVelocity,
    bool isUnderwater
) {
    if (m_elements.empty()) return;

    deltaTime = std::min(deltaTime, 0.033f);

    float gravityScale = isUnderwater ? (1.0f - m_config.buoyancy) : 1.0f;
    float dragScale = isUnderwater ? m_config.waterDrag : 0.3f;

    glm::vec3 gravity(0, -9.81f * gravityScale, 0);

    glm::vec3 parentPos = basePosition;
    glm::quat parentRot = baseRotation;

    for (size_t i = 0; i < m_elements.size(); ++i) {
        ChainElement& elem = m_elements[i];

        // Calculate target position
        glm::vec3 localDir(0, 0, -1);

        // Add reach toward target
        if (m_reachStrength > 0.001f && i == m_elements.size() - 1) {
            glm::vec3 toTarget = m_reachTarget - elem.position;
            if (glm::length(toTarget) > 0.01f) {
                localDir = glm::mix(localDir, glm::normalize(toTarget), m_reachStrength);
            }
        }

        // Add coil
        if (std::abs(m_coilAmount) > 0.001f) {
            float coilAngle = m_coilAmount * 0.3f;
            localDir = glm::angleAxis(coilAngle, glm::vec3(1, 0, 0)) * localDir;
        }

        glm::vec3 targetDir = parentRot * localDir;
        glm::vec3 targetPos = parentPos + targetDir * elem.length;

        // Forces
        glm::vec3 force(0);

        // Spring to target
        force += (targetPos - elem.position) * elem.stiffness;

        // Damping
        force -= elem.velocity * elem.damping;

        // Gravity (reduced underwater)
        force += gravity * elem.mass;

        // Drag
        float speed = glm::length(elem.velocity);
        if (speed > 0.001f) {
            force -= glm::normalize(elem.velocity) * speed * speed * dragScale;
        }

        // Buoyancy (underwater)
        if (isUnderwater) {
            force += glm::vec3(0, 9.81f * m_config.buoyancy * elem.mass, 0);
        }

        // Body inertia
        force -= bodyVelocity * elem.mass * 0.5f;

        // Integrate
        glm::vec3 acceleration = force / elem.mass;
        elem.velocity += acceleration * deltaTime;
        elem.position += elem.velocity * deltaTime;

        // Constrain length
        glm::vec3 dir = elem.position - parentPos;
        float dist = glm::length(dir);
        if (dist > 0.001f) {
            dir /= dist;
            elem.position = parentPos + dir * elem.length;
        }

        // Calculate rotation
        glm::vec3 parentForward = parentRot * glm::vec3(0, 0, -1);
        glm::vec3 currentDir = glm::normalize(elem.position - parentPos);

        float dot = glm::dot(parentForward, currentDir);
        if (dot < 0.999f && dot > -0.999f) {
            glm::vec3 axis = glm::cross(parentForward, currentDir);
            float axisLen = glm::length(axis);
            if (axisLen > 0.001f) {
                axis /= axisLen;
                float angle = std::acos(glm::clamp(dot, -1.0f, 1.0f));
                m_rotations[i] = glm::angleAxis(angle, axis);
            }
        } else {
            m_rotations[i] = glm::quat(1, 0, 0, 0);
        }

        m_positions[i] = elem.position;

        parentPos = elem.position;
        parentRot = parentRot * m_rotations[i];
    }
}

void TentacleDynamics::reachToward(const glm::vec3& target, float strength) {
    m_reachTarget = target;
    m_reachStrength = glm::clamp(strength, 0.0f, 1.0f);
}

void TentacleDynamics::coil(float amount) {
    m_coilAmount = amount;
}

void TentacleDynamics::relax() {
    m_reachStrength = 0.0f;
    m_coilAmount = 0.0f;
}

// =============================================================================
// FEATHER DYNAMICS IMPLEMENTATION
// =============================================================================

void FeatherDynamics::initialize(const FeatherParams& params) {
    m_params = params;
    m_ruffleAmount = 0.0f;
    m_spreadAmount = 0.0f;
    m_windDir = glm::vec3(0);
}

void FeatherDynamics::update(
    float deltaTime,
    const glm::vec3& bodyVelocity,
    const glm::vec3& windDirection,
    float windStrength
) {
    // Ruffle from motion
    float speed = glm::length(bodyVelocity);
    float motionRuffle = speed * 0.05f * m_params.windSensitivity;

    // Ruffle from wind
    float windRuffle = windStrength * 0.1f * m_params.windSensitivity;

    // Total ruffle target
    float targetRuffle = std::min(1.0f, motionRuffle + windRuffle);

    // Spring toward target with decay
    float ruffleForce = (targetRuffle - m_ruffleAmount) * m_params.ruffleStiffness;
    ruffleForce -= m_ruffleAmount * m_params.ruffleDecay;

    m_ruffleAmount += ruffleForce * deltaTime;
    m_ruffleAmount = glm::clamp(m_ruffleAmount, 0.0f, 1.0f);

    // Spread interpolation
    m_spreadAmount = glm::mix(m_spreadAmount, m_targetSpread, deltaTime * 3.0f);

    // Wind direction (local space)
    if (windStrength > 0.001f) {
        m_windDir = glm::normalize(windDirection);
    }
}

void FeatherDynamics::triggerRuffle(float intensity) {
    m_ruffleAmount = std::min(1.0f, m_ruffleAmount + intensity);
}

void FeatherDynamics::setSpread(float amount) {
    m_targetSpread = glm::clamp(amount, 0.0f, m_params.spreadAmount);
}

// =============================================================================
// SECONDARY MOTION SYSTEM IMPLEMENTATION
// =============================================================================

void SecondaryMotionSystem::initializeTail(const TailConfig& config) {
    m_tail = std::make_unique<TailDynamics>();
    m_tail->initialize(config);
}

void SecondaryMotionSystem::initializeEars(const EarConfig& left, const EarConfig& right) {
    m_ears = std::make_unique<EarDynamics>();
    m_ears->initialize(left, right);
}

void SecondaryMotionSystem::initializeAntennae(const AntennaConfig& left, const AntennaConfig& right) {
    m_antennae = std::make_unique<AntennaDynamics>();
    m_antennae->initialize(left, right);
}

void SecondaryMotionSystem::initializeSoftBody(const std::vector<SoftBodyConfig>& regions) {
    m_softBody = std::make_unique<SoftBodyDynamics>();
    m_softBody->initialize(regions);
}

void SecondaryMotionSystem::initializeTentacles(const std::vector<TentacleConfig>& configs) {
    m_tentacles.clear();
    for (const auto& config : configs) {
        auto tentacle = std::make_unique<TentacleDynamics>();
        tentacle->initialize(config);
        m_tentacles.push_back(std::move(tentacle));
    }
}

void SecondaryMotionSystem::initializeFeathers(const FeatherParams& params) {
    m_feathers = std::make_unique<FeatherDynamics>();
    m_feathers->initialize(params);
}

void SecondaryMotionSystem::update(
    float deltaTime,
    const glm::vec3& bodyPosition,
    const glm::quat& bodyRotation,
    const glm::vec3& bodyVelocity,
    const glm::vec3& bodyAngularVelocity,
    const glm::vec3& headPosition,
    const glm::quat& headRotation,
    const glm::vec3& windDirection,
    float windStrength,
    bool isUnderwater
) {
    // Calculate acceleration from velocity change
    glm::vec3 bodyAcceleration(0);
    if (m_initialized) {
        bodyAcceleration = (bodyVelocity - m_prevVelocity) / std::max(deltaTime, 0.001f);
    }

    // Calculate head velocity from position change
    glm::vec3 headVelocity(0);
    glm::vec3 headAngularVelocity(0);
    if (m_initialized) {
        // Approximate head velocity
        headVelocity = bodyVelocity;  // Simplified
    }

    // Update tail
    if (m_tail) {
        m_tail->update(deltaTime, bodyPosition, bodyRotation, bodyVelocity, bodyAngularVelocity);
    }

    // Update ears
    if (m_ears) {
        m_ears->update(deltaTime, headVelocity, headAngularVelocity, 0.5f);
    }

    // Update antennae
    if (m_antennae) {
        m_antennae->update(deltaTime, headVelocity, headAngularVelocity);
    }

    // Update soft body
    if (m_softBody) {
        m_softBody->update(deltaTime, bodyVelocity, bodyAcceleration);
    }

    // Update tentacles
    for (auto& tentacle : m_tentacles) {
        tentacle->update(deltaTime, bodyPosition, bodyRotation, bodyVelocity, isUnderwater);
    }

    // Update feathers
    if (m_feathers) {
        m_feathers->update(deltaTime, bodyVelocity, windDirection, windStrength);
    }

    // Store for next frame
    m_prevPosition = bodyPosition;
    m_prevRotation = bodyRotation;
    m_prevVelocity = bodyVelocity;
    m_initialized = true;
}

void SecondaryMotionSystem::setMood(float alertness, float happiness, float fear, float aggression) {
    if (m_ears) {
        m_ears->setMood(alertness, happiness, fear);
    }

    if (m_tail) {
        if (happiness > 0.7f) {
            m_tail->setWag(0.3f, 3.0f);  // Happy wagging
        } else if (fear > 0.5f) {
            m_tail->setCurl(-0.5f);      // Tail tucked
            m_tail->setWag(0.0f, 0.0f);
        } else if (aggression > 0.5f) {
            m_tail->setRaise(0.3f);       // Raised tail
            m_tail->setCurl(0.2f);
        } else {
            m_tail->setWag(0.0f, 0.0f);
            m_tail->setCurl(0.0f);
            m_tail->setRaise(0.0f);
        }
    }

    if (m_feathers) {
        if (aggression > 0.5f || fear > 0.7f) {
            m_feathers->triggerRuffle(0.5f);
            m_feathers->setSpread(0.3f);
        } else {
            m_feathers->setSpread(0.0f);
        }
    }
}

void SecondaryMotionSystem::lookAt(const glm::vec3& worldTarget) {
    if (m_ears) {
        m_ears->pointAt(worldTarget);
    }
    if (m_antennae) {
        m_antennae->pointAt(worldTarget);
    }
}

void SecondaryMotionSystem::applyImpact(const glm::vec3& direction, float intensity) {
    if (m_feathers) {
        m_feathers->triggerRuffle(intensity);
    }

    // Could also apply impulse to soft body, tail, etc.
}

} // namespace animation
