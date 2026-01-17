#pragma once

/**
 * @file PerformancePanel.h
 * @brief Performance metrics visualization panel
 *
 * Displays:
 * - FPS graph with history
 * - Frame time breakdown
 * - Draw call statistics
 * - Memory usage
 * - LOD distribution
 * - Creature culling statistics
 */

#include "imgui.h"
#include "implot.h"
#include "StatisticsDataManager.h"
#include "../../core/PerformanceManager.h"
#include <vector>
#include <deque>
#include <string>

namespace ui {
namespace statistics {

// ============================================================================
// Performance Panel Widget
// ============================================================================

/**
 * @class PerformancePanel
 * @brief Renders performance metrics and diagnostics
 */
class PerformancePanel {
public:
    PerformancePanel();
    ~PerformancePanel() = default;

    /**
     * @brief Update from performance manager
     */
    void update(const Forge::PerformanceManager* performance, float deltaTime);

    /**
     * @brief Render full performance panel
     */
    void render(const StatisticsDataManager& data,
               const Forge::PerformanceManager* performance = nullptr);

    /**
     * @brief Render compact FPS overlay
     */
    void renderCompact(const StatisticsDataManager& data);

    /**
     * @brief Render minimal FPS counter
     */
    void renderFPSCounter(float fps);

    // ========================================================================
    // Individual Components
    // ========================================================================

    /**
     * @brief Render FPS graph
     */
    void renderFPSGraph(const StatisticsDataManager& data);

    /**
     * @brief Render frame time breakdown
     */
    void renderFrameTimeBreakdown(const Forge::PerformanceManager* performance);

    /**
     * @brief Render LOD distribution chart
     */
    void renderLODDistribution(const Forge::PerformanceManager* performance);

    /**
     * @brief Render memory usage
     */
    void renderMemoryUsage(const StatisticsDataManager& data);

    /**
     * @brief Render culling statistics
     */
    void renderCullingStats(const Forge::PerformanceManager* performance);

    // ========================================================================
    // Configuration
    // ========================================================================

    void setTargetFPS(float fps) { m_targetFPS = fps; }
    void setShowGraph(bool show) { m_showGraph = show; }
    void setGraphHeight(float height) { m_graphHeight = height; }

private:
    // Configuration
    float m_targetFPS = 60.0f;
    bool m_showGraph = true;
    float m_graphHeight = 100.0f;

    // Internal tracking (when PerformanceManager not available)
    std::deque<float> m_fpsHistory;
    std::deque<float> m_frameTimeHistory;
    static constexpr size_t MAX_HISTORY = 300;

    float m_updateTimer = 0.0f;
    float m_updateInterval = 0.1f;

    // Statistics
    float m_minFPS = 999.0f;
    float m_maxFPS = 0.0f;
    float m_avgFPS = 0.0f;
    int m_frameCount = 0;
    float m_fpsAccum = 0.0f;
    float m_onePercentLow = 0.0f;

    // Helper methods
    void calculateStatistics();
    ImU32 getFPSColor(float fps) const;
    const char* getPerformanceRating(float fps) const;
    std::string formatMemorySize(size_t bytes) const;
};

// ============================================================================
// Inline Implementation
// ============================================================================

inline ImU32 PerformancePanel::getFPSColor(float fps) const {
    float ratio = fps / m_targetFPS;
    if (ratio >= 0.95f) return IM_COL32(50, 220, 50, 255);   // Green
    if (ratio >= 0.8f) return IM_COL32(150, 220, 50, 255);   // Light green
    if (ratio >= 0.6f) return IM_COL32(220, 220, 50, 255);   // Yellow
    if (ratio >= 0.4f) return IM_COL32(220, 150, 50, 255);   // Orange
    return IM_COL32(220, 50, 50, 255);                       // Red
}

inline const char* PerformancePanel::getPerformanceRating(float fps) const {
    float ratio = fps / m_targetFPS;
    if (ratio >= 0.95f) return "Excellent";
    if (ratio >= 0.8f) return "Good";
    if (ratio >= 0.6f) return "Fair";
    if (ratio >= 0.4f) return "Poor";
    return "Critical";
}

inline std::string PerformancePanel::formatMemorySize(size_t bytes) const {
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
    if (bytes < 1024 * 1024 * 1024) return std::to_string(bytes / (1024 * 1024)) + " MB";
    return std::to_string(bytes / (1024 * 1024 * 1024)) + " GB";
}

} // namespace statistics
} // namespace ui
