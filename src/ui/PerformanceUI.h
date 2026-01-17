#pragma once

// PerformanceUI - ImGui-based performance monitoring dashboard
// Displays FPS, frame timing, memory usage, and quality settings

#include <imgui.h>
#include <array>
#include <deque>
#include <string>

namespace Forge {

// Forward declarations
class PerformanceManager;
class QualityScaler;
class MemoryOptimizer;
class CreatureUpdateScheduler;
class RenderingOptimizer;
class CreatureManager;

// ============================================================================
// Graph Data
// ============================================================================

template<size_t MaxSamples = 120>
class GraphData {
public:
    void push(float value) {
        if (m_values.size() >= MaxSamples) {
            m_values.pop_front();
        }
        m_values.push_back(value);

        // Update min/max
        m_min = value;
        m_max = value;
        for (float v : m_values) {
            m_min = std::min(m_min, v);
            m_max = std::max(m_max, v);
        }
    }

    const float* data() const {
        // Copy to contiguous array for ImGui
        m_buffer.clear();
        m_buffer.reserve(m_values.size());
        for (float v : m_values) {
            m_buffer.push_back(v);
        }
        return m_buffer.data();
    }

    size_t size() const { return m_values.size(); }
    float min() const { return m_min; }
    float max() const { return m_max; }
    float last() const { return m_values.empty() ? 0.0f : m_values.back(); }

    float average() const {
        if (m_values.empty()) return 0.0f;
        float sum = 0.0f;
        for (float v : m_values) sum += v;
        return sum / m_values.size();
    }

private:
    std::deque<float> m_values;
    mutable std::vector<float> m_buffer;  // For contiguous access
    float m_min = 0.0f;
    float m_max = 0.0f;
};

// ============================================================================
// Performance UI Configuration
// ============================================================================

struct PerformanceUIConfig {
    bool showFPSGraph = true;
    bool showFrameTimeGraph = true;
    bool showMemoryGraph = true;
    bool showCreatureStats = true;
    bool showRenderingStats = true;
    bool showQualitySettings = true;
    bool showSystemBreakdown = true;
    bool showMinimalOverlay = false;  // Small corner overlay

    // Graph settings
    float graphHeight = 50.0f;
    float graphWidth = 200.0f;
    int graphHistoryFrames = 120;

    // Colors
    ImVec4 goodColor = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);    // Green
    ImVec4 warningColor = ImVec4(0.9f, 0.7f, 0.0f, 1.0f);  // Yellow
    ImVec4 badColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);      // Red
};

// ============================================================================
// Performance UI
// ============================================================================

class PerformanceUI {
public:
    PerformanceUI();
    ~PerformanceUI() = default;

    // Configuration
    void setConfig(const PerformanceUIConfig& config) { m_config = config; }
    const PerformanceUIConfig& getConfig() const { return m_config; }

    // ========================================================================
    // Main Render
    // ========================================================================

    // Render the full performance dashboard
    void render(const PerformanceManager* perfManager,
                QualityScaler* qualityScaler,
                const MemoryOptimizer* memOptimizer,
                const CreatureUpdateScheduler* scheduler,
                const RenderingOptimizer* renderOptimizer,
                const CreatureManager* creatureManager);

    // Render minimal overlay (small corner display)
    void renderOverlay(const PerformanceManager* perfManager);

    // ========================================================================
    // Visibility
    // ========================================================================

    void show() { m_visible = true; }
    void hide() { m_visible = false; }
    void toggle() { m_visible = !m_visible; }
    bool isVisible() const { return m_visible; }

    // ========================================================================
    // Data Recording
    // ========================================================================

    // Record frame data (call every frame)
    void recordFrame(float fps, float frameTimeMs, float updateMs, float renderMs);

private:
    PerformanceUIConfig m_config;
    bool m_visible = false;

    // Graph data
    GraphData<120> m_fpsGraph;
    GraphData<120> m_frameTimeGraph;
    GraphData<120> m_updateTimeGraph;
    GraphData<120> m_renderTimeGraph;
    GraphData<120> m_memoryGraph;

    // Section renderers
    void renderFPSSection(const PerformanceManager* perfManager);
    void renderFrameTimeSection(const PerformanceManager* perfManager);
    void renderCreatureSection(const PerformanceManager* perfManager,
                                const CreatureManager* creatureManager,
                                const CreatureUpdateScheduler* scheduler);
    void renderRenderingSection(const RenderingOptimizer* renderOptimizer,
                                 const PerformanceManager* perfManager);
    void renderMemorySection(const MemoryOptimizer* memOptimizer);
    void renderQualitySection(QualityScaler* qualityScaler);
    void renderSystemBreakdown(const PerformanceManager* perfManager);

    // Helpers
    ImVec4 getFPSColor(float fps) const;
    void drawPlotLines(const char* label, const GraphData<120>& data,
                       float height, const char* overlayText = nullptr);
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline PerformanceUI::PerformanceUI() {
    // Initialize with some default values
    for (int i = 0; i < 120; ++i) {
        m_fpsGraph.push(60.0f);
        m_frameTimeGraph.push(16.67f);
    }
}

inline void PerformanceUI::recordFrame(float fps, float frameTimeMs,
                                         float updateMs, float renderMs) {
    m_fpsGraph.push(fps);
    m_frameTimeGraph.push(frameTimeMs);
    m_updateTimeGraph.push(updateMs);
    m_renderTimeGraph.push(renderMs);
}

inline ImVec4 PerformanceUI::getFPSColor(float fps) const {
    if (fps >= 58.0f) return m_config.goodColor;
    if (fps >= 45.0f) return m_config.warningColor;
    return m_config.badColor;
}

inline void PerformanceUI::drawPlotLines(const char* label, const GraphData<120>& data,
                                          float height, const char* overlayText) {
    ImGui::PlotLines(label, data.data(), static_cast<int>(data.size()),
                     0, overlayText, data.min() * 0.9f, data.max() * 1.1f,
                     ImVec2(m_config.graphWidth, height));
}

} // namespace Forge
