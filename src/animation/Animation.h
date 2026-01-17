#pragma once

// Animation System - Unified Header
// Include this single header to access all animation functionality

#include "Skeleton.h"
#include "Pose.h"
#include "IKSolver.h"
#include "ProceduralLocomotion.h"
#include "ActivitySystem.h"

namespace animation {

// Animation system constants
constexpr float DEFAULT_ANIMATION_FPS = 60.0f;
constexpr float DEFAULT_BLEND_DURATION = 0.2f;

// Creature animation controller - combines skeletal animation with procedural locomotion
class CreatureAnimator {
public:
    CreatureAnimator() = default;

    // Initialize for a creature type
    void initializeBiped(float height = 1.0f);
    void initializeQuadruped(float length = 1.0f, float height = 0.5f);
    void initializeFlying(float wingspan = 1.5f);
    void initializeAquatic(float length = 1.0f);
    void initializeSerpentine(float length = 2.0f);

    // PHASE 7 - Agent 3: Amphibious animation support
    void initializeAmphibious(float length = 1.0f, float height = 0.5f);

    // Set amphibious animation blend (0 = swim, 1 = walk)
    void setAmphibiousBlend(float blend) { m_amphibiousBlend = blend; }
    float getAmphibiousBlend() const { return m_amphibiousBlend; }

    // Set custom skeleton
    void setSkeleton(const Skeleton& skeleton);
    void setSkeleton(Skeleton&& skeleton);

    // Access skeleton
    const Skeleton& getSkeleton() const { return m_skeleton; }
    Skeleton& getSkeleton() { return m_skeleton; }

    // Access pose
    const SkeletonPose& getPose() const { return m_pose; }
    SkeletonPose& getPose() { return m_pose; }

    // Access locomotion controller
    const ProceduralLocomotion& getLocomotion() const { return m_locomotion; }
    ProceduralLocomotion& getLocomotion() { return m_locomotion; }

    // Access IK system
    const IKSystem& getIKSystem() const { return m_ikSystem; }
    IKSystem& getIKSystem() { return m_ikSystem; }

    // Access activity animation driver
    const ActivityAnimationDriver& getActivityDriver() const { return m_activityDriver; }
    ActivityAnimationDriver& getActivityDriver() { return m_activityDriver; }

    // Set activity state machine (called by Creature)
    void setActivityStateMachine(ActivityStateMachine* stateMachine) {
        m_activityDriver.setStateMachine(stateMachine);
    }

    // Update animation (call each frame)
    void update(float deltaTime);

    // Set movement parameters
    void setVelocity(const glm::vec3& velocity);
    void setAngularVelocity(float omega);
    void setPosition(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);

    // Get skinning matrices for GPU upload
    const std::vector<glm::mat4>& getSkinningMatrices() const { return m_pose.getSkinningMatrices(); }

    // Get bone count
    uint32_t getBoneCount() const { return m_skeleton.getBoneCount(); }

    // Reset to bind pose
    void resetToBindPose();

private:
    Skeleton m_skeleton;
    SkeletonPose m_pose;
    ProceduralLocomotion m_locomotion;
    IKSystem m_ikSystem;
    ActivityAnimationDriver m_activityDriver;

    glm::vec3 m_position{0.0f};
    glm::quat m_rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 m_velocity{0.0f};
    float m_angularVelocity = 0.0f;

    // PHASE 7 - Agent 3: Amphibious animation blending
    float m_amphibiousBlend = 0.0f;     // 0 = swim animation, 1 = walk animation
    bool m_isAmphibious = false;         // True if initialized as amphibious
};

// GPU skinning data for upload
struct GPUSkinningData {
    static constexpr uint32_t MAX_BONES = animation::MAX_BONES;

    glm::mat4 boneMatrices[MAX_BONES];
    uint32_t activeBoneCount = 0;
    float padding[3] = {0.0f, 0.0f, 0.0f};

    void uploadFromPose(const SkeletonPose& pose);
    void clear();
};

// Inline implementations

inline void CreatureAnimator::initializeBiped(float height) {
    m_skeleton = SkeletonFactory::createBiped(height);
    m_pose = SkeletonPose(m_skeleton);
    m_locomotion.initialize(m_skeleton);
    LocomotionSetup::setupBiped(m_locomotion, m_skeleton);
}

inline void CreatureAnimator::initializeQuadruped(float length, float height) {
    m_skeleton = SkeletonFactory::createQuadruped(length, height);
    m_pose = SkeletonPose(m_skeleton);
    m_locomotion.initialize(m_skeleton);
    LocomotionSetup::setupQuadruped(m_locomotion, m_skeleton);
}

inline void CreatureAnimator::initializeFlying(float wingspan) {
    m_skeleton = SkeletonFactory::createFlying(wingspan);
    m_pose = SkeletonPose(m_skeleton);
    m_locomotion.initialize(m_skeleton);
    LocomotionSetup::setupFlying(m_locomotion, m_skeleton);
}

inline void CreatureAnimator::initializeAquatic(float length) {
    m_skeleton = SkeletonFactory::createAquatic(length);
    m_pose = SkeletonPose(m_skeleton);
    m_locomotion.initialize(m_skeleton);
    LocomotionSetup::setupAquatic(m_locomotion, m_skeleton);
}

inline void CreatureAnimator::initializeSerpentine(float length) {
    m_skeleton = SkeletonFactory::createSerpentine(length);
    m_pose = SkeletonPose(m_skeleton);
    m_locomotion.initialize(m_skeleton);
    LocomotionSetup::setupSerpentine(m_locomotion, m_skeleton);
}

// PHASE 7 - Agent 3: Amphibious initialization
// Creates a hybrid skeleton that can blend between aquatic and quadruped locomotion
inline void CreatureAnimator::initializeAmphibious(float length, float height) {
    // Start with a quadruped base (can transition to aquatic-like movement)
    m_skeleton = SkeletonFactory::createQuadruped(length, height);
    m_pose = SkeletonPose(m_skeleton);
    m_locomotion.initialize(m_skeleton);

    // Use quadruped setup as base - locomotion blending handles the rest
    LocomotionSetup::setupQuadruped(m_locomotion, m_skeleton);

    m_isAmphibious = true;
    m_amphibiousBlend = 0.5f;  // Start at 50% blend (can swim or walk)
}

inline void CreatureAnimator::setSkeleton(const Skeleton& skeleton) {
    m_skeleton = skeleton;
    m_pose = SkeletonPose(m_skeleton);
}

inline void CreatureAnimator::setSkeleton(Skeleton&& skeleton) {
    m_skeleton = std::move(skeleton);
    m_pose = SkeletonPose(m_skeleton);
}

inline void CreatureAnimator::update(float deltaTime) {
    // Update locomotion state
    m_locomotion.setBodyPosition(m_position);
    m_locomotion.setBodyRotation(m_rotation);
    m_locomotion.setVelocity(m_velocity);
    m_locomotion.setAngularVelocity(m_angularVelocity);

    // Update locomotion
    m_locomotion.update(deltaTime);

    // Update activity animation driver
    m_activityDriver.update(deltaTime);

    // Apply locomotion to pose (base layer)
    m_locomotion.applyToPose(m_skeleton, m_pose, m_ikSystem);

    // Blend in activity animations (overlay layer)
    m_activityDriver.applyToPose(m_skeleton, m_pose, &m_locomotion, &m_ikSystem);

    // Update pose matrices
    m_pose.updateMatrices(m_skeleton);
}

inline void CreatureAnimator::setVelocity(const glm::vec3& velocity) {
    m_velocity = velocity;
}

inline void CreatureAnimator::setAngularVelocity(float omega) {
    m_angularVelocity = omega;
}

inline void CreatureAnimator::setPosition(const glm::vec3& position) {
    m_position = position;
}

inline void CreatureAnimator::setRotation(const glm::quat& rotation) {
    m_rotation = rotation;
}

inline void CreatureAnimator::resetToBindPose() {
    m_pose.setToBindPose(m_skeleton);
    m_pose.updateMatrices(m_skeleton);
}

inline void GPUSkinningData::uploadFromPose(const SkeletonPose& pose) {
    const auto& matrices = pose.getSkinningMatrices();
    activeBoneCount = static_cast<uint32_t>(std::min(matrices.size(), size_t(MAX_BONES)));

    for (uint32_t i = 0; i < activeBoneCount; ++i) {
        boneMatrices[i] = matrices[i];
    }

    // Clear remaining matrices to identity
    for (uint32_t i = activeBoneCount; i < MAX_BONES; ++i) {
        boneMatrices[i] = glm::mat4(1.0f);
    }
}

inline void GPUSkinningData::clear() {
    activeBoneCount = 0;
    for (uint32_t i = 0; i < MAX_BONES; ++i) {
        boneMatrices[i] = glm::mat4(1.0f);
    }
}

} // namespace animation
