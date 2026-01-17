#pragma once

#include "Skeleton.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <array>

namespace animation {

// Per-vertex skinning weight data
struct SkinWeight {
    std::array<uint32_t, MAX_BONES_PER_VERTEX> boneIndices = {0, 0, 0, 0};
    std::array<float, MAX_BONES_PER_VERTEX> weights = {0.0f, 0.0f, 0.0f, 0.0f};

    // Normalize weights to sum to 1.0
    void normalize();

    // Get number of non-zero influences
    uint32_t getInfluenceCount() const;

    // Add a bone influence (maintains sorted by weight)
    void addInfluence(uint32_t boneIndex, float weight);
};

// Runtime skeleton pose - stores current bone transforms
class SkeletonPose {
public:
    SkeletonPose() = default;
    explicit SkeletonPose(const Skeleton& skeleton);

    // Resize to match skeleton
    void resize(uint32_t boneCount);

    // Get/set local transforms
    BoneTransform& getLocalTransform(uint32_t boneIndex) { return m_localTransforms[boneIndex]; }
    const BoneTransform& getLocalTransform(uint32_t boneIndex) const { return m_localTransforms[boneIndex]; }

    // Get global transforms (after calculation)
    const glm::mat4& getGlobalTransform(uint32_t boneIndex) const { return m_globalTransforms[boneIndex]; }

    // Calculate global transforms from local transforms
    void calculateGlobalTransforms(const Skeleton& skeleton);

    // Calculate skinning matrices (global * inverse bind)
    void calculateSkinningMatrices(const Skeleton& skeleton, std::vector<glm::mat4>& outMatrices) const;

    // Get skinning matrices for GPU upload
    const std::vector<glm::mat4>& getSkinningMatrices() const { return m_skinningMatrices; }

    // Convenience: calculate both global and skinning matrices
    void updateMatrices(const Skeleton& skeleton);

    // Reset to bind pose
    void setToBindPose(const Skeleton& skeleton);

    // Blend with another pose
    static SkeletonPose lerp(const SkeletonPose& a, const SkeletonPose& b, float t);

    // Additive blend (add b's delta from bind pose to a)
    static SkeletonPose additive(const SkeletonPose& base, const SkeletonPose& additive,
                                  const SkeletonPose& additiveBindPose, float weight);

    // Bone masking - blend only specified bones
    void blendMasked(const SkeletonPose& other, float weight,
                     const std::vector<bool>& boneMask);

    // Get bone count
    uint32_t getBoneCount() const { return static_cast<uint32_t>(m_localTransforms.size()); }

private:
    std::vector<BoneTransform> m_localTransforms;
    std::vector<glm::mat4> m_globalTransforms;
    std::vector<glm::mat4> m_skinningMatrices;
};

// Skinned mesh vertex data (for GPU upload)
struct SkinnedVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::uvec4 boneIndices;  // Up to 4 bone influences
    glm::vec4 boneWeights;   // Corresponding weights

    SkinnedVertex() : position(0.0f), normal(0.0f, 1.0f, 0.0f), texCoord(0.0f),
                      boneIndices(0), boneWeights(0.0f) {}
};

// Skinned mesh data container
struct SkinnedMeshData {
    std::vector<SkinnedVertex> vertices;
    std::vector<uint32_t> indices;
    const Skeleton* skeleton = nullptr;

    // Validate that all bone indices are within skeleton bounds
    bool validate() const;

    // Calculate skinned positions for CPU skinning fallback
    void applyCPUSkinning(const SkeletonPose& pose,
                          std::vector<glm::vec3>& outPositions,
                          std::vector<glm::vec3>& outNormals) const;
};

// Helper functions for skinning calculations
namespace SkinningUtils {
    // Calculate skinned position for a single vertex
    glm::vec3 calculateSkinnedPosition(
        const glm::vec3& position,
        const glm::uvec4& boneIndices,
        const glm::vec4& weights,
        const std::vector<glm::mat4>& skinMatrices);

    // Calculate skinned normal for a single vertex
    glm::vec3 calculateSkinnedNormal(
        const glm::vec3& normal,
        const glm::uvec4& boneIndices,
        const glm::vec4& weights,
        const std::vector<glm::mat4>& skinMatrices);

    // Auto-assign skin weights based on bone proximity
    void autoSkinWeights(
        const std::vector<glm::vec3>& vertices,
        const Skeleton& skeleton,
        std::vector<SkinWeight>& outWeights,
        float falloffRadius = 0.5f);

    // Normalize all weights in a mesh
    void normalizeWeights(std::vector<SkinWeight>& weights);

    // Remove weights below threshold and redistribute
    void pruneWeights(std::vector<SkinWeight>& weights, float threshold = 0.01f);
}

} // namespace animation
