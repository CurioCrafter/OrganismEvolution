#include "Pose.h"
#include <algorithm>
#include <numeric>

namespace animation {

void SkinWeight::normalize() {
    float sum = 0.0f;
    for (float w : weights) {
        sum += w;
    }
    if (sum > 0.0001f) {
        for (float& w : weights) {
            w /= sum;
        }
    }
}

uint32_t SkinWeight::getInfluenceCount() const {
    uint32_t count = 0;
    for (float w : weights) {
        if (w > 0.0001f) ++count;
    }
    return count;
}

void SkinWeight::addInfluence(uint32_t boneIndex, float weight) {
    if (weight < 0.0001f) return;

    // Find position to insert (sorted by weight descending)
    int insertPos = -1;
    for (uint32_t i = 0; i < MAX_BONES_PER_VERTEX; ++i) {
        if (weight > weights[i]) {
            insertPos = static_cast<int>(i);
            break;
        }
    }

    if (insertPos < 0) return; // Weight too small

    // Shift lower weights down
    for (int i = MAX_BONES_PER_VERTEX - 1; i > insertPos; --i) {
        boneIndices[i] = boneIndices[i - 1];
        weights[i] = weights[i - 1];
    }

    // Insert new weight
    boneIndices[insertPos] = boneIndex;
    weights[insertPos] = weight;
}

SkeletonPose::SkeletonPose(const Skeleton& skeleton) {
    resize(skeleton.getBoneCount());
    setToBindPose(skeleton);
}

void SkeletonPose::resize(uint32_t boneCount) {
    m_localTransforms.resize(boneCount);
    m_globalTransforms.resize(boneCount, glm::mat4(1.0f));
    m_skinningMatrices.resize(boneCount, glm::mat4(1.0f));
}

void SkeletonPose::calculateGlobalTransforms(const Skeleton& skeleton) {
    for (uint32_t i = 0; i < m_localTransforms.size(); ++i) {
        const Bone& bone = skeleton.getBone(i);
        glm::mat4 localMatrix = m_localTransforms[i].toMatrix();

        if (bone.parentIndex >= 0) {
            m_globalTransforms[i] = m_globalTransforms[bone.parentIndex] * localMatrix;
        } else {
            m_globalTransforms[i] = localMatrix;
        }
    }
}

void SkeletonPose::calculateSkinningMatrices(const Skeleton& skeleton, std::vector<glm::mat4>& outMatrices) const {
    outMatrices.resize(m_globalTransforms.size());
    for (size_t i = 0; i < m_globalTransforms.size(); ++i) {
        outMatrices[i] = m_globalTransforms[i] * skeleton.getBone(static_cast<uint32_t>(i)).inverseBindMatrix;
    }
}

void SkeletonPose::updateMatrices(const Skeleton& skeleton) {
    calculateGlobalTransforms(skeleton);
    calculateSkinningMatrices(skeleton, m_skinningMatrices);
}

void SkeletonPose::setToBindPose(const Skeleton& skeleton) {
    for (uint32_t i = 0; i < skeleton.getBoneCount(); ++i) {
        m_localTransforms[i] = skeleton.getBone(i).bindPose;
    }
}

SkeletonPose SkeletonPose::lerp(const SkeletonPose& a, const SkeletonPose& b, float t) {
    SkeletonPose result;
    result.resize(static_cast<uint32_t>(a.m_localTransforms.size()));

    for (size_t i = 0; i < a.m_localTransforms.size(); ++i) {
        result.m_localTransforms[i] = BoneTransform::lerp(a.m_localTransforms[i], b.m_localTransforms[i], t);
    }

    return result;
}

SkeletonPose SkeletonPose::additive(const SkeletonPose& base, const SkeletonPose& additive,
                                    const SkeletonPose& additiveBindPose, float weight) {
    SkeletonPose result;
    result.resize(base.getBoneCount());

    for (size_t i = 0; i < base.m_localTransforms.size(); ++i) {
        const BoneTransform& baseT = base.m_localTransforms[i];
        const BoneTransform& addT = additive.m_localTransforms[i];
        const BoneTransform& addBindT = additiveBindPose.m_localTransforms[i];

        // Calculate delta from bind pose
        glm::vec3 deltaTranslation = addT.translation - addBindT.translation;
        glm::quat deltaRotation = addT.rotation * glm::inverse(addBindT.rotation);
        glm::vec3 deltaScale = addT.scale / addBindT.scale;

        // Apply weighted delta to base
        result.m_localTransforms[i].translation = baseT.translation + deltaTranslation * weight;
        result.m_localTransforms[i].rotation = glm::slerp(glm::quat(1, 0, 0, 0), deltaRotation, weight) * baseT.rotation;
        result.m_localTransforms[i].scale = baseT.scale * glm::mix(glm::vec3(1.0f), deltaScale, weight);
    }

    return result;
}

void SkeletonPose::blendMasked(const SkeletonPose& other, float weight,
                               const std::vector<bool>& boneMask) {
    for (size_t i = 0; i < m_localTransforms.size() && i < boneMask.size(); ++i) {
        if (boneMask[i]) {
            m_localTransforms[i] = BoneTransform::lerp(m_localTransforms[i], other.m_localTransforms[i], weight);
        }
    }
}

bool SkinnedMeshData::validate() const {
    if (!skeleton) return false;

    uint32_t maxBone = skeleton->getBoneCount();
    for (const auto& vertex : vertices) {
        for (int i = 0; i < 4; ++i) {
            if (vertex.boneWeights[i] > 0.0f && vertex.boneIndices[i] >= maxBone) {
                return false;
            }
        }
    }
    return true;
}

void SkinnedMeshData::applyCPUSkinning(const SkeletonPose& pose,
                                        std::vector<glm::vec3>& outPositions,
                                        std::vector<glm::vec3>& outNormals) const {
    outPositions.resize(vertices.size());
    outNormals.resize(vertices.size());

    const auto& skinMatrices = pose.getSkinningMatrices();

    for (size_t i = 0; i < vertices.size(); ++i) {
        const auto& v = vertices[i];
        outPositions[i] = SkinningUtils::calculateSkinnedPosition(
            v.position, v.boneIndices, v.boneWeights, skinMatrices);
        outNormals[i] = SkinningUtils::calculateSkinnedNormal(
            v.normal, v.boneIndices, v.boneWeights, skinMatrices);
    }
}

namespace SkinningUtils {

glm::vec3 calculateSkinnedPosition(
    const glm::vec3& position,
    const glm::uvec4& boneIndices,
    const glm::vec4& weights,
    const std::vector<glm::mat4>& skinMatrices) {

    glm::vec3 result(0.0f);

    for (int i = 0; i < 4; ++i) {
        if (weights[i] > 0.0001f) {
            result += glm::vec3(skinMatrices[boneIndices[i]] * glm::vec4(position, 1.0f)) * weights[i];
        }
    }

    return result;
}

glm::vec3 calculateSkinnedNormal(
    const glm::vec3& normal,
    const glm::uvec4& boneIndices,
    const glm::vec4& weights,
    const std::vector<glm::mat4>& skinMatrices) {

    glm::vec3 result(0.0f);

    for (int i = 0; i < 4; ++i) {
        if (weights[i] > 0.0001f) {
            result += glm::vec3(skinMatrices[boneIndices[i]] * glm::vec4(normal, 0.0f)) * weights[i];
        }
    }

    return glm::normalize(result);
}

void autoSkinWeights(
    const std::vector<glm::vec3>& vertices,
    const Skeleton& skeleton,
    std::vector<SkinWeight>& outWeights,
    float falloffRadius) {

    outWeights.resize(vertices.size());

    // Pre-compute bone world positions
    std::vector<glm::vec3> bonePositions(skeleton.getBoneCount());
    for (uint32_t i = 0; i < skeleton.getBoneCount(); ++i) {
        glm::mat4 worldTransform = skeleton.calculateBoneWorldTransform(i);
        bonePositions[i] = glm::vec3(worldTransform[3]);
    }

    // For each vertex, find nearest bones and assign weights
    for (size_t vi = 0; vi < vertices.size(); ++vi) {
        const glm::vec3& vertexPos = vertices[vi];
        SkinWeight& weight = outWeights[vi];

        // Calculate distances to all bones
        std::vector<std::pair<float, uint32_t>> distances;
        for (uint32_t bi = 0; bi < skeleton.getBoneCount(); ++bi) {
            float dist = glm::length(vertexPos - bonePositions[bi]);
            if (dist < falloffRadius * 2.0f) {
                distances.push_back({dist, bi});
            }
        }

        // Sort by distance
        std::sort(distances.begin(), distances.end());

        // Assign weights based on inverse distance
        for (size_t i = 0; i < std::min(distances.size(), size_t(MAX_BONES_PER_VERTEX)); ++i) {
            float dist = distances[i].first;
            uint32_t boneIndex = distances[i].second;

            // Smooth falloff
            float w = 1.0f - glm::smoothstep(0.0f, falloffRadius, dist);
            weight.addInfluence(boneIndex, w);
        }

        weight.normalize();
    }
}

void normalizeWeights(std::vector<SkinWeight>& weights) {
    for (auto& w : weights) {
        w.normalize();
    }
}

void pruneWeights(std::vector<SkinWeight>& weights, float threshold) {
    for (auto& w : weights) {
        for (uint32_t i = 0; i < MAX_BONES_PER_VERTEX; ++i) {
            if (w.weights[i] < threshold) {
                w.weights[i] = 0.0f;
            }
        }
        w.normalize();
    }
}

} // namespace SkinningUtils

} // namespace animation
