#pragma once

// PerformanceIntegration - Integrates all performance optimization systems
// This header provides a unified interface for the performance subsystems

#include "CreatureUpdateScheduler.h"
#include "MemoryOptimizer.h"
#include "QualityScaler.h"
#include "ProfilerReport.h"
#include "../graphics/RenderingOptimizer.h"
#include "../utils/HierarchicalSpatialGrid.h"
#include "../ai/GPUBehaviorCompute.h"

#include <memory>

class Creature;
class Frustum;
class Camera;
struct ID3D12GraphicsCommandList;

namespace Forge {

class CreatureManager;
class PerformanceManager;

// ============================================================================
// Performance Subsystem Manager
// ============================================================================

class PerformanceSubsystems {
public:
    PerformanceSubsystems();
    ~PerformanceSubsystems();

    // Non-copyable
    PerformanceSubsystems(const PerformanceSubsystems&) = delete;
    PerformanceSubsystems& operator=(const PerformanceSubsystems&) = delete;

    // ========================================================================
    // Initialization
    // ========================================================================

    // Initialize all subsystems
    bool initialize(float worldWidth, float worldDepth);

    // Initialize GPU subsystems (requires DX12 device)
    bool initializeGPU(DX12Device* device);

    // Shutdown
    void shutdown();

    // ========================================================================
    // Frame Interface
    // ========================================================================

    // Call at start of frame
    void beginFrame();

    // Schedule and classify creatures for update
    void scheduleCreatureUpdates(const std::vector<Creature*>& creatures,
                                  const glm::vec3& cameraPosition,
                                  const glm::mat4& viewProjection,
                                  size_t selectedIndex = SIZE_MAX);

    // Execute scheduled updates (returns delta time accounting for LOD)
    void executeScheduledUpdates(float deltaTime);

    // Prepare rendering (cull, sort, batch)
    void prepareRendering(const std::vector<Creature*>& creatures,
                          const Frustum& frustum,
                          const glm::vec3& cameraPosition,
                          const glm::mat4& viewProjection,
                          float screenWidth, float screenHeight);

    // Update quality scaling based on frame time
    void updateQuality(float frameTimeMs);

    // Call at end of frame
    void endFrame();

    // ========================================================================
    // GPU Compute
    // ========================================================================

    // Dispatch GPU behavior computation
    void dispatchGPUBehaviors(ID3D12GraphicsCommandList* cmdList, float deltaTime);

    // Get computed behaviors (requires GPU sync)
    glm::vec3 getComputedSteering(size_t creatureIndex) const;

    // ========================================================================
    // Spatial Grid
    // ========================================================================

    // Rebuild spatial grid with all creatures
    void rebuildSpatialGrid(const std::vector<Creature*>& creatures);

    // Query nearby creatures
    const std::vector<Creature*>& queryNearby(const glm::vec3& position, float radius) const;

    // Find nearest creature
    Creature* findNearest(const glm::vec3& position, float radius,
                          int typeFilter = -1) const;

    // ========================================================================
    // Memory Management
    // ========================================================================

    // Get frame arena for temporary allocations
    MemoryArena& getFrameArena();

    // Trigger defragmentation (call during loading or pauses)
    void defragment();

    // ========================================================================
    // Subsystem Access
    // ========================================================================

    CreatureUpdateScheduler* getScheduler() { return m_scheduler.get(); }
    RenderingOptimizer* getRenderingOptimizer() { return m_renderOptimizer.get(); }
    MemoryOptimizer* getMemoryOptimizer() { return m_memoryOptimizer.get(); }
    QualityScaler* getQualityScaler() { return m_qualityScaler.get(); }
    ProfilerReport* getProfiler() { return m_profiler.get(); }
    HierarchicalSpatialGrid* getSpatialGrid() { return m_spatialGrid.get(); }
    GPUBehaviorCompute* getGPUBehaviors() { return m_gpuBehaviors.get(); }

    const CreatureUpdateScheduler* getScheduler() const { return m_scheduler.get(); }
    const RenderingOptimizer* getRenderingOptimizer() const { return m_renderOptimizer.get(); }
    const MemoryOptimizer* getMemoryOptimizer() const { return m_memoryOptimizer.get(); }
    const QualityScaler* getQualityScaler() const { return m_qualityScaler.get(); }
    const ProfilerReport* getProfiler() const { return m_profiler.get(); }
    const HierarchicalSpatialGrid* getSpatialGrid() const { return m_spatialGrid.get(); }
    const GPUBehaviorCompute* getGPUBehaviors() const { return m_gpuBehaviors.get(); }

    // ========================================================================
    // Statistics
    // ========================================================================

    // Get comprehensive performance report
    const FrameReport& getFrameReport() const { return m_profiler->getCurrentReport(); }

    // Get optimization suggestions
    std::vector<std::string> getOptimizationSuggestions() const;

    // Should quality be reduced?
    bool shouldReduceQuality() const;

    // Should quality be increased?
    bool shouldIncreaseQuality() const;

    // ========================================================================
    // Configuration
    // ========================================================================

    // Apply quality settings to all subsystems
    void applyQualitySettings(const QualitySettings& settings);

private:
    // Core subsystems
    std::unique_ptr<CreatureUpdateScheduler> m_scheduler;
    std::unique_ptr<RenderingOptimizer> m_renderOptimizer;
    std::unique_ptr<MemoryOptimizer> m_memoryOptimizer;
    std::unique_ptr<QualityScaler> m_qualityScaler;
    std::unique_ptr<ProfilerReport> m_profiler;

    // Spatial partitioning
    std::unique_ptr<HierarchicalSpatialGrid> m_spatialGrid;

    // GPU compute
    std::unique_ptr<GPUBehaviorCompute> m_gpuBehaviors;
    DX12Device* m_dx12Device = nullptr;

    // Configuration
    bool m_initialized = false;
    bool m_gpuInitialized = false;
    float m_worldWidth = 500.0f;
    float m_worldDepth = 500.0f;
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline PerformanceSubsystems::PerformanceSubsystems()
    : m_scheduler(std::make_unique<CreatureUpdateScheduler>())
    , m_renderOptimizer(std::make_unique<RenderingOptimizer>())
    , m_memoryOptimizer(std::make_unique<MemoryOptimizer>())
    , m_qualityScaler(std::make_unique<QualityScaler>())
    , m_profiler(std::make_unique<ProfilerReport>())
    , m_gpuBehaviors(std::make_unique<GPUBehaviorCompute>()) {
}

inline PerformanceSubsystems::~PerformanceSubsystems() {
    shutdown();
}

inline bool PerformanceSubsystems::initialize(float worldWidth, float worldDepth) {
    m_worldWidth = worldWidth;
    m_worldDepth = worldDepth;

    // Initialize spatial grid
    HierarchicalGridConfig gridConfig;
    gridConfig.worldWidth = worldWidth;
    gridConfig.worldDepth = worldDepth;
    gridConfig.coarseGridSize = 8;
    gridConfig.fineGridSize = 32;
    m_spatialGrid = std::make_unique<HierarchicalSpatialGrid>(gridConfig);

    // Configure quality scaler
    QualityScalerConfig qualityConfig;
    qualityConfig.targetFPS = 60.0f;
    qualityConfig.autoAdjust = true;
    m_qualityScaler->setConfig(qualityConfig);

    // Configure profiler
    ProfilerConfig profilerConfig;
    profilerConfig.enableDetailedProfiling = true;
    profilerConfig.enableAlerts = true;
    m_profiler->setConfig(profilerConfig);

    m_initialized = true;
    return true;
}

inline bool PerformanceSubsystems::initializeGPU(DX12Device* device) {
    if (!device) return false;
    m_dx12Device = device;

    if (m_gpuBehaviors->initialize(device)) {
        m_gpuInitialized = true;
        return true;
    }
    return false;
}

inline void PerformanceSubsystems::shutdown() {
    if (m_gpuBehaviors) {
        m_gpuBehaviors->shutdown();
    }
    m_gpuInitialized = false;
    m_initialized = false;
}

inline void PerformanceSubsystems::beginFrame() {
    m_profiler->beginFrame();
    m_memoryOptimizer->beginFrame();
}

inline void PerformanceSubsystems::scheduleCreatureUpdates(
    const std::vector<Creature*>& creatures,
    const glm::vec3& cameraPosition,
    const glm::mat4& viewProjection,
    size_t selectedIndex) {

    ProfilerReport::ScopedSection section(*m_profiler, ProfileSection::UPDATE_CREATURES);
    m_scheduler->scheduleUpdates(creatures, cameraPosition, viewProjection, selectedIndex);
}

inline void PerformanceSubsystems::executeScheduledUpdates(float deltaTime) {
    m_scheduler->executeUpdates(deltaTime);
}

inline void PerformanceSubsystems::prepareRendering(
    const std::vector<Creature*>& creatures,
    const Frustum& frustum,
    const glm::vec3& cameraPosition,
    const glm::mat4& viewProjection,
    float screenWidth, float screenHeight) {

    ProfilerReport::ScopedSection section(*m_profiler, ProfileSection::RENDER_CULL);
    m_renderOptimizer->cullAndSort(creatures, frustum, cameraPosition, viewProjection,
                                    screenWidth, screenHeight);
    m_renderOptimizer->buildBatches();
}

inline void PerformanceSubsystems::updateQuality(float frameTimeMs) {
    m_qualityScaler->update(frameTimeMs);
}

inline void PerformanceSubsystems::endFrame() {
    m_profiler->endFrame();
    m_memoryOptimizer->endFrame();
}

inline void PerformanceSubsystems::dispatchGPUBehaviors(ID3D12GraphicsCommandList* cmdList,
                                                         float deltaTime) {
    if (!m_gpuInitialized) return;
    m_gpuBehaviors->dispatchAll(cmdList, deltaTime);
}

inline glm::vec3 PerformanceSubsystems::getComputedSteering(size_t creatureIndex) const {
    if (!m_gpuInitialized) return glm::vec3(0.0f);
    return m_gpuBehaviors->getSteeringForce(creatureIndex);
}

inline void PerformanceSubsystems::rebuildSpatialGrid(const std::vector<Creature*>& creatures) {
    ProfilerReport::ScopedSection section(*m_profiler, ProfileSection::SPATIAL_GRID);
    m_spatialGrid->rebuild(creatures);
}

inline const std::vector<Creature*>& PerformanceSubsystems::queryNearby(
    const glm::vec3& position, float radius) const {
    return m_spatialGrid->query(position, radius);
}

inline Creature* PerformanceSubsystems::findNearest(const glm::vec3& position,
                                                      float radius, int typeFilter) const {
    return m_spatialGrid->findNearest(position, radius, typeFilter);
}

inline MemoryArena& PerformanceSubsystems::getFrameArena() {
    return m_memoryOptimizer->getFrameArena();
}

inline void PerformanceSubsystems::defragment() {
    m_memoryOptimizer->defragment();
}

inline std::vector<std::string> PerformanceSubsystems::getOptimizationSuggestions() const {
    return m_profiler->getOptimizationSuggestions();
}

inline bool PerformanceSubsystems::shouldReduceQuality() const {
    const auto& stats = m_qualityScaler->getStats();
    return stats.averageFPS < 55.0f || stats.fps1PercentLow < 40.0f;
}

inline bool PerformanceSubsystems::shouldIncreaseQuality() const {
    const auto& stats = m_qualityScaler->getStats();
    return stats.averageFPS > 58.0f && stats.fps1PercentLow > 50.0f;
}

inline void PerformanceSubsystems::applyQualitySettings(const QualitySettings& settings) {
    // Apply LOD bias to scheduler
    UpdateSchedulerConfig schedConfig = m_scheduler->getConfig();
    schedConfig.criticalDistance *= settings.lodBias;
    schedConfig.highDistance *= settings.lodBias;
    schedConfig.mediumDistance *= settings.lodBias;
    schedConfig.lowDistance *= settings.lodBias;
    schedConfig.minimalDistance *= settings.lodBias;
    m_scheduler->setConfig(schedConfig);

    // Apply LOD bias to rendering
    RenderingConfig renderConfig = m_renderOptimizer->getConfig();
    renderConfig.qualityScale = settings.lodBias;
    m_renderOptimizer->setConfig(renderConfig);
}

} // namespace Forge
