#pragma once

#include "Skeleton.h"
#include "Pose.h"
#include "../physics/Morphology.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <array>
#include <functional>
#include <map>
#include <cmath>

namespace animation {

// =============================================================================
// GAIT PATTERN TYPES
// =============================================================================

enum class GaitPattern {
    // Bipedal gaits
    BIPED_WALK,
    BIPED_RUN,
    BIPED_SKIP,
    BIPED_HOP,

    // Quadrupedal gaits
    QUADRUPED_WALK,
    QUADRUPED_TROT,
    QUADRUPED_PACE,
    QUADRUPED_CANTER,
    QUADRUPED_GALLOP,
    QUADRUPED_BOUND,
    QUADRUPED_PRONK,  // All legs together (deer)

    // Multi-legged gaits
    HEXAPOD_TRIPOD,       // Alternating tripods (insects)
    HEXAPOD_RIPPLE,       // Wave-like (slower insects)
    HEXAPOD_METACHRONAL,  // Sequential wave
    OCTOPOD_WAVE,         // 8-leg wave (spiders)
    OCTOPOD_ALTERNATING,  // 8-leg alternating sets

    // Specialized gaits
    SERPENTINE_LATERAL,   // Snake sidewinding
    SERPENTINE_RECTILINEAR, // Caterpillar movement
    SERPENTINE_CONCERTINA, // Accordion movement (tight spaces)
    MILLIPEDE_WAVE,       // Many-legged wave

    // Aquatic
    SWIMMING_FISH,        // Tail propulsion
    SWIMMING_FROG,        // Leg kick
    SWIMMING_ROWING,      // Synchronized leg rowing

    // Aerial
    FLIGHT_FLAPPING,
    FLIGHT_HOVERING,
    FLIGHT_GLIDING,

    CUSTOM
};

// =============================================================================
// FOOT TRAJECTORY PROFILES
// =============================================================================

enum class FootTrajectory {
    STANDARD,        // Simple arc
    EXTENDED_REACH,  // Higher lift, forward reach
    QUICK_STEP,      // Fast, low arc
    DRAG,            // Ground contact during swing
    STOMP,           // Sharp downward strike
    CAREFUL,         // Slow, precise placement
    SPRING,          // Bouncy, elastic motion
};

// =============================================================================
// GAIT PARAMETERS
// =============================================================================

struct GaitParameters {
    GaitPattern pattern = GaitPattern::BIPED_WALK;

    // Timing
    float cycleTime = 1.0f;       // Duration of one complete gait cycle
    float dutyFactor = 0.6f;      // Fraction of cycle foot is on ground (0-1)
    float transitionTime = 0.3f;  // Time to blend between gaits

    // Kinematics
    float strideLength = 0.5f;    // Distance covered per stride
    float stepHeight = 0.15f;     // Maximum foot lift height
    float footClearance = 0.02f;  // Minimum height during swing

    // Dynamics
    float speedMin = 0.0f;        // Minimum speed for this gait
    float speedMax = 3.0f;        // Maximum speed before transitioning
    float energyCost = 1.0f;      // Relative metabolic cost

    // Phase offsets for each leg (normalized 0-1)
    std::vector<float> legPhases;

    // Body motion
    float bodyBobAmplitude = 0.02f;
    float bodySwayAmplitude = 0.01f;
    float bodyPitchAmplitude = 0.03f;  // Forward/back tilt
    float bodyRollAmplitude = 0.02f;   // Side-to-side tilt

    // Foot trajectory
    FootTrajectory trajectory = FootTrajectory::STANDARD;
};

// =============================================================================
// LEG CONFIGURATION (per-leg settings)
// =============================================================================

struct LegConfiguration {
    // Bone indices
    std::vector<uint32_t> boneChain;  // From hip to toe

    // Rest pose
    glm::vec3 restPosition;           // World position when standing
    glm::vec3 hipOffset;              // Offset from body center to hip

    // Constraints
    float maxReach = 0.5f;            // Maximum leg extension
    float minReach = 0.1f;            // Minimum (fully bent)
    float preferredAngle = 0.0f;      // Preferred forward angle

    // Animation params
    float liftHeight = 0.15f;
    float stepLength = 0.3f;
    float phaseOffset = 0.0f;

    // Foot properties
    glm::vec3 footSize = glm::vec3(0.05f);
    bool hasToes = false;
    int toeCount = 0;
};

// =============================================================================
// GAIT STATE (runtime tracking)
// =============================================================================

struct GaitState {
    float phase = 0.0f;           // Current phase in cycle (0-1)
    float time = 0.0f;            // Total elapsed time
    float speed = 0.0f;           // Current movement speed
    float targetSpeed = 0.0f;     // Desired speed
    float turnRate = 0.0f;        // Angular velocity

    GaitPattern currentGait = GaitPattern::BIPED_WALK;
    GaitPattern targetGait = GaitPattern::BIPED_WALK;
    float gaitBlend = 1.0f;       // Blend factor during transitions

    // Per-leg state
    struct LegState {
        glm::vec3 currentTarget;  // Current IK target
        glm::vec3 plantedPosition; // Where foot is planted
        glm::vec3 nextTarget;     // Target for next step
        float legPhase = 0.0f;    // Individual leg phase
        bool isSwinging = false;  // In swing vs stance
        bool isPlanted = true;    // Foot on ground
        float blendWeight = 1.0f; // IK blend weight
        float groundHeight = 0.0f; // Detected ground height
        glm::vec3 groundNormal = glm::vec3(0, 1, 0);
    };
    std::vector<LegState> legs;

    // Body motion state
    glm::vec3 bodyOffset = glm::vec3(0);
    glm::quat bodyTilt = glm::quat(1, 0, 0, 0);
};

// =============================================================================
// GROUND CALLBACK
// =============================================================================

using GroundRaycastFn = std::function<bool(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float maxDist,
    glm::vec3& hitPoint,
    glm::vec3& hitNormal
)>;

// =============================================================================
// GAIT GENERATOR - Main class for procedural gait generation
// =============================================================================

class GaitGenerator {
public:
    GaitGenerator();
    ~GaitGenerator() = default;

    // Initialization
    void initialize(int legCount);
    void initializeFromMorphology(const MorphologyGenes& genes);

    // Configuration
    void setLeg(int index, const LegConfiguration& config);
    void setGaitParameters(GaitPattern pattern, const GaitParameters& params);
    void setGroundCallback(GroundRaycastFn callback) { m_groundCallback = std::move(callback); }

    // Runtime control
    void setVelocity(const glm::vec3& velocity);
    void setBodyTransform(const glm::vec3& position, const glm::quat& rotation);
    void setTargetSpeed(float speed);
    void setTurnRate(float rate);
    void requestGait(GaitPattern pattern);

    // Update
    void update(float deltaTime);

    // Output
    const GaitState& getState() const { return m_state; }
    glm::vec3 getFootTarget(int legIndex) const;
    float getFootBlendWeight(int legIndex) const;
    bool isFootGrounded(int legIndex) const;
    glm::vec3 getBodyOffset() const { return m_state.bodyOffset; }
    glm::quat getBodyTilt() const { return m_state.bodyTilt; }

    // Query
    GaitPattern getCurrentGait() const { return m_state.currentGait; }
    GaitPattern getBestGaitForSpeed(float speed) const;
    float getGaitPhase() const { return m_state.phase; }
    bool isTransitioning() const { return m_state.gaitBlend < 1.0f; }

    // Presets
    static GaitParameters createBipedWalk();
    static GaitParameters createBipedRun();
    static GaitParameters createQuadrupedWalk();
    static GaitParameters createQuadrupedTrot();
    static GaitParameters createQuadrupedGallop();
    static GaitParameters createHexapodTripod();
    static GaitParameters createHexapodRipple();
    static GaitParameters createOctopodWave();
    static GaitParameters createSerpentineLateral();
    static GaitParameters createSerpentineRectilinear();

private:
    // Configuration
    std::vector<LegConfiguration> m_legs;
    std::map<GaitPattern, GaitParameters> m_gaits;

    // State
    GaitState m_state;
    glm::vec3 m_bodyPosition;
    glm::quat m_bodyRotation;
    glm::vec3 m_velocity;

    // Callbacks
    GroundRaycastFn m_groundCallback;

    // Internal methods
    void updatePhase(float deltaTime);
    void updateGaitTransition(float deltaTime);
    void updateLegStates(float deltaTime);
    void updateBodyMotion(float deltaTime);

    // Foot trajectory calculation
    glm::vec3 calculateFootPosition(int legIndex, float phase);
    glm::vec3 calculateSwingTrajectory(int legIndex, float swingPhase);
    glm::vec3 calculateNextStepTarget(int legIndex);

    // Ground interaction
    bool raycastGround(const glm::vec3& origin, glm::vec3& hitPoint, glm::vec3& hitNormal);
    float getGroundHeight(const glm::vec3& position);

    // Helpers
    float getLegPhase(int legIndex) const;
    bool isLegInSwing(int legIndex) const;
    const GaitParameters& getCurrentGaitParams() const;
    const GaitParameters& getTargetGaitParams() const;

    // Default initialization
    void setupDefaultGaits();
};

// =============================================================================
// GAIT ANALYZER - Analyzes creature properties to suggest gaits
// =============================================================================

class GaitAnalyzer {
public:
    // Analyze creature and return appropriate gaits
    static std::vector<GaitPattern> getSupportedGaits(int legCount, bool hasWings, bool hasTail);

    // Get optimal gait for given conditions
    static GaitPattern getOptimalGait(
        int legCount,
        float speed,
        float terrainRoughness,
        float slopeAngle,
        bool isSwimming
    );

    // Calculate gait transition speed thresholds
    static std::vector<std::pair<float, GaitPattern>> getGaitTransitionPoints(
        int legCount,
        float maxSpeed
    );
};

// =============================================================================
// MORPHOLOGY-TO-GAIT MAPPER
// =============================================================================

namespace MorphologyGaitMapping {
    // Generate leg configurations from morphology
    std::vector<LegConfiguration> generateLegConfigs(const MorphologyGenes& genes);

    // Generate appropriate gait parameters
    GaitParameters generateGaitParams(const MorphologyGenes& genes, GaitPattern pattern);

    // Auto-detect best default gait
    GaitPattern detectDefaultGait(const MorphologyGenes& genes);

    // Calculate gait scaling factors
    float calculateStrideLength(const MorphologyGenes& genes);
    float calculateStepHeight(const MorphologyGenes& genes);
    float calculateCycleTime(const MorphologyGenes& genes);
}

} // namespace animation
