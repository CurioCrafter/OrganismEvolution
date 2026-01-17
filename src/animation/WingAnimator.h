#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>
#include <algorithm>
#include <vector>
#include <functional>
#include <memory>

// Forward declarations
struct MorphologyGenes;

namespace animation {

// Wing type determines animation characteristics
enum class WingType {
    FEATHERED,      // Bird wings - smooth flapping, gliding capable
    MEMBRANE,       // Bat-like wings - flexible, less gliding
    INSECT_SINGLE,  // Two-wing insects (flies) - very fast beats, figure-8
    INSECT_DOUBLE,  // Four-wing insects (dragonflies) - independent wing control
    INSECT_COUPLED  // Four-wing coupled (bees, butterflies) - wings move together
};

// Flight state for animation blending
enum class FlightAnimState {
    GROUNDED,       // On ground, wings folded
    TAKING_OFF,     // Transitioning to flight
    FLAPPING,       // Active powered flight
    GLIDING,        // Wings extended, minimal flapping
    DIVING,         // Fast descent, wings tucked
    HOVERING,       // Stationary flight (insects, hummingbirds)
    LANDING         // Transitioning to ground
};

// Configuration for wing animation
struct WingAnimConfig {
    WingType type = WingType::FEATHERED;

    // Bone indices in skeleton
    int shoulderBone = -1;
    int elbowBone = -1;
    int wristBone = -1;
    int tipBone = -1;

    // Physical parameters
    float span = 1.0f;              // Total wingspan in world units
    float flapFrequency = 3.0f;     // Base flaps per second (Hz)
    float flapAmplitude = 60.0f;    // Degrees of shoulder rotation
    float glideFactor = 0.5f;       // 0 = always flap, 1 = always glide

    // Advanced parameters
    float downstrokeDuration = 0.55f;   // Fraction of cycle for downstroke
    float elbowFoldAmount = 0.3f;       // How much elbow folds on upstroke
    float wristFoldAmount = 0.25f;      // How much wrist folds (coupled to elbow)
    float featherSpreadAmount = 0.7f;   // How much feathers spread on upstroke

    // Insect-specific
    float phaseOffset = 0.0f;           // For 4-wing insects, offset between fore/hind
    float figureEightAmplitude = 15.0f; // Deviation for figure-8 pattern
};

// Output wing pose for a single wing
struct WingPose {
    // Joint rotations (local space)
    glm::quat shoulderRotation;
    glm::quat elbowRotation;
    glm::quat wristRotation;
    glm::quat tipRotation;

    // Additional animation data
    float featherSpread = 0.0f;     // 0 = closed, 1 = fully spread
    float wingTipBend = 0.0f;       // Backward bend from air pressure

    WingPose()
        : shoulderRotation(1.0f, 0.0f, 0.0f, 0.0f)
        , elbowRotation(1.0f, 0.0f, 0.0f, 0.0f)
        , wristRotation(1.0f, 0.0f, 0.0f, 0.0f)
        , tipRotation(1.0f, 0.0f, 0.0f, 0.0f)
    {}
};

// Wing animator handles procedural wing animation
class WingAnimator {
public:
    WingAnimator() = default;

    // Initialize with configuration
    void initialize(const WingAnimConfig& config);

    // Set flight parameters (call each frame before update)
    void setFlightState(FlightAnimState state);
    void setVelocity(float velocity);           // Forward velocity affects flap rate
    void setVerticalVelocity(float vVel);       // Climbing/diving affects animation
    void setBankAngle(float angle);             // Banking affects wing poses

    // Update animation (call each frame)
    void update(float deltaTime);

    // Get current poses for left and right wings
    const WingPose& getLeftWingPose() const { return m_leftWing; }
    const WingPose& getRightWingPose() const { return m_rightWing; }

    // Get current animation phase (0-1)
    float getPhase() const { return m_phase; }

    // Get effective flap frequency (may vary with velocity)
    float getEffectiveFrequency() const { return m_effectiveFrequency; }

    // Is currently in downstroke phase?
    bool isDownstroke() const { return m_phase < m_config.downstrokeDuration; }

    // Get config for modification
    WingAnimConfig& getConfig() { return m_config; }
    const WingAnimConfig& getConfig() const { return m_config; }

private:
    WingAnimConfig m_config;

    // Current state
    FlightAnimState m_state = FlightAnimState::GROUNDED;
    float m_velocity = 0.0f;
    float m_verticalVelocity = 0.0f;
    float m_bankAngle = 0.0f;

    // Animation state
    float m_phase = 0.0f;           // 0-1 through flap cycle
    float m_time = 0.0f;            // Total time for procedural effects
    float m_effectiveFrequency = 3.0f;
    float m_stateBlend = 0.0f;      // Blend factor for state transitions

    // Output poses
    WingPose m_leftWing;
    WingPose m_rightWing;

    // Internal methods
    void updateBirdWings(float deltaTime);
    void updateInsectWings(float deltaTime);
    void calculateDownstrokePose(WingPose& pose, float t, bool isLeft);
    void calculateUpstrokePose(WingPose& pose, float t, bool isLeft);
    void calculateGlidePose(WingPose& pose, bool isLeft);
    void calculateHoverPose(WingPose& pose, float t, bool isLeft);
    void calculateFigureEightPose(WingPose& pose, float t, bool isLeft);
    void applyBankOffset(WingPose& leftPose, WingPose& rightPose);

    // Interpolation helpers
    static float easeInQuad(float t) { return t * t; }
    static float easeOutQuad(float t) { return t * (2.0f - t); }
    static float easeInOutQuad(float t) {
        return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
    }
};

// Inline implementations

inline void WingAnimator::initialize(const WingAnimConfig& config) {
    m_config = config;
    m_phase = 0.0f;
    m_time = 0.0f;
    m_effectiveFrequency = config.flapFrequency;
}

inline void WingAnimator::setFlightState(FlightAnimState state) {
    if (m_state != state) {
        m_state = state;
        m_stateBlend = 0.0f;  // Start blend transition
    }
}

inline void WingAnimator::setVelocity(float velocity) {
    m_velocity = velocity;
}

inline void WingAnimator::setVerticalVelocity(float vVel) {
    m_verticalVelocity = vVel;
}

inline void WingAnimator::setBankAngle(float angle) {
    m_bankAngle = angle;
}

inline void WingAnimator::update(float deltaTime) {
    m_time += deltaTime;

    // Update state blend
    m_stateBlend = std::min(1.0f, m_stateBlend + deltaTime * 3.0f);

    // Calculate effective frequency based on state and velocity
    switch (m_state) {
        case FlightAnimState::GROUNDED:
            m_effectiveFrequency = 0.0f;
            break;
        case FlightAnimState::TAKING_OFF:
            m_effectiveFrequency = m_config.flapFrequency * 1.3f;  // Faster during takeoff
            break;
        case FlightAnimState::FLAPPING:
            // Frequency varies slightly with velocity
            m_effectiveFrequency = m_config.flapFrequency * (0.8f + m_velocity * 0.02f);
            break;
        case FlightAnimState::GLIDING:
            m_effectiveFrequency = m_config.flapFrequency * 0.1f;  // Occasional adjustment flaps
            break;
        case FlightAnimState::DIVING:
            m_effectiveFrequency = 0.0f;  // Wings tucked
            break;
        case FlightAnimState::HOVERING:
            m_effectiveFrequency = m_config.flapFrequency * 1.5f;  // Fast hovering
            break;
        case FlightAnimState::LANDING:
            m_effectiveFrequency = m_config.flapFrequency * 1.2f;
            break;
    }

    // Clamp frequency
    m_effectiveFrequency = std::clamp(m_effectiveFrequency, 0.0f, 200.0f);

    // Update phase
    if (m_effectiveFrequency > 0.0f) {
        m_phase += m_effectiveFrequency * deltaTime;
        while (m_phase >= 1.0f) m_phase -= 1.0f;
    }

    // Update wing poses based on type
    if (m_config.type == WingType::FEATHERED || m_config.type == WingType::MEMBRANE) {
        updateBirdWings(deltaTime);
    } else {
        updateInsectWings(deltaTime);
    }

    // Apply banking offset
    applyBankOffset(m_leftWing, m_rightWing);
}

inline void WingAnimator::updateBirdWings(float deltaTime) {
    // Handle special states
    if (m_state == FlightAnimState::GROUNDED) {
        // Wings folded at sides
        m_leftWing = WingPose();
        m_rightWing = WingPose();

        // Fold wings against body
        float foldAngle = glm::radians(-80.0f);
        m_leftWing.shoulderRotation = glm::angleAxis(foldAngle, glm::vec3(0, 0, 1));
        m_rightWing.shoulderRotation = glm::angleAxis(foldAngle, glm::vec3(0, 0, 1));

        m_leftWing.elbowRotation = glm::angleAxis(glm::radians(120.0f), glm::vec3(0, 1, 0));
        m_rightWing.elbowRotation = glm::angleAxis(glm::radians(-120.0f), glm::vec3(0, 1, 0));
        return;
    }

    if (m_state == FlightAnimState::GLIDING) {
        calculateGlidePose(m_leftWing, true);
        calculateGlidePose(m_rightWing, false);
        return;
    }

    if (m_state == FlightAnimState::DIVING) {
        // Wings partially tucked for dive
        float tuckAngle = glm::radians(-30.0f);
        m_leftWing.shoulderRotation = glm::angleAxis(tuckAngle, glm::vec3(0, 0, 1));
        m_rightWing.shoulderRotation = glm::angleAxis(tuckAngle, glm::vec3(0, 0, 1));

        m_leftWing.elbowRotation = glm::angleAxis(glm::radians(60.0f), glm::vec3(0, 1, 0));
        m_rightWing.elbowRotation = glm::angleAxis(glm::radians(-60.0f), glm::vec3(0, 1, 0));
        return;
    }

    // Normal flapping animation
    if (m_phase < m_config.downstrokeDuration) {
        float t = m_phase / m_config.downstrokeDuration;
        calculateDownstrokePose(m_leftWing, t, true);
        calculateDownstrokePose(m_rightWing, t, false);
    } else {
        float t = (m_phase - m_config.downstrokeDuration) / (1.0f - m_config.downstrokeDuration);
        calculateUpstrokePose(m_leftWing, t, true);
        calculateUpstrokePose(m_rightWing, t, false);
    }
}

inline void WingAnimator::updateInsectWings(float deltaTime) {
    if (m_state == FlightAnimState::GROUNDED) {
        // Wings folded or flat on back
        m_leftWing = WingPose();
        m_rightWing = WingPose();
        return;
    }

    // Insects use figure-8 pattern
    calculateFigureEightPose(m_leftWing, m_phase, true);
    calculateFigureEightPose(m_rightWing, m_phase, false);
}

inline void WingAnimator::calculateDownstrokePose(WingPose& pose, float t, bool isLeft) {
    float side = isLeft ? 1.0f : -1.0f;

    // Ease-in for power feeling
    float easedT = easeInQuad(t);

    // Shoulder: rotate from up to down position
    float startAngle = glm::radians(m_config.flapAmplitude * 0.5f);
    float endAngle = glm::radians(-m_config.flapAmplitude * 0.5f);
    float shoulderAngle = glm::mix(startAngle, endAngle, easedT);

    // Rotate around X axis (forward/back) for main flap
    pose.shoulderRotation = glm::angleAxis(shoulderAngle, glm::vec3(0, 0, side));

    // Add slight forward rotation during downstroke
    float forwardAngle = glm::radians(glm::mix(-10.0f, 15.0f, t) * side);
    pose.shoulderRotation = glm::angleAxis(forwardAngle, glm::vec3(1, 0, 0)) * pose.shoulderRotation;

    // Elbow: fully extended during downstroke
    pose.elbowRotation = glm::quat(1, 0, 0, 0);

    // Wrist: fully extended
    pose.wristRotation = glm::quat(1, 0, 0, 0);

    // Feathers closed for maximum lift
    pose.featherSpread = 0.0f;

    // Wing tip bends backward from air pressure
    pose.wingTipBend = glm::mix(0.0f, -15.0f, t);
}

inline void WingAnimator::calculateUpstrokePose(WingPose& pose, float t, bool isLeft) {
    float side = isLeft ? 1.0f : -1.0f;

    // Faster ease-out for recovery stroke
    float easedT = easeOutQuad(t);

    // Shoulder: rotate from down to up position (faster)
    float startAngle = glm::radians(-m_config.flapAmplitude * 0.5f);
    float endAngle = glm::radians(m_config.flapAmplitude * 0.5f);
    float shoulderAngle = glm::mix(startAngle, endAngle, easedT);

    pose.shoulderRotation = glm::angleAxis(shoulderAngle, glm::vec3(0, 0, side));

    // Return from forward rotation
    float forwardAngle = glm::radians(glm::mix(15.0f, -10.0f, t) * side);
    pose.shoulderRotation = glm::angleAxis(forwardAngle, glm::vec3(1, 0, 0)) * pose.shoulderRotation;

    // Elbow: flexes during upstroke (peak at mid-stroke)
    float elbowFlex = std::sin(t * 3.14159f) * m_config.elbowFoldAmount;
    float elbowAngle = glm::radians(elbowFlex * 90.0f * side);
    pose.elbowRotation = glm::angleAxis(elbowAngle, glm::vec3(0, 1, 0));

    // Wrist: coupled to elbow
    float wristFlex = elbowFlex * (m_config.wristFoldAmount / m_config.elbowFoldAmount);
    float wristAngle = glm::radians(wristFlex * 60.0f * side);
    pose.wristRotation = glm::angleAxis(wristAngle, glm::vec3(0, 1, 0));

    // Feathers spread for low drag
    pose.featherSpread = m_config.featherSpreadAmount;

    // No tip bend during upstroke
    pose.wingTipBend = 0.0f;
}

inline void WingAnimator::calculateGlidePose(WingPose& pose, bool isLeft) {
    float side = isLeft ? 1.0f : -1.0f;

    // Wings fully extended, slight dihedral (upward V)
    float dihedralAngle = glm::radians(5.0f);
    pose.shoulderRotation = glm::angleAxis(dihedralAngle, glm::vec3(0, 0, side));

    // Fully extended joints
    pose.elbowRotation = glm::quat(1, 0, 0, 0);
    pose.wristRotation = glm::quat(1, 0, 0, 0);

    // Feathers somewhat spread for optimal lift-to-drag
    pose.featherSpread = 0.3f;
    pose.wingTipBend = 0.0f;
}

inline void WingAnimator::calculateHoverPose(WingPose& pose, float t, bool isLeft) {
    // Hovering uses a more horizontal flap plane
    // Similar to hummingbird flight
    float side = isLeft ? 1.0f : -1.0f;

    // Horizontal figure-8 motion
    float flapAngle = std::sin(t * 6.28318f) * glm::radians(60.0f);
    float rotationAngle = std::cos(t * 6.28318f) * glm::radians(45.0f);

    // Rotate primarily horizontally
    pose.shoulderRotation = glm::angleAxis(flapAngle, glm::vec3(0, 1, 0));
    pose.shoulderRotation = glm::angleAxis(rotationAngle * side, glm::vec3(1, 0, 0)) * pose.shoulderRotation;

    pose.elbowRotation = glm::quat(1, 0, 0, 0);
    pose.wristRotation = glm::quat(1, 0, 0, 0);
    pose.featherSpread = 0.2f;
}

inline void WingAnimator::calculateFigureEightPose(WingPose& pose, float t, bool isLeft) {
    float side = isLeft ? 1.0f : -1.0f;
    float twoPi = 6.28318f;

    // Primary stroke motion (forward/backward)
    float strokeAngle = std::sin(t * twoPi) * glm::radians(m_config.flapAmplitude);

    // Figure-8 deviation (creates the "8" shape) - oscillates at 2x frequency
    float deviationAngle = std::sin(2.0f * t * twoPi) * glm::radians(m_config.figureEightAmplitude);

    // Wing rotation (pronation/supination at stroke reversals)
    float rotationAngle = std::sin(t * twoPi) * glm::radians(45.0f);

    // Build rotation
    pose.shoulderRotation = glm::angleAxis(strokeAngle, glm::vec3(0, 0, side));
    pose.shoulderRotation = glm::angleAxis(deviationAngle, glm::vec3(1, 0, 0)) * pose.shoulderRotation;
    pose.shoulderRotation = glm::angleAxis(rotationAngle * side, glm::vec3(0, 1, 0)) * pose.shoulderRotation;

    // Insect wings don't fold
    pose.elbowRotation = glm::quat(1, 0, 0, 0);
    pose.wristRotation = glm::quat(1, 0, 0, 0);
    pose.featherSpread = 0.0f;  // Insects don't have feathers
    pose.wingTipBend = 0.0f;
}

inline void WingAnimator::applyBankOffset(WingPose& leftPose, WingPose& rightPose) {
    if (std::abs(m_bankAngle) < 0.01f) return;

    // When banking, the inside wing raises more, outside wing drops
    float bankEffect = m_bankAngle * 0.3f;  // Scale down effect

    // Apply to shoulder rotation
    glm::quat leftBank = glm::angleAxis(-bankEffect, glm::vec3(0, 0, 1));
    glm::quat rightBank = glm::angleAxis(bankEffect, glm::vec3(0, 0, 1));

    leftPose.shoulderRotation = leftBank * leftPose.shoulderRotation;
    rightPose.shoulderRotation = rightBank * rightPose.shoulderRotation;
}

// =============================================================================
// FLIGHT SEQUENCE SYSTEM
// Manages complex flight maneuvers and transitions
// =============================================================================

// Flight maneuver types
enum class FlightManeuver {
    NONE,
    TAKEOFF_RUN,          // Ground-based takeoff with running
    VERTICAL_TAKEOFF,     // VTOL-style takeoff (insects, hummingbirds)
    DIVE_ATTACK,          // Predatory dive
    BARREL_ROLL,          // Evasive roll
    LOOP,                 // Vertical loop
    SPIRAL_DESCENT,       // Controlled descending spiral
    HOVER_SEARCH,         // Hovering while searching
    LANDING_APPROACH,     // Final approach to landing
    CRASH_LAND,           // Emergency landing
    SOAR_THERMAL,         // Riding thermal updrafts
    FORMATION_FLIGHT      // V-formation or echelon
};

// Flight sequence keyframe
struct FlightKeyframe {
    float time;
    FlightAnimState state;
    float flapAmplitude;
    float flapFrequency;
    float bankAngle;
    float pitchAngle;
    glm::vec3 velocity;
    float bodyRoll;
};

// Flight sequence controller
class FlightSequence {
public:
    FlightSequence() = default;

    // Add keyframe to sequence
    void addKeyframe(const FlightKeyframe& keyframe);

    // Clear all keyframes
    void clear();

    // Get interpolated state at time
    FlightKeyframe evaluate(float time) const;

    // Get total duration
    float getDuration() const;

    // Is sequence complete at given time?
    bool isComplete(float time) const;

private:
    std::vector<FlightKeyframe> m_keyframes;
};

// Predefined flight sequences
namespace FlightSequences {
    FlightSequence createTakeoffSequence(WingType type, float mass, float wingSpan);
    FlightSequence createLandingSequence(WingType type, float approachSpeed);
    FlightSequence createDiveAttackSequence(float diveAngle, float pulloutHeight);
    FlightSequence createThermalSoarSequence(float thermalRadius, float climbRate);
    FlightSequence createHoverSearchSequence(float duration, float searchRadius);
    FlightSequence createBarrelRollSequence(float rollDuration);
}

// =============================================================================
// MORPHOLOGY-DRIVEN WING CONTROLLER
// Generates wing animation from creature body plan
// =============================================================================

class MorphologyWingController {
public:
    MorphologyWingController();
    ~MorphologyWingController() = default;

    // Initialize from morphology genes
    void initializeFromMorphology(const MorphologyGenes& genes);

    // Set flight state
    void setFlightState(FlightAnimState state);
    void startManeuver(FlightManeuver maneuver);
    void setTargetPosition(const glm::vec3& target);
    void setTargetVelocity(const glm::vec3& velocity);

    // Movement input
    void setVelocity(const glm::vec3& velocity);
    void setAngularVelocity(const glm::vec3& angularVel);
    void setAltitude(float altitude);
    void setGroundDistance(float distance);

    // Environment
    void setWindDirection(const glm::vec3& wind);
    void setWindSpeed(float speed);
    void setAirDensity(float density);    // For altitude effects
    void setThermalStrength(float strength);

    // Update animation
    void update(float deltaTime);

    // Get wing poses
    const WingPose& getLeftWingPose() const;
    const WingPose& getRightWingPose() const;

    // For 4-wing creatures (dragonflies, etc.)
    const WingPose& getLeftHindWingPose() const;
    const WingPose& getRightHindWingPose() const;
    bool hasFourWings() const { return m_hasFourWings; }

    // Get body motion (bob, tilt, roll)
    glm::vec3 getBodyOffset() const;
    glm::quat getBodyRotation() const;

    // State queries
    FlightAnimState getCurrentState() const { return m_currentState; }
    FlightManeuver getCurrentManeuver() const { return m_currentManeuver; }
    float getManeuverProgress() const { return m_maneuverProgress; }
    bool isInManeuver() const { return m_currentManeuver != FlightManeuver::NONE; }
    float getFlightEfficiency() const;
    float getStaminaCost() const;

    // Access internal animator
    WingAnimator& getPrimaryAnimator() { return m_primaryAnimator; }
    WingAnimator& getSecondaryAnimator() { return m_secondaryAnimator; }

private:
    WingAnimator m_primaryAnimator;       // Fore wings (or only wings)
    WingAnimator m_secondaryAnimator;     // Hind wings (for 4-wing)

    // Configuration from morphology
    WingType m_wingType = WingType::FEATHERED;
    bool m_hasFourWings = false;
    float m_wingSpan = 1.0f;
    float m_wingArea = 0.5f;
    float m_bodyMass = 1.0f;
    float m_maxFlapFrequency = 5.0f;
    bool m_canGlide = true;
    bool m_canHover = false;

    // Current state
    FlightAnimState m_currentState = FlightAnimState::GROUNDED;
    FlightManeuver m_currentManeuver = FlightManeuver::NONE;
    float m_maneuverProgress = 0.0f;
    float m_maneuverDuration = 0.0f;

    // Movement state
    glm::vec3 m_velocity{0.0f};
    glm::vec3 m_angularVelocity{0.0f};
    glm::vec3 m_targetPosition{0.0f};
    glm::vec3 m_targetVelocity{0.0f};
    float m_altitude = 0.0f;
    float m_groundDistance = 0.0f;

    // Environment
    glm::vec3 m_windDirection{0.0f, 0.0f, 1.0f};
    float m_windSpeed = 0.0f;
    float m_airDensity = 1.0f;
    float m_thermalStrength = 0.0f;

    // Body motion
    glm::vec3 m_bodyOffset{0.0f};
    glm::quat m_bodyRotation{1.0f, 0.0f, 0.0f, 0.0f};

    // Hind wing poses (for 4-wing creatures)
    WingPose m_leftHindWing;
    WingPose m_rightHindWing;

    // Sequence playback
    FlightSequence m_activeSequence;
    float m_sequenceTime = 0.0f;

    // Internal methods
    WingType determineWingType(const MorphologyGenes& genes) const;
    void updateFlightState(float deltaTime);
    void updateManeuver(float deltaTime);
    void updateBodyMotion(float deltaTime);
    void applyWindEffects();
    float calculateLiftCoefficient() const;
    float calculateDragCoefficient() const;
};

// =============================================================================
// WING PRESET CONFIGURATIONS
// =============================================================================

namespace WingPresets {
    WingAnimConfig smallBird();       // Sparrow, finch
    WingAnimConfig largeBird();       // Eagle, hawk
    WingAnimConfig seabird();         // Albatross, pelican (gliding)
    WingAnimConfig hummingbird();     // Hovering specialist
    WingAnimConfig bat();             // Membrane wings
    WingAnimConfig butterfly();       // Large slow wings
    WingAnimConfig dragonfly();       // Independent 4-wing
    WingAnimConfig bee();             // Coupled fast 4-wing
    WingAnimConfig fly();             // Very fast 2-wing
    WingAnimConfig pterosaur();       // Large membrane (for fantasy)
    WingAnimConfig dragon();          // Fantasy dragon wings
}

// =============================================================================
// WING PHYSICS HELPERS
// =============================================================================

namespace WingPhysics {
    // Calculate lift force
    float calculateLift(float airDensity, float velocity, float wingArea, float liftCoeff);

    // Calculate drag force
    float calculateDrag(float airDensity, float velocity, float wingArea, float dragCoeff);

    // Calculate required flap frequency to maintain altitude
    float calculateRequiredFrequency(float mass, float wingArea, float liftCoeff, float airDensity);

    // Calculate glide ratio
    float calculateGlideRatio(float liftCoeff, float dragCoeff);

    // Calculate minimum glide speed
    float calculateMinGlideSpeed(float mass, float wingArea, float airDensity, float maxLiftCoeff);

    // Calculate turn radius at given bank angle and speed
    float calculateTurnRadius(float speed, float bankAngle);

    // Calculate max sustainable climb rate
    float calculateMaxClimbRate(float thrust, float drag, float weight, float velocity);
}

// =============================================================================
// FORMATION FLIGHT HELPER
// =============================================================================

class FormationController {
public:
    // Set formation type
    enum class FormationType {
        V_FORMATION,      // Classic V (geese)
        ECHELON,          // Diagonal line
        LINE_ABREAST,     // Side by side
        COLUMN,           // Single file
        CLUSTER           // Loose group (starlings)
    };

    FormationController() = default;

    void setFormationType(FormationType type) { m_formationType = type; }
    void setLeaderPosition(const glm::vec3& pos) { m_leaderPosition = pos; }
    void setLeaderVelocity(const glm::vec3& vel) { m_leaderVelocity = vel; }

    // Calculate position for creature at given index in formation
    glm::vec3 calculateFormationPosition(int index, int totalCount) const;

    // Calculate velocity adjustment to maintain formation
    glm::vec3 calculateFormationVelocity(
        const glm::vec3& currentPos,
        const glm::vec3& targetPos,
        const glm::vec3& currentVel
    ) const;

    // Get wing tip vortex benefit (energy savings from drafting)
    float getVortexBenefit(int positionIndex) const;

private:
    FormationType m_formationType = FormationType::V_FORMATION;
    glm::vec3 m_leaderPosition{0.0f};
    glm::vec3 m_leaderVelocity{0.0f};
    float m_spacing = 2.0f;
    float m_vAngle = 0.5f;  // V formation angle in radians
};

} // namespace animation
