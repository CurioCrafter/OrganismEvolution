#pragma once

#include "Morphology.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <functional>

// =============================================================================
// LOCOMOTION SYSTEM
// Physics-based movement using joint torques and gait patterns
// =============================================================================

// Gait types for different movement styles
enum class GaitType {
    WALK,           // Slow, stable (3+ feet on ground)
    TROT,           // Medium speed (diagonal pairs)
    GALLOP,         // Fast (suspension phases)
    CRAWL,          // Very slow, close to ground
    SLITHER,        // Serpentine/no legs
    SWIM,           // Aquatic propulsion
    FLY,            // Aerial locomotion
    HOP,            // Bipedal hopping
    TRIPOD,         // Hexapod tripod gait
    WAVE            // Multi-leg wave gait
};

// State of a single limb in the gait cycle
struct LimbState {
    int limbIndex = 0;
    float phase = 0.0f;         // 0-1 position in gait cycle
    bool inStance = true;       // True if foot on ground
    glm::vec3 footPosition;     // Current foot world position
    glm::vec3 footTarget;       // Target foot position
    float supportForce = 0.0f;  // Ground reaction force
};

// Joint state for physics simulation
struct JointState {
    int segmentIndex = 0;
    float currentAngle = 0.0f;
    float targetAngle = 0.0f;
    float angularVelocity = 0.0f;
    float appliedTorque = 0.0f;
};

// =============================================================================
// CENTRAL PATTERN GENERATOR (CPG)
// Generates rhythmic signals for coordinated locomotion
// =============================================================================

class CentralPatternGenerator {
public:
    CentralPatternGenerator() = default;

    // Initialize for a specific body plan
    void initialize(const BodyPlan& bodyPlan, GaitType gait);

    // Update the CPG (call each frame)
    void update(float deltaTime, float speedMultiplier = 1.0f);

    // Get current phase for a limb
    float getLimbPhase(int limbIndex) const;

    // Get whether limb is in stance or swing
    bool isLimbInStance(int limbIndex) const;

    // Set gait type
    void setGaitType(GaitType gait);
    GaitType getGaitType() const { return currentGait; }

    // Set base frequency
    void setFrequency(float freq) { baseFrequency = freq; }
    float getFrequency() const { return baseFrequency; }

private:
    GaitType currentGait = GaitType::WALK;
    float baseFrequency = 1.0f;     // Cycles per second
    float globalPhase = 0.0f;       // Master phase (0-1)

    std::vector<float> limbPhaseOffsets;  // Phase offset for each limb
    std::vector<float> dutyFactors;       // Stance duration ratio (0-1)

    int limbCount = 0;

    void calculatePhaseOffsets();
};

// =============================================================================
// INVERSE KINEMATICS SOLVER
// Calculates joint angles to reach target positions
// =============================================================================

class IKSolver {
public:
    // Solve for a single limb chain
    // Returns true if solution found
    static bool solveLimb(
        const std::vector<BodySegment>& segments,
        const std::vector<int>& limbSegmentIndices,
        const glm::vec3& targetPosition,
        std::vector<float>& outJointAngles,
        int maxIterations = 10
    );

    // FABRIK algorithm for multi-joint chains
    static bool solveFABRIK(
        std::vector<glm::vec3>& jointPositions,
        const std::vector<float>& segmentLengths,
        const glm::vec3& target,
        const glm::vec3& basePosition,
        float tolerance = 0.01f,
        int maxIterations = 10
    );

    // Simple 2-segment analytical IK
    static bool solve2Bone(
        float upperLength,
        float lowerLength,
        const glm::vec3& target,
        const glm::vec3& poleVector,
        float& outUpperAngle,
        float& outLowerAngle
    );
};

// =============================================================================
// LOCOMOTION CONTROLLER
// High-level controller that coordinates movement
// =============================================================================

class LocomotionController {
public:
    LocomotionController() = default;

    // Initialize with body plan
    void initialize(const BodyPlan& bodyPlan, const MorphologyGenes& genes);

    // Update locomotion (call each frame)
    void update(float deltaTime, const glm::vec3& desiredVelocity, float groundHeight);

    // Get joint torques for physics system
    const std::vector<JointState>& getJointStates() const { return jointStates; }

    // Get limb states for visualization/collision
    const std::vector<LimbState>& getLimbStates() const { return limbStates; }

    // Get current body state
    glm::vec3 getPosition() const { return position; }
    glm::quat getOrientation() const { return orientation; }
    glm::vec3 getVelocity() const { return velocity; }

    // Set position/orientation directly
    void setPosition(const glm::vec3& pos) { position = pos; }
    void setOrientation(const glm::quat& orient) { orientation = orient; }

    // Get movement stats
    float getCurrentSpeed() const { return glm::length(velocity); }
    float getEnergyExpenditure() const { return energyExpenditure; }
    bool isStable() const { return stable; }

    // Switch gaits
    void setGaitType(GaitType gait);
    GaitType getCurrentGait() const { return cpg.getGaitType(); }

private:
    const BodyPlan* bodyPlan = nullptr;
    MorphologyGenes genes;

    // State
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat orientation = glm::quat(1, 0, 0, 0);
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 angularVelocity = glm::vec3(0.0f);

    // Physics state
    std::vector<JointState> jointStates;
    std::vector<LimbState> limbStates;

    // CPG for gait timing
    CentralPatternGenerator cpg;

    // Tracking
    float energyExpenditure = 0.0f;
    bool stable = true;
    float balanceError = 0.0f;

    // Limb tracking
    std::vector<std::vector<int>> limbChains; // Segment indices for each limb

    // Internal methods
    void updateCPG(float deltaTime, float speed);
    void updateLimbTargets(float groundHeight);
    void solveIK();
    void calculateTorques(float deltaTime);
    void updateBalance();
    void calculateEnergyExpenditure(float deltaTime);

    // Helpers
    GaitType selectOptimalGait(float speed) const;
    glm::vec3 calculateFootTarget(int limbIndex, float phase, float groundHeight) const;
    float calculateStepHeight(int limbIndex) const;
    float calculateStepLength(int limbIndex) const;
};

// =============================================================================
// PHYSICS BODY - Full physics simulation of articulated body
// =============================================================================

class PhysicsBody {
public:
    PhysicsBody() = default;

    // Initialize from body plan
    void initialize(const BodyPlan& bodyPlan);

    // Physics update
    void update(float deltaTime);

    // Apply external forces
    void applyForce(const glm::vec3& force, const glm::vec3& point);
    void applyTorque(int segmentIndex, const glm::vec3& torque);
    void applyJointTorque(int segmentIndex, float torque);

    // Ground contact
    void setGroundHeight(float height) { groundHeight = height; }
    void resolveGroundContact();

    // Get state
    const std::vector<glm::vec3>& getSegmentPositions() const { return segmentPositions; }
    const std::vector<glm::quat>& getSegmentOrientations() const { return segmentOrientations; }
    glm::vec3 getCenterOfMass() const;
    glm::vec3 getVelocity() const;
    float getKineticEnergy() const;

    // Set state
    void setRootPosition(const glm::vec3& pos);
    void setRootOrientation(const glm::quat& orient);
    void setRootVelocity(const glm::vec3& vel);

private:
    const BodyPlan* bodyPlan = nullptr;

    // Per-segment state
    std::vector<glm::vec3> segmentPositions;
    std::vector<glm::quat> segmentOrientations;
    std::vector<glm::vec3> segmentVelocities;
    std::vector<glm::vec3> segmentAngularVelocities;

    // Accumulated forces
    std::vector<glm::vec3> forces;
    std::vector<glm::vec3> torques;

    // Joint angles (relative to parent)
    std::vector<float> jointAngles;
    std::vector<float> jointVelocities;

    float groundHeight = 0.0f;
    float gravity = 9.81f;

    // Constraint solving
    void integrateVelocities(float deltaTime);
    void integratePositions(float deltaTime);
    void solveJointConstraints();
    void applyJointLimits();
    void clearForces();
};

// =============================================================================
// GAIT PATTERNS - Pre-defined phase relationships for different gaits
// =============================================================================

namespace GaitPatterns {
    // Get phase offset for each leg in a quadruped walk
    inline std::vector<float> quadrupedWalk() {
        return {0.0f, 0.5f, 0.25f, 0.75f}; // LF, RF, LH, RH
    }

    // Trot: diagonal pairs move together
    inline std::vector<float> quadrupedTrot() {
        return {0.0f, 0.5f, 0.5f, 0.0f}; // LF, RF, LH, RH
    }

    // Gallop: front pair leads, back pair follows
    inline std::vector<float> quadrupedGallop() {
        return {0.0f, 0.1f, 0.5f, 0.6f}; // LF, RF, LH, RH
    }

    // Hexapod tripod gait
    inline std::vector<float> hexapodTripod() {
        return {0.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f}; // L1, R1, L2, R2, L3, R3
    }

    // Hexapod wave gait (slow, stable)
    inline std::vector<float> hexapodWave() {
        return {0.0f, 0.5f, 0.167f, 0.667f, 0.333f, 0.833f};
    }

    // Biped alternating
    inline std::vector<float> bipedWalk() {
        return {0.0f, 0.5f};
    }

    // Get duty factor (stance phase ratio) for gait
    inline float getDutyFactor(GaitType gait) {
        switch (gait) {
            case GaitType::WALK:    return 0.7f;   // Long stance
            case GaitType::TROT:    return 0.5f;   // Equal stance/swing
            case GaitType::GALLOP:  return 0.3f;   // Short stance
            case GaitType::CRAWL:   return 0.8f;   // Very long stance
            case GaitType::TRIPOD:  return 0.5f;
            case GaitType::WAVE:    return 0.85f;
            case GaitType::HOP:     return 0.3f;
            default:                return 0.5f;
        }
    }
}
