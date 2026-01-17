#pragma once

// PerformanceManager - LOD system, frustum culling, update frequency management
// Ensures 60 FPS with large creature counts through intelligent optimization

#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <chrono>
#include <functional>

// Forward declaration (global namespace)
class Camera;

namespace Forge {

class CreatureManager;

// Import global Camera into Forge namespace
using ::Camera;

// ============================================================================
// LOD Levels
// ============================================================================

enum class LODLevel {
    FULL,       // Full detail (close)
    MEDIUM,     // Reduced detail
    LOW,        // Minimal detail
    BILLBOARD,  // Sprite representation
    CULLED      // Not rendered
};

// Distance thresholds for LOD transitions
struct LODThresholds {
    float fullToMedium = 50.0f;
    float mediumToLow = 100.0f;
    float lowToBillboard = 200.0f;
    float billboardToCulled = 400.0f;
};

// ============================================================================
// Update Frequency Buckets
// ============================================================================

enum class UpdateBucket {
    EVERY_FRAME,    // Critical/nearby creatures
    EVERY_2ND,      // Medium distance
    EVERY_4TH,      // Far creatures
    EVERY_8TH,      // Very far creatures
    PAUSED          // Out of range, minimal updates
};

// ============================================================================
// Performance Statistics
// ============================================================================

struct PerformanceStats {
    // Frame timing (in milliseconds)
    float frameTime = 0.0f;
    float updateTime = 0.0f;
    float renderTime = 0.0f;
    float gpuTime = 0.0f;

    // FPS
    float currentFPS = 0.0f;
    float avgFPS = 0.0f;
    float minFPS = 999.0f;
    float maxFPS = 0.0f;

    // Creature counts by LOD
    std::array<int, 5> creaturesByLOD{};  // FULL, MEDIUM, LOW, BILLBOARD, CULLED

    // Update bucket counts
    std::array<int, 5> creaturesByBucket{};

    // Culling stats
    int totalCreatures = 0;
    int visibleCreatures = 0;
    int culledByFrustum = 0;
    int culledByDistance = 0;
    int culledByOcclusion = 0;

    // Memory
    size_t creaturePoolMemory = 0;
    size_t gpuMemoryUsed = 0;

    // Batching
    int drawCalls = 0;
    int trianglesRendered = 0;
    int instancesRendered = 0;

    void reset() {
        creaturesByLOD.fill(0);
        creaturesByBucket.fill(0);
        totalCreatures = visibleCreatures = 0;
        culledByFrustum = culledByDistance = culledByOcclusion = 0;
        drawCalls = trianglesRendered = instancesRendered = 0;
    }
};

// ============================================================================
// Performance Configuration
// ============================================================================

struct PerformanceConfig {
    // Target frame rate
    float targetFPS = 60.0f;

    // LOD thresholds
    LODThresholds lodThresholds;

    // Update frequency distances
    float everyFrameDistance = 30.0f;
    float every2ndDistance = 80.0f;
    float every4thDistance = 150.0f;
    float every8thDistance = 300.0f;

    // Adaptive quality
    bool enableAdaptiveQuality = true;
    float qualityScaleMin = 0.5f;
    float qualityScaleMax = 1.0f;

    // Culling options
    bool enableFrustumCulling = true;
    bool enableDistanceCulling = true;
    bool enableOcclusionCulling = false;  // Advanced, optional

    // Batching
    bool enableInstancing = true;
    int maxInstancesPerBatch = 1024;

    // Memory limits
    size_t maxCreaturePoolSize = 65536;
    size_t maxGPUMemory = 512 * 1024 * 1024;  // 512 MB
};

// ============================================================================
// Creature Render Info
// ============================================================================

struct CreatureRenderInfo {
    size_t creatureIndex;
    LODLevel lod;
    UpdateBucket updateBucket;
    float distanceToCamera;
    bool visible;
    bool needsUpdate;
};

// ============================================================================
// Performance Manager
// ============================================================================

class PerformanceManager {
public:
    PerformanceManager();
    ~PerformanceManager() = default;

    // Initialization
    void init(CreatureManager* creatures, Camera* camera);
    void setConfig(const PerformanceConfig& config) { m_config = config; }
    const PerformanceConfig& getConfig() const { return m_config; }

    // ========================================================================
    // Frame Management
    // ========================================================================

    // Call at start of frame
    void beginFrame();

    // Call at end of frame
    void endFrame();

    // Get delta time with frame limiting
    float getDeltaTime() const { return m_deltaTime; }

    // ========================================================================
    // LOD & Culling
    // ========================================================================

    // Classify all creatures for LOD and visibility
    void classifyCreatures(const glm::vec3& cameraPos, const glm::mat4& viewProjection);

    // Get LOD level for a distance
    LODLevel getLODForDistance(float distance) const;

    // Get update bucket for a distance
    UpdateBucket getUpdateBucketForDistance(float distance) const;

    // Check if creature should be updated this frame
    bool shouldUpdateCreature(size_t creatureIndex) const;

    // Check if creature is visible
    bool isCreatureVisible(size_t creatureIndex) const;

    // Get render info for all creatures (for batching)
    const std::vector<CreatureRenderInfo>& getRenderInfo() const { return m_renderInfo; }

    // Get creatures to render at specific LOD
    std::vector<size_t> getCreaturesAtLOD(LODLevel lod) const;

    // ========================================================================
    // Adaptive Quality
    // ========================================================================

    // Get current quality scale (0.5-1.0)
    float getQualityScale() const { return m_qualityScale; }

    // Force quality level (for settings menu)
    void setQualityScale(float scale);

    // ========================================================================
    // Statistics
    // ========================================================================

    const PerformanceStats& getStats() const { return m_stats; }

    // ========================================================================
    // Profiling
    // ========================================================================

    // Start a profiling section
    void beginSection(const char* name);

    // End a profiling section
    void endSection(const char* name);

    // Get section time (ms)
    float getSectionTime(const char* name) const;

private:
    // References
    CreatureManager* m_creatures = nullptr;
    Camera* m_camera = nullptr;

    // Configuration
    PerformanceConfig m_config;

    // Frame timing
    std::chrono::high_resolution_clock::time_point m_frameStart;
    std::chrono::high_resolution_clock::time_point m_lastFrameEnd;
    float m_deltaTime = 0.016f;
    int m_frameNumber = 0;

    // Adaptive quality
    float m_qualityScale = 1.0f;
    float m_fpsHistory[60] = {60.0f};
    int m_fpsHistoryIndex = 0;

    // Per-creature info
    std::vector<CreatureRenderInfo> m_renderInfo;

    // Statistics
    PerformanceStats m_stats;

    // Profiling sections
    struct ProfilingSection {
        std::chrono::high_resolution_clock::time_point start;
        float duration = 0.0f;
    };
    std::vector<std::pair<std::string, ProfilingSection>> m_sections;

    // Helper methods
    void updateAdaptiveQuality();
    bool isInFrustum(const glm::vec3& position, float radius,
                     const glm::mat4& viewProjection) const;
    void updateFPSHistory(float fps);
};

// ============================================================================
// Implementation (Inline for header-only usage)
// ============================================================================

inline PerformanceManager::PerformanceManager() {
    m_renderInfo.reserve(65536);
    m_sections.reserve(16);
}

inline void PerformanceManager::init(CreatureManager* creatures, Camera* camera) {
    m_creatures = creatures;
    m_camera = camera;
    m_lastFrameEnd = std::chrono::high_resolution_clock::now();
}

inline void PerformanceManager::beginFrame() {
    m_frameStart = std::chrono::high_resolution_clock::now();

    // Calculate delta time
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        m_frameStart - m_lastFrameEnd);
    m_deltaTime = duration.count() / 1000000.0f;

    // Clamp delta time to prevent huge jumps
    m_deltaTime = std::min(m_deltaTime, 0.1f);

    m_stats.reset();
    m_frameNumber++;
}

inline void PerformanceManager::endFrame() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - m_frameStart);

    m_stats.frameTime = duration.count() / 1000.0f;
    m_stats.currentFPS = 1000.0f / std::max(0.001f, m_stats.frameTime);

    updateFPSHistory(m_stats.currentFPS);
    updateAdaptiveQuality();

    // Frame rate limiting
    float targetFrameTime = 1000.0f / m_config.targetFPS;
    if (m_stats.frameTime < targetFrameTime) {
        // Sleep for remaining time (rough estimate)
        float sleepMs = targetFrameTime - m_stats.frameTime;
        // Note: Actual sleep implementation depends on platform
    }

    m_lastFrameEnd = now;
}

inline void PerformanceManager::classifyCreatures(const glm::vec3& cameraPos,
                                                   const glm::mat4& viewProjection) {
    if (!m_creatures) return;

    m_renderInfo.clear();
    auto& creatures = m_creatures->getAllCreatures();

    m_stats.totalCreatures = 0;

    for (size_t i = 0; i < creatures.size(); ++i) {
        const auto& creature = creatures[i];
        if (!creature || !creature->isActive()) continue;

        m_stats.totalCreatures++;

        CreatureRenderInfo info;
        info.creatureIndex = i;

        // Calculate distance
        glm::vec3 pos = creature->getPosition();
        glm::vec3 diff = pos - cameraPos;
        info.distanceToCamera = glm::length(diff);

        // Determine LOD
        info.lod = getLODForDistance(info.distanceToCamera);

        // Determine update bucket
        info.updateBucket = getUpdateBucketForDistance(info.distanceToCamera);

        // Check visibility
        info.visible = true;

        // Distance culling
        if (m_config.enableDistanceCulling &&
            info.distanceToCamera > m_config.lodThresholds.billboardToCulled) {
            info.visible = false;
            info.lod = LODLevel::CULLED;
            m_stats.culledByDistance++;
        }

        // Frustum culling
        if (info.visible && m_config.enableFrustumCulling) {
            float radius = creature->getSize() * 2.0f;
            if (!isInFrustum(pos, radius, viewProjection)) {
                info.visible = false;
                info.lod = LODLevel::CULLED;
                m_stats.culledByFrustum++;
            }
        }

        // Determine if needs update this frame
        info.needsUpdate = false;
        switch (info.updateBucket) {
            case UpdateBucket::EVERY_FRAME:
                info.needsUpdate = true;
                break;
            case UpdateBucket::EVERY_2ND:
                info.needsUpdate = (m_frameNumber % 2 == 0);
                break;
            case UpdateBucket::EVERY_4TH:
                info.needsUpdate = (m_frameNumber % 4 == 0);
                break;
            case UpdateBucket::EVERY_8TH:
                info.needsUpdate = (m_frameNumber % 8 == 0);
                break;
            case UpdateBucket::PAUSED:
                info.needsUpdate = (m_frameNumber % 32 == 0);
                break;
        }

        // Update stats
        m_stats.creaturesByLOD[static_cast<int>(info.lod)]++;
        m_stats.creaturesByBucket[static_cast<int>(info.updateBucket)]++;
        if (info.visible) {
            m_stats.visibleCreatures++;
        }

        m_renderInfo.push_back(info);
    }
}

inline LODLevel PerformanceManager::getLODForDistance(float distance) const {
    const auto& t = m_config.lodThresholds;

    // Apply quality scale to thresholds
    float scale = m_qualityScale;

    if (distance < t.fullToMedium * scale) return LODLevel::FULL;
    if (distance < t.mediumToLow * scale) return LODLevel::MEDIUM;
    if (distance < t.lowToBillboard * scale) return LODLevel::LOW;
    if (distance < t.billboardToCulled * scale) return LODLevel::BILLBOARD;
    return LODLevel::CULLED;
}

inline UpdateBucket PerformanceManager::getUpdateBucketForDistance(float distance) const {
    if (distance < m_config.everyFrameDistance) return UpdateBucket::EVERY_FRAME;
    if (distance < m_config.every2ndDistance) return UpdateBucket::EVERY_2ND;
    if (distance < m_config.every4thDistance) return UpdateBucket::EVERY_4TH;
    if (distance < m_config.every8thDistance) return UpdateBucket::EVERY_8TH;
    return UpdateBucket::PAUSED;
}

inline bool PerformanceManager::shouldUpdateCreature(size_t creatureIndex) const {
    for (const auto& info : m_renderInfo) {
        if (info.creatureIndex == creatureIndex) {
            return info.needsUpdate;
        }
    }
    return true;  // Default to updating
}

inline bool PerformanceManager::isCreatureVisible(size_t creatureIndex) const {
    for (const auto& info : m_renderInfo) {
        if (info.creatureIndex == creatureIndex) {
            return info.visible;
        }
    }
    return false;
}

inline std::vector<size_t> PerformanceManager::getCreaturesAtLOD(LODLevel lod) const {
    std::vector<size_t> result;
    for (const auto& info : m_renderInfo) {
        if (info.lod == lod && info.visible) {
            result.push_back(info.creatureIndex);
        }
    }
    return result;
}

inline void PerformanceManager::setQualityScale(float scale) {
    m_qualityScale = glm::clamp(scale, m_config.qualityScaleMin, m_config.qualityScaleMax);
}

inline void PerformanceManager::beginSection(const char* name) {
    ProfilingSection section;
    section.start = std::chrono::high_resolution_clock::now();
    m_sections.push_back({name, section});
}

inline void PerformanceManager::endSection(const char* name) {
    auto now = std::chrono::high_resolution_clock::now();
    for (auto& [sectionName, section] : m_sections) {
        if (sectionName == name) {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                now - section.start);
            section.duration = duration.count() / 1000.0f;
            return;
        }
    }
}

inline float PerformanceManager::getSectionTime(const char* name) const {
    for (const auto& [sectionName, section] : m_sections) {
        if (sectionName == name) {
            return section.duration;
        }
    }
    return 0.0f;
}

inline void PerformanceManager::updateAdaptiveQuality() {
    if (!m_config.enableAdaptiveQuality) return;

    // Calculate average FPS
    float sum = 0.0f;
    for (float fps : m_fpsHistory) {
        sum += fps;
    }
    m_stats.avgFPS = sum / 60.0f;

    // Adjust quality based on FPS
    float targetFPS = m_config.targetFPS;

    if (m_stats.avgFPS < targetFPS * 0.8f) {
        // Below 80% of target, reduce quality
        m_qualityScale = std::max(m_config.qualityScaleMin, m_qualityScale - 0.01f);
    } else if (m_stats.avgFPS > targetFPS * 0.95f) {
        // Above 95% of target, increase quality
        m_qualityScale = std::min(m_config.qualityScaleMax, m_qualityScale + 0.005f);
    }
}

inline bool PerformanceManager::isInFrustum(const glm::vec3& position, float radius,
                                            const glm::mat4& viewProjection) const {
    // Simple sphere-frustum test using clip space
    glm::vec4 clipPos = viewProjection * glm::vec4(position, 1.0f);

    // Check against all 6 planes
    // Left: x > -w
    if (clipPos.x < -clipPos.w - radius) return false;
    // Right: x < w
    if (clipPos.x > clipPos.w + radius) return false;
    // Bottom: y > -w
    if (clipPos.y < -clipPos.w - radius) return false;
    // Top: y < w
    if (clipPos.y > clipPos.w + radius) return false;
    // Near: z > 0 (for standard projection)
    if (clipPos.z < -radius) return false;
    // Far: z < w
    if (clipPos.z > clipPos.w + radius) return false;

    return true;
}

inline void PerformanceManager::updateFPSHistory(float fps) {
    m_fpsHistory[m_fpsHistoryIndex] = fps;
    m_fpsHistoryIndex = (m_fpsHistoryIndex + 1) % 60;

    // Update min/max
    m_stats.minFPS = std::min(m_stats.minFPS, fps);
    m_stats.maxFPS = std::max(m_stats.maxFPS, fps);
}

} // namespace Forge
