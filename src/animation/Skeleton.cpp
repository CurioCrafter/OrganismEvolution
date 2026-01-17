#include "Skeleton.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <algorithm>

namespace animation {

glm::mat4 BoneTransform::toMatrix() const {
    glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
    glm::mat4 R = glm::mat4_cast(rotation);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
    return T * R * S;
}

BoneTransform BoneTransform::lerp(const BoneTransform& a, const BoneTransform& b, float t) {
    BoneTransform result;
    result.translation = glm::mix(a.translation, b.translation, t);
    result.rotation = glm::slerp(a.rotation, b.rotation, t);
    result.scale = glm::mix(a.scale, b.scale, t);
    return result;
}

BoneTransform BoneTransform::identity() {
    return BoneTransform{};
}

int32_t Skeleton::addBone(const std::string& name, int32_t parentIndex, const BoneTransform& bindPose) {
    if (m_bones.size() >= MAX_BONES) {
        return -1; // Maximum bones reached
    }

    if (parentIndex >= static_cast<int32_t>(m_bones.size())) {
        return -1; // Invalid parent
    }

    int32_t newIndex = static_cast<int32_t>(m_bones.size());

    Bone bone;
    bone.name = name;
    bone.parentIndex = parentIndex;
    bone.bindPose = bindPose;

    m_bones.push_back(bone);
    m_boneNameToIndex[name] = newIndex;

    // Calculate inverse bind matrix
    calculateInverseBindMatrix(newIndex);

    return newIndex;
}

int32_t Skeleton::findBoneIndex(const std::string& name) const {
    auto it = m_boneNameToIndex.find(name);
    if (it != m_boneNameToIndex.end()) {
        return it->second;
    }
    return -1;
}

std::vector<int32_t> Skeleton::getRootBones() const {
    std::vector<int32_t> roots;
    for (size_t i = 0; i < m_bones.size(); ++i) {
        if (m_bones[i].parentIndex < 0) {
            roots.push_back(static_cast<int32_t>(i));
        }
    }
    return roots;
}

std::vector<int32_t> Skeleton::getChildBones(int32_t parentIndex) const {
    std::vector<int32_t> children;
    for (size_t i = 0; i < m_bones.size(); ++i) {
        if (m_bones[i].parentIndex == parentIndex) {
            children.push_back(static_cast<int32_t>(i));
        }
    }
    return children;
}

bool Skeleton::isDescendant(int32_t boneIndex, int32_t ancestorIndex) const {
    if (boneIndex < 0 || boneIndex >= static_cast<int32_t>(m_bones.size())) {
        return false;
    }

    int32_t current = boneIndex;
    while (current >= 0) {
        if (current == ancestorIndex) {
            return true;
        }
        current = m_bones[current].parentIndex;
    }
    return false;
}

glm::mat4 Skeleton::calculateBoneWorldTransform(uint32_t boneIndex) const {
    if (boneIndex >= m_bones.size()) {
        return glm::mat4(1.0f);
    }

    glm::mat4 worldTransform = m_bones[boneIndex].bindPose.toMatrix();
    int32_t parent = m_bones[boneIndex].parentIndex;

    while (parent >= 0) {
        worldTransform = m_bones[parent].bindPose.toMatrix() * worldTransform;
        parent = m_bones[parent].parentIndex;
    }

    return worldTransform;
}

void Skeleton::calculateInverseBindMatrix(uint32_t boneIndex) {
    glm::mat4 worldTransform = calculateBoneWorldTransform(boneIndex);
    m_bones[boneIndex].inverseBindMatrix = glm::inverse(worldTransform);
}

void Skeleton::calculateBoneLengths() {
    for (size_t i = 0; i < m_bones.size(); ++i) {
        auto children = getChildBones(static_cast<int32_t>(i));
        if (!children.empty()) {
            // Use distance to first child
            glm::vec3 parentPos = glm::vec3(calculateBoneWorldTransform(static_cast<uint32_t>(i))[3]);
            glm::vec3 childPos = glm::vec3(calculateBoneWorldTransform(children[0])[3]);
            m_bones[i].length = glm::length(childPos - parentPos);
        } else {
            // Leaf bone - estimate from parent
            if (m_bones[i].parentIndex >= 0) {
                m_bones[i].length = m_bones[m_bones[i].parentIndex].length * 0.5f;
            } else {
                m_bones[i].length = 0.1f; // Default
            }
        }
    }
}

bool Skeleton::isValid() const {
    if (m_bones.empty()) {
        return false;
    }

    // Check all parent indices are valid
    for (size_t i = 0; i < m_bones.size(); ++i) {
        int32_t parent = m_bones[i].parentIndex;
        if (parent >= static_cast<int32_t>(i)) {
            return false; // Parent must come before child
        }
    }

    // Check at least one root bone exists
    return !getRootBones().empty();
}

// Skeleton factory implementations
namespace SkeletonFactory {

Skeleton createBiped(float height) {
    Skeleton skeleton;
    float scale = height / 1.8f; // Normalize to ~1.8m humanoid

    // Spine (root is pelvis)
    int32_t pelvis = skeleton.addBone("pelvis", -1,
        BoneTransform{glm::vec3(0.0f, 0.9f * scale, 0.0f)});
    int32_t spine1 = skeleton.addBone("spine_lower", pelvis,
        BoneTransform{glm::vec3(0.0f, 0.15f * scale, 0.0f)});
    int32_t spine2 = skeleton.addBone("spine_middle", spine1,
        BoneTransform{glm::vec3(0.0f, 0.15f * scale, 0.0f)});
    int32_t spine3 = skeleton.addBone("spine_upper", spine2,
        BoneTransform{glm::vec3(0.0f, 0.15f * scale, 0.0f)});
    int32_t neck = skeleton.addBone("neck", spine3,
        BoneTransform{glm::vec3(0.0f, 0.1f * scale, 0.0f)});
    skeleton.addBone("head", neck,
        BoneTransform{glm::vec3(0.0f, 0.15f * scale, 0.0f)});

    // Left arm
    int32_t lClavicle = skeleton.addBone("clavicle_l", spine3,
        BoneTransform{glm::vec3(-0.1f * scale, 0.05f * scale, 0.0f)});
    int32_t lShoulder = skeleton.addBone("shoulder_l", lClavicle,
        BoneTransform{glm::vec3(-0.1f * scale, 0.0f, 0.0f)});
    int32_t lElbow = skeleton.addBone("elbow_l", lShoulder,
        BoneTransform{glm::vec3(-0.25f * scale, 0.0f, 0.0f)});
    skeleton.addBone("wrist_l", lElbow,
        BoneTransform{glm::vec3(-0.25f * scale, 0.0f, 0.0f)});

    // Right arm
    int32_t rClavicle = skeleton.addBone("clavicle_r", spine3,
        BoneTransform{glm::vec3(0.1f * scale, 0.05f * scale, 0.0f)});
    int32_t rShoulder = skeleton.addBone("shoulder_r", rClavicle,
        BoneTransform{glm::vec3(0.1f * scale, 0.0f, 0.0f)});
    int32_t rElbow = skeleton.addBone("elbow_r", rShoulder,
        BoneTransform{glm::vec3(0.25f * scale, 0.0f, 0.0f)});
    skeleton.addBone("wrist_r", rElbow,
        BoneTransform{glm::vec3(0.25f * scale, 0.0f, 0.0f)});

    // Left leg
    int32_t lHip = skeleton.addBone("hip_l", pelvis,
        BoneTransform{glm::vec3(-0.1f * scale, -0.05f * scale, 0.0f)});
    int32_t lKnee = skeleton.addBone("knee_l", lHip,
        BoneTransform{glm::vec3(0.0f, -0.4f * scale, 0.0f)});
    int32_t lAnkle = skeleton.addBone("ankle_l", lKnee,
        BoneTransform{glm::vec3(0.0f, -0.4f * scale, 0.0f)});
    skeleton.addBone("foot_l", lAnkle,
        BoneTransform{glm::vec3(0.0f, 0.0f, 0.1f * scale)});

    // Right leg
    int32_t rHip = skeleton.addBone("hip_r", pelvis,
        BoneTransform{glm::vec3(0.1f * scale, -0.05f * scale, 0.0f)});
    int32_t rKnee = skeleton.addBone("knee_r", rHip,
        BoneTransform{glm::vec3(0.0f, -0.4f * scale, 0.0f)});
    int32_t rAnkle = skeleton.addBone("ankle_r", rKnee,
        BoneTransform{glm::vec3(0.0f, -0.4f * scale, 0.0f)});
    skeleton.addBone("foot_r", rAnkle,
        BoneTransform{glm::vec3(0.0f, 0.0f, 0.1f * scale)});

    skeleton.calculateBoneLengths();
    return skeleton;
}

Skeleton createQuadruped(float length, float height) {
    Skeleton skeleton;

    // Spine
    int32_t pelvis = skeleton.addBone("pelvis", -1,
        BoneTransform{glm::vec3(0.0f, height, -length * 0.3f)});
    int32_t spine1 = skeleton.addBone("spine_lower", pelvis,
        BoneTransform{glm::vec3(0.0f, 0.0f, length * 0.2f)});
    int32_t spine2 = skeleton.addBone("spine_middle", spine1,
        BoneTransform{glm::vec3(0.0f, 0.0f, length * 0.2f)});
    int32_t spine3 = skeleton.addBone("spine_upper", spine2,
        BoneTransform{glm::vec3(0.0f, 0.0f, length * 0.2f)});
    int32_t neck = skeleton.addBone("neck", spine3,
        BoneTransform{glm::vec3(0.0f, height * 0.2f, length * 0.1f)});
    skeleton.addBone("head", neck,
        BoneTransform{glm::vec3(0.0f, height * 0.1f, length * 0.15f)});

    // Tail
    int32_t tail1 = skeleton.addBone("tail_1", pelvis,
        BoneTransform{glm::vec3(0.0f, 0.0f, -length * 0.15f)});
    int32_t tail2 = skeleton.addBone("tail_2", tail1,
        BoneTransform{glm::vec3(0.0f, -0.02f, -length * 0.1f)});
    skeleton.addBone("tail_3", tail2,
        BoneTransform{glm::vec3(0.0f, -0.02f, -length * 0.1f)});

    // Front left leg
    int32_t flShoulder = skeleton.addBone("shoulder_fl", spine3,
        BoneTransform{glm::vec3(-length * 0.15f, -height * 0.1f, 0.0f)});
    int32_t flElbow = skeleton.addBone("elbow_fl", flShoulder,
        BoneTransform{glm::vec3(0.0f, -height * 0.4f, 0.0f)});
    int32_t flWrist = skeleton.addBone("wrist_fl", flElbow,
        BoneTransform{glm::vec3(0.0f, -height * 0.35f, 0.0f)});
    skeleton.addBone("foot_fl", flWrist,
        BoneTransform{glm::vec3(0.0f, -height * 0.1f, length * 0.05f)});

    // Front right leg
    int32_t frShoulder = skeleton.addBone("shoulder_fr", spine3,
        BoneTransform{glm::vec3(length * 0.15f, -height * 0.1f, 0.0f)});
    int32_t frElbow = skeleton.addBone("elbow_fr", frShoulder,
        BoneTransform{glm::vec3(0.0f, -height * 0.4f, 0.0f)});
    int32_t frWrist = skeleton.addBone("wrist_fr", frElbow,
        BoneTransform{glm::vec3(0.0f, -height * 0.35f, 0.0f)});
    skeleton.addBone("foot_fr", frWrist,
        BoneTransform{glm::vec3(0.0f, -height * 0.1f, length * 0.05f)});

    // Back left leg
    int32_t blHip = skeleton.addBone("hip_bl", pelvis,
        BoneTransform{glm::vec3(-length * 0.15f, -height * 0.1f, 0.0f)});
    int32_t blKnee = skeleton.addBone("knee_bl", blHip,
        BoneTransform{glm::vec3(0.0f, -height * 0.4f, 0.0f)});
    int32_t blAnkle = skeleton.addBone("ankle_bl", blKnee,
        BoneTransform{glm::vec3(0.0f, -height * 0.35f, 0.0f)});
    skeleton.addBone("foot_bl", blAnkle,
        BoneTransform{glm::vec3(0.0f, -height * 0.1f, -length * 0.03f)});

    // Back right leg
    int32_t brHip = skeleton.addBone("hip_br", pelvis,
        BoneTransform{glm::vec3(length * 0.15f, -height * 0.1f, 0.0f)});
    int32_t brKnee = skeleton.addBone("knee_br", brHip,
        BoneTransform{glm::vec3(0.0f, -height * 0.4f, 0.0f)});
    int32_t brAnkle = skeleton.addBone("ankle_br", brKnee,
        BoneTransform{glm::vec3(0.0f, -height * 0.35f, 0.0f)});
    skeleton.addBone("foot_br", brAnkle,
        BoneTransform{glm::vec3(0.0f, -height * 0.1f, -length * 0.03f)});

    skeleton.calculateBoneLengths();
    return skeleton;
}

Skeleton createSerpentine(float length, int segments) {
    Skeleton skeleton;
    float segmentLength = length / segments;

    int32_t parent = -1;
    for (int i = 0; i < segments; ++i) {
        std::string name = "segment_" + std::to_string(i);
        BoneTransform transform;
        transform.translation = glm::vec3(0.0f, 0.0f, segmentLength);

        parent = skeleton.addBone(name, parent, transform);
    }

    // Head at the front
    skeleton.addBone("head", parent,
        BoneTransform{glm::vec3(0.0f, 0.02f, segmentLength * 0.5f)});

    skeleton.calculateBoneLengths();
    return skeleton;
}

Skeleton createFlying(float wingspan) {
    Skeleton skeleton;
    float halfWing = wingspan * 0.5f;

    // Body
    int32_t body = skeleton.addBone("body", -1,
        BoneTransform{glm::vec3(0.0f)});
    int32_t chest = skeleton.addBone("chest", body,
        BoneTransform{glm::vec3(0.0f, 0.0f, 0.15f)});
    int32_t neck = skeleton.addBone("neck", chest,
        BoneTransform{glm::vec3(0.0f, 0.05f, 0.1f)});
    skeleton.addBone("head", neck,
        BoneTransform{glm::vec3(0.0f, 0.03f, 0.08f)});

    // Tail
    int32_t tail1 = skeleton.addBone("tail_1", body,
        BoneTransform{glm::vec3(0.0f, 0.0f, -0.15f)});
    int32_t tail2 = skeleton.addBone("tail_2", tail1,
        BoneTransform{glm::vec3(0.0f, 0.0f, -0.1f)});
    skeleton.addBone("tail_3", tail2,
        BoneTransform{glm::vec3(0.0f, 0.0f, -0.08f)});

    // Left wing
    int32_t lWing1 = skeleton.addBone("wing_l_1", chest,
        BoneTransform{glm::vec3(-0.05f, 0.0f, 0.0f)});
    int32_t lWing2 = skeleton.addBone("wing_l_2", lWing1,
        BoneTransform{glm::vec3(-halfWing * 0.4f, 0.0f, 0.0f)});
    int32_t lWing3 = skeleton.addBone("wing_l_3", lWing2,
        BoneTransform{glm::vec3(-halfWing * 0.35f, 0.0f, 0.0f)});
    skeleton.addBone("wing_l_tip", lWing3,
        BoneTransform{glm::vec3(-halfWing * 0.25f, 0.0f, 0.0f)});

    // Right wing
    int32_t rWing1 = skeleton.addBone("wing_r_1", chest,
        BoneTransform{glm::vec3(0.05f, 0.0f, 0.0f)});
    int32_t rWing2 = skeleton.addBone("wing_r_2", rWing1,
        BoneTransform{glm::vec3(halfWing * 0.4f, 0.0f, 0.0f)});
    int32_t rWing3 = skeleton.addBone("wing_r_3", rWing2,
        BoneTransform{glm::vec3(halfWing * 0.35f, 0.0f, 0.0f)});
    skeleton.addBone("wing_r_tip", rWing3,
        BoneTransform{glm::vec3(halfWing * 0.25f, 0.0f, 0.0f)});

    // Legs (small for flying creature)
    int32_t lLeg = skeleton.addBone("leg_l", body,
        BoneTransform{glm::vec3(-0.03f, -0.05f, 0.0f)});
    skeleton.addBone("foot_l", lLeg,
        BoneTransform{glm::vec3(0.0f, -0.08f, 0.0f)});

    int32_t rLeg = skeleton.addBone("leg_r", body,
        BoneTransform{glm::vec3(0.03f, -0.05f, 0.0f)});
    skeleton.addBone("foot_r", rLeg,
        BoneTransform{glm::vec3(0.0f, -0.08f, 0.0f)});

    skeleton.calculateBoneLengths();
    return skeleton;
}

Skeleton createAquatic(float length, int segments) {
    Skeleton skeleton;
    float bodyLength = length * 0.7f;
    float segmentLength = bodyLength / segments;

    // Main body segments
    int32_t parent = -1;
    for (int i = 0; i < segments; ++i) {
        std::string name = "body_" + std::to_string(i);
        BoneTransform transform;
        transform.translation = glm::vec3(0.0f, 0.0f, segmentLength);

        parent = skeleton.addBone(name, parent, transform);
    }

    // Head
    skeleton.addBone("head", parent,
        BoneTransform{glm::vec3(0.0f, 0.0f, length * 0.15f)});

    // Tail fin
    int32_t body0 = skeleton.findBoneIndex("body_0");
    int32_t tailBase = skeleton.addBone("tail_base", body0,
        BoneTransform{glm::vec3(0.0f, 0.0f, -length * 0.1f)});
    skeleton.addBone("tail_fin", tailBase,
        BoneTransform{glm::vec3(0.0f, 0.0f, -length * 0.15f)});

    // Pectoral fins (left and right)
    int32_t midBody = skeleton.findBoneIndex("body_" + std::to_string(segments / 2));
    skeleton.addBone("fin_l", midBody,
        BoneTransform{glm::vec3(-length * 0.15f, -0.02f, 0.0f)});
    skeleton.addBone("fin_r", midBody,
        BoneTransform{glm::vec3(length * 0.15f, -0.02f, 0.0f)});

    // Dorsal fin
    skeleton.addBone("dorsal", midBody,
        BoneTransform{glm::vec3(0.0f, length * 0.1f, 0.0f)});

    skeleton.calculateBoneLengths();
    return skeleton;
}

} // namespace SkeletonFactory

} // namespace animation
