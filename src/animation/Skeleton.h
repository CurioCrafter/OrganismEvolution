#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace animation {

// Maximum bones per skeleton (GPU uniform limit)
constexpr uint32_t MAX_BONES = 64;

// Maximum bone influences per vertex
constexpr uint32_t MAX_BONES_PER_VERTEX = 4;

// Bone transform in local space (translation, rotation, scale)
struct BoneTransform {
    glm::vec3 translation = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    // Convert to 4x4 matrix (T * R * S order)
    glm::mat4 toMatrix() const;

    // Interpolate between two transforms
    static BoneTransform lerp(const BoneTransform& a, const BoneTransform& b, float t);

    // Identity transform
    static BoneTransform identity();
};

// A single bone in the skeleton hierarchy
struct Bone {
    std::string name;
    int32_t parentIndex = -1;           // -1 for root bones
    BoneTransform bindPose;              // Local bind pose transform
    glm::mat4 inverseBindMatrix;         // World-space inverse bind pose

    // Bone length (distance to child or estimated)
    float length = 0.0f;

    // Joint constraints (for IK)
    glm::vec3 minAngles = glm::vec3(-3.14159f);
    glm::vec3 maxAngles = glm::vec3(3.14159f);
};

// Skeleton definition - the bone hierarchy
class Skeleton {
public:
    Skeleton() = default;

    // Add a bone to the skeleton (parent must already exist or be -1)
    int32_t addBone(const std::string& name, int32_t parentIndex, const BoneTransform& bindPose);

    // Find bone index by name (-1 if not found)
    int32_t findBoneIndex(const std::string& name) const;

    // Get bone by index
    const Bone& getBone(uint32_t index) const { return m_bones[index]; }
    Bone& getBone(uint32_t index) { return m_bones[index]; }

    // Get all bones
    const std::vector<Bone>& getBones() const { return m_bones; }
    uint32_t getBoneCount() const { return static_cast<uint32_t>(m_bones.size()); }

    // Get root bone indices
    std::vector<int32_t> getRootBones() const;

    // Get child bone indices
    std::vector<int32_t> getChildBones(int32_t parentIndex) const;

    // Check if a bone is descendant of another
    bool isDescendant(int32_t boneIndex, int32_t ancestorIndex) const;

    // Calculate world transform for a bone from bind pose
    glm::mat4 calculateBoneWorldTransform(uint32_t boneIndex) const;

    // Calculate bone lengths from hierarchy
    void calculateBoneLengths();

    // Validate skeleton integrity
    bool isValid() const;

private:
    std::vector<Bone> m_bones;
    std::unordered_map<std::string, int32_t> m_boneNameToIndex;

    // Calculate inverse bind matrix for a bone
    void calculateInverseBindMatrix(uint32_t boneIndex);
};

// Factory functions for creating common skeleton types
namespace SkeletonFactory {
    // Create a bipedal skeleton (humanoid-like creature)
    Skeleton createBiped(float height = 1.0f);

    // Create a quadruped skeleton (four-legged creature)
    Skeleton createQuadruped(float length = 1.0f, float height = 0.5f);

    // Create a serpentine skeleton (snake/worm)
    Skeleton createSerpentine(float length = 2.0f, int segments = 8);

    // Create a flying skeleton (bird/bat-like)
    Skeleton createFlying(float wingspan = 1.5f);

    // Create an aquatic skeleton (fish-like)
    Skeleton createAquatic(float length = 1.0f, int segments = 5);
}

} // namespace animation
