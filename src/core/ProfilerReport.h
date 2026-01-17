#pragma once

// ProfilerReport - Comprehensive performance profiling and reporting
// Generates detailed reports of all simulation subsystems for optimization

#include <string>
#include <vector>
#include <array>
#include <chrono>
#include <map>
#include <functional>

namespace Forge {

// Forward declarations
class SimulationOrchestrator;
class PerformanceManager;
class CreatureManager;
class CreatureUpdateScheduler;
class RenderingOptimizer;
class MemoryOptimizer;
class QualityScaler;

// ============================================================================
// Profiler Section IDs (for fast lookup)
// ============================================================================

enum class ProfileSection : int {
    FRAME_TOTAL,
    UPDATE_TOTAL,
    RENDER_TOTAL,

    // Update subsections
    UPDATE_CREATURES,
    UPDATE_BEHAVIORS,
    UPDATE_FLOCKING,
    UPDATE_PREDATOR_PREY,
    UPDATE_FOOD_SEEKING,
    UPDATE_PHYSICS,
    UPDATE_REPRODUCTION,

    // Environment
    UPDATE_WEATHER,
    UPDATE_CLIMATE,
    UPDATE_VEGETATION,
    UPDATE_ECOSYSTEM,

    // Rendering subsections
    RENDER_CULL,
    RENDER_SORT,
    RENDER_BATCH,
    RENDER_CREATURES,
    RENDER_TERRAIN,
    RENDER_VEGETATION,
    RENDER_WATER,
    RENDER_PARTICLES,
    RENDER_SKY,
    RENDER_POST_PROCESS,
    RENDER_UI,

    // Other
    AUDIO,
    SPATIAL_GRID,
    GPU_COMPUTE,

    COUNT
};

// ============================================================================
// Section Timing Data
// ============================================================================

struct SectionTiming {
    float currentMs = 0.0f;
    float avgMs = 0.0f;
    float minMs = FLT_MAX;
    float maxMs = 0.0f;
    int sampleCount = 0;

    // History for graphs
    static constexpr int HISTORY_SIZE = 120;
    float history[HISTORY_SIZE] = {0.0f};
    int historyIndex = 0;

    void record(float ms) {
        currentMs = ms;
        avgMs = avgMs * 0.95f + ms * 0.05f;  // EMA
        minMs = std::min(minMs, ms);
        maxMs = std::max(maxMs, ms);
        sampleCount++;

        history[historyIndex] = ms;
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    }

    void reset() {
        currentMs = avgMs = 0.0f;
        minMs = FLT_MAX;
        maxMs = 0.0f;
        sampleCount = 0;
        for (float& h : history) h = 0.0f;
        historyIndex = 0;
    }

    float getPercentile(float pct) const {
        // Simple percentile from history
        std::vector<float> sorted(history, history + HISTORY_SIZE);
        std::sort(sorted.begin(), sorted.end());
        int idx = static_cast<int>(pct * HISTORY_SIZE);
        return sorted[std::min(idx, HISTORY_SIZE - 1)];
    }
};

// ============================================================================
// Frame Report
// ============================================================================

struct FrameReport {
    float totalMs = 0.0f;
    float fps = 0.0f;
    int frameNumber = 0;

    // Subsystem times
    std::array<float, static_cast<size_t>(ProfileSection::COUNT)> sectionTimes{};

    // Creature stats
    int totalCreatures = 0;
    int visibleCreatures = 0;
    int updatedCreatures = 0;

    // Render stats
    int drawCalls = 0;
    int triangles = 0;
    int instances = 0;

    // Memory stats
    size_t cpuMemory = 0;
    size_t gpuMemory = 0;
};

// ============================================================================
// Performance Alert
// ============================================================================

struct PerformanceAlert {
    enum class Severity { INFO, WARNING, CRITICAL };
    enum class Type {
        LOW_FPS,
        HIGH_FRAME_TIME,
        MEMORY_PRESSURE,
        GPU_BOUND,
        CPU_BOUND,
        MANY_DRAW_CALLS,
        SPATIAL_GRID_OVERFLOW,
        CREATURE_LIMIT
    };

    Severity severity = Severity::INFO;
    Type type = Type::LOW_FPS;
    std::string message;
    float value = 0.0f;
    float threshold = 0.0f;
    int frameNumber = 0;
};

// ============================================================================
// Profiler Configuration
// ============================================================================

struct ProfilerConfig {
    // Targets
    float targetFPS = 60.0f;
    float frameTimeBudgetMs = 16.67f;

    // Thresholds for alerts
    float lowFPSThreshold = 55.0f;
    float criticalFPSThreshold = 30.0f;
    float highFrameTimeMs = 20.0f;
    int maxDrawCallsWarning = 500;
    int maxDrawCallsCritical = 1000;
    size_t memoryWarningMB = 1024;

    // Profiling options
    bool enableDetailedProfiling = true;
    bool enableAlerts = true;
    bool logToFile = false;
    std::string logFilePath = "perf_log.csv";
};

// ============================================================================
// ProfilerReport
// ============================================================================

class ProfilerReport {
public:
    ProfilerReport();
    ~ProfilerReport();

    // Configuration
    void setConfig(const ProfilerConfig& config) { m_config = config; }
    const ProfilerConfig& getConfig() const { return m_config; }

    // ========================================================================
    // Profiling Interface
    // ========================================================================

    // Begin/end frame profiling
    void beginFrame();
    void endFrame();

    // Begin/end section profiling
    void beginSection(ProfileSection section);
    void endSection(ProfileSection section);

    // RAII section profiler
    class ScopedSection {
    public:
        ScopedSection(ProfilerReport& profiler, ProfileSection section)
            : m_profiler(profiler), m_section(section) {
            m_profiler.beginSection(m_section);
        }
        ~ScopedSection() {
            m_profiler.endSection(m_section);
        }
    private:
        ProfilerReport& m_profiler;
        ProfileSection m_section;
    };

    // ========================================================================
    // Report Generation
    // ========================================================================

    // Generate comprehensive report from all systems
    void generateReport(const SimulationOrchestrator* orchestrator,
                        const PerformanceManager* perfManager,
                        const CreatureManager* creatureManager,
                        const CreatureUpdateScheduler* scheduler,
                        const RenderingOptimizer* renderOptimizer,
                        const MemoryOptimizer* memOptimizer,
                        const QualityScaler* qualityScaler);

    // Get current frame report
    const FrameReport& getCurrentReport() const { return m_currentReport; }

    // Get section timing
    const SectionTiming& getSectionTiming(ProfileSection section) const;

    // Get section name
    static const char* getSectionName(ProfileSection section);

    // ========================================================================
    // Alerts
    // ========================================================================

    // Get active alerts
    const std::vector<PerformanceAlert>& getAlerts() const { return m_alerts; }

    // Clear alerts
    void clearAlerts() { m_alerts.clear(); }

    // Check specific conditions
    bool hasLowFPS() const;
    bool isGPUBound() const;
    bool isCPUBound() const;

    // ========================================================================
    // Analysis
    // ========================================================================

    // Get bottleneck analysis
    std::string analyzeBottlenecks() const;

    // Get optimization suggestions
    std::vector<std::string> getOptimizationSuggestions() const;

    // ========================================================================
    // Export
    // ========================================================================

    // Export report to string
    std::string exportToString() const;

    // Export to CSV line
    std::string exportToCSV() const;

    // ========================================================================
    // Statistics
    // ========================================================================

    float getAverageFPS() const { return m_avgFPS; }
    float get1PercentLowFPS() const;
    int getTotalFrames() const { return m_frameCount; }

private:
    ProfilerConfig m_config;

    // Section timings
    std::array<SectionTiming, static_cast<size_t>(ProfileSection::COUNT)> m_sections;
    std::array<std::chrono::high_resolution_clock::time_point, static_cast<size_t>(ProfileSection::COUNT)> m_sectionStarts;

    // Frame timing
    std::chrono::high_resolution_clock::time_point m_frameStart;
    int m_frameCount = 0;
    float m_avgFPS = 60.0f;

    // Current report
    FrameReport m_currentReport;

    // Alert history
    std::vector<PerformanceAlert> m_alerts;

    // Helper methods
    void checkAlerts();
    void addAlert(PerformanceAlert::Severity severity,
                  PerformanceAlert::Type type,
                  const std::string& message,
                  float value, float threshold);
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline ProfilerReport::ProfilerReport() {
    m_alerts.reserve(100);
}

inline ProfilerReport::~ProfilerReport() = default;

inline void ProfilerReport::beginFrame() {
    m_frameStart = std::chrono::high_resolution_clock::now();
    m_frameCount++;
    m_currentReport = FrameReport{};
    m_currentReport.frameNumber = m_frameCount;
}

inline void ProfilerReport::endFrame() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - m_frameStart);

    float frameMs = duration.count() / 1000.0f;
    m_currentReport.totalMs = frameMs;
    m_currentReport.fps = (frameMs > 0.001f) ? 1000.0f / frameMs : 0.0f;

    m_sections[static_cast<size_t>(ProfileSection::FRAME_TOTAL)].record(frameMs);
    m_avgFPS = m_avgFPS * 0.95f + m_currentReport.fps * 0.05f;

    if (m_config.enableAlerts) {
        checkAlerts();
    }
}

inline void ProfilerReport::beginSection(ProfileSection section) {
    m_sectionStarts[static_cast<size_t>(section)] = std::chrono::high_resolution_clock::now();
}

inline void ProfilerReport::endSection(ProfileSection section) {
    auto now = std::chrono::high_resolution_clock::now();
    auto start = m_sectionStarts[static_cast<size_t>(section)];
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start);

    float ms = duration.count() / 1000.0f;
    m_sections[static_cast<size_t>(section)].record(ms);
    m_currentReport.sectionTimes[static_cast<size_t>(section)] = ms;
}

inline const SectionTiming& ProfilerReport::getSectionTiming(ProfileSection section) const {
    return m_sections[static_cast<size_t>(section)];
}

inline const char* ProfilerReport::getSectionName(ProfileSection section) {
    static const char* names[] = {
        "Frame Total",
        "Update Total",
        "Render Total",
        "Creature Update",
        "Behaviors",
        "Flocking",
        "Predator/Prey",
        "Food Seeking",
        "Physics",
        "Reproduction",
        "Weather",
        "Climate",
        "Vegetation",
        "Ecosystem",
        "Render Cull",
        "Render Sort",
        "Render Batch",
        "Render Creatures",
        "Render Terrain",
        "Render Vegetation",
        "Render Water",
        "Render Particles",
        "Render Sky",
        "Post Process",
        "Render UI",
        "Audio",
        "Spatial Grid",
        "GPU Compute"
    };
    return names[static_cast<size_t>(section)];
}

inline bool ProfilerReport::hasLowFPS() const {
    return m_avgFPS < m_config.lowFPSThreshold;
}

inline void ProfilerReport::checkAlerts() {
    // Check FPS
    if (m_currentReport.fps < m_config.criticalFPSThreshold) {
        addAlert(PerformanceAlert::Severity::CRITICAL,
                 PerformanceAlert::Type::LOW_FPS,
                 "Critical FPS drop",
                 m_currentReport.fps, m_config.criticalFPSThreshold);
    } else if (m_currentReport.fps < m_config.lowFPSThreshold) {
        addAlert(PerformanceAlert::Severity::WARNING,
                 PerformanceAlert::Type::LOW_FPS,
                 "Low FPS",
                 m_currentReport.fps, m_config.lowFPSThreshold);
    }

    // Check frame time
    if (m_currentReport.totalMs > m_config.highFrameTimeMs) {
        addAlert(PerformanceAlert::Severity::WARNING,
                 PerformanceAlert::Type::HIGH_FRAME_TIME,
                 "High frame time",
                 m_currentReport.totalMs, m_config.highFrameTimeMs);
    }

    // Limit alerts to last N
    if (m_alerts.size() > 100) {
        m_alerts.erase(m_alerts.begin(), m_alerts.begin() + 50);
    }
}

inline void ProfilerReport::addAlert(PerformanceAlert::Severity severity,
                                      PerformanceAlert::Type type,
                                      const std::string& message,
                                      float value, float threshold) {
    m_alerts.push_back({severity, type, message, value, threshold, m_frameCount});
}

inline float ProfilerReport::get1PercentLowFPS() const {
    return m_sections[static_cast<size_t>(ProfileSection::FRAME_TOTAL)].getPercentile(0.01f);
}

} // namespace Forge
