#pragma once

#include "Skeleton.h"
#include "Pose.h"
#include "ActivitySystem.h"
#include "ProceduralRig.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <functional>

namespace animation {

// Forward declarations
class ProceduralLocomotion;
class IKSystem;

// =============================================================================
// MOTION CLIP TYPES
// =============================================================================

// Represents a single bone pose at a specific time
struct BonePoseKey {
    float time;                         // Time in seconds
    glm::vec3 position;                 // Local position offset
    glm::quat rotation;                 // Local rotation
    glm::vec3 scale;                    // Local scale (usually 1,1,1)

    static BonePoseKey identity(float t = 0.0f);
    static BonePoseKey lerp(const BonePoseKey& a, const BonePoseKey& b, float t);
};

// A channel of keyframes for one bone
struct BoneChannel {
    std::string boneName;
    int32_t boneIndex = -1;             // Resolved at runtime
    std::vector<BonePoseKey> keys;

    // Get interpolated pose at time t
    BonePoseKey sample(float time) const;

    // Add keyframe
    void addKey(const BonePoseKey& key);

    // Get duration
    float getDuration() const;
};

// A complete motion clip for an activity
struct ActivityMotionClip {
    std::string name;
    ActivityType activityType;
    float duration;
    bool isLooping;

    // Per-bone animation channels
    std::vector<BoneChannel> boneChannels;

    // Root motion
    bool hasRootMotion = false;
    std::vector<BonePoseKey> rootMotionKeys;

    // Additive animation (blended on top of base)
    bool isAdditive = false;
    float additiveWeight = 1.0f;

    // Blend masks (per-bone weights)
    std::vector<float> blendMask;       // If empty, all bones = 1.0

    // Sample all channels at time t
    void samplePose(float time, SkeletonPose& outPose, const Skeleton& skeleton) const;

    // Get root motion at time t
    BonePoseKey sampleRootMotion(float time) const;
};

// =============================================================================
// SECONDARY MOTION LAYER
// Procedural overlays for breathing, tail wagging, etc.
// =============================================================================

struct SecondaryMotionConfig {
    // Breathing
    bool enableBreathing = true;
    float breathingRate = 0.3f;         // Breaths per second
    float breathingAmplitude = 0.02f;   // Chest expansion amount

    // Tail motion
    bool enableTailMotion = true;
    float tailWagSpeed = 2.0f;
    float tailWagAmplitude = 0.3f;      // Radians
    float tailDroop = 0.1f;             // Gravity effect

    // Head bob
    bool enableHeadBob = true;
    float headBobSpeed = 1.0f;
    float headBobAmplitude = 0.01f;

    // Eye blink
    bool enableBlinking = true;
    float blinkRate = 0.15f;            // Blinks per second
    float blinkDuration = 0.1f;

    // Body sway (idle)
    bool enableBodySway = true;
    float swaySpeed = 0.3f;
    float swayAmplitude = 0.005f;

    // Ear/antenna twitch
    bool enableEarTwitch = true;
    float twitchRate = 0.2f;
    float twitchAmplitude = 0.15f;
};

class SecondaryMotionLayer {
public:
    SecondaryMotionLayer() = default;

    // Configure
    void setConfig(const SecondaryMotionConfig& config) { m_config = config; }
    void setMorphology(const MorphologyGenes& genes);

    // Update
    void update(float deltaTime);

    // Apply to pose
    void applyToPose(SkeletonPose& pose, const Skeleton& skeleton);

    // Get individual components
    glm::vec3 getBreathingOffset() const { return m_breathingOffset; }
    glm::quat getTailRotation(int segment) const;
    float getBlinkAmount() const { return m_blinkAmount; }

    // Set activity-based modifiers
    void setActivityState(ActivityType activity);
    void setMovementSpeed(float speed);
    void setArousalLevel(float arousal);    // 0=calm, 1=excited

private:
    SecondaryMotionConfig m_config;

    // Animation time
    float m_time = 0.0f;

    // Breathing state
    glm::vec3 m_breathingOffset{0.0f};
    float m_breathPhase = 0.0f;

    // Tail state
    std::vector<glm::quat> m_tailRotations;
    float m_tailPhase = 0.0f;

    // Head bob
    glm::vec3 m_headBobOffset{0.0f};

    // Blink state
    float m_blinkAmount = 0.0f;
    float m_nextBlinkTime = 0.0f;
    bool m_isBlinking = false;

    // Body sway
    glm::vec3 m_swayOffset{0.0f};

    // Ear twitch
    float m_earTwitchAmount = 0.0f;

    // Modifiers
    ActivityType m_currentActivity = ActivityType::IDLE;
    float m_movementSpeed = 0.0f;
    float m_arousalLevel = 0.5f;

    // Morphology info
    bool m_hasTail = true;
    int m_tailSegments = 5;
    bool m_hasEars = true;
};

// =============================================================================
// ACTIVITY MOTION GENERATOR
// Procedurally generates motion clips for activities
// =============================================================================

class ActivityMotionGenerator {
public:
    ActivityMotionGenerator() = default;

    // Configure for morphology
    void setMorphology(const MorphologyGenes& genes);
    void setRigDefinition(const RigDefinition& rig);

    // Generate clips for all activities
    void generateAllClips();

    // Generate specific activity clip
    ActivityMotionClip generateClip(ActivityType type) const;

    // Get generated clip
    const ActivityMotionClip* getClip(ActivityType type) const;

    // Procedural clip generators
    ActivityMotionClip generateIdleClip() const;
    ActivityMotionClip generateEatingClip() const;
    ActivityMotionClip generateDrinkingClip() const;
    ActivityMotionClip generateMatingClip() const;
    ActivityMotionClip generateSleepingClip() const;
    ActivityMotionClip generateExcretingClip(ExcretionType type) const;
    ActivityMotionClip generateGroomingClip(GroomingType type) const;
    ActivityMotionClip generateThreatDisplayClip() const;
    ActivityMotionClip generateSubmissiveClip() const;
    ActivityMotionClip generateMatingDisplayClip() const;
    ActivityMotionClip generatePlayingClip() const;
    ActivityMotionClip generateInvestigatingClip() const;
    ActivityMotionClip generateCallingClip() const;

private:
    MorphologyGenes m_genes;
    RigDefinition m_rig;

    // Cached clips
    std::vector<ActivityMotionClip> m_clips;

    // Helper functions
    void addSpineWave(BoneChannel& channel, float amplitude, float frequency, float duration) const;
    void addHeadBob(BoneChannel& channel, float amplitude, float frequency, float duration) const;
    void addTailWag(BoneChannel& channel, float amplitude, float frequency, float duration) const;
    void addBodySquat(BoneChannel& channel, float depth, float holdTime, float duration) const;
    void addLimbRaise(BoneChannel& channel, float angle, float holdTime, float duration) const;

    // Motion primitives
    BonePoseKey createSquatKey(float depth) const;
    BonePoseKey createLeanKey(float angle, const glm::vec3& axis) const;
    BonePoseKey createStretchKey(float amount) const;
};

// =============================================================================
// ACTIVITY ANIMATION BLENDER
// Blends multiple motion clips together
// =============================================================================

struct BlendedMotion {
    const ActivityMotionClip* clip;
    float weight;
    float time;
    float playbackSpeed;
};

class ActivityAnimationBlender {
public:
    ActivityAnimationBlender() = default;

    // Set skeleton reference
    void setSkeleton(const Skeleton* skeleton) { m_skeleton = skeleton; }

    // Add motion to blend
    void addMotion(const ActivityMotionClip* clip, float weight, float time = 0.0f, float speed = 1.0f);
    void clearMotions();

    // Set base pose (before activity animations)
    void setBasePose(const SkeletonPose& basePose);

    // Update and blend
    void update(float deltaTime);

    // Get blended result
    void getBlendedPose(SkeletonPose& outPose) const;

    // Transition helpers
    void transitionTo(const ActivityMotionClip* newClip, float transitionTime);
    bool isTransitioning() const { return m_isTransitioning; }
    float getTransitionProgress() const { return m_transitionProgress; }

private:
    const Skeleton* m_skeleton = nullptr;
    SkeletonPose m_basePose;

    std::vector<BlendedMotion> m_motions;

    // Transition state
    bool m_isTransitioning = false;
    float m_transitionProgress = 0.0f;
    float m_transitionDuration = 0.3f;
    const ActivityMotionClip* m_transitionTarget = nullptr;
};

// =============================================================================
// ACTIVITY POSE MODIFIERS
// Special pose modifications for specific activities
// =============================================================================

namespace ActivityPoseModifiers {

    // Eating pose - head down, mouth open
    void applyEatingPose(SkeletonPose& pose, const Skeleton& skeleton,
                         float progress, const glm::vec3& foodPosition);

    // Drinking pose - head lowered to water
    void applyDrinkingPose(SkeletonPose& pose, const Skeleton& skeleton,
                           float progress, const glm::vec3& waterPosition);

    // Sleeping pose - body lowered, curled
    void applySleepingPose(SkeletonPose& pose, const Skeleton& skeleton,
                           float progress, bool curledUp = false);

    // Excretion pose - squat/leg lift
    void applyExcretionPose(SkeletonPose& pose, const Skeleton& skeleton,
                            float progress, ExcretionType type);

    // Grooming poses
    void applyGroomingScratchPose(SkeletonPose& pose, const Skeleton& skeleton,
                                  float progress, bool leftSide);
    void applyGroomingLickPose(SkeletonPose& pose, const Skeleton& skeleton,
                               float progress, const glm::vec3& targetSpot);
    void applyGroomingShakePose(SkeletonPose& pose, const Skeleton& skeleton,
                                float progress);
    void applyGroomingStretchPose(SkeletonPose& pose, const Skeleton& skeleton,
                                  float progress);

    // Display poses
    void applyThreatDisplayPose(SkeletonPose& pose, const Skeleton& skeleton,
                                float progress, bool hasWings, bool hasCrest);
    void applySubmissivePose(SkeletonPose& pose, const Skeleton& skeleton,
                             float progress);
    void applyMatingDisplayPose(SkeletonPose& pose, const Skeleton& skeleton,
                                float progress, bool hasWings, bool hasTail);

    // Play poses
    void applyPlayBowPose(SkeletonPose& pose, const Skeleton& skeleton,
                          float progress);
    void applyPlayPounce(SkeletonPose& pose, const Skeleton& skeleton,
                         float progress);

    // Calling pose
    void applyCallingPose(SkeletonPose& pose, const Skeleton& skeleton,
                          float progress);
}

// =============================================================================
// IK TARGETS FOR ACTIVITIES
// =============================================================================

struct ActivityIKTargets {
    // Head look-at target
    bool hasLookTarget = false;
    glm::vec3 lookTarget;

    // Foot IK targets (for standing activities)
    std::vector<glm::vec3> footTargets;
    std::vector<bool> footGrounded;

    // Hand IK targets (for grooming, eating)
    bool hasLeftHandTarget = false;
    bool hasRightHandTarget = false;
    glm::vec3 leftHandTarget;
    glm::vec3 rightHandTarget;

    // Tail IK target (for balance)
    bool hasTailTarget = false;
    glm::vec3 tailTarget;
};

class ActivityIKController {
public:
    ActivityIKController() = default;

    // Set IK system to control
    void setIKSystem(IKSystem* ikSystem) { m_ikSystem = ikSystem; }
    void setSkeleton(const Skeleton* skeleton) { m_skeleton = skeleton; }

    // Update targets based on activity
    void updateForActivity(ActivityType activity, float progress,
                           const glm::vec3& creaturePosition,
                           const glm::vec3& targetPosition);

    // Get current targets
    const ActivityIKTargets& getTargets() const { return m_targets; }

    // Apply IK to pose
    void applyIK(SkeletonPose& pose);

private:
    IKSystem* m_ikSystem = nullptr;
    const Skeleton* m_skeleton = nullptr;
    ActivityIKTargets m_targets;

    // Bone indices (cached)
    int32_t m_headBone = -1;
    int32_t m_neckBone = -1;
    std::vector<int32_t> m_footBones;
    int32_t m_leftHandBone = -1;
    int32_t m_rightHandBone = -1;

    void cacheBoneIndices();
};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

namespace ActivityAnimationUtils {

    // Create smooth ease-in-out curve
    float smoothstep(float t);
    float smootherstep(float t);

    // Create bounce effect
    float bounce(float t, int bounces = 3);

    // Create spring effect
    float spring(float t, float stiffness = 0.5f, float damping = 0.3f);

    // Interpolate quaternions along a path
    glm::quat slerpPath(const std::vector<glm::quat>& path, float t);

    // Generate wave motion
    glm::quat waveRotation(float phase, float amplitude, const glm::vec3& axis);

    // Calculate body tilt from movement
    glm::quat calculateMovementTilt(const glm::vec3& velocity, float maxTilt = 0.1f);
}

} // namespace animation
