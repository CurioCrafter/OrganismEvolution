#include "ExpressionSystem.h"
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <random>

namespace animation {

// =============================================================================
// FACE MORPHS IMPLEMENTATION
// =============================================================================

void FaceMorphs::blend(const FaceMorphs& other, float t) {
    t = glm::clamp(t, 0.0f, 1.0f);

    eyeOpenLeft = glm::mix(eyeOpenLeft, other.eyeOpenLeft, t);
    eyeOpenRight = glm::mix(eyeOpenRight, other.eyeOpenRight, t);
    eyeSquintLeft = glm::mix(eyeSquintLeft, other.eyeSquintLeft, t);
    eyeSquintRight = glm::mix(eyeSquintRight, other.eyeSquintRight, t);
    eyeWideLeft = glm::mix(eyeWideLeft, other.eyeWideLeft, t);
    eyeWideRight = glm::mix(eyeWideRight, other.eyeWideRight, t);

    pupilDilateLeft = glm::mix(pupilDilateLeft, other.pupilDilateLeft, t);
    pupilDilateRight = glm::mix(pupilDilateRight, other.pupilDilateRight, t);

    browRaiseLeft = glm::mix(browRaiseLeft, other.browRaiseLeft, t);
    browRaiseRight = glm::mix(browRaiseRight, other.browRaiseRight, t);
    browFurrow = glm::mix(browFurrow, other.browFurrow, t);
    browSad = glm::mix(browSad, other.browSad, t);

    mouthOpen = glm::mix(mouthOpen, other.mouthOpen, t);
    mouthSmile = glm::mix(mouthSmile, other.mouthSmile, t);
    mouthFrown = glm::mix(mouthFrown, other.mouthFrown, t);
    mouthSnarl = glm::mix(mouthSnarl, other.mouthSnarl, t);
    mouthPucker = glm::mix(mouthPucker, other.mouthPucker, t);
    jawOpen = glm::mix(jawOpen, other.jawOpen, t);

    noseFlare = glm::mix(noseFlare, other.noseFlare, t);
    noseWrinkle = glm::mix(noseWrinkle, other.noseWrinkle, t);

    cheekPuff = glm::mix(cheekPuff, other.cheekPuff, t);
    cheekSuck = glm::mix(cheekSuck, other.cheekSuck, t);

    tongueOut = glm::mix(tongueOut, other.tongueOut, t);
    teethBare = glm::mix(teethBare, other.teethBare, t);
    gillFlare = glm::mix(gillFlare, other.gillFlare, t);
    crestRaise = glm::mix(crestRaise, other.crestRaise, t);
    dewlapExtend = glm::mix(dewlapExtend, other.dewlapExtend, t);
}

void FaceMorphs::reset() {
    *this = FaceMorphs();
}

// =============================================================================
// EYE CONTROLLER IMPLEMENTATION
// =============================================================================

EyeController::EyeController() {
    setEyeCount(2);
}

void EyeController::setEyeCount(int count) {
    m_eyes.resize(count);
    m_eyePositions.resize(count);

    // Default positions (symmetric)
    if (count >= 2) {
        m_eyePositions[0] = glm::vec3(-0.05f, 0.02f, 0.1f);  // Left
        m_eyePositions[1] = glm::vec3(0.05f, 0.02f, 0.1f);   // Right
    }
    if (count > 2) {
        // Additional eyes arranged around head
        for (int i = 2; i < count; ++i) {
            float angle = (i - 2) * 6.28318f / (count - 2);
            m_eyePositions[i] = glm::vec3(std::cos(angle) * 0.08f, 0.02f, std::sin(angle) * 0.08f);
        }
    }
}

void EyeController::setEyePosition(int index, const glm::vec3& localPos) {
    if (index >= 0 && index < static_cast<int>(m_eyePositions.size())) {
        m_eyePositions[index] = localPos;
    }
}

void EyeController::setEyeConstraints(float maxYaw, float maxPitch) {
    m_maxYaw = maxYaw;
    m_maxPitch = maxPitch;
}

void EyeController::setBlinkRate(float blinksPerMinute) {
    m_baseBlinkRate = blinksPerMinute;
}

void EyeController::lookAt(const glm::vec3& worldTarget, const glm::vec3& headPosition, const glm::quat& headRotation) {
    m_currentLookTarget = worldTarget;

    // Calculate look direction in head space
    glm::vec3 toTarget = worldTarget - headPosition;
    if (glm::length(toTarget) > 0.001f) {
        toTarget = glm::normalize(toTarget);
        // Transform to head-local space
        m_targetLookDirection = glm::inverse(headRotation) * toTarget;
    }
}

void EyeController::setFocus(float distance) {
    // Affects eye convergence for near objects
    // Closer = more convergent
    // For now, just adjust pupil size based on distance
    if (distance < 1.0f) {
        m_targetPupilSize = 0.4f;  // Constrict for near focus
    } else {
        m_targetPupilSize = 0.5f;
    }
}

void EyeController::setPupilSize(float size) {
    m_targetPupilSize = glm::clamp(size, 0.0f, 1.0f);
}

void EyeController::triggerBlink() {
    if (!m_eyes.empty() && !m_eyes[0].isBlinking) {
        for (auto& eye : m_eyes) {
            eye.isBlinking = true;
            eye.blinkPhase = 0.0f;
        }
    }
}

void EyeController::setAlertness(float level) {
    m_alertness = glm::clamp(level, 0.0f, 1.0f);
}

void EyeController::update(float deltaTime) {
    // Update blink timer
    float effectiveBlinkRate = m_baseBlinkRate * (1.0f - m_alertness * 0.5f);
    m_blinkTimer += deltaTime;

    float blinkInterval = 60.0f / std::max(effectiveBlinkRate, 1.0f);
    if (m_blinkTimer >= blinkInterval) {
        m_blinkTimer = 0.0f;
        triggerBlink();
    }

    // Update blink animation
    const float BLINK_DURATION = 0.15f;
    bool anyBlinking = false;

    for (auto& eye : m_eyes) {
        if (eye.isBlinking) {
            eye.blinkPhase += deltaTime / BLINK_DURATION;
            if (eye.blinkPhase >= 1.0f) {
                eye.blinkPhase = 0.0f;
                eye.isBlinking = false;
            }
            anyBlinking = true;
        }

        // Update look direction
        eye.lookDirection = glm::mix(eye.lookDirection, m_targetLookDirection, deltaTime * 10.0f);

        // Clamp to constraints
        float yaw = std::atan2(eye.lookDirection.x, eye.lookDirection.z);
        float pitch = std::asin(glm::clamp(eye.lookDirection.y, -1.0f, 1.0f));

        yaw = glm::clamp(yaw, -m_maxYaw, m_maxYaw);
        pitch = glm::clamp(pitch, -m_maxPitch, m_maxPitch);

        eye.lookDirection = glm::vec3(
            std::sin(yaw) * std::cos(pitch),
            std::sin(pitch),
            std::cos(yaw) * std::cos(pitch)
        );

        // Update pupil
        eye.pupilSize = glm::mix(eye.pupilSize, m_targetPupilSize, deltaTime * 5.0f);
    }

    // Calculate overall blink amount
    if (!m_eyes.empty()) {
        float phase = m_eyes[0].blinkPhase;
        // Smooth blink curve: fast close, slower open
        if (phase < 0.3f) {
            m_blinkAmount = phase / 0.3f;  // Closing
        } else {
            m_blinkAmount = 1.0f - (phase - 0.3f) / 0.7f;  // Opening
        }
        m_blinkAmount = glm::clamp(m_blinkAmount, 0.0f, 1.0f);
    }

    // Smooth pupil transition
    m_currentPupilSize = glm::mix(m_currentPupilSize, m_targetPupilSize, deltaTime * 3.0f);
}

glm::quat EyeController::getEyeRotation(int index) const {
    if (index < 0 || index >= static_cast<int>(m_eyes.size())) {
        return glm::quat(1, 0, 0, 0);
    }

    const EyeState& eye = m_eyes[index];

    // Convert look direction to rotation
    glm::vec3 forward(0, 0, 1);
    glm::vec3 lookDir = glm::normalize(eye.lookDirection);

    float dot = glm::dot(forward, lookDir);
    if (dot > 0.9999f) {
        return glm::quat(1, 0, 0, 0);
    }
    if (dot < -0.9999f) {
        return glm::angleAxis(3.14159f, glm::vec3(0, 1, 0));
    }

    glm::vec3 axis = glm::cross(forward, lookDir);
    float angle = std::acos(glm::clamp(dot, -1.0f, 1.0f));

    return glm::angleAxis(angle, glm::normalize(axis));
}

// =============================================================================
// MOUTH CONTROLLER IMPLEMENTATION
// =============================================================================

MouthController::MouthController() {}

void MouthController::initialize(const MouthConfig& config) {
    m_config = config;
}

void MouthController::setState(MouthState state) {
    m_state = state;
    m_phase = 0.0f;
    m_stateTimer = 0.0f;
}

void MouthController::setOpenAmount(float amount) {
    m_openAmount = glm::clamp(amount, 0.0f, 1.0f);
    m_jawAngle = m_openAmount * m_config.maxJawAngle;
}

void MouthController::startChewing(float duration) {
    m_state = MouthState::CHEWING;
    m_stateTimer = duration;
    m_phase = 0.0f;
}

void MouthController::startYawning() {
    m_state = MouthState::YAWNING;
    m_stateTimer = 3.0f;  // Typical yawn duration
    m_phase = 0.0f;
}

void MouthController::setPanting(bool enabled) {
    m_isPanting = enabled;
    if (enabled && m_state != MouthState::PANTING) {
        m_state = MouthState::PANTING;
        m_phase = 0.0f;
    } else if (!enabled && m_state == MouthState::PANTING) {
        m_state = MouthState::CLOSED;
    }
}

void MouthController::vocalize(float duration, float intensity) {
    m_state = MouthState::VOCALIZING;
    m_stateTimer = duration;
    m_vocalizeIntensity = intensity;
    m_phase = 0.0f;
}

void MouthController::update(float deltaTime) {
    switch (m_state) {
        case MouthState::CLOSED:
            m_openAmount = glm::mix(m_openAmount, 0.0f, deltaTime * 5.0f);
            break;

        case MouthState::OPEN:
            // Maintain current open amount
            break;

        case MouthState::CHEWING:
            m_phase += deltaTime * m_config.chewFrequency * 6.28318f;
            m_openAmount = 0.3f + 0.2f * std::sin(m_phase);
            m_stateTimer -= deltaTime;
            if (m_stateTimer <= 0) {
                m_state = MouthState::CLOSED;
            }
            break;

        case MouthState::BREATHING:
            m_phase += deltaTime * m_config.breathFrequency * 6.28318f;
            m_openAmount = 0.05f + 0.03f * std::sin(m_phase);
            break;

        case MouthState::VOCALIZING: {
            // Oscillate mouth during vocalization
            m_phase += deltaTime * 8.0f * 6.28318f;  // Fast oscillation
            float baseOpen = 0.3f + 0.2f * m_vocalizeIntensity;
            m_openAmount = baseOpen + 0.1f * std::sin(m_phase);
            m_stateTimer -= deltaTime;
            if (m_stateTimer <= 0) {
                m_state = MouthState::CLOSED;
            }
            break;
        }

        case MouthState::YAWNING: {
            // Slow open, hold, slow close
            float progress = 1.0f - (m_stateTimer / 3.0f);
            if (progress < 0.3f) {
                // Opening phase
                m_openAmount = progress / 0.3f;
            } else if (progress < 0.7f) {
                // Hold open
                m_openAmount = 1.0f;
            } else {
                // Closing phase
                m_openAmount = 1.0f - (progress - 0.7f) / 0.3f;
            }
            m_stateTimer -= deltaTime;
            if (m_stateTimer <= 0) {
                m_state = MouthState::CLOSED;
            }
            break;
        }

        case MouthState::PANTING:
            m_phase += deltaTime * m_config.pantFrequency * 6.28318f;
            m_openAmount = 0.4f + 0.2f * std::sin(m_phase);
            m_tongueExtension = 0.6f + 0.1f * std::sin(m_phase * 0.5f);
            break;

        case MouthState::BITING:
            // Quick snap motion
            m_phase += deltaTime * 10.0f;
            if (m_phase < 0.3f) {
                m_openAmount = 0.8f;  // Open wide
            } else {
                m_openAmount = 0.0f;  // Snap closed
            }
            if (m_phase >= 1.0f) {
                m_state = MouthState::CLOSED;
            }
            break;
    }

    m_jawAngle = m_openAmount * m_config.maxJawAngle;

    // Reset tongue if not panting
    if (m_state != MouthState::PANTING) {
        m_tongueExtension = glm::mix(m_tongueExtension, 0.0f, deltaTime * 3.0f);
    }
}

// =============================================================================
// DISPLAY BEHAVIOR IMPLEMENTATION
// =============================================================================

void DisplayBehavior::initialize(const DisplayConfig& threatDisplay, const DisplayConfig& matingDisplay) {
    m_threatDisplay = threatDisplay;
    m_matingDisplay = matingDisplay;
}

void DisplayBehavior::startThreatDisplay() {
    m_isDisplaying = true;
    m_isThreat = true;
    m_progress = 0.0f;
    m_displayTimer = 0.0f;
}

void DisplayBehavior::startMatingDisplay() {
    m_isDisplaying = true;
    m_isThreat = false;
    m_progress = 0.0f;
    m_displayTimer = 0.0f;
}

void DisplayBehavior::stopDisplay() {
    m_isDisplaying = false;
}

void DisplayBehavior::update(float deltaTime) {
    if (!m_isDisplaying) {
        // Decay values
        m_crestRaise = glm::mix(m_crestRaise, 0.0f, deltaTime * 3.0f);
        m_dewlapExtend = glm::mix(m_dewlapExtend, 0.0f, deltaTime * 3.0f);
        m_bodyInflate = glm::mix(m_bodyInflate, 0.0f, deltaTime * 3.0f);
        m_colorChange = glm::mix(m_colorChange, 0.0f, deltaTime * 3.0f);
        return;
    }

    const DisplayConfig& config = m_isThreat ? m_threatDisplay : m_matingDisplay;

    m_displayTimer += deltaTime;
    float totalDuration = config.buildupTime + config.displayDuration + config.cooldownTime;

    if (m_displayTimer >= totalDuration) {
        m_isDisplaying = false;
        return;
    }

    // Calculate progress within phases
    if (m_displayTimer < config.buildupTime) {
        // Buildup phase
        float t = m_displayTimer / config.buildupTime;
        t = t * t;  // Ease in
        m_progress = t;
    } else if (m_displayTimer < config.buildupTime + config.displayDuration) {
        // Hold phase
        m_progress = 1.0f;
    } else {
        // Cooldown phase
        float t = (m_displayTimer - config.buildupTime - config.displayDuration) / config.cooldownTime;
        t = 1.0f - t;
        t = t * t;  // Ease out
        m_progress = t;
    }

    // Apply to components
    if (config.raisesCrest) {
        m_crestRaise = m_progress;
    }
    if (config.extendsDeWlap) {
        m_dewlapExtend = m_progress;
    }
    if (config.inflatesBody) {
        m_bodyInflate = m_progress * 0.3f;
    }
    if (config.changesColor) {
        m_colorChange = m_progress;
    }
}

// =============================================================================
// EXPRESSION SYSTEM IMPLEMENTATION
// =============================================================================

ExpressionSystem::ExpressionSystem() {
    setupDefaultPresets();
}

void ExpressionSystem::setEyeCount(int count) {
    m_eyes.setEyeCount(count);
}

void ExpressionSystem::setHasJaw(bool hasJaw) {
    if (hasJaw) {
        MouthConfig config;
        m_mouth.initialize(config);
    }
}

void ExpressionSystem::setHasCrest(bool hasCrest) {
    m_hasCrest = hasCrest;
}

void ExpressionSystem::setHasDewlap(bool hasDewlap) {
    m_hasDewlap = hasDewlap;
}

void ExpressionSystem::setHasGills(bool hasGills) {
    m_hasGills = hasGills;
}

void ExpressionSystem::addExpression(ExpressionType type, const ExpressionPreset& preset) {
    m_presets[type] = preset;
}

void ExpressionSystem::addCustomExpression(const std::string& name, const ExpressionPreset& preset) {
    m_customPresets[name] = preset;
}

void ExpressionSystem::setThreatDisplay(const DisplayConfig& config) {
    DisplayConfig matingConfig;
    m_display.initialize(config, matingConfig);
}

void ExpressionSystem::setMatingDisplay(const DisplayConfig& config) {
    DisplayConfig threatConfig;
    m_display.initialize(threatConfig, config);
}

void ExpressionSystem::setEmotionalState(float happiness, float fear, float anger, float excitement) {
    m_happiness = glm::clamp(happiness, 0.0f, 1.0f);
    m_fear = glm::clamp(fear, 0.0f, 1.0f);
    m_anger = glm::clamp(anger, 0.0f, 1.0f);
    m_excitement = glm::clamp(excitement, 0.0f, 1.0f);

    // Auto-select expression based on emotional state
    ExpressionType selectedType = selectExpressionFromState();
    setExpression(selectedType, 1.0f);

    // Update eye alertness
    float alertness = std::max(m_fear, std::max(m_anger, m_excitement));
    m_eyes.setAlertness(alertness);

    // Update pupil size (fear/excitement dilates, anger constricts)
    float pupilSize = 0.5f + m_fear * 0.3f + m_excitement * 0.2f - m_anger * 0.2f;
    m_eyes.setPupilSize(pupilSize);
}

void ExpressionSystem::setPhysiologicalState(float hunger, float tiredness, float pain) {
    m_hunger = glm::clamp(hunger, 0.0f, 1.0f);
    m_tiredness = glm::clamp(tiredness, 0.0f, 1.0f);
    m_pain = glm::clamp(pain, 0.0f, 1.0f);
}

void ExpressionSystem::setExpression(ExpressionType type, float intensity) {
    auto it = m_presets.find(type);
    if (it != m_presets.end()) {
        const ExpressionPreset& preset = it->second;

        // Set target morphs
        m_targetMorphs = preset.morphs;

        // Scale by intensity
        // (In a real implementation, you'd scale each morph value)

        // Start blend
        m_blendProgress = 0.0f;
        m_blendDuration = preset.transitionTime;
    }
}

void ExpressionSystem::setExpression(const std::string& customName, float intensity) {
    auto it = m_customPresets.find(customName);
    if (it != m_customPresets.end()) {
        const ExpressionPreset& preset = it->second;
        m_targetMorphs = preset.morphs;
        m_blendProgress = 0.0f;
        m_blendDuration = preset.transitionTime;
    }
}

void ExpressionSystem::blendExpression(ExpressionType type, float weight) {
    auto it = m_presets.find(type);
    if (it != m_presets.end()) {
        m_targetMorphs.blend(it->second.morphs, weight);
    }
}

void ExpressionSystem::lookAt(const glm::vec3& worldTarget) {
    // Will be applied in update()
}

void ExpressionSystem::setEyeFocus(float distance) {
    m_eyes.setFocus(distance);
}

void ExpressionSystem::openMouth(float amount) {
    m_mouth.setOpenAmount(amount);
}

void ExpressionSystem::startEating() {
    m_mouth.startChewing(2.0f);
}

void ExpressionSystem::startVocalizing(float duration) {
    m_mouth.vocalize(duration, 0.5f);
}

void ExpressionSystem::startThreatDisplay() {
    m_display.startThreatDisplay();
    setExpression(ExpressionType::AGGRESSIVE, 1.0f);
}

void ExpressionSystem::startMatingDisplay() {
    m_display.startMatingDisplay();
    setExpression(ExpressionType::MATING_DISPLAY, 1.0f);
}

void ExpressionSystem::update(float deltaTime, const glm::vec3& headPosition, const glm::quat& headRotation) {
    // Update subsystems
    m_eyes.update(deltaTime);
    m_mouth.update(deltaTime);
    m_display.update(deltaTime);

    // Update morph blending
    updateBlending(deltaTime);

    // Apply display behavior to morphs
    if (m_display.isDisplaying()) {
        if (m_hasCrest) {
            m_currentMorphs.crestRaise = m_display.getCrestRaise();
        }
        if (m_hasDewlap) {
            m_currentMorphs.dewlapExtend = m_display.getDewlapExtend();
        }
    }

    // Apply mouth state to morphs
    m_currentMorphs.jawOpen = m_mouth.getOpenAmount();
    m_currentMorphs.tongueOut = m_mouth.getTongueExtension();

    // Apply eye state to morphs
    float blinkAmount = m_eyes.getBlinkAmount();
    m_currentMorphs.eyeOpenLeft = 1.0f - blinkAmount;
    m_currentMorphs.eyeOpenRight = 1.0f - blinkAmount;
}

void ExpressionSystem::applyToSkeleton(
    std::vector<glm::quat>& boneRotations,
    uint32_t jawBone,
    const std::vector<uint32_t>& eyeBones,
    uint32_t neckBone
) const {
    // Apply jaw rotation
    if (jawBone < boneRotations.size()) {
        float jawAngle = m_mouth.getJawAngle();
        boneRotations[jawBone] = glm::angleAxis(-jawAngle, glm::vec3(1, 0, 0));
    }

    // Apply eye rotations
    for (size_t i = 0; i < eyeBones.size() && i < 2; ++i) {
        if (eyeBones[i] < boneRotations.size()) {
            boneRotations[eyeBones[i]] = m_eyes.getEyeRotation(static_cast<int>(i));
        }
    }
}

ExpressionType ExpressionSystem::selectExpressionFromState() const {
    // Priority-based selection

    // Pain overrides everything
    if (m_pain > 0.7f) return ExpressionType::PAIN;

    // Strong emotions
    if (m_fear > 0.8f) return ExpressionType::FEARFUL;
    if (m_anger > 0.8f) return ExpressionType::AGGRESSIVE;

    // Physiological states
    if (m_tiredness > 0.8f) return ExpressionType::SLEEPY;
    if (m_hunger > 0.8f) return ExpressionType::HUNGRY;

    // Moderate emotions
    if (m_fear > 0.5f) return ExpressionType::ALERT;
    if (m_anger > 0.5f) return ExpressionType::ANGRY;
    if (m_happiness > 0.7f) return ExpressionType::HAPPY;
    if (m_excitement > 0.6f) return ExpressionType::CURIOUS;

    // Slight states
    if (m_tiredness > 0.5f) return ExpressionType::TIRED;
    if (m_happiness > 0.5f) return ExpressionType::RELAXED;

    return ExpressionType::NEUTRAL;
}

void ExpressionSystem::updateBlending(float deltaTime) {
    if (m_blendProgress < 1.0f) {
        m_blendProgress += deltaTime / std::max(m_blendDuration, 0.01f);
        m_blendProgress = std::min(m_blendProgress, 1.0f);

        // Smooth interpolation
        float t = m_blendProgress;
        t = t * t * (3.0f - 2.0f * t);  // Smoothstep

        m_currentMorphs.blend(m_targetMorphs, t);
    }
}

void ExpressionSystem::setupDefaultPresets() {
    // Neutral
    {
        ExpressionPreset neutral;
        neutral.type = ExpressionType::NEUTRAL;
        neutral.name = "Neutral";
        neutral.morphs.reset();
        neutral.transitionTime = 0.3f;
        m_presets[ExpressionType::NEUTRAL] = neutral;
    }

    // Happy
    {
        ExpressionPreset happy;
        happy.type = ExpressionType::HAPPY;
        happy.name = "Happy";
        happy.morphs.reset();
        happy.morphs.mouthSmile = 0.6f;
        happy.morphs.eyeSquintLeft = 0.3f;
        happy.morphs.eyeSquintRight = 0.3f;
        happy.morphs.browRaiseLeft = 0.2f;
        happy.morphs.browRaiseRight = 0.2f;
        happy.transitionTime = 0.4f;
        m_presets[ExpressionType::HAPPY] = happy;
    }

    // Sad
    {
        ExpressionPreset sad;
        sad.type = ExpressionType::SAD;
        sad.name = "Sad";
        sad.morphs.reset();
        sad.morphs.mouthFrown = 0.5f;
        sad.morphs.browSad = 0.6f;
        sad.morphs.eyeOpenLeft = 0.7f;
        sad.morphs.eyeOpenRight = 0.7f;
        sad.transitionTime = 0.5f;
        m_presets[ExpressionType::SAD] = sad;
    }

    // Angry
    {
        ExpressionPreset angry;
        angry.type = ExpressionType::ANGRY;
        angry.name = "Angry";
        angry.morphs.reset();
        angry.morphs.browFurrow = 0.8f;
        angry.morphs.mouthSnarl = 0.4f;
        angry.morphs.noseWrinkle = 0.3f;
        angry.morphs.eyeSquintLeft = 0.4f;
        angry.morphs.eyeSquintRight = 0.4f;
        angry.morphs.teethBare = 0.3f;
        angry.transitionTime = 0.2f;
        m_presets[ExpressionType::ANGRY] = angry;
    }

    // Fearful
    {
        ExpressionPreset fearful;
        fearful.type = ExpressionType::FEARFUL;
        fearful.name = "Fearful";
        fearful.morphs.reset();
        fearful.morphs.eyeWideLeft = 0.7f;
        fearful.morphs.eyeWideRight = 0.7f;
        fearful.morphs.browRaiseLeft = 0.6f;
        fearful.morphs.browRaiseRight = 0.6f;
        fearful.morphs.browSad = 0.3f;
        fearful.morphs.mouthOpen = 0.3f;
        fearful.morphs.pupilDilateLeft = 0.9f;
        fearful.morphs.pupilDilateRight = 0.9f;
        fearful.transitionTime = 0.1f;
        m_presets[ExpressionType::FEARFUL] = fearful;
    }

    // Surprised
    {
        ExpressionPreset surprised;
        surprised.type = ExpressionType::SURPRISED;
        surprised.name = "Surprised";
        surprised.morphs.reset();
        surprised.morphs.eyeWideLeft = 0.9f;
        surprised.morphs.eyeWideRight = 0.9f;
        surprised.morphs.browRaiseLeft = 0.8f;
        surprised.morphs.browRaiseRight = 0.8f;
        surprised.morphs.mouthOpen = 0.5f;
        surprised.morphs.jawOpen = 0.3f;
        surprised.transitionTime = 0.1f;
        m_presets[ExpressionType::SURPRISED] = surprised;
    }

    // Alert
    {
        ExpressionPreset alert;
        alert.type = ExpressionType::ALERT;
        alert.name = "Alert";
        alert.morphs.reset();
        alert.morphs.eyeWideLeft = 0.3f;
        alert.morphs.eyeWideRight = 0.3f;
        alert.morphs.browRaiseLeft = 0.3f;
        alert.morphs.browRaiseRight = 0.3f;
        alert.transitionTime = 0.2f;
        m_presets[ExpressionType::ALERT] = alert;
    }

    // Sleepy
    {
        ExpressionPreset sleepy;
        sleepy.type = ExpressionType::SLEEPY;
        sleepy.name = "Sleepy";
        sleepy.morphs.reset();
        sleepy.morphs.eyeOpenLeft = 0.3f;
        sleepy.morphs.eyeOpenRight = 0.3f;
        sleepy.morphs.browSad = 0.2f;
        sleepy.morphs.mouthOpen = 0.1f;  // Slight jaw relax
        sleepy.transitionTime = 0.8f;
        m_presets[ExpressionType::SLEEPY] = sleepy;
    }

    // Aggressive
    {
        ExpressionPreset aggressive;
        aggressive.type = ExpressionType::AGGRESSIVE;
        aggressive.name = "Aggressive";
        aggressive.morphs.reset();
        aggressive.morphs.browFurrow = 1.0f;
        aggressive.morphs.mouthSnarl = 0.8f;
        aggressive.morphs.teethBare = 0.7f;
        aggressive.morphs.noseWrinkle = 0.5f;
        aggressive.morphs.eyeSquintLeft = 0.5f;
        aggressive.morphs.eyeSquintRight = 0.5f;
        aggressive.morphs.crestRaise = 1.0f;
        aggressive.morphs.gillFlare = 0.8f;
        aggressive.transitionTime = 0.15f;
        m_presets[ExpressionType::AGGRESSIVE] = aggressive;
    }

    // Threat Display
    {
        ExpressionPreset threat;
        threat.type = ExpressionType::THREAT_DISPLAY;
        threat.name = "Threat Display";
        threat.morphs.reset();
        threat.morphs.mouthOpen = 0.7f;
        threat.morphs.teethBare = 1.0f;
        threat.morphs.browFurrow = 0.8f;
        threat.morphs.noseFlare = 0.6f;
        threat.morphs.crestRaise = 1.0f;
        threat.morphs.dewlapExtend = 1.0f;
        threat.morphs.gillFlare = 1.0f;
        threat.transitionTime = 0.3f;
        m_presets[ExpressionType::THREAT_DISPLAY] = threat;
    }

    // Mating Display
    {
        ExpressionPreset mating;
        mating.type = ExpressionType::MATING_DISPLAY;
        mating.name = "Mating Display";
        mating.morphs.reset();
        mating.morphs.crestRaise = 1.0f;
        mating.morphs.dewlapExtend = 0.8f;
        mating.morphs.eyeWideLeft = 0.2f;
        mating.morphs.eyeWideRight = 0.2f;
        mating.morphs.pupilDilateLeft = 0.7f;
        mating.morphs.pupilDilateRight = 0.7f;
        mating.transitionTime = 0.5f;
        m_presets[ExpressionType::MATING_DISPLAY] = mating;
    }

    // Pain
    {
        ExpressionPreset pain;
        pain.type = ExpressionType::PAIN;
        pain.name = "Pain";
        pain.morphs.reset();
        pain.morphs.eyeSquintLeft = 0.8f;
        pain.morphs.eyeSquintRight = 0.8f;
        pain.morphs.browFurrow = 0.6f;
        pain.morphs.browSad = 0.4f;
        pain.morphs.mouthOpen = 0.4f;
        pain.morphs.cheekSuck = 0.3f;
        pain.transitionTime = 0.1f;
        m_presets[ExpressionType::PAIN] = pain;
    }
}

// =============================================================================
// PROCEDURAL EXPRESSION GENERATORS
// =============================================================================

namespace ExpressionGenerator {

FaceMorphs fromEmotions(float happiness, float fear, float anger, float surprise, float disgust) {
    FaceMorphs result;
    result.reset();

    // Happiness
    result.mouthSmile = happiness * 0.7f;
    result.eyeSquintLeft += happiness * 0.3f;
    result.eyeSquintRight += happiness * 0.3f;

    // Fear
    result.eyeWideLeft += fear * 0.6f;
    result.eyeWideRight += fear * 0.6f;
    result.browRaiseLeft += fear * 0.5f;
    result.browRaiseRight += fear * 0.5f;
    result.browSad += fear * 0.3f;
    result.pupilDilateLeft += fear * 0.4f;
    result.pupilDilateRight += fear * 0.4f;

    // Anger
    result.browFurrow += anger * 0.8f;
    result.mouthSnarl += anger * 0.5f;
    result.noseWrinkle += anger * 0.3f;
    result.eyeSquintLeft += anger * 0.3f;
    result.eyeSquintRight += anger * 0.3f;

    // Surprise
    result.eyeWideLeft += surprise * 0.8f;
    result.eyeWideRight += surprise * 0.8f;
    result.browRaiseLeft += surprise * 0.7f;
    result.browRaiseRight += surprise * 0.7f;
    result.mouthOpen += surprise * 0.5f;
    result.jawOpen += surprise * 0.3f;

    // Disgust
    result.noseWrinkle += disgust * 0.6f;
    result.mouthFrown += disgust * 0.4f;
    result.browFurrow += disgust * 0.3f;

    // Clamp all values
    result.eyeSquintLeft = glm::clamp(result.eyeSquintLeft, 0.0f, 1.0f);
    result.eyeSquintRight = glm::clamp(result.eyeSquintRight, 0.0f, 1.0f);
    result.eyeWideLeft = glm::clamp(result.eyeWideLeft, 0.0f, 1.0f);
    result.eyeWideRight = glm::clamp(result.eyeWideRight, 0.0f, 1.0f);

    return result;
}

FaceMorphs threatDisplay(float intensity) {
    FaceMorphs result;
    result.reset();

    result.mouthOpen = intensity * 0.7f;
    result.teethBare = intensity;
    result.mouthSnarl = intensity * 0.8f;
    result.browFurrow = intensity * 0.9f;
    result.noseFlare = intensity * 0.6f;
    result.crestRaise = intensity;
    result.dewlapExtend = intensity;
    result.gillFlare = intensity;

    return result;
}

FaceMorphs matingDisplay(float intensity) {
    FaceMorphs result;
    result.reset();

    result.crestRaise = intensity;
    result.dewlapExtend = intensity * 0.8f;
    result.pupilDilateLeft = 0.5f + intensity * 0.3f;
    result.pupilDilateRight = 0.5f + intensity * 0.3f;
    result.eyeWideLeft = intensity * 0.2f;
    result.eyeWideRight = intensity * 0.2f;

    return result;
}

FaceMorphs painExpression(float intensity) {
    FaceMorphs result;
    result.reset();

    result.eyeSquintLeft = intensity * 0.9f;
    result.eyeSquintRight = intensity * 0.9f;
    result.browFurrow = intensity * 0.7f;
    result.browSad = intensity * 0.5f;
    result.mouthOpen = intensity * 0.4f;
    result.cheekSuck = intensity * 0.3f;

    return result;
}

FaceMorphs tiredExpression(float intensity) {
    FaceMorphs result;
    result.reset();

    result.eyeOpenLeft = 1.0f - intensity * 0.6f;
    result.eyeOpenRight = 1.0f - intensity * 0.6f;
    result.browSad = intensity * 0.3f;
    result.mouthFrown = intensity * 0.2f;

    return result;
}

FaceMorphs blendExpressions(const std::vector<std::pair<FaceMorphs, float>>& expressions) {
    FaceMorphs result;
    result.reset();

    float totalWeight = 0.0f;
    for (const auto& [morphs, weight] : expressions) {
        totalWeight += weight;
    }

    if (totalWeight < 0.001f) return result;

    for (const auto& [morphs, weight] : expressions) {
        float normalizedWeight = weight / totalWeight;
        result.blend(morphs, normalizedWeight);
    }

    return result;
}

} // namespace ExpressionGenerator

} // namespace animation
