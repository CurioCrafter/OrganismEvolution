#pragma once

#include <chrono>
#include <array>
#include <string>
#include <algorithm>

/**
 * @class PerformanceMetrics
 * @brief Comprehensive performance tracking and statistics.
 *
 * Tracks frame times, FPS, CPU/GPU timing, rendering statistics,
 * and provides data for ImGui visualization.
 *
 * Usage:
 * @code
 * PerformanceMetrics metrics;
 *
 * // Each frame:
 * metrics.beginFrame();
 *
 * metrics.beginPhase("Update");
 * // ... update logic
 * metrics.endPhase("Update");
 *
 * metrics.beginPhase("Render");
 * // ... render logic
 * metrics.endPhase("Render");
 *
 * metrics.endFrame();
 *
 * // Get stats
 * float fps = metrics.getFPS();
 * float updateTime = metrics.getPhaseTime("Update");
 * @endcode
 */
class PerformanceMetrics {
public:
    static constexpr size_t HISTORY_SIZE = 120;  // 2 seconds at 60 FPS
    static constexpr size_t MAX_PHASES = 16;

    PerformanceMetrics() {
        reset();
    }

    // Frame timing
    void beginFrame() {
        m_frameStart = std::chrono::high_resolution_clock::now();
    }

    void endFrame() {
        auto now = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::milli>(now - m_frameStart).count();

        // Update frame time history
        m_frameTimeHistory[m_historyIndex] = frameTime;
        m_historyIndex = (m_historyIndex + 1) % HISTORY_SIZE;

        // Update FPS
        m_frameCount++;
        m_fpsAccumulator += frameTime;
        if (m_fpsAccumulator >= 1000.0f) {
            m_currentFPS = m_frameCount * 1000.0f / m_fpsAccumulator;
            m_frameCount = 0;
            m_fpsAccumulator = 0.0f;
        }

        // Track min/max
        m_minFrameTime = std::min(m_minFrameTime, frameTime);
        m_maxFrameTime = std::max(m_maxFrameTime, frameTime);
    }

    // Phase timing (for measuring subsystems)
    void beginPhase(const std::string& name) {
        size_t idx = findOrCreatePhase(name);
        if (idx < MAX_PHASES) {
            m_phases[idx].start = std::chrono::high_resolution_clock::now();
        }
    }

    void endPhase(const std::string& name) {
        auto now = std::chrono::high_resolution_clock::now();
        size_t idx = findPhase(name);
        if (idx < MAX_PHASES) {
            float duration = std::chrono::duration<float, std::milli>(now - m_phases[idx].start).count();
            m_phases[idx].lastTime = duration;
            m_phases[idx].totalTime += duration;
            m_phases[idx].callCount++;
        }
    }

    float getPhaseTime(const std::string& name) const {
        size_t idx = findPhase(name);
        return (idx < MAX_PHASES) ? m_phases[idx].lastTime : 0.0f;
    }

    float getPhaseAverage(const std::string& name) const {
        size_t idx = findPhase(name);
        if (idx < MAX_PHASES && m_phases[idx].callCount > 0) {
            return m_phases[idx].totalTime / m_phases[idx].callCount;
        }
        return 0.0f;
    }

    // Rendering stats
    void setDrawCalls(int count) { m_drawCalls = count; }
    void setTriangleCount(int count) { m_triangleCount = count; }
    void setInstanceCount(int count) { m_instanceCount = count; }
    void setVisibleCreatures(int count) { m_visibleCreatures = count; }
    void setCulledCreatures(int count) { m_culledCreatures = count; }

    void addDrawCalls(int count) { m_drawCalls += count; }
    void addTriangles(int count) { m_triangleCount += count; }
    void addInstances(int count) { m_instanceCount += count; }

    // LOD stats
    void setLODCounts(int lod0, int lod1, int lod2) {
        m_lodCounts[0] = lod0;
        m_lodCounts[1] = lod1;
        m_lodCounts[2] = lod2;
    }

    // Memory stats
    void setInstanceBufferSize(size_t bytes) { m_instanceBufferBytes = bytes; }
    void setCreatureMemory(size_t bytes) { m_creatureMemoryBytes = bytes; }

    // Population stats
    void setCreatureCount(size_t total) { m_totalCreatures = total; }
    void setHerbivoreCount(int count) { m_herbivoreCount = count; }
    void setCarnivoreCount(int count) { m_carnivoreCount = count; }
    void setFoodCount(int count) { m_foodCount = count; }

    // Spatial grid stats
    void setSpatialGridStats(size_t totalCreatures, size_t maxOccupancy, size_t queryCount) {
        m_gridTotalCreatures = totalCreatures;
        m_gridMaxOccupancy = maxOccupancy;
        m_gridQueryCount = queryCount;
    }

    // Getters
    float getFPS() const { return m_currentFPS; }
    float getFrameTime() const { return 1000.0f / std::max(1.0f, m_currentFPS); }
    float getMinFrameTime() const { return m_minFrameTime; }
    float getMaxFrameTime() const { return m_maxFrameTime; }

    int getDrawCalls() const { return m_drawCalls; }
    int getTriangleCount() const { return m_triangleCount; }
    int getInstanceCount() const { return m_instanceCount; }
    int getVisibleCreatures() const { return m_visibleCreatures; }
    int getCulledCreatures() const { return m_culledCreatures; }

    float getCullRatio() const {
        int total = m_visibleCreatures + m_culledCreatures;
        return (total > 0) ? (100.0f * m_culledCreatures / total) : 0.0f;
    }

    const int* getLODCounts() const { return m_lodCounts; }

    size_t getInstanceBufferSize() const { return m_instanceBufferBytes; }
    size_t getCreatureMemory() const { return m_creatureMemoryBytes; }

    size_t getTotalCreatures() const { return m_totalCreatures; }
    int getHerbivoreCount() const { return m_herbivoreCount; }
    int getCarnivoreCount() const { return m_carnivoreCount; }
    int getFoodCount() const { return m_foodCount; }

    // History for graphing
    const float* getFrameTimeHistory() const { return m_frameTimeHistory.data(); }
    size_t getHistorySize() const { return HISTORY_SIZE; }
    size_t getCurrentHistoryIndex() const { return m_historyIndex; }

    // Reset all stats
    void reset() {
        m_frameCount = 0;
        m_fpsAccumulator = 0.0f;
        m_currentFPS = 0.0f;
        m_minFrameTime = 1000.0f;
        m_maxFrameTime = 0.0f;

        m_frameTimeHistory.fill(0.0f);
        m_historyIndex = 0;

        m_drawCalls = 0;
        m_triangleCount = 0;
        m_instanceCount = 0;
        m_visibleCreatures = 0;
        m_culledCreatures = 0;

        m_lodCounts[0] = m_lodCounts[1] = m_lodCounts[2] = 0;

        m_instanceBufferBytes = 0;
        m_creatureMemoryBytes = 0;

        m_totalCreatures = 0;
        m_herbivoreCount = 0;
        m_carnivoreCount = 0;
        m_foodCount = 0;

        m_gridTotalCreatures = 0;
        m_gridMaxOccupancy = 0;
        m_gridQueryCount = 0;

        m_phaseCount = 0;
        for (auto& phase : m_phases) {
            phase = Phase{};
        }
    }

    // Reset per-frame stats (call at beginning of frame)
    void resetFrameStats() {
        m_drawCalls = 0;
        m_triangleCount = 0;
        m_instanceCount = 0;
    }

    // ImGui rendering helper
    void renderImGui() const;

private:
    struct Phase {
        std::string name;
        std::chrono::high_resolution_clock::time_point start;
        float lastTime = 0.0f;
        float totalTime = 0.0f;
        int callCount = 0;
    };

    size_t findPhase(const std::string& name) const {
        for (size_t i = 0; i < m_phaseCount; ++i) {
            if (m_phases[i].name == name) return i;
        }
        return MAX_PHASES;  // Not found
    }

    size_t findOrCreatePhase(const std::string& name) {
        size_t idx = findPhase(name);
        if (idx < MAX_PHASES) return idx;

        if (m_phaseCount < MAX_PHASES) {
            m_phases[m_phaseCount].name = name;
            return m_phaseCount++;
        }
        return MAX_PHASES;  // Full
    }

    // Frame timing
    std::chrono::high_resolution_clock::time_point m_frameStart;
    int m_frameCount = 0;
    float m_fpsAccumulator = 0.0f;
    float m_currentFPS = 0.0f;
    float m_minFrameTime = 1000.0f;
    float m_maxFrameTime = 0.0f;

    // Frame time history
    std::array<float, HISTORY_SIZE> m_frameTimeHistory{};
    size_t m_historyIndex = 0;

    // Rendering stats
    int m_drawCalls = 0;
    int m_triangleCount = 0;
    int m_instanceCount = 0;
    int m_visibleCreatures = 0;
    int m_culledCreatures = 0;

    // LOD distribution
    int m_lodCounts[3] = {0, 0, 0};

    // Memory
    size_t m_instanceBufferBytes = 0;
    size_t m_creatureMemoryBytes = 0;

    // Population
    size_t m_totalCreatures = 0;
    int m_herbivoreCount = 0;
    int m_carnivoreCount = 0;
    int m_foodCount = 0;

    // Spatial grid
    size_t m_gridTotalCreatures = 0;
    size_t m_gridMaxOccupancy = 0;
    size_t m_gridQueryCount = 0;

    // Phases
    std::array<Phase, MAX_PHASES> m_phases;
    size_t m_phaseCount = 0;
};

/**
 * @class ScopedTimer
 * @brief RAII timer for automatic phase timing.
 *
 * Usage:
 * @code
 * void update(PerformanceMetrics& metrics) {
 *     ScopedTimer timer(metrics, "Update");
 *     // ... update logic
 * }  // Timer automatically ends phase when destroyed
 * @endcode
 */
class ScopedTimer {
public:
    ScopedTimer(PerformanceMetrics& metrics, const std::string& phaseName)
        : m_metrics(metrics), m_phaseName(phaseName) {
        m_metrics.beginPhase(m_phaseName);
    }

    ~ScopedTimer() {
        m_metrics.endPhase(m_phaseName);
    }

    // Non-copyable, non-movable
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
    PerformanceMetrics& m_metrics;
    std::string m_phaseName;
};

// Macro for convenient scoped timing
#define PERF_SCOPE(metrics, name) ScopedTimer _timer_##__LINE__(metrics, name)
