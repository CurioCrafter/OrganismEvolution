#include "PerformancePanel.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace ui {
namespace statistics {

// ============================================================================
// Constructor
// ============================================================================

PerformancePanel::PerformancePanel() {
    m_fpsHistory.resize(MAX_HISTORY, 60.0f);
    m_frameTimeHistory.resize(MAX_HISTORY, 16.67f);
}

// ============================================================================
// Update
// ============================================================================

void PerformancePanel::update(const Forge::PerformanceManager* performance, float deltaTime) {
    m_updateTimer += deltaTime;

    if (m_updateTimer >= m_updateInterval) {
        m_updateTimer = 0.0f;

        float currentFPS = 0.0f;
        float frameTime = deltaTime * 1000.0f;

        if (performance) {
            const auto& stats = performance->getStats();
            currentFPS = stats.currentFPS;
            frameTime = stats.frameTime;
        } else {
            currentFPS = deltaTime > 0.0001f ? 1.0f / deltaTime : 0.0f;
        }

        // Add to history
        m_fpsHistory.push_back(currentFPS);
        while (m_fpsHistory.size() > MAX_HISTORY) {
            m_fpsHistory.pop_front();
        }

        m_frameTimeHistory.push_back(frameTime);
        while (m_frameTimeHistory.size() > MAX_HISTORY) {
            m_frameTimeHistory.pop_front();
        }

        // Update statistics
        m_frameCount++;
        m_fpsAccum += currentFPS;
        m_minFPS = std::min(m_minFPS, currentFPS);
        m_maxFPS = std::max(m_maxFPS, currentFPS);

        calculateStatistics();
    }
}

void PerformancePanel::calculateStatistics() {
    if (m_fpsHistory.empty()) return;

    // Average FPS
    float sum = 0.0f;
    for (float fps : m_fpsHistory) {
        sum += fps;
    }
    m_avgFPS = sum / static_cast<float>(m_fpsHistory.size());

    // 1% low (worst 1% of frames)
    std::vector<float> sorted(m_fpsHistory.begin(), m_fpsHistory.end());
    std::sort(sorted.begin(), sorted.end());

    size_t onePercentIdx = std::max(size_t(1), sorted.size() / 100);
    float onePercentSum = 0.0f;
    for (size_t i = 0; i < onePercentIdx; ++i) {
        onePercentSum += sorted[i];
    }
    m_onePercentLow = onePercentSum / static_cast<float>(onePercentIdx);
}

// ============================================================================
// Main Render
// ============================================================================

void PerformancePanel::render(const StatisticsDataManager& data,
                              const Forge::PerformanceManager* performance) {
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.12f, 1.0f));

    // FPS overview
    if (ImGui::CollapsingHeader("FPS & Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Current FPS display
        float currentFPS = data.getCurrentFPS();
        ImU32 fpsColor = getFPSColor(currentFPS);

        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(fpsColor));
        ImGui::Text("%.1f FPS", currentFPS);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(%s)", getPerformanceRating(currentFPS));

        // FPS graph
        if (m_showGraph) {
            renderFPSGraph(data);
        }

        // Statistics row
        ImGui::Columns(4, nullptr, false);
        ImGui::Text("Avg: %.1f", m_avgFPS);
        ImGui::NextColumn();
        ImGui::Text("Min: %.1f", m_minFPS);
        ImGui::NextColumn();
        ImGui::Text("Max: %.1f", m_maxFPS);
        ImGui::NextColumn();
        ImGui::Text("1%% Low: %.1f", m_onePercentLow);
        ImGui::Columns(1);
    }

    // Frame time breakdown
    if (performance && ImGui::CollapsingHeader("Frame Time Breakdown")) {
        renderFrameTimeBreakdown(performance);
    }

    // LOD distribution
    if (performance && ImGui::CollapsingHeader("LOD Distribution")) {
        renderLODDistribution(performance);
    }

    // Culling stats
    if (performance && ImGui::CollapsingHeader("Culling Statistics")) {
        renderCullingStats(performance);
    }

    // Memory usage
    if (ImGui::CollapsingHeader("Memory Usage")) {
        renderMemoryUsage(data);
    }

    ImGui::PopStyleColor();
}

void PerformancePanel::renderCompact(const StatisticsDataManager& data) {
    float fps = data.getCurrentFPS();
    ImU32 color = getFPSColor(fps);

    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(color));
    ImGui::Text("%.0f FPS", fps);
    ImGui::PopStyleColor();

    // Mini sparkline
    if (!m_fpsHistory.empty()) {
        std::vector<float> history(m_fpsHistory.begin(), m_fpsHistory.end());

        if (ImPlot::BeginPlot("##FPSMini", ImVec2(-1, 40),
                              ImPlotFlags_NoTitle | ImPlotFlags_NoLegend |
                              ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect)) {
            ImPlot::SetupAxes(nullptr, nullptr,
                              ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels,
                              ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, m_targetFPS * 1.2f);

            ImPlot::PushStyleColor(ImPlotCol_Line, ImGui::ColorConvertU32ToFloat4(color));
            ImPlot::PlotLine("##fps", history.data(), static_cast<int>(history.size()));
            ImPlot::PopStyleColor();

            // Target line
            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
            float target = m_targetFPS;
            ImPlot::PlotHLines("##target", &target, 1);
            ImPlot::PopStyleColor();

            ImPlot::EndPlot();
        }
    }
}

void PerformancePanel::renderFPSCounter(float fps) {
    ImU32 color = getFPSColor(fps);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(color));
    ImGui::Text("%.0f", fps);
    ImGui::PopStyleColor();
}

// ============================================================================
// Individual Components
// ============================================================================

void PerformancePanel::renderFPSGraph(const StatisticsDataManager& data) {
    const auto& fpsHistory = data.getFPSHistory();

    if (fpsHistory.empty()) return;

    std::vector<float> history(fpsHistory.begin(), fpsHistory.end());
    std::vector<float> indices(history.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = static_cast<float>(i);
    }

    if (ImPlot::BeginPlot("FPS History", ImVec2(-1, m_graphHeight))) {
        ImPlot::SetupAxes("Time", "FPS");
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, m_targetFPS * 1.5f, ImPlotCond_Always);

        // Target FPS line
        float target = m_targetFPS;
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.3f, 0.8f, 0.3f, 0.5f));
        ImPlot::PlotHLines("Target", &target, 1);
        ImPlot::PopStyleColor();

        // FPS line with color based on current value
        float currentFPS = history.empty() ? 0.0f : history.back();
        ImVec4 lineColor = ImGui::ColorConvertU32ToFloat4(getFPSColor(currentFPS));
        ImPlot::PushStyleColor(ImPlotCol_Line, lineColor);
        ImPlot::PlotLine("FPS", indices.data(), history.data(), static_cast<int>(history.size()));
        ImPlot::PopStyleColor();

        ImPlot::EndPlot();
    }
}

void PerformancePanel::renderFrameTimeBreakdown(const Forge::PerformanceManager* performance) {
    if (!performance) return;

    const auto& stats = performance->getStats();

    float total = stats.frameTime;
    float update = stats.updateTime;
    float render = stats.renderTime;
    float gpu = stats.gpuTime;
    float other = total - update - render - gpu;
    if (other < 0) other = 0;

    // Stacked bar showing breakdown
    ImGui::Text("Frame Time: %.2f ms", total);

    // Progress bar style breakdown
    float barWidth = ImGui::GetContentRegionAvail().x;

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
    ImGui::ProgressBar(update / total, ImVec2(barWidth * (update / total), 20), nullptr);
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 0);

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.9f, 0.6f, 0.2f, 1.0f));
    ImGui::ProgressBar(render / total, ImVec2(barWidth * (render / total), 20), nullptr);
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 0);

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.6f, 0.2f, 0.9f, 1.0f));
    ImGui::ProgressBar(gpu / total, ImVec2(barWidth * (gpu / total), 20), nullptr);
    ImGui::PopStyleColor();

    // Legend
    ImGui::Columns(4, nullptr, false);
    ImGui::TextColored(ImVec4(0.2f, 0.6f, 0.9f, 1.0f), "Update: %.2fms", update);
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.0f), "Render: %.2fms", render);
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(0.6f, 0.2f, 0.9f, 1.0f), "GPU: %.2fms", gpu);
    ImGui::NextColumn();
    ImGui::Text("Other: %.2fms", other);
    ImGui::Columns(1);

    // Frame budget indicator
    float budget = 1000.0f / m_targetFPS;
    float usage = (total / budget) * 100.0f;
    ImGui::Text("Frame Budget: %.0f%% (%.2f / %.2f ms)", usage, total, budget);
}

void PerformancePanel::renderLODDistribution(const Forge::PerformanceManager* performance) {
    if (!performance) return;

    const auto& stats = performance->getStats();
    const auto& lodCounts = stats.creaturesByLOD;

    const char* lodNames[] = {"Full", "Medium", "Low", "Billboard", "Culled"};
    ImVec4 lodColors[] = {
        ImVec4(0.2f, 0.8f, 0.2f, 1.0f),  // Full - green
        ImVec4(0.6f, 0.8f, 0.2f, 1.0f),  // Medium - yellow-green
        ImVec4(0.8f, 0.8f, 0.2f, 1.0f),  // Low - yellow
        ImVec4(0.8f, 0.6f, 0.2f, 1.0f),  // Billboard - orange
        ImVec4(0.5f, 0.5f, 0.5f, 1.0f),  // Culled - gray
    };

    int total = 0;
    for (int count : lodCounts) {
        total += count;
    }
    if (total == 0) total = 1;

    // Bar chart
    std::vector<float> values;
    std::vector<float> positions;
    for (int i = 0; i < 5; ++i) {
        values.push_back(static_cast<float>(lodCounts[i]));
        positions.push_back(static_cast<float>(i));
    }

    if (ImPlot::BeginPlot("LOD Distribution", ImVec2(-1, 120))) {
        ImPlot::SetupAxes("LOD Level", "Creatures");
        ImPlot::SetupAxisTicks(ImAxis_X1, positions.data(), 5, lodNames);

        for (int i = 0; i < 5; ++i) {
            float pos = static_cast<float>(i);
            float val = static_cast<float>(lodCounts[i]);
            ImPlot::PushStyleColor(ImPlotCol_Fill, lodColors[i]);
            ImPlot::PlotBars(lodNames[i], &pos, &val, 1, 0.8);
            ImPlot::PopStyleColor();
        }

        ImPlot::EndPlot();
    }

    // Summary row
    ImGui::Columns(5, nullptr, false);
    for (int i = 0; i < 5; ++i) {
        float pct = 100.0f * lodCounts[i] / total;
        ImGui::TextColored(lodColors[i], "%s: %d (%.0f%%)", lodNames[i], lodCounts[i], pct);
        ImGui::NextColumn();
    }
    ImGui::Columns(1);
}

void PerformancePanel::renderCullingStats(const Forge::PerformanceManager* performance) {
    if (!performance) return;

    const auto& stats = performance->getStats();

    ImGui::Text("Total Creatures: %d", stats.totalCreatures);
    ImGui::Text("Visible: %d", stats.visibleCreatures);

    // Culling breakdown
    int totalCulled = stats.culledByFrustum + stats.culledByDistance + stats.culledByOcclusion;
    float cullRate = stats.totalCreatures > 0 ?
        100.0f * totalCulled / stats.totalCreatures : 0.0f;

    ImGui::Text("Culling Rate: %.1f%%", cullRate);

    ImGui::Separator();

    // Culling method breakdown
    if (totalCulled > 0) {
        ImGui::Text("Culled by:");
        ImGui::BulletText("Frustum: %d", stats.culledByFrustum);
        ImGui::BulletText("Distance: %d", stats.culledByDistance);
        ImGui::BulletText("Occlusion: %d", stats.culledByOcclusion);
    }

    // Rendering stats
    ImGui::Separator();
    ImGui::Text("Draw Calls: %d", stats.drawCalls);
    ImGui::Text("Triangles: %d", stats.trianglesRendered);
    ImGui::Text("Instances: %d", stats.instancesRendered);
}

void PerformancePanel::renderMemoryUsage(const StatisticsDataManager& data) {
    size_t memUsage = data.getMemoryUsage();

    ImGui::Text("Memory Usage: %s", formatMemorySize(memUsage).c_str());

    // Memory bar
    // Assume 512MB budget
    size_t budget = 512 * 1024 * 1024;
    float usage = static_cast<float>(memUsage) / budget;
    usage = std::clamp(usage, 0.0f, 1.0f);

    ImU32 color;
    if (usage < 0.5f) color = IM_COL32(50, 220, 50, 255);
    else if (usage < 0.75f) color = IM_COL32(220, 220, 50, 255);
    else color = IM_COL32(220, 100, 50, 255);

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImGui::ColorConvertU32ToFloat4(color));
    ImGui::ProgressBar(usage, ImVec2(-1, 0), nullptr);
    ImGui::PopStyleColor();

    ImGui::Text("Budget: %s (%.0f%% used)", formatMemorySize(budget).c_str(), usage * 100.0f);
}

} // namespace statistics
} // namespace ui
