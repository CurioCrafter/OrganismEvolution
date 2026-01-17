#pragma once

// RenderingOptimizer - Efficient culling, sorting, and instanced rendering for 10,000+ creatures
// Implements frustum culling, LOD sorting, and batch preparation for GPU instancing
// Integrated with centralized LODSystem for consistent distance thresholds

#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <algorithm>
#include <unordered_map>
#include "LODSystem.h"

class Creature;
class Frustum;

namespace Forge {

class Camera;

// ============================================================================
// LOD Mesh Levels
// ============================================================================

enum class MeshLOD {
    HIGH,       // Full geometry (< 30m)
    MEDIUM,     // Reduced geometry (< 80m)
    LOW,        // Simplified geometry (< 150m)
    BILLBOARD,  // Impostor/sprite (< 300m)
    POINT,      // Point sprite (< 500m)
    CULLED,     // Not rendered
    COUNT
};

// ============================================================================
// Visible Creature Info
// ============================================================================

struct VisibleCreature {
    Creature* creature = nullptr;
    size_t index = 0;
    MeshLOD lod = MeshLOD::CULLED;
    float distance = 0.0f;
    float screenSize = 0.0f;  // Approximate screen space size
    bool isOccluded = false;

    // Instance data for GPU
    glm::mat4 worldMatrix;
    glm::vec4 color;
    float animationTime = 0.0f;
};

// ============================================================================
// Instance Batch - Groups creatures for instanced rendering
// ============================================================================

struct InstanceBatch {
    MeshLOD lod = MeshLOD::CULLED;
    int creatureTypeID = 0;  // For mesh selection
    std::vector<VisibleCreature*> instances;

    // GPU instance buffer data (optimized for cache efficiency)
    struct InstanceData {
        glm::mat4 worldMatrix;    // 64 bytes
        glm::vec4 color;          // 16 bytes - rgb + alpha/fade
        glm::vec4 params;         // 16 bytes - animTime, size, distance, fadeFactor
    };
    std::vector<InstanceData> gpuData;

    // Sort key for optimal draw order
    uint64_t sortKey = 0;

    void clear() {
        instances.clear();
        gpuData.clear();
        sortKey = 0;
    }

    void reserve(size_t count) {
        instances.reserve(count);
        gpuData.reserve(count);
    }

    void buildGPUData() {
        gpuData.clear();
        gpuData.reserve(instances.size());
        for (const auto* vc : instances) {
            InstanceData data;
            data.worldMatrix = vc->worldMatrix;
            data.color = vc->color;

            // Calculate fade factor for smooth LOD transitions
            float fadeFactor = 1.0f;  // Full visibility by default
            data.params = glm::vec4(
                vc->animationTime,
                vc->creature->getSize(),
                vc->distance,
                fadeFactor
            );
            gpuData.push_back(data);
        }
    }

    // Generate sort key for batch ordering (minimize state changes)
    void generateSortKey() {
        // Pack: LOD (8 bits) | CreatureType (24 bits) | Distance (32 bits)
        uint64_t lodBits = static_cast<uint64_t>(lod) << 56;
        uint64_t typeBits = static_cast<uint64_t>(creatureTypeID & 0xFFFFFF) << 32;
        uint64_t distBits = instances.empty() ? 0 :
            static_cast<uint64_t>(instances[0]->distance * 1000.0f) & 0xFFFFFFFF;
        sortKey = lodBits | typeBits | distBits;
    }

    // Compare for sorting batches
    bool operator<(const InstanceBatch& other) const {
        return sortKey < other.sortKey;
    }
};

// ============================================================================
// Culling Configuration
// ============================================================================

struct RenderingConfig {
    // LOD distance thresholds (synced from LODSystem)
    float highLODDistance = 30.0f;
    float mediumLODDistance = 80.0f;
    float lowLODDistance = 150.0f;
    float billboardDistance = 300.0f;
    float pointDistance = 500.0f;

    // Screen-space LOD (min pixels for each LOD)
    float highLODMinPixels = 100.0f;
    float mediumLODMinPixels = 50.0f;
    float lowLODMinPixels = 20.0f;
    float billboardMinPixels = 5.0f;

    // Culling options
    bool enableFrustumCulling = true;
    bool enableDistanceCulling = true;
    bool enableOcclusionCulling = false;
    bool enableScreenSpaceLOD = true;

    // Batching optimization settings
    int maxInstancesPerBatch = 4096;       // Increased for better GPU utilization
    bool sortByDistance = true;
    bool sortByMaterial = false;
    bool batchByCreatureType = true;       // Group similar creatures together
    bool enableMegaBatching = true;        // Combine multiple small batches

    // Quality scale (0.5 - 1.5, affects all distances)
    float qualityScale = 1.0f;

    // Fade transition settings
    float fadeRange = 15.0f;               // Distance over which LOD fades

    // Sync from LODSystem
    void syncFromLODConfig(const LOD::LODConfig& lodConfig) {
        highLODDistance = lodConfig.creatureFull;
        mediumLODDistance = lodConfig.creatureMedium;
        lowLODDistance = lodConfig.creatureLow;
        billboardDistance = lodConfig.creatureBillboard;
        pointDistance = lodConfig.creaturePoint;
        qualityScale = lodConfig.qualityScale;
        fadeRange = lodConfig.creatureFadeRange;
    }
};

// ============================================================================
// Rendering Statistics
// ============================================================================

struct RenderingStats {
    // Counts per LOD
    std::array<int, static_cast<size_t>(MeshLOD::COUNT)> countByLOD{};

    // Culling stats
    int totalCreatures = 0;
    int visibleCreatures = 0;
    int culledByFrustum = 0;
    int culledByDistance = 0;
    int culledByOcclusion = 0;
    int culledByScreenSize = 0;

    // Batching stats
    int totalBatches = 0;
    int totalInstances = 0;
    int drawCalls = 0;

    // Performance
    float cullTimeMs = 0.0f;
    float sortTimeMs = 0.0f;
    float batchTimeMs = 0.0f;

    void reset() {
        countByLOD.fill(0);
        totalCreatures = visibleCreatures = 0;
        culledByFrustum = culledByDistance = culledByOcclusion = culledByScreenSize = 0;
        totalBatches = totalInstances = drawCalls = 0;
        cullTimeMs = sortTimeMs = batchTimeMs = 0.0f;
    }
};

// ============================================================================
// RenderingOptimizer
// ============================================================================

class RenderingOptimizer {
public:
    RenderingOptimizer();
    ~RenderingOptimizer() = default;

    // Configuration
    void setConfig(const RenderingConfig& config) { m_config = config; }
    const RenderingConfig& getConfig() const { return m_config; }

    // ========================================================================
    // Main Interface
    // ========================================================================

    // Cull and sort creatures for rendering
    void cullAndSort(const std::vector<Creature*>& creatures,
                     const Frustum& frustum,
                     const glm::vec3& cameraPosition,
                     const glm::mat4& viewProjection,
                     float screenWidth, float screenHeight);

    // Build instance batches for GPU
    void buildBatches();

    // ========================================================================
    // Query Interface
    // ========================================================================

    // Get all visible creatures (sorted by LOD, then distance)
    const std::vector<VisibleCreature>& getVisibleCreatures() const { return m_visibleCreatures; }

    // Get creatures at specific LOD
    std::vector<const VisibleCreature*> getCreaturesAtLOD(MeshLOD lod) const;

    // Get instance batches for rendering
    const std::vector<InstanceBatch>& getBatches() const { return m_batches; }

    // Get LOD for a specific creature index
    MeshLOD getCreatureLOD(size_t creatureIndex) const;

    // ========================================================================
    // Utilities
    // ========================================================================

    // Calculate LOD based on distance and screen size
    MeshLOD calculateLOD(float distance, float screenSize) const;

    // Calculate approximate screen size (in pixels)
    float calculateScreenSize(float worldSize, float distance,
                               float fovY, float screenHeight) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    const RenderingStats& getStats() const { return m_stats; }

    // LOD name for debugging
    static const char* getLODName(MeshLOD lod);

private:
    RenderingConfig m_config;

    // Visible creatures (after culling)
    std::vector<VisibleCreature> m_visibleCreatures;

    // Index map for quick lookup
    std::vector<MeshLOD> m_creatureLODs;

    // Instance batches
    std::vector<InstanceBatch> m_batches;

    // Statistics
    RenderingStats m_stats;

    // Helper methods
    bool frustumCull(const glm::vec3& position, float radius, const Frustum& frustum) const;
    glm::mat4 buildWorldMatrix(const Creature* creature) const;
    glm::vec4 getCreatureColor(const Creature* creature) const;
    void mergSmallBatches();
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline RenderingOptimizer::RenderingOptimizer() {
    m_visibleCreatures.reserve(65536);
    m_creatureLODs.resize(65536, MeshLOD::CULLED);
    m_batches.reserve(64);
}

inline MeshLOD RenderingOptimizer::getCreatureLOD(size_t creatureIndex) const {
    if (creatureIndex >= m_creatureLODs.size()) return MeshLOD::CULLED;
    return m_creatureLODs[creatureIndex];
}

inline const char* RenderingOptimizer::getLODName(MeshLOD lod) {
    static const char* names[] = {
        "High", "Medium", "Low", "Billboard", "Point", "Culled"
    };
    return names[static_cast<size_t>(lod)];
}

inline MeshLOD RenderingOptimizer::calculateLOD(float distance, float screenSize) const {
    float scale = m_config.qualityScale;

    // Screen-space based LOD (takes precedence if enabled)
    if (m_config.enableScreenSpaceLOD && screenSize > 0.0f) {
        if (screenSize >= m_config.highLODMinPixels) return MeshLOD::HIGH;
        if (screenSize >= m_config.mediumLODMinPixels) return MeshLOD::MEDIUM;
        if (screenSize >= m_config.lowLODMinPixels) return MeshLOD::LOW;
        if (screenSize >= m_config.billboardMinPixels) return MeshLOD::BILLBOARD;
        if (screenSize >= 1.0f) return MeshLOD::POINT;
        return MeshLOD::CULLED;
    }

    // Distance-based LOD
    if (distance < m_config.highLODDistance * scale) return MeshLOD::HIGH;
    if (distance < m_config.mediumLODDistance * scale) return MeshLOD::MEDIUM;
    if (distance < m_config.lowLODDistance * scale) return MeshLOD::LOW;
    if (distance < m_config.billboardDistance * scale) return MeshLOD::BILLBOARD;
    if (distance < m_config.pointDistance * scale) return MeshLOD::POINT;
    return MeshLOD::CULLED;
}

inline float RenderingOptimizer::calculateScreenSize(float worldSize, float distance,
                                                       float fovY, float screenHeight) const {
    if (distance < 0.001f) return screenHeight;  // Very close

    // Project world size to screen space
    // screenSize = (worldSize / distance) * (screenHeight / (2 * tan(fovY/2)))
    float tanHalfFov = std::tan(fovY * 0.5f);
    return (worldSize / distance) * (screenHeight * 0.5f / tanHalfFov);
}

} // namespace Forge
