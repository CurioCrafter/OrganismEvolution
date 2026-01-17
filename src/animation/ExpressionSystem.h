#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <cmath>

namespace animation {

// =============================================================================
// EXPRESSION TYPES
// =============================================================================

enum class ExpressionType {
    // Basic emotions
    NEUTRAL,
    HAPPY,
    SAD,
    ANGRY,
    FEARFUL,
    SURPRISED,
    DISGUSTED,

    // Behavioral states
    ALERT,
    RELAXED,
    SLEEPY,
    CURIOUS,
    AGGRESSIVE,
    SUBMISSIVE,
    PLAYFUL,

    // Physiological states
    HUNGRY,
    TIRED,
    PAIN,
    SICK,

    // Social expressions
    THREAT_DISPLAY,
    MATING_DISPLAY,
    SUBMISSION_DISPLAY,
    TERRITORIAL,

    // Custom
    CUSTOM
};

// =============================================================================
// FACE COMPONENTS
// =============================================================================

// Morph target weights for facial animation
struct FaceMorphs {
    // Eyes
    float eyeOpenLeft = 1.0f;     // 0 = closed, 1 = open
    float eyeOpenRight = 1.0f;
    float eyeSquintLeft = 0.0f;   // Squinting
    float eyeSquintRight = 0.0f;
    float eyeWideLeft = 0.0f;     // Wide/surprised
    float eyeWideRight = 0.0f;

    // Pupils
    float pupilDilateLeft = 0.5f;  // 0 = constricted, 1 = dilated
    float pupilDilateRight = 0.5f;

    // Eyebrows/Brow ridge
    float browRaiseLeft = 0.0f;   // Raise eyebrow
    float browRaiseRight = 0.0f;
    float browFurrow = 0.0f;      // Furrowed (angry/concentration)
    float browSad = 0.0f;         // Inner brow raise (sad)

    // Mouth
    float mouthOpen = 0.0f;       // 0 = closed, 1 = fully open
    float mouthSmile = 0.0f;      // Corners up
    float mouthFrown = 0.0f;      // Corners down
    float mouthSnarl = 0.0f;      // Upper lip raised (showing teeth)
    float mouthPucker = 0.0f;     // Lips pushed forward
    float jawOpen = 0.0f;         // Jaw drop separate from lips

    // Nose
    float noseFlare = 0.0f;       // Nostrils flaring
    float noseWrinkle = 0.0f;     // Wrinkle (disgust)

    // Cheeks
    float cheekPuff = 0.0f;       // Puffed cheeks
    float cheekSuck = 0.0f;       // Sucked in cheeks

    // Specialized features
    float tongueOut = 0.0f;       // Tongue extension
    float teethBare = 0.0f;       // Teeth visibility
    float gillFlare = 0.0f;       // Fish gills
    float crestRaise = 0.0f;      // Head crest/frill
    float dewlapExtend = 0.0f;    // Throat dewlap

    // Blend with another set
    void blend(const FaceMorphs& other, float t);

    // Reset to neutral
    void reset();
};

// =============================================================================
// EYE CONTROL
// =============================================================================

struct EyeState {
    glm::vec3 lookDirection = glm::vec3(0, 0, 1);  // Forward
    float pupilSize = 0.5f;       // 0-1
    float blinkPhase = 0.0f;      // Current blink animation phase
    bool isBlinking = false;

    // Nictitating membrane (third eyelid)
    float nictitateAmount = 0.0f;
};

class EyeController {
public:
    EyeController();

    // Configuration
    void setEyeCount(int count);
    void setEyePosition(int index, const glm::vec3& localPos);
    void setEyeConstraints(float maxYaw, float maxPitch);
    void setBlinkRate(float blinksPerMinute);

    // Control
    void lookAt(const glm::vec3& worldTarget, const glm::vec3& headPosition, const glm::quat& headRotation);
    void setFocus(float distance);  // Affects convergence
    void setPupilSize(float size);
    void triggerBlink();
    void setAlertness(float level);  // Affects blink rate

    // Update
    void update(float deltaTime);

    // Output
    glm::quat getEyeRotation(int index) const;
    float getBlinkAmount() const { return m_blinkAmount; }
    float getPupilSize() const { return m_currentPupilSize; }
    const std::vector<EyeState>& getEyeStates() const { return m_eyes; }

private:
    std::vector<EyeState> m_eyes;
    std::vector<glm::vec3> m_eyePositions;

    float m_maxYaw = 0.7f;
    float m_maxPitch = 0.5f;
    float m_baseBlinkRate = 15.0f;  // Per minute
    float m_blinkTimer = 0.0f;
    float m_blinkAmount = 0.0f;
    float m_currentPupilSize = 0.5f;
    float m_targetPupilSize = 0.5f;
    float m_alertness = 0.5f;

    glm::vec3 m_currentLookTarget;
    glm::vec3 m_targetLookDirection;
};

// =============================================================================
// MOUTH CONTROL
// =============================================================================

enum class MouthState {
    CLOSED,
    OPEN,
    CHEWING,
    BREATHING,
    VOCALIZING,
    YAWNING,
    PANTING,
    BITING
};

struct MouthConfig {
    uint32_t jawBoneIndex = UINT32_MAX;
    uint32_t upperLipBone = UINT32_MAX;
    uint32_t lowerLipBone = UINT32_MAX;
    uint32_t tongueBone = UINT32_MAX;

    float maxJawAngle = 0.5f;      // Radians
    float chewFrequency = 2.0f;    // Hz
    float breathFrequency = 0.3f;  // Hz
    float pantFrequency = 3.0f;    // Hz
};

class MouthController {
public:
    MouthController();

    void initialize(const MouthConfig& config);

    // State control
    void setState(MouthState state);
    void setOpenAmount(float amount);  // Manual override

    // Behavioral triggers
    void startChewing(float duration);
    void startYawning();
    void setPanting(bool enabled);
    void vocalize(float duration, float intensity);

    // Update
    void update(float deltaTime);

    // Output
    float getJawAngle() const { return m_jawAngle; }
    float getOpenAmount() const { return m_openAmount; }
    float getTongueExtension() const { return m_tongueExtension; }
    MouthState getState() const { return m_state; }

private:
    MouthConfig m_config;
    MouthState m_state = MouthState::CLOSED;

    float m_jawAngle = 0.0f;
    float m_openAmount = 0.0f;
    float m_tongueExtension = 0.0f;
    float m_phase = 0.0f;
    float m_stateTimer = 0.0f;
    float m_vocalizeIntensity = 0.0f;
    bool m_isPanting = false;
};

// =============================================================================
// EXPRESSION PRESET
// =============================================================================

struct ExpressionPreset {
    ExpressionType type = ExpressionType::NEUTRAL;
    std::string name;

    FaceMorphs morphs;
    float eyeOpenAmount = 1.0f;
    float pupilSize = 0.5f;
    float mouthOpen = 0.0f;

    // Animation parameters
    float transitionTime = 0.3f;   // Seconds to blend to this expression
    float holdTime = 0.0f;         // 0 = indefinite
    float intensity = 1.0f;        // Overall expression strength

    // Body language association
    float bodyPoseIndex = -1;      // Link to body pose if applicable
};

// =============================================================================
// DISPLAY BEHAVIOR (Threat/Mating displays)
// =============================================================================

struct DisplayConfig {
    // Visual components
    bool raisesCrest = false;
    bool extendsDeWlap = false;
    bool flaresGills = false;
    bool spreadsWings = false;
    bool raisesFeathers = false;
    bool inflatesBody = false;
    bool changesColor = false;

    // Animation
    float displayDuration = 3.0f;
    float buildupTime = 0.5f;
    float cooldownTime = 1.0f;

    // Sound
    bool hasVocalization = true;
    float vocalizationStart = 0.2f;  // Fraction into display
};

class DisplayBehavior {
public:
    void initialize(const DisplayConfig& threatDisplay, const DisplayConfig& matingDisplay);

    void startThreatDisplay();
    void startMatingDisplay();
    void stopDisplay();

    void update(float deltaTime);

    bool isDisplaying() const { return m_isDisplaying; }
    bool isThreatDisplay() const { return m_isThreat; }
    float getDisplayProgress() const { return m_progress; }

    // Output for animation
    float getCrestRaise() const { return m_crestRaise; }
    float getDewlapExtend() const { return m_dewlapExtend; }
    float getBodyInflate() const { return m_bodyInflate; }
    float getColorChange() const { return m_colorChange; }

private:
    DisplayConfig m_threatDisplay;
    DisplayConfig m_matingDisplay;

    bool m_isDisplaying = false;
    bool m_isThreat = false;
    float m_progress = 0.0f;
    float m_displayTimer = 0.0f;

    float m_crestRaise = 0.0f;
    float m_dewlapExtend = 0.0f;
    float m_bodyInflate = 0.0f;
    float m_colorChange = 0.0f;
};

// =============================================================================
// EXPRESSION SYSTEM - Main controller
// =============================================================================

class ExpressionSystem {
public:
    ExpressionSystem();

    // Configuration
    void setEyeCount(int count);
    void setHasJaw(bool hasJaw);
    void setHasCrest(bool hasCrest);
    void setHasDewlap(bool hasDewlap);
    void setHasGills(bool hasGills);

    // Add expression presets
    void addExpression(ExpressionType type, const ExpressionPreset& preset);
    void addCustomExpression(const std::string& name, const ExpressionPreset& preset);

    // Configure displays
    void setThreatDisplay(const DisplayConfig& config);
    void setMatingDisplay(const DisplayConfig& config);

    // Emotional state (drives expression selection)
    void setEmotionalState(float happiness, float fear, float anger, float excitement);
    void setPhysiologicalState(float hunger, float tiredness, float pain);

    // Direct control
    void setExpression(ExpressionType type, float intensity = 1.0f);
    void setExpression(const std::string& customName, float intensity = 1.0f);
    void blendExpression(ExpressionType type, float weight);

    // Eye control
    void lookAt(const glm::vec3& worldTarget);
    void setEyeFocus(float distance);

    // Mouth control
    void openMouth(float amount);
    void startEating();
    void startVocalizing(float duration);

    // Display behaviors
    void startThreatDisplay();
    void startMatingDisplay();

    // Update
    void update(float deltaTime, const glm::vec3& headPosition, const glm::quat& headRotation);

    // Output
    const FaceMorphs& getCurrentMorphs() const { return m_currentMorphs; }
    EyeController& getEyeController() { return m_eyes; }
    MouthController& getMouthController() { return m_mouth; }
    DisplayBehavior& getDisplayBehavior() { return m_display; }

    // Apply to skeleton pose (bone rotations)
    void applyToSkeleton(
        std::vector<glm::quat>& boneRotations,
        uint32_t jawBone,
        const std::vector<uint32_t>& eyeBones,
        uint32_t neckBone = UINT32_MAX
    ) const;

private:
    // Expression presets
    std::map<ExpressionType, ExpressionPreset> m_presets;
    std::map<std::string, ExpressionPreset> m_customPresets;

    // Current state
    FaceMorphs m_currentMorphs;
    FaceMorphs m_targetMorphs;
    float m_blendProgress = 1.0f;
    float m_blendDuration = 0.3f;

    // Subsystems
    EyeController m_eyes;
    MouthController m_mouth;
    DisplayBehavior m_display;

    // Emotional state
    float m_happiness = 0.5f;
    float m_fear = 0.0f;
    float m_anger = 0.0f;
    float m_excitement = 0.0f;
    float m_hunger = 0.0f;
    float m_tiredness = 0.0f;
    float m_pain = 0.0f;

    // Features
    bool m_hasCrest = false;
    bool m_hasDewlap = false;
    bool m_hasGills = false;

    // Helpers
    ExpressionType selectExpressionFromState() const;
    void updateBlending(float deltaTime);
    void setupDefaultPresets();
};

// =============================================================================
// PROCEDURAL EXPRESSION GENERATORS
// =============================================================================

namespace ExpressionGenerator {
    // Generate expression based on emotional state
    FaceMorphs fromEmotions(float happiness, float fear, float anger, float surprise, float disgust);

    // Generate threat display morphs
    FaceMorphs threatDisplay(float intensity);

    // Generate mating display morphs
    FaceMorphs matingDisplay(float intensity);

    // Generate pain expression
    FaceMorphs painExpression(float intensity);

    // Generate tired expression
    FaceMorphs tiredExpression(float intensity);

    // Blend multiple expressions with weights
    FaceMorphs blendExpressions(const std::vector<std::pair<FaceMorphs, float>>& expressions);
}

} // namespace animation
