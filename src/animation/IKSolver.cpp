#include "IKSolver.h"
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>

namespace animation {

// ============================================================================
// Two-Bone IK Solver
// ============================================================================

bool TwoBoneIK::solve(const Skeleton& skeleton, SkeletonPose& pose,
                      uint32_t upperBone, uint32_t lowerBone, uint32_t effectorBone,
                      const IKTarget& target) const {
    // Get current world positions
    pose.calculateGlobalTransforms(skeleton);

    glm::vec3 upperPos = glm::vec3(pose.getGlobalTransform(upperBone)[3]);
    glm::vec3 lowerPos = glm::vec3(pose.getGlobalTransform(lowerBone)[3]);
    glm::vec3 effectorPos = glm::vec3(pose.getGlobalTransform(effectorBone)[3]);

    // Calculate bone lengths
    float upperLen = glm::length(lowerPos - upperPos);
    float lowerLen = glm::length(effectorPos - lowerPos);
    float totalLen = upperLen + lowerLen;

    // Vector from upper bone to target
    glm::vec3 toTarget = target.position - upperPos;
    float targetDist = glm::length(toTarget);

    // Check if target is reachable
    if (targetDist < 0.0001f) {
        return false; // Target too close
    }

    // Clamp target distance to reachable range
    float softTotal = totalLen * m_config.softLimit;
    if (targetDist > softTotal) {
        targetDist = softTotal;
        toTarget = glm::normalize(toTarget) * targetDist;
    }

    // Calculate bend angle using law of cosines
    float bendAngle = calculateBendAngle(upperLen, lowerLen, targetDist);

    // Calculate upper bone rotation
    glm::vec3 targetDir = glm::normalize(toTarget);
    glm::vec3 currentDir = glm::normalize(lowerPos - upperPos);

    // Angle for upper bone to point at correct position
    float upperAngle = std::acos(glm::clamp((upperLen * upperLen + targetDist * targetDist - lowerLen * lowerLen)
                                            / (2.0f * upperLen * targetDist), -1.0f, 1.0f));

    // Rotate upper bone
    glm::quat rotToTarget = IKUtils::rotationBetweenVectors(currentDir, targetDir);
    glm::vec3 axis = glm::cross(targetDir, glm::vec3(0, 1, 0));
    if (glm::length(axis) < 0.0001f) {
        axis = glm::vec3(1, 0, 0);
    }
    axis = glm::normalize(axis);

    glm::quat bendRot = glm::angleAxis(-upperAngle, axis);

    BoneTransform& upperTransform = pose.getLocalTransform(upperBone);
    upperTransform.rotation = rotToTarget * bendRot * upperTransform.rotation;

    // Recalculate to get new lower position
    pose.calculateGlobalTransforms(skeleton);
    glm::vec3 newLowerPos = glm::vec3(pose.getGlobalTransform(lowerBone)[3]);

    // Rotate lower bone to reach target
    glm::vec3 lowerDir = glm::normalize(effectorPos - newLowerPos);
    glm::vec3 targetLowerDir = glm::normalize(target.position - newLowerPos);

    glm::quat lowerRot = IKUtils::rotationBetweenVectors(lowerDir, targetLowerDir);

    BoneTransform& lowerTransform = pose.getLocalTransform(lowerBone);
    lowerTransform.rotation = lowerRot * lowerTransform.rotation;

    // Apply target weight
    if (target.weight < 1.0f) {
        upperTransform.rotation = glm::slerp(skeleton.getBone(upperBone).bindPose.rotation,
                                             upperTransform.rotation, target.weight);
        lowerTransform.rotation = glm::slerp(skeleton.getBone(lowerBone).bindPose.rotation,
                                             lowerTransform.rotation, target.weight);
    }

    return true;
}

bool TwoBoneIK::solveWithPole(const Skeleton& skeleton, SkeletonPose& pose,
                              uint32_t upperBone, uint32_t lowerBone, uint32_t effectorBone,
                              const IKTarget& target, const PoleVector& pole) const {
    // First solve without pole
    if (!solve(skeleton, pose, upperBone, lowerBone, effectorBone, target)) {
        return false;
    }

    if (!pole.enabled) {
        return true;
    }

    // Apply pole vector
    pose.calculateGlobalTransforms(skeleton);

    glm::vec3 upperPos = glm::vec3(pose.getGlobalTransform(upperBone)[3]);
    glm::vec3 lowerPos = glm::vec3(pose.getGlobalTransform(lowerBone)[3]);

    BoneTransform& upperTransform = pose.getLocalTransform(upperBone);
    applyPoleVector(upperTransform.rotation, upperPos, lowerPos, target.position, pole);

    return true;
}

float TwoBoneIK::calculateBendAngle(float upperLen, float lowerLen, float targetDist) const {
    // Law of cosines: c^2 = a^2 + b^2 - 2ab*cos(C)
    // Solve for angle at lower joint
    float cosAngle = (upperLen * upperLen + lowerLen * lowerLen - targetDist * targetDist)
                     / (2.0f * upperLen * lowerLen);
    cosAngle = glm::clamp(cosAngle, -1.0f, 1.0f);

    float angle = std::acos(cosAngle);

    // Enforce minimum bend
    if (angle < m_config.minBendAngle) {
        angle = m_config.minBendAngle;
    }

    return angle;
}

void TwoBoneIK::applyPoleVector(glm::quat& upperRot, const glm::vec3& upperPos,
                                const glm::vec3& lowerPos, const glm::vec3& targetPos,
                                const PoleVector& pole) const {
    // Create plane from upper -> target
    glm::vec3 limbAxis = glm::normalize(targetPos - upperPos);

    // Project pole and lower onto this plane
    glm::vec3 toMid = lowerPos - upperPos;
    glm::vec3 toPole = pole.position - upperPos;

    // Remove component along limb axis
    toMid = toMid - glm::dot(toMid, limbAxis) * limbAxis;
    toPole = toPole - glm::dot(toPole, limbAxis) * limbAxis;

    if (glm::length(toMid) < 0.0001f || glm::length(toPole) < 0.0001f) {
        return;
    }

    toMid = glm::normalize(toMid);
    toPole = glm::normalize(toPole);

    // Calculate rotation around limb axis
    float angle = std::acos(glm::clamp(glm::dot(toMid, toPole), -1.0f, 1.0f));
    glm::vec3 cross = glm::cross(toMid, toPole);
    if (glm::dot(cross, limbAxis) < 0) {
        angle = -angle;
    }

    // Apply weighted rotation
    glm::quat poleRot = glm::angleAxis(angle * pole.weight, limbAxis);
    upperRot = poleRot * upperRot;
}

// ============================================================================
// FABRIK Solver
// ============================================================================

bool FABRIKSolver::solve(const Skeleton& skeleton, SkeletonPose& pose,
                         const std::vector<uint32_t>& chainBones,
                         const IKTarget& target) const {
    if (chainBones.size() < 2) {
        return false;
    }

    // Get current positions
    pose.calculateGlobalTransforms(skeleton);
    std::vector<glm::vec3> positions = IKUtils::getChainPositions(skeleton, pose, chainBones);
    std::vector<float> boneLengths = IKUtils::getChainLengths(positions);

    glm::vec3 rootPos = positions[0];
    float totalLength = 0.0f;
    for (float len : boneLengths) {
        totalLength += len;
    }

    // Check if target is reachable
    float targetDist = glm::length(target.position - rootPos);
    if (targetDist > totalLength) {
        // Target unreachable - stretch toward it
        glm::vec3 dir = glm::normalize(target.position - rootPos);
        for (size_t i = 1; i < positions.size(); ++i) {
            positions[i] = positions[i-1] + dir * boneLengths[i-1];
        }
    } else {
        // Iterative solve
        for (uint32_t iter = 0; iter < m_config.maxIterations; ++iter) {
            // Forward pass
            forwardPass(positions, target.position);

            // Backward pass
            backwardPass(positions, rootPos, boneLengths);

            // Check convergence
            float dist = glm::length(positions.back() - target.position);
            if (dist < m_config.tolerance) {
                break;
            }
        }
    }

    // Apply positions back to pose
    IKUtils::applyChainPositions(skeleton, pose, chainBones, positions);

    return true;
}

bool FABRIKSolver::solveConstrained(const Skeleton& skeleton, SkeletonPose& pose,
                                    const std::vector<uint32_t>& chainBones,
                                    const IKTarget& target,
                                    const std::vector<glm::vec2>& angleConstraints) const {
    if (chainBones.size() < 2) {
        return false;
    }

    pose.calculateGlobalTransforms(skeleton);
    std::vector<glm::vec3> positions = IKUtils::getChainPositions(skeleton, pose, chainBones);
    std::vector<float> boneLengths = IKUtils::getChainLengths(positions);

    glm::vec3 rootPos = positions[0];

    for (uint32_t iter = 0; iter < m_config.maxIterations; ++iter) {
        forwardPass(positions, target.position);
        backwardPass(positions, rootPos, boneLengths);
        applyConstraints(positions, boneLengths, angleConstraints);

        float dist = glm::length(positions.back() - target.position);
        if (dist < m_config.tolerance) {
            break;
        }
    }

    IKUtils::applyChainPositions(skeleton, pose, chainBones, positions);
    return true;
}

void FABRIKSolver::forwardPass(std::vector<glm::vec3>& positions, const glm::vec3& targetPos) const {
    // Move effector to target
    positions.back() = targetPos;

    // Propagate backwards maintaining bone lengths
    for (int i = static_cast<int>(positions.size()) - 2; i >= 0; --i) {
        glm::vec3 dir = glm::normalize(positions[i] - positions[i + 1]);
        float len = glm::length(positions[i + 1] - positions[i]);
        positions[i] = positions[i + 1] + dir * len;
    }
}

void FABRIKSolver::backwardPass(std::vector<glm::vec3>& positions, const glm::vec3& rootPos,
                                const std::vector<float>& boneLengths) const {
    // Pin root
    positions[0] = rootPos;

    // Propagate forward maintaining bone lengths
    for (size_t i = 0; i < positions.size() - 1; ++i) {
        glm::vec3 dir = glm::normalize(positions[i + 1] - positions[i]);
        positions[i + 1] = positions[i] + dir * boneLengths[i];
    }
}

void FABRIKSolver::applyConstraints(std::vector<glm::vec3>& positions,
                                    const std::vector<float>& boneLengths,
                                    const std::vector<glm::vec2>& constraints) const {
    // Simple cone constraints
    for (size_t i = 1; i < positions.size() - 1 && i < constraints.size(); ++i) {
        glm::vec3 parentDir = glm::normalize(positions[i] - positions[i - 1]);
        glm::vec3 childDir = glm::normalize(positions[i + 1] - positions[i]);

        float angle = std::acos(glm::clamp(glm::dot(parentDir, childDir), -1.0f, 1.0f));
        float minAngle = constraints[i].x;
        float maxAngle = constraints[i].y;

        if (angle < minAngle || angle > maxAngle) {
            float clampedAngle = glm::clamp(angle, minAngle, maxAngle);
            glm::vec3 axis = glm::cross(parentDir, childDir);
            if (glm::length(axis) > 0.0001f) {
                axis = glm::normalize(axis);
                glm::quat rot = glm::angleAxis(clampedAngle - angle, axis);
                childDir = rot * childDir;
                positions[i + 1] = positions[i] + childDir * boneLengths[i];
            }
        }
    }
}

// ============================================================================
// CCD Solver
// ============================================================================

bool CCDSolver::solve(const Skeleton& skeleton, SkeletonPose& pose,
                      const std::vector<uint32_t>& chainBones,
                      const IKTarget& target) const {
    if (chainBones.size() < 2) {
        return false;
    }

    for (uint32_t iter = 0; iter < m_config.maxIterations; ++iter) {
        pose.calculateGlobalTransforms(skeleton);

        glm::vec3 effectorPos = glm::vec3(pose.getGlobalTransform(chainBones.back())[3]);
        float dist = glm::length(effectorPos - target.position);

        if (dist < m_config.tolerance) {
            return true;
        }

        // Iterate through joints from effector to root
        for (int i = static_cast<int>(chainBones.size()) - 2; i >= 0; --i) {
            pose.calculateGlobalTransforms(skeleton);
            effectorPos = glm::vec3(pose.getGlobalTransform(chainBones.back())[3]);
            rotateJointToward(pose, chainBones[i], effectorPos, target.position);
        }
    }

    return true;
}

void CCDSolver::rotateJointToward(SkeletonPose& pose, uint32_t jointIndex,
                                  const glm::vec3& effectorPos, const glm::vec3& targetPos) const {
    glm::vec3 jointPos = glm::vec3(pose.getGlobalTransform(jointIndex)[3]);

    glm::vec3 toEffector = glm::normalize(effectorPos - jointPos);
    glm::vec3 toTarget = glm::normalize(targetPos - jointPos);

    float dot = glm::clamp(glm::dot(toEffector, toTarget), -1.0f, 1.0f);
    float angle = std::acos(dot) * m_config.damping;

    if (angle < 0.0001f) {
        return;
    }

    glm::vec3 axis = glm::cross(toEffector, toTarget);
    if (glm::length(axis) < 0.0001f) {
        return;
    }
    axis = glm::normalize(axis);

    // Convert to local space rotation
    glm::mat4 globalInverse = glm::inverse(pose.getGlobalTransform(jointIndex));
    glm::vec3 localAxis = glm::normalize(glm::vec3(globalInverse * glm::vec4(axis, 0.0f)));

    glm::quat rot = glm::angleAxis(angle, localAxis);
    BoneTransform& transform = pose.getLocalTransform(jointIndex);
    transform.rotation = rot * transform.rotation;
}

// ============================================================================
// IK System Manager
// ============================================================================

IKSystem::ChainHandle IKSystem::addChain(const IKChain& chain, SolverType solverType, uint32_t priority) {
    ChainEntry entry;
    entry.chain = chain;
    entry.solverType = solverType;
    entry.priority = priority;
    entry.enabled = true;

    ChainHandle handle = m_nextHandle++;
    m_handleToIndex.resize(handle + 1, INVALID_HANDLE);
    m_handleToIndex[handle] = static_cast<ChainHandle>(m_chains.size());
    m_chains.push_back(entry);

    return handle;
}

void IKSystem::removeChain(ChainHandle handle) {
    if (handle >= m_handleToIndex.size() || m_handleToIndex[handle] == INVALID_HANDLE) {
        return;
    }

    uint32_t index = m_handleToIndex[handle];
    m_chains.erase(m_chains.begin() + index);
    m_handleToIndex[handle] = INVALID_HANDLE;

    // Update indices
    for (auto& idx : m_handleToIndex) {
        if (idx != INVALID_HANDLE && idx > index) {
            --idx;
        }
    }
}

void IKSystem::setTarget(ChainHandle handle, const IKTarget& target) {
    if (handle < m_handleToIndex.size() && m_handleToIndex[handle] != INVALID_HANDLE) {
        m_chains[m_handleToIndex[handle]].target = target;
    }
}

void IKSystem::setPoleVector(ChainHandle handle, const PoleVector& pole) {
    if (handle < m_handleToIndex.size() && m_handleToIndex[handle] != INVALID_HANDLE) {
        m_chains[m_handleToIndex[handle]].pole = pole;
    }
}

void IKSystem::setEnabled(ChainHandle handle, bool enabled) {
    if (handle < m_handleToIndex.size() && m_handleToIndex[handle] != INVALID_HANDLE) {
        m_chains[m_handleToIndex[handle]].enabled = enabled;
    }
}

void IKSystem::solve(const Skeleton& skeleton, SkeletonPose& pose) {
    // Sort chains by priority (higher first)
    std::vector<size_t> sortedIndices(m_chains.size());
    for (size_t i = 0; i < sortedIndices.size(); ++i) {
        sortedIndices[i] = i;
    }
    std::sort(sortedIndices.begin(), sortedIndices.end(),
              [this](size_t a, size_t b) {
                  return m_chains[a].priority > m_chains[b].priority;
              });

    // Solve each chain
    for (size_t idx : sortedIndices) {
        const ChainEntry& entry = m_chains[idx];
        if (!entry.enabled) continue;

        std::vector<uint32_t> chainBones = buildChainBones(skeleton, entry.chain);
        if (chainBones.empty()) continue;

        switch (entry.solverType) {
            case SolverType::TwoBone:
                if (chainBones.size() >= 3) {
                    m_twoBone.solveWithPole(skeleton, pose,
                                            chainBones[0], chainBones[1], chainBones[2],
                                            entry.target, entry.pole);
                }
                break;

            case SolverType::FABRIK:
                m_fabrik.solve(skeleton, pose, chainBones, entry.target);
                break;

            case SolverType::CCD:
                m_ccd.solve(skeleton, pose, chainBones, entry.target);
                break;
        }
    }
}

std::vector<uint32_t> IKSystem::buildChainBones(const Skeleton& skeleton, const IKChain& chain) const {
    std::vector<uint32_t> bones;

    // Trace from end to start
    int32_t current = chain.endBoneIndex;
    while (current >= 0 && bones.size() <= chain.chainLength) {
        bones.push_back(current);
        if (static_cast<uint32_t>(current) == chain.startBoneIndex) {
            break;
        }
        current = skeleton.getBone(current).parentIndex;
    }

    // Reverse to get root-to-effector order
    std::reverse(bones.begin(), bones.end());

    return bones;
}

// ============================================================================
// IK Utilities
// ============================================================================

namespace IKUtils {

glm::quat rotationBetweenVectors(const glm::vec3& from, const glm::vec3& to) {
    glm::vec3 f = glm::normalize(from);
    glm::vec3 t = glm::normalize(to);

    float dot = glm::dot(f, t);

    if (dot > 0.9999f) {
        return glm::quat(1, 0, 0, 0); // Identity
    }

    if (dot < -0.9999f) {
        // Opposite directions - find perpendicular axis
        glm::vec3 axis = glm::cross(glm::vec3(1, 0, 0), f);
        if (glm::length(axis) < 0.0001f) {
            axis = glm::cross(glm::vec3(0, 1, 0), f);
        }
        axis = glm::normalize(axis);
        return glm::angleAxis(glm::pi<float>(), axis);
    }

    glm::vec3 axis = glm::cross(f, t);
    float s = std::sqrt((1.0f + dot) * 2.0f);
    float invs = 1.0f / s;

    return glm::quat(s * 0.5f, axis.x * invs, axis.y * invs, axis.z * invs);
}

std::vector<glm::vec3> getChainPositions(const Skeleton& skeleton,
                                          const SkeletonPose& pose,
                                          const std::vector<uint32_t>& chainBones) {
    std::vector<glm::vec3> positions;
    positions.reserve(chainBones.size());

    for (uint32_t boneIndex : chainBones) {
        positions.push_back(glm::vec3(pose.getGlobalTransform(boneIndex)[3]));
    }

    return positions;
}

std::vector<float> getChainLengths(const std::vector<glm::vec3>& positions) {
    std::vector<float> lengths;
    lengths.reserve(positions.size() - 1);

    for (size_t i = 0; i < positions.size() - 1; ++i) {
        lengths.push_back(glm::length(positions[i + 1] - positions[i]));
    }

    return lengths;
}

void applyChainPositions(const Skeleton& skeleton, SkeletonPose& pose,
                         const std::vector<uint32_t>& chainBones,
                         const std::vector<glm::vec3>& positions) {
    if (chainBones.size() != positions.size()) {
        return;
    }

    // For each bone, calculate rotation needed to point at next position
    for (size_t i = 0; i < chainBones.size() - 1; ++i) {
        uint32_t boneIndex = chainBones[i];
        const Bone& bone = skeleton.getBone(boneIndex);

        // Current direction (from bind pose)
        glm::vec3 bindDir(0, 0, 1); // Assume bones point along Z
        if (i < chainBones.size() - 1) {
            glm::mat4 bindWorld = skeleton.calculateBoneWorldTransform(boneIndex);
            glm::mat4 childBindWorld = skeleton.calculateBoneWorldTransform(chainBones[i + 1]);
            bindDir = glm::normalize(glm::vec3(childBindWorld[3]) - glm::vec3(bindWorld[3]));
        }

        // Target direction
        glm::vec3 targetDir = glm::normalize(positions[i + 1] - positions[i]);

        // Calculate rotation
        glm::quat rot = rotationBetweenVectors(bindDir, targetDir);

        // Apply to local transform
        BoneTransform& transform = pose.getLocalTransform(boneIndex);
        transform.rotation = rot * bone.bindPose.rotation;
    }
}

float clampAngle(float angle, float min, float max) {
    return glm::clamp(angle, min, max);
}

} // namespace IKUtils

// ============================================================================
// Look-At IK Solver
// ============================================================================

void LookAtIK::initialize(const LookAtConfig& config) {
    m_config = config;
    m_currentDirection = glm::vec3(0, 0, 1);  // Forward
    m_targetDirection = m_currentDirection;
    m_bodyRotation = glm::quat(1, 0, 0, 0);
}

void LookAtIK::setTarget(const glm::vec3& worldTarget) {
    m_targetPosition = worldTarget;
    m_hasTarget = true;
}

void LookAtIK::clearTarget() {
    m_hasTarget = false;
}

void LookAtIK::setBodyTransform(const glm::vec3& position, const glm::quat& rotation) {
    m_bodyPosition = position;
    m_bodyRotation = rotation;
}

void LookAtIK::update(float deltaTime) {
    // Calculate target direction
    if (m_hasTarget) {
        glm::vec3 toTarget = m_targetPosition - m_bodyPosition;
        if (glm::length(toTarget) > 0.001f) {
            m_targetDirection = glm::normalize(toTarget);
        }
    } else {
        // Return to forward
        m_targetDirection = m_bodyRotation * glm::vec3(0, 0, 1);
    }

    // Convert to local space
    glm::vec3 localTarget = glm::inverse(m_bodyRotation) * m_targetDirection;

    // Calculate target yaw and pitch
    float targetYaw = std::atan2(localTarget.x, localTarget.z);
    float targetPitch = std::asin(glm::clamp(localTarget.y, -1.0f, 1.0f));

    // Distribute rotation among head, neck, and spine based on limits and weights
    float totalYaw = targetYaw;
    float totalPitch = targetPitch;

    // Target angles for each joint
    float targetHeadYaw = glm::clamp(totalYaw * m_config.headWeight, -m_config.maxHeadYaw, m_config.maxHeadYaw);
    float targetHeadPitch = glm::clamp(totalPitch * m_config.headWeight, -m_config.maxHeadPitch, m_config.maxHeadPitch);

    float remainingYaw = totalYaw - targetHeadYaw;
    float remainingPitch = totalPitch - targetHeadPitch;

    float targetNeckYaw = glm::clamp(remainingYaw * m_config.neckWeight / (1.0f - m_config.headWeight),
                                      -m_config.maxNeckYaw, m_config.maxNeckYaw);
    float targetNeckPitch = glm::clamp(remainingPitch * m_config.neckWeight / (1.0f - m_config.headWeight),
                                        -m_config.maxNeckPitch, m_config.maxNeckPitch);

    remainingYaw -= targetNeckYaw;
    remainingPitch -= targetNeckPitch;

    float targetSpineYaw = glm::clamp(remainingYaw, -m_config.maxSpineYaw, m_config.maxSpineYaw);
    float targetSpinePitch = glm::clamp(remainingPitch, -m_config.maxSpinePitch, m_config.maxSpinePitch);

    // Smooth interpolation
    float trackSpeed = m_hasTarget ? m_config.trackingSpeed : m_config.returnSpeed;

    m_currentHeadYaw = glm::mix(m_currentHeadYaw, targetHeadYaw, deltaTime * trackSpeed);
    m_currentHeadPitch = glm::mix(m_currentHeadPitch, targetHeadPitch, deltaTime * trackSpeed);
    m_currentNeckYaw = glm::mix(m_currentNeckYaw, targetNeckYaw, deltaTime * trackSpeed);
    m_currentNeckPitch = glm::mix(m_currentNeckPitch, targetNeckPitch, deltaTime * trackSpeed);
    m_currentSpineYaw = glm::mix(m_currentSpineYaw, targetSpineYaw, deltaTime * trackSpeed);
    m_currentSpinePitch = glm::mix(m_currentSpinePitch, targetSpinePitch, deltaTime * trackSpeed);

    // Update current look direction
    float totalCurrentYaw = m_currentHeadYaw + m_currentNeckYaw + m_currentSpineYaw;
    float totalCurrentPitch = m_currentHeadPitch + m_currentNeckPitch + m_currentSpinePitch;

    m_currentDirection = m_bodyRotation * glm::vec3(
        std::sin(totalCurrentYaw) * std::cos(totalCurrentPitch),
        std::sin(totalCurrentPitch),
        std::cos(totalCurrentYaw) * std::cos(totalCurrentPitch)
    );
}

void LookAtIK::applyToPose(const Skeleton& skeleton, SkeletonPose& pose) const {
    // Apply spine rotation
    if (m_config.spineBoneIndex < skeleton.getBoneCount()) {
        glm::quat spineYawRot = glm::angleAxis(m_currentSpineYaw, glm::vec3(0, 1, 0));
        glm::quat spinePitchRot = glm::angleAxis(m_currentSpinePitch, glm::vec3(1, 0, 0));

        BoneTransform& spineTransform = pose.getLocalTransform(m_config.spineBoneIndex);
        spineTransform.rotation = spineYawRot * spinePitchRot * spineTransform.rotation;
    }

    // Apply neck rotation
    if (m_config.neckBoneIndex < skeleton.getBoneCount()) {
        glm::quat neckYawRot = glm::angleAxis(m_currentNeckYaw, glm::vec3(0, 1, 0));
        glm::quat neckPitchRot = glm::angleAxis(m_currentNeckPitch, glm::vec3(1, 0, 0));

        BoneTransform& neckTransform = pose.getLocalTransform(m_config.neckBoneIndex);
        neckTransform.rotation = neckYawRot * neckPitchRot * neckTransform.rotation;
    }

    // Apply head rotation
    if (m_config.headBoneIndex < skeleton.getBoneCount()) {
        glm::quat headYawRot = glm::angleAxis(m_currentHeadYaw, glm::vec3(0, 1, 0));
        glm::quat headPitchRot = glm::angleAxis(m_currentHeadPitch, glm::vec3(1, 0, 0));

        BoneTransform& headTransform = pose.getLocalTransform(m_config.headBoneIndex);
        headTransform.rotation = headYawRot * headPitchRot * headTransform.rotation;
    }
}

// ============================================================================
// Terrain Foot Placement
// ============================================================================

void TerrainFootPlacement::initialize(const std::vector<FootPlacementConfig>& footConfigs) {
    m_footConfigs = footConfigs;
    m_footStates.resize(footConfigs.size());
    m_footRestPositions.resize(footConfigs.size(), glm::vec3(0));
    m_footSwinging.resize(footConfigs.size(), false);

    for (auto& state : m_footStates) {
        state.groundNormal = glm::vec3(0, 1, 0);
        state.footRotation = glm::quat(1, 0, 0, 0);
    }
}

void TerrainFootPlacement::setTerrainCallback(TerrainRaycastFn callback) {
    m_terrainCallback = std::move(callback);
}

void TerrainFootPlacement::setBodyTransform(const glm::vec3& position, const glm::quat& rotation) {
    m_bodyPosition = position;
    m_bodyRotation = rotation;
}

void TerrainFootPlacement::setFootRestPositions(const std::vector<glm::vec3>& restPositions) {
    for (size_t i = 0; i < restPositions.size() && i < m_footRestPositions.size(); ++i) {
        m_footRestPositions[i] = restPositions[i];
    }
}

void TerrainFootPlacement::setFootSwinging(size_t footIndex, bool swinging) {
    if (footIndex < m_footSwinging.size()) {
        m_footSwinging[footIndex] = swinging;
    }
}

void TerrainFootPlacement::update(float deltaTime, const Skeleton& skeleton, SkeletonPose& pose) {
    for (size_t i = 0; i < m_footConfigs.size(); ++i) {
        const FootPlacementConfig& config = m_footConfigs[i];
        FootPlacementState& state = m_footStates[i];

        // Calculate world-space rest position
        glm::vec3 restWorld = m_bodyPosition + m_bodyRotation * m_footRestPositions[i];

        // Sample terrain
        glm::vec3 hitPoint, hitNormal;
        bool hit = sampleTerrain(restWorld, config.raycastHeight, hitPoint, hitNormal);

        state.isValid = hit;

        if (hit) {
            state.groundPosition = hitPoint;
            state.groundNormal = hitNormal;
            state.groundDistance = hitPoint.y - restWorld.y;

            // Clamp step height
            if (state.groundDistance > config.maxStepUp) {
                state.groundDistance = config.maxStepUp;
            } else if (state.groundDistance < -config.maxStepDown) {
                state.groundDistance = -config.maxStepDown;
            }

            // Target position is on ground
            state.targetPosition = hitPoint + glm::vec3(0, config.ankleHeight, 0);

            // Calculate foot rotation to align with ground
            glm::vec3 forward = m_bodyRotation * glm::vec3(0, 0, 1);
            state.footRotation = calculateFootRotation(hitNormal, forward);
        } else {
            // No ground hit - use rest position
            state.targetPosition = restWorld;
            state.groundDistance = 0.0f;
            state.footRotation = glm::quat(1, 0, 0, 0);
        }

        // Update planted amount
        if (m_footSwinging[i]) {
            state.plantedAmount = glm::mix(state.plantedAmount, 0.0f, deltaTime * config.liftBlendSpeed);
        } else {
            state.plantedAmount = glm::mix(state.plantedAmount, 1.0f, deltaTime * config.plantBlendSpeed);
        }
    }
}

const FootPlacementState& TerrainFootPlacement::getFootState(size_t index) const {
    static FootPlacementState empty;
    if (index < m_footStates.size()) {
        return m_footStates[index];
    }
    return empty;
}

void TerrainFootPlacement::applyToPose(const Skeleton& skeleton, SkeletonPose& pose, IKSystem& ikSystem) {
    TwoBoneIK solver;

    for (size_t i = 0; i < m_footConfigs.size(); ++i) {
        const FootPlacementConfig& config = m_footConfigs[i];
        const FootPlacementState& state = m_footStates[i];

        if (!state.isValid || state.plantedAmount < 0.1f) {
            continue;
        }

        // Apply IK to reach target
        IKTarget target;
        target.position = state.targetPosition;
        target.weight = state.plantedAmount;

        solver.solve(skeleton, pose,
                     config.hipBoneIndex, config.kneeBoneIndex, config.ankleBoneIndex,
                     target);

        // Apply foot rotation
        if (config.footBoneIndex < skeleton.getBoneCount()) {
            BoneTransform& footTransform = pose.getLocalTransform(config.footBoneIndex);
            footTransform.rotation = glm::slerp(footTransform.rotation,
                                                 state.footRotation * footTransform.rotation,
                                                 state.plantedAmount);
        }
    }
}

float TerrainFootPlacement::getBodyHeightAdjustment() const {
    if (m_footStates.empty()) return 0.0f;

    // Find the average ground adjustment of planted feet
    float totalAdjust = 0.0f;
    float totalWeight = 0.0f;

    for (const auto& state : m_footStates) {
        if (state.isValid && state.plantedAmount > 0.5f) {
            totalAdjust += state.groundDistance * state.plantedAmount;
            totalWeight += state.plantedAmount;
        }
    }

    if (totalWeight > 0.0f) {
        return totalAdjust / totalWeight;
    }
    return 0.0f;
}

glm::quat TerrainFootPlacement::getBodyTiltAdjustment() const {
    if (m_footStates.size() < 2) return glm::quat(1, 0, 0, 0);

    // Calculate average ground normal
    glm::vec3 avgNormal(0, 0, 0);
    float totalWeight = 0.0f;

    for (const auto& state : m_footStates) {
        if (state.isValid && state.plantedAmount > 0.5f) {
            avgNormal += state.groundNormal * state.plantedAmount;
            totalWeight += state.plantedAmount;
        }
    }

    if (totalWeight > 0.0f) {
        avgNormal = glm::normalize(avgNormal / totalWeight);

        // Calculate rotation from up to average normal
        glm::vec3 up(0, 1, 0);
        return IKUtils::rotationBetweenVectors(up, avgNormal);
    }

    return glm::quat(1, 0, 0, 0);
}

bool TerrainFootPlacement::sampleTerrain(const glm::vec3& position, float rayHeight,
                                          glm::vec3& hitPoint, glm::vec3& hitNormal) {
    if (m_terrainCallback) {
        glm::vec3 origin = position + glm::vec3(0, rayHeight, 0);
        return m_terrainCallback(origin, glm::vec3(0, -1, 0), rayHeight * 2.0f, hitPoint, hitNormal);
    }

    // Default: flat ground
    hitPoint = glm::vec3(position.x, 0.0f, position.z);
    hitNormal = glm::vec3(0, 1, 0);
    return true;
}

glm::quat TerrainFootPlacement::calculateFootRotation(const glm::vec3& groundNormal,
                                                       const glm::vec3& movementDir) const {
    // Rotate foot to align with ground while keeping forward direction
    glm::vec3 up(0, 1, 0);
    glm::quat normalAlign = IKUtils::rotationBetweenVectors(up, groundNormal);

    // Limit the rotation to prevent extreme angles
    glm::vec3 axis;
    float angle;
    if (glm::length(glm::vec3(normalAlign.x, normalAlign.y, normalAlign.z)) > 0.001f) {
        axis = glm::normalize(glm::vec3(normalAlign.x, normalAlign.y, normalAlign.z));
        angle = 2.0f * std::acos(glm::clamp(normalAlign.w, -1.0f, 1.0f));
        angle = glm::clamp(angle, -0.5f, 0.5f);  // Max 30 degrees
        return glm::angleAxis(angle, axis);
    }

    return glm::quat(1, 0, 0, 0);
}

// ============================================================================
// Reach IK
// ============================================================================

void ReachIK::initialize(const ReachConfig& leftArm, const ReachConfig& rightArm) {
    m_leftConfig = leftArm;
    m_rightConfig = rightArm;
}

void ReachIK::reachToward(bool isLeftHand, const glm::vec3& worldTarget, float urgency) {
    if (isLeftHand) {
        m_leftTarget = worldTarget;
        m_leftReaching = true;
        m_leftUrgency = urgency;
    } else {
        m_rightTarget = worldTarget;
        m_rightReaching = true;
        m_rightUrgency = urgency;
    }
}

void ReachIK::release(bool isLeftHand) {
    if (isLeftHand) {
        m_leftReaching = false;
    } else {
        m_rightReaching = false;
    }
}

void ReachIK::update(float deltaTime) {
    // Left hand
    if (m_leftReaching) {
        float speed = m_leftConfig.reachSpeed * m_leftUrgency;
        m_leftCurrentTarget = glm::mix(m_leftCurrentTarget, m_leftTarget, deltaTime * speed);
        m_leftReachAmount = glm::min(1.0f, m_leftReachAmount + deltaTime * speed);
    } else {
        m_leftReachAmount = glm::max(0.0f, m_leftReachAmount - deltaTime * m_leftConfig.returnSpeed);
    }

    // Right hand
    if (m_rightReaching) {
        float speed = m_rightConfig.reachSpeed * m_rightUrgency;
        m_rightCurrentTarget = glm::mix(m_rightCurrentTarget, m_rightTarget, deltaTime * speed);
        m_rightReachAmount = glm::min(1.0f, m_rightReachAmount + deltaTime * speed);
    } else {
        m_rightReachAmount = glm::max(0.0f, m_rightReachAmount - deltaTime * m_rightConfig.returnSpeed);
    }
}

void ReachIK::applyToPose(const Skeleton& skeleton, SkeletonPose& pose, IKSystem& ikSystem) {
    TwoBoneIK solver;

    // Left arm
    if (m_leftReachAmount > 0.01f) {
        IKTarget target;
        target.position = m_leftCurrentTarget;
        target.weight = m_leftReachAmount;

        solver.solve(skeleton, pose,
                     m_leftConfig.shoulderBoneIndex,
                     m_leftConfig.elbowBoneIndex,
                     m_leftConfig.wristBoneIndex,
                     target);
    }

    // Right arm
    if (m_rightReachAmount > 0.01f) {
        IKTarget target;
        target.position = m_rightCurrentTarget;
        target.weight = m_rightReachAmount;

        solver.solve(skeleton, pose,
                     m_rightConfig.shoulderBoneIndex,
                     m_rightConfig.elbowBoneIndex,
                     m_rightConfig.wristBoneIndex,
                     target);
    }
}

bool ReachIK::isReaching(bool isLeftHand) const {
    return isLeftHand ? m_leftReaching : m_rightReaching;
}

float ReachIK::getReachProgress(bool isLeftHand) const {
    return isLeftHand ? m_leftReachAmount : m_rightReachAmount;
}

glm::vec3 ReachIK::getCurrentHandPosition(bool isLeftHand) const {
    return isLeftHand ? m_leftCurrentTarget : m_rightCurrentTarget;
}

// ============================================================================
// Full Body IK System
// ============================================================================

void FullBodyIK::initializeLookAt(const LookAtConfig& config) {
    m_lookAt.initialize(config);
    m_hasLookAt = true;
}

void FullBodyIK::initializeFootPlacement(const std::vector<FootPlacementConfig>& footConfigs) {
    m_footPlacement.initialize(footConfigs);
    m_hasFootPlacement = true;
}

void FullBodyIK::initializeReach(const ReachConfig& leftArm, const ReachConfig& rightArm) {
    m_reach.initialize(leftArm, rightArm);
    m_hasReach = true;
}

void FullBodyIK::setTerrainCallback(TerrainRaycastFn callback) {
    m_footPlacement.setTerrainCallback(callback);
}

void FullBodyIK::setBodyTransform(const glm::vec3& position, const glm::quat& rotation) {
    if (m_hasLookAt) {
        m_lookAt.setBodyTransform(position, rotation);
    }
    if (m_hasFootPlacement) {
        m_footPlacement.setBodyTransform(position, rotation);
    }
}

void FullBodyIK::lookAt(const glm::vec3& target) {
    if (m_hasLookAt) {
        m_lookAt.setTarget(target);
    }
}

void FullBodyIK::clearLookAt() {
    if (m_hasLookAt) {
        m_lookAt.clearTarget();
    }
}

void FullBodyIK::reachWith(bool isLeftHand, const glm::vec3& target) {
    if (m_hasReach) {
        m_reach.reachToward(isLeftHand, target);
    }
}

void FullBodyIK::releaseHand(bool isLeftHand) {
    if (m_hasReach) {
        m_reach.release(isLeftHand);
    }
}

void FullBodyIK::setFootSwinging(size_t index, bool swinging) {
    if (m_hasFootPlacement) {
        m_footPlacement.setFootSwinging(index, swinging);
    }
}

void FullBodyIK::update(float deltaTime, const Skeleton& skeleton, SkeletonPose& pose) {
    if (m_hasLookAt) {
        m_lookAt.update(deltaTime);
    }
    if (m_hasFootPlacement) {
        m_footPlacement.update(deltaTime, skeleton, pose);
    }
    if (m_hasReach) {
        m_reach.update(deltaTime);
    }
}

void FullBodyIK::applyToPose(const Skeleton& skeleton, SkeletonPose& pose, IKSystem& ikSystem) {
    // Apply in order: feet first (affects body), then reach, then look-at
    if (m_hasFootPlacement) {
        m_footPlacement.applyToPose(skeleton, pose, ikSystem);
    }
    if (m_hasReach) {
        m_reach.applyToPose(skeleton, pose, ikSystem);
    }
    if (m_hasLookAt) {
        m_lookAt.applyToPose(skeleton, pose);
    }
}

float FullBodyIK::getBodyHeightAdjustment() const {
    if (m_hasFootPlacement) {
        return m_footPlacement.getBodyHeightAdjustment();
    }
    return 0.0f;
}

glm::quat FullBodyIK::getBodyTiltAdjustment() const {
    if (m_hasFootPlacement) {
        return m_footPlacement.getBodyTiltAdjustment();
    }
    return glm::quat(1, 0, 0, 0);
}

} // namespace animation
