#pragma once

#include "Skeleton.h"
#include "Pose.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <optional>
#include <functional>

namespace animation {

// IK solver configuration
struct IKConfig {
    uint32_t maxIterations = 10;
    float tolerance = 0.001f;       // Distance to target for convergence
    float damping = 1.0f;           // For CCD: 1.0 = no damping
    float softLimit = 0.99f;        // Two-bone: soft limit for near-extension
    float minBendAngle = 0.01f;     // Two-bone: minimum bend angle
};

// IK target specification
struct IKTarget {
    glm::vec3 position;
    std::optional<glm::quat> rotation;  // Optional effector rotation
    float weight = 1.0f;                 // Blend weight [0, 1]
};

// Pole vector for controlling bend direction
struct PoleVector {
    glm::vec3 position;     // World-space pole position
    float weight = 1.0f;    // Influence weight [0, 1]
    bool enabled = false;
};

// IK chain definition
struct IKChain {
    uint32_t startBoneIndex;    // Root of chain
    uint32_t endBoneIndex;      // Effector (end)
    uint32_t chainLength;       // Number of bones in chain
};

// Two-Bone IK Solver (Analytical)
// Best for: limbs (shoulder-elbow-wrist, hip-knee-ankle)
class TwoBoneIK {
public:
    TwoBoneIK() = default;
    explicit TwoBoneIK(const IKConfig& config) : m_config(config) {}

    // Solve IK for a two-bone chain
    // Returns true if solution found
    bool solve(const Skeleton& skeleton, SkeletonPose& pose,
               uint32_t upperBone, uint32_t lowerBone, uint32_t effectorBone,
               const IKTarget& target) const;

    // Solve with pole vector control
    bool solveWithPole(const Skeleton& skeleton, SkeletonPose& pose,
                       uint32_t upperBone, uint32_t lowerBone, uint32_t effectorBone,
                       const IKTarget& target, const PoleVector& pole) const;

    void setConfig(const IKConfig& config) { m_config = config; }
    const IKConfig& getConfig() const { return m_config; }

private:
    IKConfig m_config;

    // Calculate bend angle using law of cosines
    float calculateBendAngle(float upperLen, float lowerLen, float targetDist) const;

    // Apply pole vector rotation
    void applyPoleVector(glm::quat& upperRot, const glm::vec3& upperPos,
                         const glm::vec3& lowerPos, const glm::vec3& targetPos,
                         const PoleVector& pole) const;
};

// FABRIK Solver (Forward And Backward Reaching IK)
// Best for: spines, tails, tentacles, chains with many bones
class FABRIKSolver {
public:
    FABRIKSolver() = default;
    explicit FABRIKSolver(const IKConfig& config) : m_config(config) {}

    // Solve IK for a chain of bones
    bool solve(const Skeleton& skeleton, SkeletonPose& pose,
               const std::vector<uint32_t>& chainBones,
               const IKTarget& target) const;

    // Solve with constraints
    bool solveConstrained(const Skeleton& skeleton, SkeletonPose& pose,
                          const std::vector<uint32_t>& chainBones,
                          const IKTarget& target,
                          const std::vector<glm::vec2>& angleConstraints) const;

    void setConfig(const IKConfig& config) { m_config = config; }
    const IKConfig& getConfig() const { return m_config; }

private:
    IKConfig m_config;

    // Forward pass: move effector toward target
    void forwardPass(std::vector<glm::vec3>& positions, const glm::vec3& targetPos) const;

    // Backward pass: maintain root position
    void backwardPass(std::vector<glm::vec3>& positions, const glm::vec3& rootPos,
                      const std::vector<float>& boneLengths) const;

    // Apply angle constraints
    void applyConstraints(std::vector<glm::vec3>& positions,
                          const std::vector<float>& boneLengths,
                          const std::vector<glm::vec2>& constraints) const;
};

// CCD Solver (Cyclic Coordinate Descent)
// Alternative iterative solver, good for constrained joints
class CCDSolver {
public:
    CCDSolver() = default;
    explicit CCDSolver(const IKConfig& config) : m_config(config) {}

    // Solve IK for a chain
    bool solve(const Skeleton& skeleton, SkeletonPose& pose,
               const std::vector<uint32_t>& chainBones,
               const IKTarget& target) const;

    void setConfig(const IKConfig& config) { m_config = config; }
    const IKConfig& getConfig() const { return m_config; }

private:
    IKConfig m_config;

    // Rotate a single joint toward target
    void rotateJointToward(SkeletonPose& pose, uint32_t jointIndex,
                           const glm::vec3& effectorPos, const glm::vec3& targetPos) const;
};

// IK System Manager - handles multiple IK chains
class IKSystem {
public:
    enum class SolverType {
        TwoBone,
        FABRIK,
        CCD
    };

    struct ChainEntry {
        IKChain chain;
        IKTarget target;
        PoleVector pole;
        SolverType solverType;
        bool enabled = true;
        uint32_t priority = 0;  // Higher priority solved first
    };

    using ChainHandle = uint32_t;
    static constexpr ChainHandle INVALID_HANDLE = ~0u;

    IKSystem() = default;

    // Add an IK chain
    ChainHandle addChain(const IKChain& chain, SolverType solverType, uint32_t priority = 0);

    // Remove a chain
    void removeChain(ChainHandle handle);

    // Set target for a chain
    void setTarget(ChainHandle handle, const IKTarget& target);

    // Set pole vector for a chain
    void setPoleVector(ChainHandle handle, const PoleVector& pole);

    // Enable/disable a chain
    void setEnabled(ChainHandle handle, bool enabled);

    // Solve all enabled chains
    void solve(const Skeleton& skeleton, SkeletonPose& pose);

    // Configure solvers
    void setTwoBoneConfig(const IKConfig& config) { m_twoBone.setConfig(config); }
    void setFABRIKConfig(const IKConfig& config) { m_fabrik.setConfig(config); }
    void setCCDConfig(const IKConfig& config) { m_ccd.setConfig(config); }

    // Simple look-at control (for activity system)
    void setLookAtTarget(const glm::vec3& target) { m_lookAtTarget = target; m_hasLookAt = true; }
    void clearLookAtTarget() { m_hasLookAt = false; }
    void setLookAtWeight(float weight) { m_lookAtWeight = weight; }
    bool hasLookAtTarget() const { return m_hasLookAt; }
    glm::vec3 getLookAtTarget() const { return m_lookAtTarget; }
    float getLookAtWeight() const { return m_lookAtWeight; }

private:
    std::vector<ChainEntry> m_chains;
    std::vector<ChainHandle> m_handleToIndex;
    uint32_t m_nextHandle = 0;

    TwoBoneIK m_twoBone;
    FABRIKSolver m_fabrik;
    CCDSolver m_ccd;

    // Simple look-at state
    glm::vec3 m_lookAtTarget{0.0f};
    float m_lookAtWeight = 0.0f;
    bool m_hasLookAt = false;

    // Build bone chain indices from start to end
    std::vector<uint32_t> buildChainBones(const Skeleton& skeleton, const IKChain& chain) const;
};

// Utility functions
namespace IKUtils {
    // Calculate rotation between two vectors
    glm::quat rotationBetweenVectors(const glm::vec3& from, const glm::vec3& to);

    // Get bone positions for a chain
    std::vector<glm::vec3> getChainPositions(const Skeleton& skeleton,
                                              const SkeletonPose& pose,
                                              const std::vector<uint32_t>& chainBones);

    // Get bone lengths for a chain
    std::vector<float> getChainLengths(const std::vector<glm::vec3>& positions);

    // Apply positions back to pose
    void applyChainPositions(const Skeleton& skeleton, SkeletonPose& pose,
                             const std::vector<uint32_t>& chainBones,
                             const std::vector<glm::vec3>& positions);

    // Clamp angle within limits
    float clampAngle(float angle, float min, float max);
}

// =============================================================================
// LOOK-AT IK SOLVER
// For head/eye tracking targets
// =============================================================================

struct LookAtConfig {
    uint32_t headBoneIndex = UINT32_MAX;
    uint32_t neckBoneIndex = UINT32_MAX;
    uint32_t spineBoneIndex = UINT32_MAX;  // Optional spine involvement

    // Rotation limits (radians)
    float maxHeadYaw = 1.2f;     // Left/right
    float maxHeadPitch = 0.8f;   // Up/down
    float maxHeadRoll = 0.3f;    // Tilt

    float maxNeckYaw = 0.6f;
    float maxNeckPitch = 0.4f;

    float maxSpineYaw = 0.3f;
    float maxSpinePitch = 0.2f;

    // Distribution of rotation
    float headWeight = 0.6f;    // How much head contributes
    float neckWeight = 0.3f;    // How much neck contributes
    float spineWeight = 0.1f;   // How much spine contributes

    // Smoothing
    float trackingSpeed = 5.0f;  // How fast to track target
    float returnSpeed = 2.0f;    // How fast to return to neutral
};

class LookAtIK {
public:
    LookAtIK() = default;

    void initialize(const LookAtConfig& config);

    // Set the target to look at (world space)
    void setTarget(const glm::vec3& worldTarget);

    // Clear target (return to neutral)
    void clearTarget();

    // Set body transform for calculating relative direction
    void setBodyTransform(const glm::vec3& position, const glm::quat& rotation);

    // Update (call each frame for smooth tracking)
    void update(float deltaTime);

    // Apply to skeleton pose
    void applyToPose(const Skeleton& skeleton, SkeletonPose& pose) const;

    // Get current look direction (normalized, world space)
    glm::vec3 getCurrentLookDirection() const { return m_currentDirection; }

    // Is currently looking at target?
    bool hasTarget() const { return m_hasTarget; }

private:
    LookAtConfig m_config;

    glm::vec3 m_bodyPosition;
    glm::quat m_bodyRotation;

    glm::vec3 m_targetPosition;
    bool m_hasTarget = false;

    glm::vec3 m_currentDirection;   // Current look direction
    glm::vec3 m_targetDirection;    // Target look direction

    // Current rotation values
    float m_currentHeadYaw = 0.0f;
    float m_currentHeadPitch = 0.0f;
    float m_currentNeckYaw = 0.0f;
    float m_currentNeckPitch = 0.0f;
    float m_currentSpineYaw = 0.0f;
    float m_currentSpinePitch = 0.0f;
};

// =============================================================================
// TERRAIN FOOT PLACEMENT
// Adapts foot positions to uneven terrain
// =============================================================================

using TerrainRaycastFn = std::function<bool(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float maxDistance,
    glm::vec3& hitPoint,
    glm::vec3& hitNormal
)>;

struct FootPlacementConfig {
    uint32_t hipBoneIndex;
    uint32_t kneeBoneIndex;
    uint32_t ankleBoneIndex;
    uint32_t footBoneIndex;
    uint32_t toeBoneIndex = UINT32_MAX;  // Optional

    // Foot properties
    float footLength = 0.2f;        // Length of foot for toe placement
    float footWidth = 0.1f;         // Width for stability checks
    float ankleHeight = 0.05f;      // Height of ankle above ground

    // Raycasting
    float raycastHeight = 1.0f;     // How high above rest position to start ray
    float maxStepUp = 0.3f;         // Maximum height difference
    float maxStepDown = 0.5f;

    // Blending
    float plantBlendSpeed = 10.0f;  // How fast foot plants
    float liftBlendSpeed = 8.0f;    // How fast foot lifts

    // Stability
    float minGroundContact = 0.7f;  // Minimum contact for "planted"
};

struct FootPlacementState {
    glm::vec3 targetPosition;       // IK target position
    glm::vec3 groundPosition;       // Actual ground hit point
    glm::vec3 groundNormal;         // Ground surface normal
    glm::quat footRotation;         // Rotation to align with ground

    float groundDistance = 0.0f;    // Distance from rest to ground
    float plantedAmount = 0.0f;     // 0 = in air, 1 = fully planted
    bool isValid = false;           // Did raycast hit?
};

class TerrainFootPlacement {
public:
    TerrainFootPlacement() = default;

    void initialize(const std::vector<FootPlacementConfig>& footConfigs);
    void setTerrainCallback(TerrainRaycastFn callback);

    // Set body transform
    void setBodyTransform(const glm::vec3& position, const glm::quat& rotation);

    // Set foot rest positions (relative to body)
    void setFootRestPositions(const std::vector<glm::vec3>& restPositions);

    // Set which feet are currently in swing phase
    void setFootSwinging(size_t footIndex, bool swinging);

    // Update terrain sampling
    void update(float deltaTime, const Skeleton& skeleton, SkeletonPose& pose);

    // Get foot placement state
    const FootPlacementState& getFootState(size_t index) const;
    const std::vector<FootPlacementState>& getAllFootStates() const { return m_footStates; }

    // Apply IK to pose
    void applyToPose(const Skeleton& skeleton, SkeletonPose& pose, IKSystem& ikSystem);

    // Calculate body height adjustment based on terrain
    float getBodyHeightAdjustment() const;

    // Calculate body tilt to match average ground slope
    glm::quat getBodyTiltAdjustment() const;

private:
    std::vector<FootPlacementConfig> m_footConfigs;
    std::vector<FootPlacementState> m_footStates;
    std::vector<glm::vec3> m_footRestPositions;
    std::vector<bool> m_footSwinging;

    TerrainRaycastFn m_terrainCallback;

    glm::vec3 m_bodyPosition;
    glm::quat m_bodyRotation;

    // Sample terrain at foot position
    bool sampleTerrain(const glm::vec3& position, float rayHeight,
                       glm::vec3& hitPoint, glm::vec3& hitNormal);

    // Calculate foot rotation to align with ground
    glm::quat calculateFootRotation(const glm::vec3& groundNormal,
                                     const glm::vec3& movementDir) const;
};

// =============================================================================
// REACH IK - For grabbing/feeding behaviors
// =============================================================================

struct ReachConfig {
    uint32_t shoulderBoneIndex;
    uint32_t elbowBoneIndex;
    uint32_t wristBoneIndex;
    uint32_t handBoneIndex;

    // Constraints
    float maxReach = 1.0f;
    float minReach = 0.2f;

    // Joint limits
    float maxShoulderAngle = 2.0f;
    float maxElbowAngle = 2.5f;
    float maxWristAngle = 1.5f;

    // Behavior
    float reachSpeed = 3.0f;
    float returnSpeed = 2.0f;
};

class ReachIK {
public:
    ReachIK() = default;

    void initialize(const ReachConfig& leftArm, const ReachConfig& rightArm);

    // Reach toward target with specified hand
    void reachToward(bool isLeftHand, const glm::vec3& worldTarget, float urgency = 1.0f);

    // Release/retract
    void release(bool isLeftHand);

    // Update
    void update(float deltaTime);

    // Apply to pose
    void applyToPose(const Skeleton& skeleton, SkeletonPose& pose, IKSystem& ikSystem);

    // Query state
    bool isReaching(bool isLeftHand) const;
    float getReachProgress(bool isLeftHand) const;
    glm::vec3 getCurrentHandPosition(bool isLeftHand) const;

private:
    ReachConfig m_leftConfig, m_rightConfig;

    glm::vec3 m_leftTarget, m_rightTarget;
    glm::vec3 m_leftCurrentTarget, m_rightCurrentTarget;
    float m_leftReachAmount = 0.0f, m_rightReachAmount = 0.0f;
    float m_leftUrgency = 1.0f, m_rightUrgency = 1.0f;
    bool m_leftReaching = false, m_rightReaching = false;
};

// =============================================================================
// FULL BODY IK SYSTEM
// Coordinates all IK subsystems for a creature
// =============================================================================

class FullBodyIK {
public:
    FullBodyIK() = default;

    // Initialize subsystems
    void initializeLookAt(const LookAtConfig& config);
    void initializeFootPlacement(const std::vector<FootPlacementConfig>& footConfigs);
    void initializeReach(const ReachConfig& leftArm, const ReachConfig& rightArm);

    // Set terrain callback for foot placement
    void setTerrainCallback(TerrainRaycastFn callback);

    // Set body transform
    void setBodyTransform(const glm::vec3& position, const glm::quat& rotation);

    // Control methods
    void lookAt(const glm::vec3& target);
    void clearLookAt();
    void reachWith(bool isLeftHand, const glm::vec3& target);
    void releaseHand(bool isLeftHand);
    void setFootSwinging(size_t index, bool swinging);

    // Update all systems
    void update(float deltaTime, const Skeleton& skeleton, SkeletonPose& pose);

    // Apply all IK to pose
    void applyToPose(const Skeleton& skeleton, SkeletonPose& pose, IKSystem& ikSystem);

    // Get adjustments
    float getBodyHeightAdjustment() const;
    glm::quat getBodyTiltAdjustment() const;

    // Access subsystems
    LookAtIK& getLookAt() { return m_lookAt; }
    TerrainFootPlacement& getFootPlacement() { return m_footPlacement; }
    ReachIK& getReach() { return m_reach; }

private:
    LookAtIK m_lookAt;
    TerrainFootPlacement m_footPlacement;
    ReachIK m_reach;

    bool m_hasLookAt = false;
    bool m_hasFootPlacement = false;
    bool m_hasReach = false;
};

} // namespace animation
