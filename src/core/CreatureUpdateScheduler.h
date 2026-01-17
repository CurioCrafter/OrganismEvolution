#pragma once

// CreatureUpdateScheduler - Distance-based LOD update scheduling for 10,000+ creatures at 60 FPS
// Implements tiered update frequencies based on camera distance and creature importance

#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <algorithm>
#include <chrono>

class Creature;

namespace Forge {

class Camera;
class PerformanceManager;

// ============================================================================
// Update Priority Tiers
// ============================================================================

enum class UpdateTier {
    CRITICAL,     // Every frame - nearby, selected, or important creatures
    HIGH,         // Every frame - within 50m
    MEDIUM,       // Every 2nd frame - within 150m
    LOW,          // Every 4th frame - within 300m
    MINIMAL,      // Every 8th frame - beyond 300m
    DORMANT,      // Every 16th frame - very far or offscreen
    COUNT
};

// ============================================================================
// Update Configuration
// ============================================================================

struct UpdateSchedulerConfig {
    // Distance thresholds (meters)
    float criticalDistance = 20.0f;      // Critical tier
    float highDistance = 50.0f;          // High tier
    float mediumDistance = 150.0f;       // Medium tier
    float lowDistance = 300.0f;          // Low tier
    float minimalDistance = 500.0f;      // Minimal tier (beyond = dormant)

    // Update frequencies (frame intervals)
    int criticalInterval = 1;
    int highInterval = 1;
    int mediumInterval = 2;
    int lowInterval = 4;
    int minimalInterval = 8;
    int dormantInterval = 16;

    // Time budgets (milliseconds) - soft limits
    float criticalBudgetMs = 4.0f;
    float highBudgetMs = 3.0f;
    float mediumBudgetMs = 2.0f;
    float lowBudgetMs = 1.0f;

    // Importance multipliers
    float selectedImportanceBoost = 2.0f;    // Selected creature is always critical
    float predatorImportanceBoost = 1.5f;    // Predators get priority
    float reproducingBoost = 1.3f;           // Creatures about to reproduce

    // Adaptive scheduling
    bool adaptiveScheduling = true;
    float targetFrameTimeMs = 16.67f;        // 60 FPS target
    float budgetScaleMin = 0.5f;
    float budgetScaleMax = 1.5f;
};

// ============================================================================
// Scheduled Creature Info
// ============================================================================

struct ScheduledCreature {
    Creature* creature = nullptr;
    size_t index = 0;
    UpdateTier tier = UpdateTier::DORMANT;
    float distance = 0.0f;
    float importance = 1.0f;
    float lastUpdateTime = 0.0f;
    float accumulatedDeltaTime = 0.0f;  // Time since last update (for interpolation)
    bool needsUpdate = false;
    bool isVisible = false;
};

// ============================================================================
// Update Batch - Groups creatures by tier for efficient processing
// ============================================================================

struct UpdateBatch {
    UpdateTier tier = UpdateTier::DORMANT;
    std::vector<ScheduledCreature*> creatures;
    float budgetMs = 1.0f;
    float usedMs = 0.0f;
    int targetCount = 0;
    int actualCount = 0;

    void clear() {
        creatures.clear();
        usedMs = 0.0f;
        actualCount = 0;
    }

    void reserve(size_t count) {
        creatures.reserve(count);
    }
};

// ============================================================================
// Scheduler Statistics
// ============================================================================

struct SchedulerStats {
    // Creature counts per tier
    std::array<int, static_cast<size_t>(UpdateTier::COUNT)> countByTier{};

    // Updates this frame
    std::array<int, static_cast<size_t>(UpdateTier::COUNT)> updatesThisFrame{};

    // Time spent per tier (ms)
    std::array<float, static_cast<size_t>(UpdateTier::COUNT)> timeByTier{};

    // Totals
    int totalCreatures = 0;
    int totalUpdates = 0;
    float totalTimeMs = 0.0f;
    float budgetScale = 1.0f;

    // Efficiency metrics
    float updateRate = 0.0f;          // Updates per creature per frame
    float avgUpdateTimeUs = 0.0f;     // Average time per update

    void reset() {
        countByTier.fill(0);
        updatesThisFrame.fill(0);
        timeByTier.fill(0.0f);
        totalCreatures = totalUpdates = 0;
        totalTimeMs = 0.0f;
    }
};

// ============================================================================
// CreatureUpdateScheduler
// ============================================================================

class CreatureUpdateScheduler {
public:
    CreatureUpdateScheduler();
    ~CreatureUpdateScheduler() = default;

    // Configuration
    void setConfig(const UpdateSchedulerConfig& config) { m_config = config; }
    const UpdateSchedulerConfig& getConfig() const { return m_config; }

    // ========================================================================
    // Main Interface
    // ========================================================================

    // Call at start of frame to classify and schedule all creatures
    void scheduleUpdates(const std::vector<Creature*>& creatures,
                         const glm::vec3& cameraPosition,
                         const glm::mat4& viewProjection,
                         size_t selectedIndex = SIZE_MAX);

    // Execute updates with time budgeting
    void executeUpdates(float deltaTime);

    // Execute specific tier (for parallel processing)
    void executeTier(UpdateTier tier, float deltaTime);

    // ========================================================================
    // Query Interface
    // ========================================================================

    // Get all creatures that need update this frame
    const std::vector<ScheduledCreature*>& getUpdateList() const { return m_updateList; }

    // Get creatures by tier
    const UpdateBatch& getBatch(UpdateTier tier) const;

    // Check if specific creature should update
    bool shouldUpdate(size_t creatureIndex) const;

    // Get accumulated delta time for a creature (for interpolation)
    float getAccumulatedDelta(size_t creatureIndex) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    const SchedulerStats& getStats() const { return m_stats; }

    // Get tier name for debugging
    static const char* getTierName(UpdateTier tier);

private:
    UpdateSchedulerConfig m_config;

    // Per-creature scheduling data
    std::vector<ScheduledCreature> m_scheduledCreatures;

    // Batches by tier
    std::array<UpdateBatch, static_cast<size_t>(UpdateTier::COUNT)> m_batches;

    // Combined update list for this frame
    std::vector<ScheduledCreature*> m_updateList;

    // Frame tracking
    int m_frameNumber = 0;
    float m_lastFrameTimeMs = 16.67f;
    float m_budgetScale = 1.0f;

    // Statistics
    SchedulerStats m_stats;

    // Timing
    std::chrono::high_resolution_clock::time_point m_tierStart;

    // Helper methods
    UpdateTier calculateTier(float distance, float importance, bool isVisible) const;
    float calculateImportance(const Creature* creature, size_t selectedIndex) const;
    bool shouldUpdateThisFrame(UpdateTier tier) const;
    void updateAdaptiveBudget();

    // Frustum culling helper
    bool isInFrustum(const glm::vec3& position, float radius,
                     const glm::mat4& viewProjection) const;
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline CreatureUpdateScheduler::CreatureUpdateScheduler() {
    m_scheduledCreatures.reserve(65536);
    m_updateList.reserve(65536);

    for (size_t i = 0; i < static_cast<size_t>(UpdateTier::COUNT); ++i) {
        m_batches[i].tier = static_cast<UpdateTier>(i);
        m_batches[i].reserve(16384);
    }
}

inline const UpdateBatch& CreatureUpdateScheduler::getBatch(UpdateTier tier) const {
    return m_batches[static_cast<size_t>(tier)];
}

inline bool CreatureUpdateScheduler::shouldUpdate(size_t creatureIndex) const {
    if (creatureIndex >= m_scheduledCreatures.size()) return true;
    return m_scheduledCreatures[creatureIndex].needsUpdate;
}

inline float CreatureUpdateScheduler::getAccumulatedDelta(size_t creatureIndex) const {
    if (creatureIndex >= m_scheduledCreatures.size()) return 0.0f;
    return m_scheduledCreatures[creatureIndex].accumulatedDeltaTime;
}

inline const char* CreatureUpdateScheduler::getTierName(UpdateTier tier) {
    static const char* names[] = {
        "Critical", "High", "Medium", "Low", "Minimal", "Dormant"
    };
    return names[static_cast<size_t>(tier)];
}

inline UpdateTier CreatureUpdateScheduler::calculateTier(float distance, float importance,
                                                          bool isVisible) const {
    // Apply importance as a distance reduction (more important = closer effective distance)
    float effectiveDistance = distance / importance;

    // Non-visible creatures go to dormant unless very close
    if (!isVisible && distance > m_config.criticalDistance) {
        return UpdateTier::DORMANT;
    }

    if (effectiveDistance < m_config.criticalDistance) return UpdateTier::CRITICAL;
    if (effectiveDistance < m_config.highDistance) return UpdateTier::HIGH;
    if (effectiveDistance < m_config.mediumDistance) return UpdateTier::MEDIUM;
    if (effectiveDistance < m_config.lowDistance) return UpdateTier::LOW;
    if (effectiveDistance < m_config.minimalDistance) return UpdateTier::MINIMAL;
    return UpdateTier::DORMANT;
}

inline bool CreatureUpdateScheduler::shouldUpdateThisFrame(UpdateTier tier) const {
    int interval = 1;
    switch (tier) {
        case UpdateTier::CRITICAL: interval = m_config.criticalInterval; break;
        case UpdateTier::HIGH: interval = m_config.highInterval; break;
        case UpdateTier::MEDIUM: interval = m_config.mediumInterval; break;
        case UpdateTier::LOW: interval = m_config.lowInterval; break;
        case UpdateTier::MINIMAL: interval = m_config.minimalInterval; break;
        case UpdateTier::DORMANT: interval = m_config.dormantInterval; break;
        default: break;
    }
    return (m_frameNumber % interval) == 0;
}

inline bool CreatureUpdateScheduler::isInFrustum(const glm::vec3& position, float radius,
                                                   const glm::mat4& viewProjection) const {
    glm::vec4 clipPos = viewProjection * glm::vec4(position, 1.0f);

    // Sphere-frustum test in clip space
    if (clipPos.x < -clipPos.w - radius) return false;
    if (clipPos.x > clipPos.w + radius) return false;
    if (clipPos.y < -clipPos.w - radius) return false;
    if (clipPos.y > clipPos.w + radius) return false;
    if (clipPos.z < -radius) return false;
    if (clipPos.z > clipPos.w + radius) return false;

    return true;
}

} // namespace Forge
