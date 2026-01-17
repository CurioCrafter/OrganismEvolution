#include "GaitGenerator.h"
#include "../physics/Morphology.h"
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <map>

namespace animation {

// =============================================================================
// GAIT GENERATOR IMPLEMENTATION
// =============================================================================

GaitGenerator::GaitGenerator() {
    setupDefaultGaits();
}

void GaitGenerator::initialize(int legCount) {
    m_legs.resize(legCount);
    m_state.legs.resize(legCount);

    // Initialize default leg states
    for (int i = 0; i < legCount; ++i) {
        m_state.legs[i].currentTarget = glm::vec3(0);
        m_state.legs[i].plantedPosition = glm::vec3(0);
        m_state.legs[i].isPlanted = true;
        m_state.legs[i].isSwinging = false;
    }

    // Auto-select appropriate gait based on leg count
    if (legCount == 2) {
        m_state.currentGait = GaitPattern::BIPED_WALK;
        m_state.targetGait = GaitPattern::BIPED_WALK;
    } else if (legCount == 4) {
        m_state.currentGait = GaitPattern::QUADRUPED_WALK;
        m_state.targetGait = GaitPattern::QUADRUPED_WALK;
    } else if (legCount == 6) {
        m_state.currentGait = GaitPattern::HEXAPOD_TRIPOD;
        m_state.targetGait = GaitPattern::HEXAPOD_TRIPOD;
    } else if (legCount == 8) {
        m_state.currentGait = GaitPattern::OCTOPOD_WAVE;
        m_state.targetGait = GaitPattern::OCTOPOD_WAVE;
    } else if (legCount == 0) {
        m_state.currentGait = GaitPattern::SERPENTINE_LATERAL;
        m_state.targetGait = GaitPattern::SERPENTINE_LATERAL;
    }
}

void GaitGenerator::initializeFromMorphology(const MorphologyGenes& genes) {
    int legCount = genes.legPairs * 2;
    initialize(legCount);

    // Generate leg configurations from morphology
    auto legConfigs = MorphologyGaitMapping::generateLegConfigs(genes);
    for (size_t i = 0; i < legConfigs.size() && i < m_legs.size(); ++i) {
        m_legs[i] = legConfigs[i];
    }

    // Set up gait parameters scaled to creature size
    GaitPattern defaultGait = MorphologyGaitMapping::detectDefaultGait(genes);
    GaitParameters params = MorphologyGaitMapping::generateGaitParams(genes, defaultGait);
    setGaitParameters(defaultGait, params);

    m_state.currentGait = defaultGait;
    m_state.targetGait = defaultGait;
}

void GaitGenerator::setLeg(int index, const LegConfiguration& config) {
    if (index >= 0 && index < static_cast<int>(m_legs.size())) {
        m_legs[index] = config;
    }
}

void GaitGenerator::setGaitParameters(GaitPattern pattern, const GaitParameters& params) {
    m_gaits[pattern] = params;
}

void GaitGenerator::setVelocity(const glm::vec3& velocity) {
    m_velocity = velocity;
    m_state.speed = glm::length(velocity);
}

void GaitGenerator::setBodyTransform(const glm::vec3& position, const glm::quat& rotation) {
    m_bodyPosition = position;
    m_bodyRotation = rotation;
}

void GaitGenerator::setTargetSpeed(float speed) {
    m_state.targetSpeed = speed;

    // Auto-select gait based on speed
    GaitPattern bestGait = getBestGaitForSpeed(speed);
    if (bestGait != m_state.targetGait) {
        requestGait(bestGait);
    }
}

void GaitGenerator::setTurnRate(float rate) {
    m_state.turnRate = rate;
}

void GaitGenerator::requestGait(GaitPattern pattern) {
    if (pattern != m_state.currentGait && m_gaits.count(pattern) > 0) {
        m_state.targetGait = pattern;
        m_state.gaitBlend = 0.0f;
    }
}

void GaitGenerator::update(float deltaTime) {
    m_state.time += deltaTime;

    updateGaitTransition(deltaTime);
    updatePhase(deltaTime);
    updateLegStates(deltaTime);
    updateBodyMotion(deltaTime);
}

glm::vec3 GaitGenerator::getFootTarget(int legIndex) const {
    if (legIndex >= 0 && legIndex < static_cast<int>(m_state.legs.size())) {
        return m_state.legs[legIndex].currentTarget;
    }
    return glm::vec3(0);
}

float GaitGenerator::getFootBlendWeight(int legIndex) const {
    if (legIndex >= 0 && legIndex < static_cast<int>(m_state.legs.size())) {
        return m_state.legs[legIndex].blendWeight;
    }
    return 1.0f;
}

bool GaitGenerator::isFootGrounded(int legIndex) const {
    if (legIndex >= 0 && legIndex < static_cast<int>(m_state.legs.size())) {
        return m_state.legs[legIndex].isPlanted;
    }
    return true;
}

GaitPattern GaitGenerator::getBestGaitForSpeed(float speed) const {
    int legCount = static_cast<int>(m_legs.size());

    if (legCount == 2) {
        // Biped transitions
        if (speed < 2.0f) return GaitPattern::BIPED_WALK;
        return GaitPattern::BIPED_RUN;
    } else if (legCount == 4) {
        // Quadruped transitions
        if (speed < 1.5f) return GaitPattern::QUADRUPED_WALK;
        if (speed < 4.0f) return GaitPattern::QUADRUPED_TROT;
        return GaitPattern::QUADRUPED_GALLOP;
    } else if (legCount == 6) {
        if (speed < 1.0f) return GaitPattern::HEXAPOD_RIPPLE;
        return GaitPattern::HEXAPOD_TRIPOD;
    } else if (legCount == 8) {
        return GaitPattern::OCTOPOD_WAVE;
    }

    return m_state.currentGait;
}

// =============================================================================
// INTERNAL UPDATE METHODS
// =============================================================================

void GaitGenerator::updatePhase(float deltaTime) {
    if (m_state.speed < 0.01f) {
        return;  // Standing still
    }

    const GaitParameters& params = getCurrentGaitParams();

    // Adjust cycle time based on speed
    float speedFactor = m_state.speed / std::max(params.speedMax, 0.1f);
    speedFactor = glm::clamp(speedFactor, 0.3f, 2.0f);

    float adjustedCycleTime = params.cycleTime / speedFactor;
    adjustedCycleTime = glm::clamp(adjustedCycleTime, 0.2f, 3.0f);

    m_state.phase += deltaTime / adjustedCycleTime;
    while (m_state.phase >= 1.0f) {
        m_state.phase -= 1.0f;
    }
}

void GaitGenerator::updateGaitTransition(float deltaTime) {
    if (m_state.gaitBlend < 1.0f) {
        const GaitParameters& params = getTargetGaitParams();
        float blendSpeed = 1.0f / std::max(params.transitionTime, 0.1f);
        m_state.gaitBlend = std::min(1.0f, m_state.gaitBlend + deltaTime * blendSpeed);

        if (m_state.gaitBlend >= 1.0f) {
            m_state.currentGait = m_state.targetGait;
        }
    }
}

void GaitGenerator::updateLegStates(float deltaTime) {
    const GaitParameters& params = getCurrentGaitParams();

    for (size_t i = 0; i < m_state.legs.size(); ++i) {
        GaitState::LegState& leg = m_state.legs[i];
        const LegConfiguration& config = m_legs[i];

        // Calculate individual leg phase
        float legPhase = getLegPhase(static_cast<int>(i));
        leg.legPhase = legPhase;

        // Determine swing vs stance
        bool wasSwinging = leg.isSwinging;
        leg.isSwinging = legPhase < (1.0f - params.dutyFactor);

        // Transition from stance to swing
        if (leg.isSwinging && !wasSwinging) {
            leg.plantedPosition = leg.currentTarget;
            leg.nextTarget = calculateNextStepTarget(static_cast<int>(i));
        }

        // Transition from swing to stance
        if (!leg.isSwinging && wasSwinging) {
            leg.plantedPosition = leg.nextTarget;
            leg.isPlanted = true;
        }

        // Calculate current target position
        leg.currentTarget = calculateFootPosition(static_cast<int>(i), legPhase);

        // Ground raycast for terrain adaptation
        glm::vec3 hitPoint, hitNormal;
        if (raycastGround(leg.currentTarget + glm::vec3(0, 1, 0), hitPoint, hitNormal)) {
            leg.groundHeight = hitPoint.y;
            leg.groundNormal = hitNormal;

            // Adjust target to ground if in stance
            if (!leg.isSwinging) {
                leg.currentTarget.y = hitPoint.y;
            }
        }

        // Set blend weight (full weight for now)
        leg.blendWeight = 1.0f;
        leg.isPlanted = !leg.isSwinging;
    }
}

void GaitGenerator::updateBodyMotion(float deltaTime) {
    if (m_state.speed < 0.01f) {
        m_state.bodyOffset = glm::vec3(0);
        m_state.bodyTilt = glm::quat(1, 0, 0, 0);
        return;
    }

    const GaitParameters& params = getCurrentGaitParams();

    // Vertical bob - synchronized with gait phase
    int legCount = static_cast<int>(m_legs.size());
    float bobFrequency = (legCount == 2) ? 2.0f : 1.0f;
    float bob = std::sin(m_state.phase * 3.14159f * 2.0f * bobFrequency);
    bob *= params.bodyBobAmplitude * (m_state.speed / std::max(params.speedMax, 0.1f));
    m_state.bodyOffset.y = bob;

    // Lateral sway
    float sway = std::sin(m_state.phase * 3.14159f * 2.0f);
    sway *= params.bodySwayAmplitude * (m_state.speed / std::max(params.speedMax, 0.1f));
    m_state.bodyOffset.x = sway;

    // Forward pitch based on acceleration
    float pitchAngle = params.bodyPitchAmplitude * std::sin(m_state.phase * 3.14159f * 2.0f);
    glm::quat pitch = glm::angleAxis(pitchAngle, glm::vec3(1, 0, 0));

    // Roll based on foot contacts
    float rollAngle = params.bodyRollAmplitude * std::sin(m_state.phase * 3.14159f * 2.0f + 1.57f);
    glm::quat roll = glm::angleAxis(rollAngle, glm::vec3(0, 0, 1));

    m_state.bodyTilt = pitch * roll;
}

// =============================================================================
// FOOT TRAJECTORY CALCULATION
// =============================================================================

glm::vec3 GaitGenerator::calculateFootPosition(int legIndex, float phase) {
    if (legIndex < 0 || legIndex >= static_cast<int>(m_legs.size())) {
        return glm::vec3(0);
    }

    const LegConfiguration& config = m_legs[legIndex];
    const GaitParameters& params = getCurrentGaitParams();
    GaitState::LegState& leg = m_state.legs[legIndex];

    if (leg.isSwinging) {
        // Calculate swing progress
        float swingPhase = phase / (1.0f - params.dutyFactor);
        return calculateSwingTrajectory(legIndex, swingPhase);
    } else {
        // Stance phase - foot planted, slight slide allowed
        return leg.plantedPosition;
    }
}

glm::vec3 GaitGenerator::calculateSwingTrajectory(int legIndex, float swingPhase) {
    const LegConfiguration& config = m_legs[legIndex];
    const GaitParameters& params = getCurrentGaitParams();
    GaitState::LegState& leg = m_state.legs[legIndex];

    // Clamp phase
    swingPhase = glm::clamp(swingPhase, 0.0f, 1.0f);

    // Base interpolation from planted to next target
    glm::vec3 position = glm::mix(leg.plantedPosition, leg.nextTarget, swingPhase);

    // Add arc based on trajectory type
    float arcHeight = 0.0f;
    switch (params.trajectory) {
        case FootTrajectory::STANDARD: {
            // Simple sine arc
            arcHeight = std::sin(swingPhase * 3.14159f) * params.stepHeight;
            break;
        }
        case FootTrajectory::EXTENDED_REACH: {
            // Higher arc with forward bias
            arcHeight = std::sin(swingPhase * 3.14159f) * params.stepHeight * 1.5f;
            // Peak earlier in swing
            if (swingPhase < 0.5f) {
                arcHeight *= 1.0f + swingPhase * 0.5f;
            }
            break;
        }
        case FootTrajectory::QUICK_STEP: {
            // Low, fast arc
            arcHeight = std::sin(swingPhase * 3.14159f) * params.stepHeight * 0.5f;
            break;
        }
        case FootTrajectory::DRAG: {
            // Very low arc, almost scraping
            arcHeight = std::sin(swingPhase * 3.14159f) * params.stepHeight * 0.2f;
            arcHeight += params.footClearance;
            break;
        }
        case FootTrajectory::STOMP: {
            // High lift, fast drop
            if (swingPhase < 0.6f) {
                float t = swingPhase / 0.6f;
                arcHeight = std::sin(t * 3.14159f * 0.5f) * params.stepHeight * 2.0f;
            } else {
                float t = (swingPhase - 0.6f) / 0.4f;
                arcHeight = (1.0f - t * t) * params.stepHeight * 2.0f;
            }
            break;
        }
        case FootTrajectory::CAREFUL: {
            // Smooth, high arc with slow peak
            float smoothPhase = swingPhase * swingPhase * (3.0f - 2.0f * swingPhase);
            arcHeight = std::sin(smoothPhase * 3.14159f) * params.stepHeight * 1.2f;
            break;
        }
        case FootTrajectory::SPRING: {
            // Bouncy motion with overshoot
            float t = swingPhase * 3.14159f;
            arcHeight = std::sin(t) * params.stepHeight;
            arcHeight += std::sin(t * 3.0f) * params.stepHeight * 0.1f;
            break;
        }
    }

    position.y += arcHeight;

    // Ensure minimum clearance
    position.y = std::max(position.y, leg.groundHeight + params.footClearance);

    return position;
}

glm::vec3 GaitGenerator::calculateNextStepTarget(int legIndex) {
    if (legIndex < 0 || legIndex >= static_cast<int>(m_legs.size())) {
        return glm::vec3(0);
    }

    const LegConfiguration& config = m_legs[legIndex];
    const GaitParameters& params = getCurrentGaitParams();

    // Calculate rest position in world space
    glm::vec3 restWorld = m_bodyPosition + m_bodyRotation * config.restPosition;

    // Add stride offset based on velocity
    glm::vec3 forward = m_bodyRotation * glm::vec3(0, 0, 1);
    glm::vec3 strideOffset = forward * params.strideLength * 0.5f;

    if (glm::length(m_velocity) > 0.01f) {
        strideOffset = glm::normalize(m_velocity) * params.strideLength * 0.5f;
    }

    glm::vec3 target = restWorld + strideOffset;

    // Raycast to find ground
    glm::vec3 hitPoint, hitNormal;
    if (raycastGround(target + glm::vec3(0, 2, 0), hitPoint, hitNormal)) {
        target.y = hitPoint.y;
    }

    return target;
}

// =============================================================================
// GROUND INTERACTION
// =============================================================================

bool GaitGenerator::raycastGround(const glm::vec3& origin, glm::vec3& hitPoint, glm::vec3& hitNormal) {
    if (m_groundCallback) {
        return m_groundCallback(origin, glm::vec3(0, -1, 0), 10.0f, hitPoint, hitNormal);
    }

    // Default flat ground
    hitPoint = glm::vec3(origin.x, 0.0f, origin.z);
    hitNormal = glm::vec3(0, 1, 0);
    return true;
}

float GaitGenerator::getGroundHeight(const glm::vec3& position) {
    glm::vec3 hitPoint, hitNormal;
    if (raycastGround(position + glm::vec3(0, 2, 0), hitPoint, hitNormal)) {
        return hitPoint.y;
    }
    return 0.0f;
}

// =============================================================================
// HELPERS
// =============================================================================

float GaitGenerator::getLegPhase(int legIndex) const {
    const GaitParameters& params = getCurrentGaitParams();

    float phaseOffset = 0.0f;
    if (legIndex < static_cast<int>(params.legPhases.size())) {
        phaseOffset = params.legPhases[legIndex];
    } else if (legIndex < static_cast<int>(m_legs.size())) {
        phaseOffset = m_legs[legIndex].phaseOffset;
    }

    float phase = m_state.phase + phaseOffset;
    while (phase >= 1.0f) phase -= 1.0f;
    while (phase < 0.0f) phase += 1.0f;
    return phase;
}

bool GaitGenerator::isLegInSwing(int legIndex) const {
    float phase = getLegPhase(legIndex);
    const GaitParameters& params = getCurrentGaitParams();
    return phase < (1.0f - params.dutyFactor);
}

const GaitParameters& GaitGenerator::getCurrentGaitParams() const {
    auto it = m_gaits.find(m_state.currentGait);
    if (it != m_gaits.end()) {
        return it->second;
    }
    static GaitParameters defaultParams;
    return defaultParams;
}

const GaitParameters& GaitGenerator::getTargetGaitParams() const {
    auto it = m_gaits.find(m_state.targetGait);
    if (it != m_gaits.end()) {
        return it->second;
    }
    return getCurrentGaitParams();
}

// =============================================================================
// DEFAULT GAITS SETUP
// =============================================================================

void GaitGenerator::setupDefaultGaits() {
    m_gaits[GaitPattern::BIPED_WALK] = createBipedWalk();
    m_gaits[GaitPattern::BIPED_RUN] = createBipedRun();
    m_gaits[GaitPattern::QUADRUPED_WALK] = createQuadrupedWalk();
    m_gaits[GaitPattern::QUADRUPED_TROT] = createQuadrupedTrot();
    m_gaits[GaitPattern::QUADRUPED_GALLOP] = createQuadrupedGallop();
    m_gaits[GaitPattern::HEXAPOD_TRIPOD] = createHexapodTripod();
    m_gaits[GaitPattern::HEXAPOD_RIPPLE] = createHexapodRipple();
    m_gaits[GaitPattern::OCTOPOD_WAVE] = createOctopodWave();
    m_gaits[GaitPattern::SERPENTINE_LATERAL] = createSerpentineLateral();
    m_gaits[GaitPattern::SERPENTINE_RECTILINEAR] = createSerpentineRectilinear();
}

// =============================================================================
// GAIT PRESETS
// =============================================================================

GaitParameters GaitGenerator::createBipedWalk() {
    GaitParameters params;
    params.pattern = GaitPattern::BIPED_WALK;
    params.cycleTime = 1.0f;
    params.dutyFactor = 0.6f;  // 60% stance
    params.strideLength = 0.6f;
    params.stepHeight = 0.12f;
    params.speedMin = 0.0f;
    params.speedMax = 2.0f;
    params.energyCost = 1.0f;
    params.legPhases = {0.0f, 0.5f};  // Opposite phase
    params.bodyBobAmplitude = 0.02f;
    params.bodySwayAmplitude = 0.015f;
    params.bodyPitchAmplitude = 0.02f;
    params.bodyRollAmplitude = 0.01f;
    params.trajectory = FootTrajectory::STANDARD;
    params.transitionTime = 0.3f;
    return params;
}

GaitParameters GaitGenerator::createBipedRun() {
    GaitParameters params;
    params.pattern = GaitPattern::BIPED_RUN;
    params.cycleTime = 0.5f;
    params.dutyFactor = 0.35f;  // Shorter stance, aerial phase
    params.strideLength = 1.2f;
    params.stepHeight = 0.2f;
    params.speedMin = 1.5f;
    params.speedMax = 6.0f;
    params.energyCost = 2.5f;
    params.legPhases = {0.0f, 0.5f};
    params.bodyBobAmplitude = 0.04f;
    params.bodySwayAmplitude = 0.01f;
    params.bodyPitchAmplitude = 0.05f;
    params.bodyRollAmplitude = 0.02f;
    params.trajectory = FootTrajectory::EXTENDED_REACH;
    params.transitionTime = 0.2f;
    return params;
}

GaitParameters GaitGenerator::createQuadrupedWalk() {
    GaitParameters params;
    params.pattern = GaitPattern::QUADRUPED_WALK;
    params.cycleTime = 1.2f;
    params.dutyFactor = 0.75f;  // Always 3 feet on ground
    params.strideLength = 0.4f;
    params.stepHeight = 0.1f;
    params.speedMin = 0.0f;
    params.speedMax = 1.5f;
    params.energyCost = 0.8f;
    // Lateral sequence: FL, BL, FR, BR
    params.legPhases = {0.0f, 0.25f, 0.5f, 0.75f};
    params.bodyBobAmplitude = 0.01f;
    params.bodySwayAmplitude = 0.02f;
    params.bodyPitchAmplitude = 0.01f;
    params.bodyRollAmplitude = 0.015f;
    params.trajectory = FootTrajectory::CAREFUL;
    params.transitionTime = 0.4f;
    return params;
}

GaitParameters GaitGenerator::createQuadrupedTrot() {
    GaitParameters params;
    params.pattern = GaitPattern::QUADRUPED_TROT;
    params.cycleTime = 0.6f;
    params.dutyFactor = 0.5f;  // Diagonal pairs
    params.strideLength = 0.7f;
    params.stepHeight = 0.15f;
    params.speedMin = 1.0f;
    params.speedMax = 4.0f;
    params.energyCost = 1.5f;
    // Diagonal pairs: FL+BR, FR+BL
    params.legPhases = {0.0f, 0.5f, 0.5f, 0.0f};
    params.bodyBobAmplitude = 0.02f;
    params.bodySwayAmplitude = 0.01f;
    params.bodyPitchAmplitude = 0.02f;
    params.bodyRollAmplitude = 0.01f;
    params.trajectory = FootTrajectory::STANDARD;
    params.transitionTime = 0.25f;
    return params;
}

GaitParameters GaitGenerator::createQuadrupedGallop() {
    GaitParameters params;
    params.pattern = GaitPattern::QUADRUPED_GALLOP;
    params.cycleTime = 0.4f;
    params.dutyFactor = 0.3f;  // Aerial phase
    params.strideLength = 1.5f;
    params.stepHeight = 0.25f;
    params.speedMin = 3.5f;
    params.speedMax = 10.0f;
    params.energyCost = 4.0f;
    // Rotary gallop: FL, FR, BR, BL
    params.legPhases = {0.0f, 0.1f, 0.5f, 0.6f};
    params.bodyBobAmplitude = 0.06f;
    params.bodySwayAmplitude = 0.02f;
    params.bodyPitchAmplitude = 0.08f;
    params.bodyRollAmplitude = 0.03f;
    params.trajectory = FootTrajectory::EXTENDED_REACH;
    params.transitionTime = 0.15f;
    return params;
}

GaitParameters GaitGenerator::createHexapodTripod() {
    GaitParameters params;
    params.pattern = GaitPattern::HEXAPOD_TRIPOD;
    params.cycleTime = 0.5f;
    params.dutyFactor = 0.5f;  // Alternating tripods
    params.strideLength = 0.3f;
    params.stepHeight = 0.08f;
    params.speedMin = 0.5f;
    params.speedMax = 3.0f;
    params.energyCost = 1.2f;
    // Tripod gait: L1+R2+L3 alternate with R1+L2+R3
    params.legPhases = {0.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f};
    params.bodyBobAmplitude = 0.005f;
    params.bodySwayAmplitude = 0.008f;
    params.bodyPitchAmplitude = 0.01f;
    params.bodyRollAmplitude = 0.005f;
    params.trajectory = FootTrajectory::QUICK_STEP;
    params.transitionTime = 0.1f;
    return params;
}

GaitParameters GaitGenerator::createHexapodRipple() {
    GaitParameters params;
    params.pattern = GaitPattern::HEXAPOD_RIPPLE;
    params.cycleTime = 1.0f;
    params.dutyFactor = 0.75f;  // Only 2 legs swing at once
    params.strideLength = 0.2f;
    params.stepHeight = 0.06f;
    params.speedMin = 0.0f;
    params.speedMax = 0.8f;
    params.energyCost = 0.7f;
    // Ripple: sequential L1, L2, L3, R1, R2, R3
    params.legPhases = {0.0f, 0.167f, 0.333f, 0.5f, 0.667f, 0.833f};
    params.bodyBobAmplitude = 0.003f;
    params.bodySwayAmplitude = 0.012f;
    params.bodyPitchAmplitude = 0.008f;
    params.bodyRollAmplitude = 0.01f;
    params.trajectory = FootTrajectory::CAREFUL;
    params.transitionTime = 0.3f;
    return params;
}

GaitParameters GaitGenerator::createOctopodWave() {
    GaitParameters params;
    params.pattern = GaitPattern::OCTOPOD_WAVE;
    params.cycleTime = 1.0f;
    params.dutyFactor = 0.75f;
    params.strideLength = 0.2f;
    params.stepHeight = 0.06f;
    params.speedMin = 0.0f;
    params.speedMax = 2.0f;
    params.energyCost = 1.0f;
    // Wave gait: sequential from back to front
    params.legPhases = {0.0f, 0.125f, 0.25f, 0.375f, 0.5f, 0.625f, 0.75f, 0.875f};
    params.bodyBobAmplitude = 0.002f;
    params.bodySwayAmplitude = 0.01f;
    params.bodyPitchAmplitude = 0.005f;
    params.bodyRollAmplitude = 0.008f;
    params.trajectory = FootTrajectory::CAREFUL;
    params.transitionTime = 0.3f;
    return params;
}

GaitParameters GaitGenerator::createSerpentineLateral() {
    GaitParameters params;
    params.pattern = GaitPattern::SERPENTINE_LATERAL;
    params.cycleTime = 1.5f;
    params.dutyFactor = 1.0f;  // Always on ground
    params.strideLength = 0.5f;
    params.stepHeight = 0.0f;  // No lift
    params.speedMin = 0.0f;
    params.speedMax = 2.0f;
    params.energyCost = 1.5f;
    params.bodyBobAmplitude = 0.0f;
    params.bodySwayAmplitude = 0.1f;  // S-wave
    params.bodyPitchAmplitude = 0.0f;
    params.bodyRollAmplitude = 0.0f;
    params.trajectory = FootTrajectory::DRAG;
    params.transitionTime = 0.5f;
    return params;
}

GaitParameters GaitGenerator::createSerpentineRectilinear() {
    GaitParameters params;
    params.pattern = GaitPattern::SERPENTINE_RECTILINEAR;
    params.cycleTime = 2.0f;
    params.dutyFactor = 1.0f;
    params.strideLength = 0.2f;
    params.stepHeight = 0.0f;
    params.speedMin = 0.0f;
    params.speedMax = 0.5f;
    params.energyCost = 0.8f;
    params.bodyBobAmplitude = 0.0f;
    params.bodySwayAmplitude = 0.02f;
    params.bodyPitchAmplitude = 0.05f;
    params.bodyRollAmplitude = 0.0f;
    params.trajectory = FootTrajectory::DRAG;
    params.transitionTime = 0.5f;
    return params;
}

// =============================================================================
// GAIT ANALYZER
// =============================================================================

std::vector<GaitPattern> GaitAnalyzer::getSupportedGaits(int legCount, bool hasWings, bool hasTail) {
    std::vector<GaitPattern> gaits;

    switch (legCount) {
        case 0:
            gaits.push_back(GaitPattern::SERPENTINE_LATERAL);
            gaits.push_back(GaitPattern::SERPENTINE_RECTILINEAR);
            gaits.push_back(GaitPattern::SERPENTINE_CONCERTINA);
            break;
        case 2:
            gaits.push_back(GaitPattern::BIPED_WALK);
            gaits.push_back(GaitPattern::BIPED_RUN);
            gaits.push_back(GaitPattern::BIPED_HOP);
            break;
        case 4:
            gaits.push_back(GaitPattern::QUADRUPED_WALK);
            gaits.push_back(GaitPattern::QUADRUPED_TROT);
            gaits.push_back(GaitPattern::QUADRUPED_PACE);
            gaits.push_back(GaitPattern::QUADRUPED_CANTER);
            gaits.push_back(GaitPattern::QUADRUPED_GALLOP);
            gaits.push_back(GaitPattern::QUADRUPED_BOUND);
            if (hasTail) {
                gaits.push_back(GaitPattern::QUADRUPED_PRONK);
            }
            break;
        case 6:
            gaits.push_back(GaitPattern::HEXAPOD_TRIPOD);
            gaits.push_back(GaitPattern::HEXAPOD_RIPPLE);
            gaits.push_back(GaitPattern::HEXAPOD_METACHRONAL);
            break;
        case 8:
            gaits.push_back(GaitPattern::OCTOPOD_WAVE);
            gaits.push_back(GaitPattern::OCTOPOD_ALTERNATING);
            break;
        default:
            if (legCount > 8) {
                gaits.push_back(GaitPattern::MILLIPEDE_WAVE);
            }
            break;
    }

    if (hasWings) {
        gaits.push_back(GaitPattern::FLIGHT_FLAPPING);
        gaits.push_back(GaitPattern::FLIGHT_GLIDING);
        gaits.push_back(GaitPattern::FLIGHT_HOVERING);
    }

    return gaits;
}

GaitPattern GaitAnalyzer::getOptimalGait(
    int legCount,
    float speed,
    float terrainRoughness,
    float slopeAngle,
    bool isSwimming
) {
    if (isSwimming) {
        if (legCount > 0) return GaitPattern::SWIMMING_FROG;
        return GaitPattern::SWIMMING_FISH;
    }

    // Rough terrain favors slower, more stable gaits
    float speedMod = 1.0f - terrainRoughness * 0.5f;
    float effectiveSpeed = speed * speedMod;

    // Steep slopes favor slower gaits
    if (std::abs(slopeAngle) > 0.3f) {
        effectiveSpeed *= 0.7f;
    }

    switch (legCount) {
        case 0:
            return GaitPattern::SERPENTINE_LATERAL;
        case 2:
            return effectiveSpeed < 2.0f ? GaitPattern::BIPED_WALK : GaitPattern::BIPED_RUN;
        case 4:
            if (effectiveSpeed < 1.5f) return GaitPattern::QUADRUPED_WALK;
            if (effectiveSpeed < 4.0f) return GaitPattern::QUADRUPED_TROT;
            return GaitPattern::QUADRUPED_GALLOP;
        case 6:
            return effectiveSpeed < 1.0f ? GaitPattern::HEXAPOD_RIPPLE : GaitPattern::HEXAPOD_TRIPOD;
        case 8:
            return GaitPattern::OCTOPOD_WAVE;
        default:
            return GaitPattern::CUSTOM;
    }
}

std::vector<std::pair<float, GaitPattern>> GaitAnalyzer::getGaitTransitionPoints(
    int legCount,
    float maxSpeed
) {
    std::vector<std::pair<float, GaitPattern>> transitions;

    switch (legCount) {
        case 2:
            transitions.emplace_back(0.0f, GaitPattern::BIPED_WALK);
            transitions.emplace_back(maxSpeed * 0.4f, GaitPattern::BIPED_RUN);
            break;
        case 4:
            transitions.emplace_back(0.0f, GaitPattern::QUADRUPED_WALK);
            transitions.emplace_back(maxSpeed * 0.2f, GaitPattern::QUADRUPED_TROT);
            transitions.emplace_back(maxSpeed * 0.6f, GaitPattern::QUADRUPED_GALLOP);
            break;
        case 6:
            transitions.emplace_back(0.0f, GaitPattern::HEXAPOD_RIPPLE);
            transitions.emplace_back(maxSpeed * 0.3f, GaitPattern::HEXAPOD_TRIPOD);
            break;
        case 8:
            transitions.emplace_back(0.0f, GaitPattern::OCTOPOD_WAVE);
            break;
        default:
            break;
    }

    return transitions;
}

// =============================================================================
// MORPHOLOGY-TO-GAIT MAPPING
// =============================================================================

namespace MorphologyGaitMapping {

std::vector<LegConfiguration> generateLegConfigs(const MorphologyGenes& genes) {
    std::vector<LegConfiguration> configs;
    int legCount = genes.legPairs * 2;

    if (legCount == 0) return configs;

    float bodyLength = genes.bodyLength;
    float legLength = genes.legLength;
    float spread = genes.legSpread;

    for (int i = 0; i < genes.legPairs; ++i) {
        // Calculate attachment position along body
        float attachZ = bodyLength * (genes.legAttachPoint - 0.5f);
        if (genes.legPairs > 1) {
            float range = bodyLength * 0.6f;
            attachZ = -range / 2.0f + (range * i) / (genes.legPairs - 1);
        }

        // Left leg
        LegConfiguration leftLeg;
        leftLeg.hipOffset = glm::vec3(-genes.bodyWidth * spread, 0, attachZ);
        leftLeg.restPosition = leftLeg.hipOffset + glm::vec3(0, -legLength, 0);
        leftLeg.maxReach = legLength * 1.1f;
        leftLeg.minReach = legLength * 0.3f;
        leftLeg.liftHeight = legLength * 0.15f;
        leftLeg.stepLength = legLength * 0.5f;
        configs.push_back(leftLeg);

        // Right leg
        LegConfiguration rightLeg;
        rightLeg.hipOffset = glm::vec3(genes.bodyWidth * spread, 0, attachZ);
        rightLeg.restPosition = rightLeg.hipOffset + glm::vec3(0, -legLength, 0);
        rightLeg.maxReach = legLength * 1.1f;
        rightLeg.minReach = legLength * 0.3f;
        rightLeg.liftHeight = legLength * 0.15f;
        rightLeg.stepLength = legLength * 0.5f;
        configs.push_back(rightLeg);
    }

    return configs;
}

GaitParameters generateGaitParams(const MorphologyGenes& genes, GaitPattern pattern) {
    GaitParameters params;
    params.pattern = pattern;

    // Scale based on creature size
    float sizeScale = std::sqrt(genes.bodyLength);

    params.strideLength = calculateStrideLength(genes);
    params.stepHeight = calculateStepHeight(genes);
    params.cycleTime = calculateCycleTime(genes);

    // Apply pattern-specific settings
    int legCount = genes.legPairs * 2;

    switch (pattern) {
        case GaitPattern::BIPED_WALK:
            params.dutyFactor = 0.6f;
            params.legPhases = {0.0f, 0.5f};
            break;
        case GaitPattern::QUADRUPED_WALK:
            params.dutyFactor = 0.75f;
            params.legPhases = {0.0f, 0.25f, 0.5f, 0.75f};
            break;
        case GaitPattern::QUADRUPED_TROT:
            params.dutyFactor = 0.5f;
            params.legPhases = {0.0f, 0.5f, 0.5f, 0.0f};
            break;
        case GaitPattern::HEXAPOD_TRIPOD:
            params.dutyFactor = 0.5f;
            params.legPhases = {0.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f};
            break;
        case GaitPattern::OCTOPOD_WAVE:
            params.dutyFactor = 0.75f;
            for (int i = 0; i < 8; ++i) {
                params.legPhases.push_back(static_cast<float>(i) / 8.0f);
            }
            break;
        default:
            // Default phases based on leg count
            for (int i = 0; i < legCount; ++i) {
                params.legPhases.push_back(static_cast<float>(i) / legCount);
            }
            break;
    }

    // Body motion scaled to size
    params.bodyBobAmplitude = 0.02f * sizeScale;
    params.bodySwayAmplitude = 0.015f * sizeScale;
    params.bodyPitchAmplitude = 0.02f;
    params.bodyRollAmplitude = 0.015f;

    return params;
}

GaitPattern detectDefaultGait(const MorphologyGenes& genes) {
    int legCount = genes.legPairs * 2;

    if (legCount == 0) {
        return GaitPattern::SERPENTINE_LATERAL;
    } else if (legCount == 2) {
        return GaitPattern::BIPED_WALK;
    } else if (legCount == 4) {
        return GaitPattern::QUADRUPED_WALK;
    } else if (legCount == 6) {
        return GaitPattern::HEXAPOD_TRIPOD;
    } else if (legCount == 8) {
        return GaitPattern::OCTOPOD_WAVE;
    } else if (legCount > 8) {
        return GaitPattern::MILLIPEDE_WAVE;
    }

    return GaitPattern::CUSTOM;
}

float calculateStrideLength(const MorphologyGenes& genes) {
    return genes.legLength * 0.6f;
}

float calculateStepHeight(const MorphologyGenes& genes) {
    return genes.legLength * 0.12f;
}

float calculateCycleTime(const MorphologyGenes& genes) {
    // Kleiber's law scaling
    float mass = genes.getExpectedMass();
    return 0.8f * std::pow(mass, 0.25f);
}

} // namespace MorphologyGaitMapping

} // namespace animation
