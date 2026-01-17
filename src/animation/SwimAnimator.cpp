#include "SwimAnimator.h"
#include "../physics/Morphology.h"
#include <algorithm>
#include <cstdlib>

namespace animation {

// Constants
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = PI * 0.5f;

SwimAnimator::SwimAnimator() : m_config(), m_state() {}

SwimAnimator::SwimAnimator(const SwimConfig& config) : m_config(config), m_state() {}

void SwimAnimator::update(float deltaTime, const glm::vec3& velocity, float maxSpeed) {
    // Calculate current speed normalized to 0-1
    float speed = glm::length(velocity);
    float normalizedSpeed = maxSpeed > 0.0f ? std::min(speed / maxSpeed, 1.0f) : 0.0f;

    // Smooth speed changes
    m_state.dampedSpeed = glm::mix(m_state.dampedSpeed, normalizedSpeed, 1.0f - DAMPING_FACTOR);
    m_state.currentSpeed = m_state.dampedSpeed;

    // Calculate turn amount from velocity direction change
    if (speed > 0.1f) {
        glm::vec3 forward(0.0f, 0.0f, 1.0f);
        glm::vec3 velocityDir = glm::normalize(velocity);

        // Cross product Y component indicates turn direction
        float turnSign = velocityDir.x; // Simplified: positive = turning right
        m_state.dampedTurn = glm::mix(m_state.dampedTurn, turnSign, 1.0f - DAMPING_FACTOR);
        m_state.turnAmount = m_state.dampedTurn;

        // Vertical component
        m_state.verticalAmount = velocityDir.y;
    } else {
        m_state.dampedTurn = glm::mix(m_state.dampedTurn, 0.0f, 1.0f - DAMPING_FACTOR);
        m_state.turnAmount = m_state.dampedTurn;
        m_state.verticalAmount = 0.0f;
    }

    // Update swim phase based on speed
    // Fish swim faster when moving faster
    float speedFactor = getSpeedFactor();
    float phaseSpeed = m_config.bodyWaveSpeed * speedFactor * TWO_PI;
    m_state.swimPhase += phaseSpeed * deltaTime;

    // Keep phase in reasonable range
    if (m_state.swimPhase > TWO_PI * 100.0f) {
        m_state.swimPhase = std::fmod(m_state.swimPhase, TWO_PI);
    }
}

float SwimAnimator::getSpeedFactor() const {
    // Interpolate between min and max speed factor based on current speed
    return glm::mix(m_config.minSpeedFactor, m_config.maxSpeedFactor, m_state.currentSpeed);
}

float SwimAnimator::calculateStiffnessMask(float bodyPosition) const {
    // bodyPosition: 0 = head, 1 = tail

    // Use smoothstep to create gradual transitions
    float headRegion = 1.0f - smoothstep(0.0f, 0.3f, bodyPosition);
    float tailRegion = smoothstep(0.6f, 1.0f, bodyPosition);
    float midRegion = 1.0f - headRegion - tailRegion;

    // Blend stiffness values
    float stiffness = headRegion * m_config.headStiffness +
                     midRegion * m_config.midStiffness +
                     tailRegion * m_config.tailStiffness;

    // Return flexibility (inverse of stiffness)
    return 1.0f - stiffness;
}

float SwimAnimator::smoothstep(float edge0, float edge1, float x) {
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

std::vector<glm::quat> SwimAnimator::calculateSpineWave(const std::vector<int>& spineBones) const {
    std::vector<glm::quat> rotations;
    rotations.reserve(spineBones.size());

    int numBones = static_cast<int>(spineBones.size());
    if (numBones == 0) {
        return rotations;
    }

    float speedFactor = getSpeedFactor();
    float basePhase = m_state.swimPhase + m_config.phaseOffset;

    for (int i = 0; i < numBones; ++i) {
        // Calculate position along body (0 = head, 1 = tail)
        float bodyPos = static_cast<float>(i) / static_cast<float>(numBones - 1);

        // Phase offset increases toward tail (wave travels head to tail)
        float phaseOffset = bodyPos * m_config.wavelength * TWO_PI;

        // Calculate wave based on swimming style
        float amplitude = m_config.bodyWaveAmp * speedFactor;
        float flexibility = calculateStiffnessMask(bodyPos);

        float waveValue = 0.0f;

        switch (m_config.style) {
            case SwimStyle::CARANGIFORM:
                // Most motion in tail, exponential increase
                amplitude *= std::pow(bodyPos, 1.5f);
                waveValue = std::sin(basePhase + phaseOffset) * amplitude * flexibility;
                break;

            case SwimStyle::ANGUILLIFORM:
                // Full body motion, constant amplitude
                waveValue = std::sin(basePhase + phaseOffset) * amplitude * flexibility;
                break;

            case SwimStyle::SUBCARANGIFORM:
                // Motion starts around 1/3 of body
                if (bodyPos > 0.33f) {
                    float adjustedPos = (bodyPos - 0.33f) / 0.67f;
                    amplitude *= adjustedPos;
                } else {
                    amplitude *= 0.1f;
                }
                waveValue = std::sin(basePhase + phaseOffset) * amplitude * flexibility;
                break;

            case SwimStyle::THUNNIFORM:
                // Only tail moves significantly
                if (bodyPos > 0.7f) {
                    float adjustedPos = (bodyPos - 0.7f) / 0.3f;
                    amplitude *= adjustedPos * 2.0f;
                } else {
                    amplitude *= 0.05f;
                }
                waveValue = std::sin(basePhase + phaseOffset) * amplitude * flexibility;
                break;

            case SwimStyle::LABRIFORM:
                // Minimal body motion
                amplitude *= 0.2f;
                waveValue = std::sin(basePhase + phaseOffset) * amplitude * flexibility;
                break;
        }

        // Add turn influence (fish bend toward turn direction)
        float turnInfluence = m_state.turnAmount * 0.15f * bodyPos;
        waveValue += turnInfluence;

        // Create rotation around vertical axis (yaw) for side-to-side motion
        glm::quat yawRotation = glm::angleAxis(waveValue, glm::vec3(0.0f, 1.0f, 0.0f));

        // Add subtle pitch for vertical swimming
        float pitchValue = m_state.verticalAmount * 0.1f * (1.0f - bodyPos);
        glm::quat pitchRotation = glm::angleAxis(pitchValue, glm::vec3(1.0f, 0.0f, 0.0f));

        // Combine rotations
        rotations.push_back(pitchRotation * yawRotation);
    }

    return rotations;
}

glm::quat SwimAnimator::calculateTailFinMotion() const {
    float speedFactor = getSpeedFactor();
    float phase = m_state.swimPhase + m_config.phaseOffset;

    // Tail fin oscillates more than body segments
    float amplitude = m_config.caudalFinAmp * speedFactor * m_config.bodyWaveAmp;
    float waveValue = std::sin(phase) * amplitude;

    // Add turn influence
    waveValue += m_state.turnAmount * 0.2f;

    return glm::angleAxis(waveValue, glm::vec3(0.0f, 1.0f, 0.0f));
}

std::pair<glm::quat, glm::quat> SwimAnimator::calculatePectoralFinMotion() const {
    float speedFactor = getSpeedFactor();
    float phase = m_state.swimPhase + m_config.phaseOffset;

    // Base oscillation (subtle when swimming straight)
    float baseAmp = m_config.pectoralFinAmp * 0.3f * speedFactor;
    float baseWave = std::sin(phase * 0.5f) * baseAmp;

    // Turn influence (fins act as rudders)
    float turnInfluence = m_state.turnAmount * m_config.pectoralFinAmp;

    // Left fin: flares out when turning right
    float leftAngle = baseWave + turnInfluence * 0.5f;
    // Right fin: flares out when turning left
    float rightAngle = baseWave - turnInfluence * 0.5f;

    // Vertical swimming influence (fins angle to provide lift/dive)
    float verticalInfluence = m_state.verticalAmount * 0.3f;
    leftAngle += verticalInfluence;
    rightAngle += verticalInfluence;

    // Create rotations (rotate around local X axis for flapping motion)
    glm::quat leftRot = glm::angleAxis(leftAngle, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::quat rightRot = glm::angleAxis(-rightAngle, glm::vec3(0.0f, 0.0f, 1.0f));

    return {leftRot, rightRot};
}

glm::quat SwimAnimator::calculateDorsalFinMotion() const {
    float speedFactor = getSpeedFactor();
    float phase = m_state.swimPhase + m_config.phaseOffset;

    // Dorsal fin trails the body motion slightly
    float phaseDelay = PI * 0.25f;
    float amplitude = m_config.dorsalFinAmp * speedFactor;
    float waveValue = std::sin(phase - phaseDelay) * amplitude;

    // Dorsal fin rotates around local Z axis
    return glm::angleAxis(waveValue, glm::vec3(0.0f, 0.0f, 1.0f));
}

void SwimAnimator::reset() {
    m_state = SwimState();
}

void SwimAnimator::applyToSkeleton(
    std::vector<glm::quat>& boneRotations,
    const std::vector<int>& spineBoneIndices,
    int tailFinIndex,
    int leftPectoralIndex,
    int rightPectoralIndex,
    int dorsalIndex
) const {
    // Apply spine wave
    auto spineRotations = calculateSpineWave(spineBoneIndices);
    for (size_t i = 0; i < spineBoneIndices.size() && i < spineRotations.size(); ++i) {
        int boneIdx = spineBoneIndices[i];
        if (boneIdx >= 0 && boneIdx < static_cast<int>(boneRotations.size())) {
            boneRotations[boneIdx] = boneRotations[boneIdx] * spineRotations[i];
        }
    }

    // Apply tail fin motion
    if (tailFinIndex >= 0 && tailFinIndex < static_cast<int>(boneRotations.size())) {
        boneRotations[tailFinIndex] = boneRotations[tailFinIndex] * calculateTailFinMotion();
    }

    // Apply pectoral fin motion
    auto [leftPectoral, rightPectoral] = calculatePectoralFinMotion();
    if (leftPectoralIndex >= 0 && leftPectoralIndex < static_cast<int>(boneRotations.size())) {
        boneRotations[leftPectoralIndex] = boneRotations[leftPectoralIndex] * leftPectoral;
    }
    if (rightPectoralIndex >= 0 && rightPectoralIndex < static_cast<int>(boneRotations.size())) {
        boneRotations[rightPectoralIndex] = boneRotations[rightPectoralIndex] * rightPectoral;
    }

    // Apply dorsal fin motion
    if (dorsalIndex >= 0 && dorsalIndex < static_cast<int>(boneRotations.size())) {
        boneRotations[dorsalIndex] = boneRotations[dorsalIndex] * calculateDorsalFinMotion();
    }
}

// Factory function to create swim config from genome traits
SwimConfig createSwimConfigFromGenome(
    float swimFrequency,
    float swimAmplitude,
    float finSize,
    float tailSize,
    float creatureSize
) {
    SwimConfig config;

    // Map genome values to animation parameters
    config.bodyWaveSpeed = swimFrequency;
    config.bodyWaveAmp = swimAmplitude;

    // Larger tail = more thrust = faster wave
    config.caudalFinAmp = 0.2f + tailSize * 0.1f;

    // Fin size affects steering capability
    config.pectoralFinAmp = 0.15f + finSize * 0.1f;
    config.dorsalFinAmp = 0.05f + finSize * 0.05f;

    // Creature size affects wavelength
    config.wavelength = 0.8f + creatureSize * 0.2f;

    // Determine swimming style based on tail/body ratio
    float tailRatio = tailSize / creatureSize;
    if (tailRatio > 0.8f) {
        config.style = SwimStyle::THUNNIFORM;  // Strong tail, minimal body motion
    } else if (tailRatio > 0.5f) {
        config.style = SwimStyle::CARANGIFORM; // Balanced
    } else {
        config.style = SwimStyle::SUBCARANGIFORM; // More body involvement
    }

    // Randomize phase offset for variety
    config.phaseOffset = static_cast<float>(rand()) / RAND_MAX * 6.28318f;

    return config;
}

// =============================================================================
// JELLYFISH ANIMATION
// =============================================================================

float SwimAnimator::calculateBellContraction() const {
    // Bell contracts quickly, recovers slowly (asymmetric motion)
    float phase = m_state.pulsePhase;
    float recovery = m_config.recoveryTime;

    if (phase < (1.0f - recovery)) {
        // Contraction phase - quick squeeze
        float t = phase / (1.0f - recovery);
        // Use ease-out for snappy contraction
        return m_config.pulseAmplitude * (1.0f - (1.0f - t) * (1.0f - t));
    } else {
        // Recovery phase - slow expansion
        float t = (phase - (1.0f - recovery)) / recovery;
        // Use ease-in-out for smooth recovery
        float eased = t * t * (3.0f - 2.0f * t);
        return m_config.pulseAmplitude * (1.0f - eased);
    }
}

void SwimAnimator::updateJellyfishPulse(float deltaTime) {
    // Update pulse phase
    m_state.pulsePhase += deltaTime * m_config.pulseFrequency;
    if (m_state.pulsePhase >= 1.0f) {
        m_state.pulsePhase -= 1.0f;
    }

    // Track contraction state
    m_state.isContracting = m_state.pulsePhase < (1.0f - m_config.recoveryTime);
}

std::vector<std::vector<glm::quat>> SwimAnimator::calculateTentacleMotion(
    const std::vector<TentacleConfig>& tentacles,
    const glm::vec3& bodyVelocity
) const {
    std::vector<std::vector<glm::quat>> result;
    result.reserve(tentacles.size());

    float bellContraction = calculateBellContraction();
    float phase = m_state.swimPhase;

    for (size_t t = 0; t < tentacles.size(); ++t) {
        const auto& tentacle = tentacles[t];
        std::vector<glm::quat> segmentRotations;
        segmentRotations.reserve(tentacle.segmentCount);

        // Each tentacle has slight phase offset for organic look
        float tentaclePhaseOffset = static_cast<float>(t) * (TWO_PI / tentacles.size());

        for (int s = 0; s < tentacle.segmentCount; ++s) {
            float segmentPos = static_cast<float>(s) / static_cast<float>(tentacle.segmentCount - 1);

            // Tentacles trail behind during contraction
            float trailDelay = segmentPos * tentacle.drag * 0.5f;
            float delayedPhase = phase - trailDelay + tentaclePhaseOffset;

            // Base sway from water movement
            float swayX = std::sin(delayedPhase * 0.7f) * 0.1f * (1.0f + segmentPos);
            float swayZ = std::cos(delayedPhase * 0.5f) * 0.08f * (1.0f + segmentPos);

            // During contraction, tentacles spread outward then collapse
            float contractionInfluence = bellContraction * 0.3f * segmentPos;

            // Velocity-based trailing
            glm::vec3 drag = calculateTentacleDrag(bodyVelocity, tentacle.drag);
            float velocityTrail = glm::length(drag) * segmentPos * 0.2f;

            // Combine into rotation
            glm::quat rotX = glm::angleAxis(swayX + contractionInfluence, glm::vec3(1.0f, 0.0f, 0.0f));
            glm::quat rotZ = glm::angleAxis(swayZ + velocityTrail, glm::vec3(0.0f, 0.0f, 1.0f));

            segmentRotations.push_back(rotX * rotZ);
        }

        result.push_back(std::move(segmentRotations));
    }

    return result;
}

float SwimAnimator::calculatePulseWave(float phase) const {
    // Asymmetric pulse wave - fast contraction, slow recovery
    float recoveryStart = 1.0f - m_config.recoveryTime;

    if (phase < recoveryStart) {
        // Contraction: ease-out curve
        float t = phase / recoveryStart;
        return 1.0f - std::pow(1.0f - t, 2.0f);
    } else {
        // Recovery: ease-in-out curve
        float t = (phase - recoveryStart) / m_config.recoveryTime;
        return 1.0f - (t * t * (3.0f - 2.0f * t));
    }
}

glm::vec3 SwimAnimator::calculateTentacleDrag(const glm::vec3& velocity, float dragCoeff) const {
    // Calculate drag force on tentacle
    float speed = glm::length(velocity);
    if (speed < 0.001f) {
        return glm::vec3(0.0f);
    }

    glm::vec3 direction = glm::normalize(velocity);
    float dragMagnitude = speed * speed * dragCoeff * 0.5f;
    return -direction * dragMagnitude;
}

// =============================================================================
// EEL/SERPENTINE ANIMATION
// =============================================================================

std::vector<glm::quat> SwimAnimator::calculateSerpentineWave(
    const std::vector<int>& spineBones,
    float waveAmplitude,
    float waveFrequency
) const {
    std::vector<glm::quat> rotations;
    rotations.reserve(spineBones.size());

    int numBones = static_cast<int>(spineBones.size());
    if (numBones == 0) {
        return rotations;
    }

    float speedFactor = getSpeedFactor();
    float phase = m_state.swimPhase * waveFrequency;

    for (int i = 0; i < numBones; ++i) {
        float bodyPos = static_cast<float>(i) / static_cast<float>(numBones - 1);

        // Full-body wave (anguilliform motion)
        float waveValue = calculateEelWave(bodyPos, phase);

        // Apply amplitude with speed scaling
        float amplitude = waveAmplitude * speedFactor;

        // Head is slightly dampened
        float headDamping = 1.0f - std::exp(-bodyPos * 3.0f) * 0.3f;
        amplitude *= headDamping;

        // Add turn influence
        float turnInfluence = m_state.turnAmount * 0.2f * bodyPos;

        float yawAngle = waveValue * amplitude + turnInfluence;

        // Create yaw rotation (side-to-side)
        glm::quat yaw = glm::angleAxis(yawAngle, glm::vec3(0.0f, 1.0f, 0.0f));

        // Add subtle roll for more organic motion
        float rollAngle = std::sin(phase + bodyPos * PI) * amplitude * 0.2f;
        glm::quat roll = glm::angleAxis(rollAngle, glm::vec3(0.0f, 0.0f, 1.0f));

        rotations.push_back(roll * yaw);
    }

    return rotations;
}

float SwimAnimator::calculateEelWave(float bodyPos, float phase) const {
    // Eel wave travels from head to tail with increasing amplitude
    float phaseOffset = bodyPos * m_config.wavelength * TWO_PI;

    // Main wave
    float wave = std::sin(phase - phaseOffset);

    // Add secondary harmonic for more organic feel
    float harmonic = std::sin(2.0f * (phase - phaseOffset)) * 0.15f;

    return wave + harmonic;
}

// =============================================================================
// CRAB/CRUSTACEAN ANIMATION
// =============================================================================

std::vector<glm::quat> SwimAnimator::calculateCrabLegMotion(int legCount) const {
    std::vector<glm::quat> rotations;
    rotations.reserve(legCount);

    float speedFactor = getSpeedFactor();
    float phase = m_state.swimPhase * m_config.legPaddleFreq;

    for (int i = 0; i < legCount; ++i) {
        float legPhase = calculateLegPhase(i, legCount);
        float paddlePhase = phase + legPhase;

        // Legs paddle in alternating pattern
        // Power stroke (backward) is faster than recovery stroke (forward)
        float powerStroke = std::sin(paddlePhase);
        float asymmetry = powerStroke > 0.0f ? 1.2f : 0.8f;

        float paddleAngle = powerStroke * asymmetry * 0.5f * speedFactor;

        // Legs also move up/down during swimming
        float liftAngle = std::cos(paddlePhase) * 0.3f * speedFactor;

        glm::quat paddle = glm::angleAxis(paddleAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat lift = glm::angleAxis(liftAngle, glm::vec3(1.0f, 0.0f, 0.0f));

        rotations.push_back(lift * paddle);
    }

    return rotations;
}

glm::quat SwimAnimator::calculateCrabBodyOrientation() const {
    // Crabs swim sideways
    glm::quat sideways = glm::angleAxis(m_config.sidewaysAngle, glm::vec3(0.0f, 1.0f, 0.0f));

    // Add subtle roll based on leg motion
    float roll = std::sin(m_state.swimPhase * m_config.legPaddleFreq * 0.5f) * 0.05f;
    glm::quat rollQuat = glm::angleAxis(roll, glm::vec3(0.0f, 0.0f, 1.0f));

    // Pitch forward slightly when moving fast
    float pitch = m_state.currentSpeed * 0.1f;
    glm::quat pitchQuat = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));

    return pitchQuat * rollQuat * sideways;
}

float SwimAnimator::calculateLegPhase(int legIndex, int totalLegs) const {
    // Alternating tripod-like pattern for crabs
    // Left and right sides are out of phase
    bool isRightSide = legIndex >= (totalLegs / 2);
    int sideIndex = isRightSide ? legIndex - (totalLegs / 2) : legIndex;

    // Adjacent legs are out of phase
    float basePhase = static_cast<float>(sideIndex) * PI / 3.0f;

    // Opposite sides are half-cycle offset
    if (isRightSide) {
        basePhase += PI;
    }

    return basePhase;
}

// =============================================================================
// RAY/MANTA ANIMATION
// =============================================================================

std::pair<std::vector<glm::quat>, std::vector<glm::quat>> SwimAnimator::calculateRayWingMotion(
    int leftWingBones,
    int rightWingBones
) const {
    std::vector<glm::quat> leftRotations;
    std::vector<glm::quat> rightRotations;
    leftRotations.reserve(leftWingBones);
    rightRotations.reserve(rightWingBones);

    float speedFactor = getSpeedFactor();
    float phase = m_state.swimPhase;

    // Left wing - wave travels from body to tip
    for (int i = 0; i < leftWingBones; ++i) {
        float wingPos = static_cast<float>(i) / static_cast<float>(leftWingBones - 1);

        // Wave amplitude increases toward tip
        float amplitude = m_config.bodyWaveAmp * (0.3f + wingPos * 0.7f) * speedFactor;

        // Phase offset creates traveling wave
        float phaseOffset = wingPos * PI * 0.8f;

        // Main flap motion
        float flapAngle = std::sin(phase - phaseOffset) * amplitude;

        // Add slight twist at the tips
        float twistAngle = std::sin(phase - phaseOffset + HALF_PI) * amplitude * 0.3f * wingPos;

        // Turn influence - banking
        flapAngle += m_state.turnAmount * 0.2f * wingPos;

        glm::quat flap = glm::angleAxis(flapAngle, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::quat twist = glm::angleAxis(twistAngle, glm::vec3(1.0f, 0.0f, 0.0f));

        leftRotations.push_back(twist * flap);
    }

    // Right wing - mirror of left with phase offset for bilateral motion
    for (int i = 0; i < rightWingBones; ++i) {
        float wingPos = static_cast<float>(i) / static_cast<float>(rightWingBones - 1);

        float amplitude = m_config.bodyWaveAmp * (0.3f + wingPos * 0.7f) * speedFactor;
        float phaseOffset = wingPos * PI * 0.8f;

        // Mirror the wave direction
        float flapAngle = std::sin(phase - phaseOffset) * amplitude;
        float twistAngle = std::sin(phase - phaseOffset + HALF_PI) * amplitude * 0.3f * wingPos;

        // Opposite turn influence
        flapAngle -= m_state.turnAmount * 0.2f * wingPos;

        // Mirror the rotation direction
        glm::quat flap = glm::angleAxis(-flapAngle, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::quat twist = glm::angleAxis(-twistAngle, glm::vec3(1.0f, 0.0f, 0.0f));

        rightRotations.push_back(twist * flap);
    }

    return {leftRotations, rightRotations};
}

// =============================================================================
// SEAHORSE ANIMATION
// =============================================================================

std::vector<glm::quat> SwimAnimator::calculateSeahorseDorsalWave(int finBoneCount) const {
    std::vector<glm::quat> rotations;
    rotations.reserve(finBoneCount);

    float speedFactor = getSpeedFactor();
    // Seahorse dorsal fin beats very rapidly
    float phase = m_state.swimPhase * 3.0f;

    for (int i = 0; i < finBoneCount; ++i) {
        float finPos = static_cast<float>(i) / static_cast<float>(finBoneCount - 1);

        // Traveling wave along the fin
        float phaseOffset = finPos * TWO_PI;
        float waveAngle = std::sin(phase - phaseOffset) * m_config.dorsalFinAmp * speedFactor;

        // Slight forward/backward tilt based on position
        float tiltAngle = std::cos(phase - phaseOffset) * m_config.dorsalFinAmp * 0.3f * speedFactor;

        glm::quat wave = glm::angleAxis(waveAngle, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::quat tilt = glm::angleAxis(tiltAngle, glm::vec3(1.0f, 0.0f, 0.0f));

        rotations.push_back(tilt * wave);
    }

    return rotations;
}

std::vector<glm::quat> SwimAnimator::calculateSeahorseTailCurl(
    const std::vector<int>& tailBones,
    float curlTarget
) const {
    std::vector<glm::quat> rotations;
    rotations.reserve(tailBones.size());

    int numBones = static_cast<int>(tailBones.size());

    for (int i = 0; i < numBones; ++i) {
        float tailPos = static_cast<float>(i) / static_cast<float>(numBones - 1);

        // Curl increases toward tip
        float curlAmount = curlTarget * tailPos * tailPos;

        // Add gentle sway
        float sway = std::sin(m_state.swimPhase * 0.5f + tailPos * PI) * 0.05f;

        // Seahorse tail curls inward (pitch rotation)
        glm::quat curl = glm::angleAxis(curlAmount + sway, glm::vec3(1.0f, 0.0f, 0.0f));

        // Slight twist
        float twist = std::sin(m_state.swimPhase * 0.3f) * 0.02f * tailPos;
        glm::quat twistQuat = glm::angleAxis(twist, glm::vec3(0.0f, 0.0f, 1.0f));

        rotations.push_back(twistQuat * curl);
    }

    return rotations;
}

// =============================================================================
// OCTOPUS ANIMATION
// =============================================================================

float SwimAnimator::calculateOctopusMantle() const {
    // Mantle contracts for jet propulsion
    float phase = m_state.pulsePhase;

    // Similar to jellyfish but with different timing
    float contractionPhase = 0.3f;  // 30% of cycle is contraction

    if (phase < contractionPhase) {
        // Quick contraction
        float t = phase / contractionPhase;
        return m_config.pulseAmplitude * std::pow(t, 0.5f);
    } else {
        // Slow recovery with water intake
        float t = (phase - contractionPhase) / (1.0f - contractionPhase);
        float eased = 1.0f - std::pow(1.0f - t, 2.0f);
        return m_config.pulseAmplitude * (1.0f - eased);
    }
}

std::vector<std::vector<glm::quat>> SwimAnimator::calculateOctopusArms(
    const std::vector<TentacleConfig>& arms,
    const glm::vec3& targetDirection
) const {
    std::vector<std::vector<glm::quat>> result;
    result.reserve(arms.size());

    float mantleContraction = calculateOctopusMantle();
    float phase = m_state.swimPhase;

    for (size_t a = 0; a < arms.size(); ++a) {
        const auto& arm = arms[a];
        std::vector<glm::quat> segmentRotations;
        segmentRotations.reserve(arm.segmentCount);

        // Each arm has unique phase and behavior
        float armAngle = static_cast<float>(a) * TWO_PI / static_cast<float>(arms.size());
        float armPhaseOffset = armAngle * 0.5f;

        // Calculate how much this arm should contribute to steering
        float steerContribution = 0.0f;
        if (glm::length(targetDirection) > 0.001f) {
            glm::vec3 armDir(std::cos(armAngle), 0.0f, std::sin(armAngle));
            steerContribution = glm::dot(glm::normalize(targetDirection), armDir);
        }

        for (int s = 0; s < arm.segmentCount; ++s) {
            float segmentPos = static_cast<float>(s) / static_cast<float>(arm.segmentCount - 1);

            // Base undulation
            float undulatePhase = phase + armPhaseOffset - segmentPos * PI;
            float undulateX = std::sin(undulatePhase) * 0.15f * (0.5f + segmentPos * 0.5f);
            float undulateZ = std::cos(undulatePhase * 0.7f) * 0.1f * (0.5f + segmentPos * 0.5f);

            // During jet propulsion, arms stream behind
            float jetTrail = mantleContraction * segmentPos * 0.5f;

            // Steering influence
            float steerAngle = steerContribution * 0.3f * segmentPos;

            // Random personality wiggle (use arm index as seed)
            float wiggle = std::sin(phase * 1.3f + static_cast<float>(a) * 0.7f) * 0.03f * segmentPos;

            glm::quat rotX = glm::angleAxis(undulateX + jetTrail, glm::vec3(1.0f, 0.0f, 0.0f));
            glm::quat rotY = glm::angleAxis(steerAngle + wiggle, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::quat rotZ = glm::angleAxis(undulateZ, glm::vec3(0.0f, 0.0f, 1.0f));

            segmentRotations.push_back(rotX * rotY * rotZ);
        }

        result.push_back(std::move(segmentRotations));
    }

    return result;
}

// =============================================================================
// ENVIRONMENT EFFECTS
// =============================================================================

void SwimAnimator::setWaterDepth(float depth) {
    m_state.currentDepth = depth;

    // Pressure increases with depth, compressing air bladders
    // This affects buoyancy and movement
    m_state.pressureCompensation = 1.0f / (1.0f + depth * 0.01f);
}

void SwimAnimator::setWaterCurrent(const glm::vec3& current) {
    m_waterCurrent = current;
}

glm::vec3 SwimAnimator::getBuoyancyOffset() const {
    // Gentle vertical bob
    float bob = std::sin(m_state.buoyancyPhase) * m_config.buoyancyAmplitude;

    // Add current influence
    glm::vec3 offset(0.0f, bob + m_config.buoyancyOffset, 0.0f);
    offset += m_waterCurrent * 0.1f;

    return offset;
}

// =============================================================================
// MORPHOLOGY SWIM CONTROLLER
// =============================================================================

MorphologySwimController::MorphologySwimController() = default;

void MorphologySwimController::initializeFromMorphology(const MorphologyGenes& genes) {
    // Determine swim style from body plan
    m_activeStyle = determineSwimStyle(genes);

    // Extract morphology data
    m_hasFins = genes.finCount > 0;
    m_hasTail = genes.hasTail;
    m_hasTentacles = genes.tentacleCount > 0;
    m_finCount = genes.finCount;
    m_tentacleCount = genes.tentacleCount;
    m_bodyLength = genes.bodyLength;

    // Configure animator based on style
    SwimConfig config;

    switch (m_activeStyle) {
        case SwimStyle::CARANGIFORM:
            config = SwimPresets::standardFish();
            break;
        case SwimStyle::THUNNIFORM:
            config = SwimPresets::pelagicFish();
            break;
        case SwimStyle::ANGUILLIFORM:
            config = SwimPresets::eelLike();
            break;
        case SwimStyle::RAJIFORM:
            config = SwimPresets::rayLike();
            break;
        case SwimStyle::JELLYFISH:
            config = SwimPresets::jellyfish();
            break;
        case SwimStyle::OCTOPOD:
            config = SwimPresets::octopus();
            break;
        case SwimStyle::CRUSTACEAN:
            config = SwimPresets::crustacean();
            break;
        case SwimStyle::SEAHORSE:
            config = SwimPresets::seahorse();
            break;
        default:
            config = SwimPresets::standardFish();
            break;
    }

    // Scale parameters by body size
    config.bodyWaveAmp *= (1.0f / m_bodyLength);  // Smaller creatures flex more
    config.wavelength *= m_bodyLength;

    // Add individual variation
    config.phaseOffset = static_cast<float>(rand()) / RAND_MAX * TWO_PI;

    m_animator.setConfig(config);
}

SwimStyle MorphologySwimController::determineSwimStyle(const MorphologyGenes& genes) const {
    // Determine best swim style based on body plan

    // No body, just tentacles = jellyfish
    if (genes.bodySegments == 0 && genes.tentacleCount > 0) {
        return SwimStyle::JELLYFISH;
    }

    // Many tentacles with body = octopus
    if (genes.tentacleCount >= 6) {
        return SwimStyle::OCTOPOD;
    }

    // Legs and exoskeleton = crustacean
    if (genes.legCount > 0 && genes.hasExoskeleton) {
        return SwimStyle::CRUSTACEAN;
    }

    // Very long body with no limbs = eel
    float aspectRatio = genes.bodyLength / std::max(genes.bodyWidth, 0.01f);
    if (aspectRatio > 8.0f && genes.legCount == 0) {
        return SwimStyle::ANGUILLIFORM;
    }

    // Wide, flat body = ray
    if (genes.bodyWidth > genes.bodyLength * 1.5f) {
        return SwimStyle::RAJIFORM;
    }

    // Small with prehensile tail = seahorse
    if (genes.hasPrehensibleTail && genes.bodyLength < 0.3f) {
        return SwimStyle::SEAHORSE;
    }

    // Strong tail, streamlined = thunniform
    if (genes.tailStrength > 0.8f && genes.bodyStreamline > 0.7f) {
        return SwimStyle::THUNNIFORM;
    }

    // Default to standard fish motion
    return SwimStyle::CARANGIFORM;
}

void MorphologySwimController::setVelocity(const glm::vec3& velocity) {
    m_velocity = velocity;
}

void MorphologySwimController::setTargetDirection(const glm::vec3& direction) {
    m_targetDirection = direction;
}

void MorphologySwimController::setMaxSpeed(float maxSpeed) {
    m_maxSpeed = maxSpeed;
}

void MorphologySwimController::setWaterDepth(float depth) {
    m_waterDepth = depth;
    m_animator.setWaterDepth(depth);
}

void MorphologySwimController::setWaterCurrent(const glm::vec3& current) {
    m_waterCurrent = current;
    m_animator.setWaterCurrent(current);
}

void MorphologySwimController::setWaterTemperature(float temp) {
    m_waterTemp = temp;
}

void MorphologySwimController::setWaterViscosity(float viscosity) {
    m_waterViscosity = viscosity;
}

void MorphologySwimController::update(float deltaTime) {
    // Temperature affects metabolism and swim speed
    float tempFactor = 1.0f + (m_waterTemp - 20.0f) * 0.02f;
    tempFactor = std::clamp(tempFactor, 0.5f, 1.5f);

    // Viscosity affects drag
    float viscosityFactor = 1.0f / m_waterViscosity;

    // Scale velocity by environmental factors
    glm::vec3 effectiveVelocity = m_velocity * tempFactor * viscosityFactor;

    // Update base animator
    m_animator.update(deltaTime, effectiveVelocity, m_maxSpeed);

    // Update style-specific animations
    if (m_activeStyle == SwimStyle::JELLYFISH || m_activeStyle == SwimStyle::OCTOPOD) {
        m_animator.updateJellyfishPulse(deltaTime);
    }

    // Calculate body offset from buoyancy
    m_bodyOffset = m_animator.getBuoyancyOffset();

    // Calculate body tilt based on velocity
    if (glm::length(m_velocity) > 0.1f) {
        glm::vec3 forward(0.0f, 0.0f, 1.0f);
        glm::vec3 velocityDir = glm::normalize(m_velocity);

        // Pitch based on vertical velocity component
        float pitch = std::asin(std::clamp(velocityDir.y, -1.0f, 1.0f)) * 0.5f;

        // Yaw based on horizontal direction
        float yaw = std::atan2(velocityDir.x, velocityDir.z);

        m_bodyTilt = glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f)) *
                     glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    }
}

void MorphologySwimController::applyToBoneRotations(
    std::vector<glm::quat>& rotations,
    const std::vector<int>& spineBones,
    const std::vector<int>& finBones,
    const std::vector<std::vector<int>>& tentacleBones
) {
    switch (m_activeStyle) {
        case SwimStyle::ANGUILLIFORM: {
            auto spineRots = m_animator.calculateSerpentineWave(
                spineBones,
                m_animator.getConfig().bodyWaveAmp,
                m_animator.getConfig().wavelength
            );
            for (size_t i = 0; i < spineBones.size() && i < spineRots.size(); ++i) {
                int idx = spineBones[i];
                if (idx >= 0 && idx < static_cast<int>(rotations.size())) {
                    rotations[idx] = rotations[idx] * spineRots[i];
                }
            }
            break;
        }

        case SwimStyle::CRUSTACEAN: {
            // Apply crab body orientation
            if (!spineBones.empty() && spineBones[0] >= 0) {
                rotations[spineBones[0]] = rotations[spineBones[0]] *
                                          m_animator.calculateCrabBodyOrientation();
            }
            // Leg motion handled separately
            break;
        }

        case SwimStyle::JELLYFISH:
        case SwimStyle::OCTOPOD: {
            // Tentacle motion
            if (!tentacleBones.empty()) {
                std::vector<TentacleConfig> configs;
                for (const auto& tentacle : tentacleBones) {
                    TentacleConfig config;
                    config.segmentCount = static_cast<int>(tentacle.size());
                    configs.push_back(config);
                }

                auto tentacleRots = (m_activeStyle == SwimStyle::JELLYFISH) ?
                    m_animator.calculateTentacleMotion(configs, m_velocity) :
                    m_animator.calculateOctopusArms(configs, m_targetDirection);

                for (size_t t = 0; t < tentacleBones.size() && t < tentacleRots.size(); ++t) {
                    const auto& bones = tentacleBones[t];
                    const auto& rots = tentacleRots[t];
                    for (size_t s = 0; s < bones.size() && s < rots.size(); ++s) {
                        int idx = bones[s];
                        if (idx >= 0 && idx < static_cast<int>(rotations.size())) {
                            rotations[idx] = rotations[idx] * rots[s];
                        }
                    }
                }
            }
            break;
        }

        default: {
            // Standard fish spine wave
            auto spineRots = m_animator.calculateSpineWave(spineBones);
            for (size_t i = 0; i < spineBones.size() && i < spineRots.size(); ++i) {
                int idx = spineBones[i];
                if (idx >= 0 && idx < static_cast<int>(rotations.size())) {
                    rotations[idx] = rotations[idx] * spineRots[i];
                }
            }
            break;
        }
    }
}

float MorphologySwimController::getSwimEfficiency() const {
    // Calculate how efficiently the creature is swimming
    float speed = glm::length(m_velocity);
    if (speed < 0.001f) return 0.0f;

    // Efficiency decreases with current resistance
    float currentResistance = glm::dot(glm::normalize(m_velocity), glm::normalize(m_waterCurrent));
    currentResistance = (currentResistance + 1.0f) * 0.5f;  // Map -1,1 to 0,1

    // Temperature affects efficiency
    float tempEfficiency = 1.0f - std::abs(m_waterTemp - 20.0f) * 0.02f;
    tempEfficiency = std::clamp(tempEfficiency, 0.5f, 1.0f);

    // Speed relative to max affects efficiency (optimal around 60-80%)
    float speedRatio = speed / m_maxSpeed;
    float speedEfficiency = 1.0f - std::abs(speedRatio - 0.7f) * 0.5f;

    return currentResistance * tempEfficiency * speedEfficiency;
}

glm::vec3 MorphologySwimController::getBodyOffset() const {
    return m_bodyOffset;
}

glm::quat MorphologySwimController::getBodyTilt() const {
    return m_bodyTilt;
}

// =============================================================================
// SWIM PRESETS
// =============================================================================

namespace SwimPresets {

SwimConfig standardFish() {
    SwimConfig config;
    config.style = SwimStyle::CARANGIFORM;
    config.bodyWaveSpeed = 3.0f;
    config.bodyWaveAmp = 0.15f;
    config.wavelength = 1.0f;
    config.headStiffness = 0.85f;
    config.midStiffness = 0.5f;
    config.tailStiffness = 0.15f;
    config.dorsalFinAmp = 0.1f;
    config.pectoralFinAmp = 0.2f;
    config.caudalFinAmp = 0.25f;
    return config;
}

SwimConfig pelagicFish() {
    SwimConfig config;
    config.style = SwimStyle::THUNNIFORM;
    config.bodyWaveSpeed = 4.0f;
    config.bodyWaveAmp = 0.1f;
    config.wavelength = 0.5f;
    config.headStiffness = 0.95f;
    config.midStiffness = 0.9f;
    config.tailStiffness = 0.2f;
    config.dorsalFinAmp = 0.05f;
    config.pectoralFinAmp = 0.15f;
    config.caudalFinAmp = 0.4f;
    config.minSpeedFactor = 0.5f;
    config.maxSpeedFactor = 2.0f;
    return config;
}

SwimConfig eelLike() {
    SwimConfig config;
    config.style = SwimStyle::ANGUILLIFORM;
    config.bodyWaveSpeed = 2.0f;
    config.bodyWaveAmp = 0.25f;
    config.wavelength = 1.5f;
    config.headStiffness = 0.6f;
    config.midStiffness = 0.3f;
    config.tailStiffness = 0.1f;
    config.dorsalFinAmp = 0.15f;
    config.pectoralFinAmp = 0.1f;
    config.caudalFinAmp = 0.2f;
    return config;
}

SwimConfig rayLike() {
    SwimConfig config;
    config.style = SwimStyle::RAJIFORM;
    config.bodyWaveSpeed = 1.5f;
    config.bodyWaveAmp = 0.3f;
    config.wavelength = 0.8f;
    config.headStiffness = 0.9f;
    config.midStiffness = 0.7f;
    config.tailStiffness = 0.3f;
    config.dorsalFinAmp = 0.05f;
    config.pectoralFinAmp = 0.4f;  // Large pectoral "wings"
    config.caudalFinAmp = 0.1f;
    return config;
}

SwimConfig jellyfish() {
    SwimConfig config;
    config.style = SwimStyle::JELLYFISH;
    config.bodyWaveSpeed = 0.5f;
    config.bodyWaveAmp = 0.05f;
    config.pulseFrequency = 1.2f;
    config.pulseAmplitude = 0.4f;
    config.recoveryTime = 0.6f;
    config.tentacleCount = 8;
    config.tentacleDrag = 0.85f;
    config.buoyancyAmplitude = 0.03f;
    return config;
}

SwimConfig octopus() {
    SwimConfig config;
    config.style = SwimStyle::OCTOPOD;
    config.bodyWaveSpeed = 1.0f;
    config.bodyWaveAmp = 0.1f;
    config.pulseFrequency = 2.0f;
    config.pulseAmplitude = 0.35f;
    config.recoveryTime = 0.5f;
    config.tentacleCount = 8;
    config.tentacleDrag = 0.7f;
    return config;
}

SwimConfig crustacean() {
    SwimConfig config;
    config.style = SwimStyle::CRUSTACEAN;
    config.bodyWaveSpeed = 2.0f;
    config.bodyWaveAmp = 0.05f;
    config.legPaddleFreq = 4.0f;
    config.sidewaysAngle = HALF_PI;
    config.headStiffness = 0.95f;
    config.midStiffness = 0.9f;
    config.tailStiffness = 0.4f;
    return config;
}

SwimConfig seahorse() {
    SwimConfig config;
    config.style = SwimStyle::SEAHORSE;
    config.bodyWaveSpeed = 0.5f;
    config.bodyWaveAmp = 0.02f;
    config.dorsalFinAmp = 0.25f;  // Main propulsion
    config.headStiffness = 0.95f;
    config.midStiffness = 0.8f;
    config.tailStiffness = 0.2f;  // Prehensile tail
    config.buoyancyAmplitude = 0.01f;
    return config;
}

SwimConfig flatfish() {
    SwimConfig config;
    config.style = SwimStyle::FLATFISH;
    config.bodyWaveSpeed = 2.5f;
    config.bodyWaveAmp = 0.12f;
    config.wavelength = 1.2f;
    config.headStiffness = 0.9f;
    config.midStiffness = 0.4f;
    config.tailStiffness = 0.15f;
    config.dorsalFinAmp = 0.15f;
    return config;
}

SwimConfig cetacean() {
    SwimConfig config;
    config.style = SwimStyle::THUNNIFORM;
    config.bodyWaveSpeed = 2.0f;
    config.bodyWaveAmp = 0.08f;
    config.wavelength = 0.3f;
    config.headStiffness = 0.98f;
    config.midStiffness = 0.95f;
    config.tailStiffness = 0.3f;
    config.caudalFinAmp = 0.5f;  // Powerful tail
    // Note: Cetaceans move tail up/down, not side-to-side
    // This would require additional handling in the animation
    return config;
}

SwimConfig seaTurtle() {
    SwimConfig config;
    config.style = SwimStyle::LABRIFORM;
    config.bodyWaveSpeed = 1.0f;
    config.bodyWaveAmp = 0.02f;
    config.pectoralFinAmp = 0.5f;  // Main propulsion via flippers
    config.headStiffness = 0.95f;
    config.midStiffness = 0.98f;  // Rigid shell
    config.tailStiffness = 0.6f;
    return config;
}

SwimConfig seaSnake() {
    SwimConfig config;
    config.style = SwimStyle::ANGUILLIFORM;
    config.bodyWaveSpeed = 3.0f;
    config.bodyWaveAmp = 0.2f;
    config.wavelength = 2.0f;
    config.headStiffness = 0.7f;
    config.midStiffness = 0.2f;
    config.tailStiffness = 0.1f;
    return config;
}

}  // namespace SwimPresets

// =============================================================================
// SWIM BEHAVIORS
// =============================================================================

glm::vec3 SwimBehaviors::calculateSchoolingVelocity(
    const glm::vec3& myPosition,
    const glm::vec3& myVelocity,
    const std::vector<glm::vec3>& neighborPositions,
    const std::vector<glm::vec3>& neighborVelocities,
    float separationDist,
    float alignmentDist,
    float cohesionDist
) {
    glm::vec3 separation(0.0f);
    glm::vec3 alignment(0.0f);
    glm::vec3 cohesion(0.0f);

    int separationCount = 0;
    int alignmentCount = 0;
    int cohesionCount = 0;

    for (size_t i = 0; i < neighborPositions.size(); ++i) {
        glm::vec3 diff = myPosition - neighborPositions[i];
        float dist = glm::length(diff);

        if (dist < 0.001f) continue;  // Skip self

        // Separation: steer away from close neighbors
        if (dist < separationDist) {
            separation += glm::normalize(diff) / dist;
            separationCount++;
        }

        // Alignment: match velocity of nearby neighbors
        if (dist < alignmentDist) {
            alignment += neighborVelocities[i];
            alignmentCount++;
        }

        // Cohesion: steer toward center of flock
        if (dist < cohesionDist) {
            cohesion += neighborPositions[i];
            cohesionCount++;
        }
    }

    glm::vec3 result(0.0f);

    if (separationCount > 0) {
        separation /= static_cast<float>(separationCount);
        result += separation * 1.5f;  // Separation is highest priority
    }

    if (alignmentCount > 0) {
        alignment /= static_cast<float>(alignmentCount);
        result += (alignment - myVelocity) * 1.0f;
    }

    if (cohesionCount > 0) {
        cohesion /= static_cast<float>(cohesionCount);
        glm::vec3 toCenter = cohesion - myPosition;
        result += toCenter * 0.5f;
    }

    return result;
}

glm::vec3 SwimBehaviors::calculateFleeDirection(
    const glm::vec3& myPosition,
    const glm::vec3& predatorPosition,
    float fleeDistance
) {
    glm::vec3 diff = myPosition - predatorPosition;
    float dist = glm::length(diff);

    if (dist >= fleeDistance || dist < 0.001f) {
        return glm::vec3(0.0f);
    }

    // Flee strength inversely proportional to distance
    float strength = (fleeDistance - dist) / fleeDistance;
    strength = strength * strength;  // Exponential increase as predator gets closer

    return glm::normalize(diff) * strength;
}

glm::vec3 SwimBehaviors::calculatePursuitDirection(
    const glm::vec3& myPosition,
    const glm::vec3& myVelocity,
    const glm::vec3& preyPosition,
    const glm::vec3& preyVelocity,
    float maxPredictionTime
) {
    glm::vec3 toPrey = preyPosition - myPosition;
    float dist = glm::length(toPrey);

    if (dist < 0.001f) {
        return glm::vec3(0.0f);
    }

    // Predict where prey will be
    float mySpeed = glm::length(myVelocity);
    float predictionTime = dist / std::max(mySpeed, 1.0f);
    predictionTime = std::min(predictionTime, maxPredictionTime);

    glm::vec3 predictedPos = preyPosition + preyVelocity * predictionTime;
    glm::vec3 toPredicted = predictedPos - myPosition;

    return glm::length(toPredicted) > 0.001f ? glm::normalize(toPredicted) : glm::vec3(0.0f);
}

float SwimBehaviors::calculateDepthAdjustment(
    float currentDepth,
    float targetDepth,
    float maxVerticalSpeed
) {
    float diff = targetDepth - currentDepth;

    // Proportional control with saturation
    float adjustment = diff * 0.5f;
    adjustment = std::clamp(adjustment, -maxVerticalSpeed, maxVerticalSpeed);

    return adjustment;
}

bool SwimBehaviors::shouldSurface(float currentOxygen, float oxygenThreshold) {
    return currentOxygen < oxygenThreshold;
}

glm::vec3 SwimBehaviors::calculateBottomSearchPattern(
    const glm::vec3& position,
    float searchRadius,
    float time
) {
    // Lazy figure-8 pattern along the bottom
    float angle = time * 0.5f;
    float radius = searchRadius * (0.5f + 0.5f * std::sin(time * 0.3f));

    return glm::vec3(
        position.x + std::sin(angle) * radius,
        position.y,  // Stay at current depth
        position.z + std::sin(angle * 2.0f) * radius * 0.5f
    );
}

glm::vec3 SwimBehaviors::calculatePatrolPath(
    const glm::vec3& territoryCenter,
    float territoryRadius,
    float time,
    int waypointCount
) {
    // Move between waypoints around territory perimeter
    float angle = time * 0.2f;
    int currentWaypoint = static_cast<int>(angle / TWO_PI * waypointCount) % waypointCount;
    float waypointAngle = static_cast<float>(currentWaypoint) * TWO_PI / waypointCount;

    return glm::vec3(
        territoryCenter.x + std::cos(waypointAngle) * territoryRadius,
        territoryCenter.y,
        territoryCenter.z + std::sin(waypointAngle) * territoryRadius
    );
}

} // namespace animation
