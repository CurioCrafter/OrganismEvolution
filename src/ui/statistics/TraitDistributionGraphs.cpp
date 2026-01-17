#include "TraitDistributionGraphs.h"
#include <cmath>
#include <algorithm>

namespace ui {
namespace statistics {

// ============================================================================
// Main Render
// ============================================================================

void TraitDistributionGraphs::render(const StatisticsDataManager& data) {
    const auto& traits = data.getTraitDistributions();

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.12f, 1.0f));

    if (ImGui::CollapsingHeader("Trait Distributions", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderAllHistograms(traits);
    }

    if (ImGui::CollapsingHeader("Trait Correlations")) {
        renderCorrelationHeatmap(traits);

        ImGui::Separator();

        // Trait pair selection for scatter plot
        ImGui::Text("Select traits for scatter plot:");
        ImGui::Combo("X Axis", &m_selectedTraitX, TRAIT_NAMES, 8);
        ImGui::SameLine();
        ImGui::Combo("Y Axis", &m_selectedTraitY, TRAIT_NAMES, 8);

        const auto* traitX = getTraitByIndex(traits, m_selectedTraitX);
        const auto* traitY = getTraitByIndex(traits, m_selectedTraitY);
        if (traitX && traitY) {
            renderTraitScatter(*traitX, *traitY,
                              TRAIT_NAMES[m_selectedTraitX],
                              TRAIT_NAMES[m_selectedTraitY]);
        }
    }

    if (ImGui::CollapsingHeader("Statistical Summary")) {
        renderStatsSummary(traits);
    }

    if (ImGui::CollapsingHeader("Genetic Diversity")) {
        const auto& fitness = data.getCurrentFitness();
        renderDiversityGauge(fitness.geneticDiversity);

        ImGui::Separator();
        renderTraitEvolution(data);
    }

    ImGui::PopStyleColor();
}

void TraitDistributionGraphs::renderCompact(const StatisticsDataManager& data) {
    const auto& traits = data.getTraitDistributions();
    const auto& fitness = data.getCurrentFitness();

    ImGui::Text("Genetic Diversity: %.1f%%", fitness.geneticDiversity * 100.0f);
    renderDiversityGauge(fitness.geneticDiversity);

    // Mini histogram for size trait
    if (!traits.size.histogram.empty()) {
        std::vector<float> hist(traits.size.histogram.begin(), traits.size.histogram.end());
        float maxVal = *std::max_element(hist.begin(), hist.end());
        if (maxVal > 0) {
            for (auto& v : hist) v /= maxVal;
        }

        if (ImPlot::BeginPlot("##SizeDist", ImVec2(-1, 60),
                              ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoMenus)) {
            ImPlot::SetupAxes(nullptr, nullptr,
                              ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels,
                              ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);
            ImPlot::PushStyleColor(ImPlotCol_Fill, TRAIT_COLORS[0]);
            ImPlot::PlotBars("##Size", hist.data(), static_cast<int>(hist.size()), 0.8);
            ImPlot::PopStyleColor();
            ImPlot::EndPlot();
        }
        ImGui::Text("Size: %.2f +/- %.2f", traits.size.mean, traits.size.stdDev);
    }
}

// ============================================================================
// Histogram Rendering
// ============================================================================

void TraitDistributionGraphs::renderTraitHistogram(const TraitStatistics& trait,
                                                    const char* name,
                                                    const char* unit,
                                                    const ImVec4& color) {
    if (trait.histogram.empty() || trait.samples.empty()) {
        ImGui::TextDisabled("%s: No data", name);
        return;
    }

    // Convert histogram to float
    std::vector<float> histData(trait.histogram.begin(), trait.histogram.end());

    char plotId[64];
    snprintf(plotId, sizeof(plotId), "%s Distribution", name);

    if (ImPlot::BeginPlot(plotId, ImVec2(-1, m_histogramHeight))) {
        ImPlot::SetupAxes(unit, "Count");

        // Setup axis limits based on trait range
        float minX = trait.min - (trait.max - trait.min) * 0.1f;
        float maxX = trait.max + (trait.max - trait.min) * 0.1f;
        ImPlot::SetupAxisLimits(ImAxis_X1, minX, maxX);

        // Calculate bin width
        float binWidth = (maxX - minX) / static_cast<float>(histData.size());

        // Create bin positions
        std::vector<float> binPos(histData.size());
        for (size_t i = 0; i < histData.size(); ++i) {
            binPos[i] = minX + binWidth * (i + 0.5f);
        }

        // Render histogram bars
        ImPlot::PushStyleColor(ImPlotCol_Fill, color);
        ImPlot::SetNextFillStyle(ImVec4(color.x, color.y, color.z, 0.7f));
        ImPlot::PlotBars(name, binPos.data(), histData.data(),
                        static_cast<int>(histData.size()), binWidth * 0.9);
        ImPlot::PopStyleColor();

        // Render normal distribution overlay
        if (m_showNormalOverlay && trait.stdDev > 0.001f) {
            renderNormalCurve(trait.mean, trait.stdDev, minX, maxX, 100,
                             ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        }

        // Add mean line
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 1.0f, 1.0f, 0.8f));
        ImPlot::PlotVLines("Mean", &trait.mean, 1);
        ImPlot::PopStyleColor();

        // Annotation for mean
        float maxHist = *std::max_element(histData.begin(), histData.end());
        ImPlot::Annotation(trait.mean, maxHist * 0.9f, ImVec4(1,1,1,1),
                          ImVec2(5, 0), false, "Mean: %.2f", trait.mean);

        ImPlot::EndPlot();
    }

    // Stats below histogram
    ImGui::TextColored(color, "%s: ", name);
    ImGui::SameLine();
    ImGui::Text("Mean=%.2f, StdDev=%.2f, Range=[%.2f, %.2f]",
                trait.mean, trait.stdDev, trait.min, trait.max);
}

void TraitDistributionGraphs::renderAllHistograms(const TraitDistributions& traits) {
    // Render in 2-column grid
    ImGui::Columns(2, nullptr, false);

    float savedHeight = m_histogramHeight;
    m_histogramHeight = 120.0f;

    renderTraitHistogram(traits.size, TRAIT_NAMES[0], TRAIT_UNITS[0], TRAIT_COLORS[0]);
    ImGui::NextColumn();
    renderTraitHistogram(traits.speed, TRAIT_NAMES[1], TRAIT_UNITS[1], TRAIT_COLORS[1]);
    ImGui::NextColumn();

    renderTraitHistogram(traits.visionRange, TRAIT_NAMES[2], TRAIT_UNITS[2], TRAIT_COLORS[2]);
    ImGui::NextColumn();
    renderTraitHistogram(traits.efficiency, TRAIT_NAMES[3], TRAIT_UNITS[3], TRAIT_COLORS[3]);
    ImGui::NextColumn();

    renderTraitHistogram(traits.aggression, TRAIT_NAMES[4], TRAIT_UNITS[4], TRAIT_COLORS[4]);
    ImGui::NextColumn();
    renderTraitHistogram(traits.reproductionRate, TRAIT_NAMES[5], TRAIT_UNITS[5], TRAIT_COLORS[5]);
    ImGui::NextColumn();

    renderTraitHistogram(traits.lifespan, TRAIT_NAMES[6], TRAIT_UNITS[6], TRAIT_COLORS[6]);
    ImGui::NextColumn();
    renderTraitHistogram(traits.mutationRate, TRAIT_NAMES[7], TRAIT_UNITS[7], TRAIT_COLORS[7]);

    ImGui::Columns(1);
    m_histogramHeight = savedHeight;
}

void TraitDistributionGraphs::renderNormalCurve(float mean, float stdDev,
                                                 float minX, float maxX,
                                                 int samples, const ImVec4& color) {
    if (stdDev < 0.001f) return;

    std::vector<float> xs(samples);
    std::vector<float> ys(samples);

    float step = (maxX - minX) / static_cast<float>(samples - 1);
    float scale = 1.0f / (stdDev * std::sqrt(2.0f * 3.14159265f));

    for (int i = 0; i < samples; ++i) {
        float x = minX + i * step;
        float z = (x - mean) / stdDev;
        float y = scale * std::exp(-0.5f * z * z);
        xs[i] = x;
        ys[i] = y;
    }

    // Scale to match histogram
    float maxY = *std::max_element(ys.begin(), ys.end());
    if (maxY > 0) {
        for (auto& y : ys) {
            y = y / maxY * 0.8f;  // Scale to 80% of plot height
        }
    }

    ImPlot::PushStyleColor(ImPlotCol_Line, color);
    ImPlot::GetStyle().LineWeight = 2.0f;
    ImPlot::PlotLine("Normal", xs.data(), ys.data(), samples);
    ImPlot::GetStyle().LineWeight = 1.0f;
    ImPlot::PopStyleColor();
}

// ============================================================================
// Correlation Heatmap
// ============================================================================

void TraitDistributionGraphs::renderCorrelationHeatmap(const TraitDistributions& traits) {
    // Convert correlations to float array for heatmap
    static float correlationData[8 * 8];

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            correlationData[i * 8 + j] = traits.correlations[i][j];
        }
    }

    static const char* traitLabels[] = {"Size", "Spd", "Vis", "Eff", "Agg", "Rep", "Life", "Mut"};

    if (ImPlot::BeginPlot("Trait Correlation Matrix", ImVec2(-1, 250),
                          ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText)) {
        ImPlot::SetupAxes(nullptr, nullptr,
                          ImPlotAxisFlags_NoGridLines,
                          ImPlotAxisFlags_NoGridLines);
        ImPlot::SetupAxisTicks(ImAxis_X1, 0.5, 7.5, 8, traitLabels);
        ImPlot::SetupAxisTicks(ImAxis_Y1, 0.5, 7.5, 8, traitLabels);

        ImPlot::PlotHeatmap("##Correlations", correlationData, 8, 8, -1.0, 1.0,
                           "%.2f", ImPlotPoint(0, 0), ImPlotPoint(8, 8));

        ImPlot::EndPlot();
    }

    // Color legend
    ImGui::Text("Correlation: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.2f, 0.2f, 1.0f, 1.0f), "-1 (Negative)");
    ImGui::SameLine();
    ImGui::Text(" to ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "+1 (Positive)");

    // Notable correlations
    ImGui::Separator();
    ImGui::Text("Notable Correlations:");

    std::vector<std::tuple<int, int, float>> notable;
    for (int i = 0; i < 8; ++i) {
        for (int j = i + 1; j < 8; ++j) {
            float corr = traits.correlations[i][j];
            if (std::abs(corr) > 0.3f) {
                notable.emplace_back(i, j, corr);
            }
        }
    }

    // Sort by absolute value
    std::sort(notable.begin(), notable.end(),
              [](const auto& a, const auto& b) {
                  return std::abs(std::get<2>(a)) > std::abs(std::get<2>(b));
              });

    // Show top 5
    int shown = 0;
    for (const auto& [i, j, corr] : notable) {
        if (shown++ >= 5) break;

        ImVec4 color = getCorrelationColor(corr);
        const char* relationship = corr > 0 ? "positively" : "negatively";
        ImGui::TextColored(color, "  %s & %s: %.2f (%s correlated)",
                          TRAIT_NAMES[i], TRAIT_NAMES[j], corr, relationship);
    }

    if (notable.empty()) {
        ImGui::TextDisabled("  No strong correlations detected");
    }
}

// ============================================================================
// Scatter Plot
// ============================================================================

void TraitDistributionGraphs::renderTraitScatter(const TraitStatistics& traitX,
                                                  const TraitStatistics& traitY,
                                                  const char* nameX,
                                                  const char* nameY) {
    if (traitX.samples.empty() || traitY.samples.empty()) {
        ImGui::TextDisabled("Not enough data for scatter plot");
        return;
    }

    size_t n = std::min(traitX.samples.size(), traitY.samples.size());

    // Limit to first 1000 points for performance
    n = std::min(n, size_t(1000));

    char plotTitle[128];
    snprintf(plotTitle, sizeof(plotTitle), "%s vs %s", nameX, nameY);

    if (ImPlot::BeginPlot(plotTitle, ImVec2(-1, 250))) {
        ImPlot::SetupAxes(nameX, nameY);

        // Plot points with transparency
        ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(0.337f, 0.706f, 0.914f, 0.5f));
        ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(0.337f, 0.706f, 0.914f, 0.8f));
        ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 4.0f);

        ImPlot::PlotScatter("Creatures", traitX.samples.data(), traitY.samples.data(),
                           static_cast<int>(n));

        ImPlot::PopStyleVar();
        ImPlot::PopStyleColor(2);

        // Add trend line if significant correlation
        int idxX = -1, idxY = -1;
        for (int i = 0; i < 8; ++i) {
            if (strcmp(nameX, TRAIT_NAMES[i]) == 0) idxX = i;
            if (strcmp(nameY, TRAIT_NAMES[i]) == 0) idxY = i;
        }

        // Calculate linear regression
        float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
        for (size_t i = 0; i < n; ++i) {
            float x = traitX.samples[i];
            float y = traitY.samples[i];
            sumX += x;
            sumY += y;
            sumXY += x * y;
            sumX2 += x * x;
        }

        float meanX = sumX / n;
        float meanY = sumY / n;
        float denom = sumX2 - n * meanX * meanX;

        if (std::abs(denom) > 0.001f) {
            float slope = (sumXY - n * meanX * meanY) / denom;
            float intercept = meanY - slope * meanX;

            // Draw trend line
            float x1 = traitX.min;
            float x2 = traitX.max;
            float y1 = slope * x1 + intercept;
            float y2 = slope * x2 + intercept;

            float xs[] = {x1, x2};
            float ys[] = {y1, y2};

            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 0.4f, 0.4f, 0.8f));
            ImPlot::GetStyle().LineWeight = 2.0f;
            ImPlot::PlotLine("Trend", xs, ys, 2);
            ImPlot::GetStyle().LineWeight = 1.0f;
            ImPlot::PopStyleColor();
        }

        ImPlot::EndPlot();
    }
}

// ============================================================================
// Stats Summary
// ============================================================================

void TraitDistributionGraphs::renderStatsSummary(const TraitDistributions& traits) {
    if (ImGui::BeginTable("TraitStats", 7, ImGuiTableFlags_Borders |
                                           ImGuiTableFlags_RowBg |
                                           ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Trait");
        ImGui::TableSetupColumn("Mean");
        ImGui::TableSetupColumn("Std Dev");
        ImGui::TableSetupColumn("Min");
        ImGui::TableSetupColumn("Max");
        ImGui::TableSetupColumn("Median");
        ImGui::TableSetupColumn("Skewness");
        ImGui::TableHeadersRow();

        auto addRow = [](const TraitStatistics& t, const char* name, const ImVec4& color) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(color, "%s", name);
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", t.mean);
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", t.stdDev);
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", t.min);
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", t.max);
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", t.median);
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", t.skewness);
        };

        addRow(traits.size, TRAIT_NAMES[0], TRAIT_COLORS[0]);
        addRow(traits.speed, TRAIT_NAMES[1], TRAIT_COLORS[1]);
        addRow(traits.visionRange, TRAIT_NAMES[2], TRAIT_COLORS[2]);
        addRow(traits.efficiency, TRAIT_NAMES[3], TRAIT_COLORS[3]);
        addRow(traits.aggression, TRAIT_NAMES[4], TRAIT_COLORS[4]);
        addRow(traits.reproductionRate, TRAIT_NAMES[5], TRAIT_COLORS[5]);
        addRow(traits.lifespan, TRAIT_NAMES[6], TRAIT_COLORS[6]);
        addRow(traits.mutationRate, TRAIT_NAMES[7], TRAIT_COLORS[7]);

        ImGui::EndTable();
    }

    // Sample size
    ImGui::Text("Sample size: %zu creatures", traits.size.samples.size());
}

// ============================================================================
// Trait Evolution
// ============================================================================

void TraitDistributionGraphs::renderTraitEvolution(const StatisticsDataManager& data) {
    const auto& fitnessHistory = data.getFitnessHistory();

    if (fitnessHistory.size() < 2) {
        ImGui::TextDisabled("Not enough data for evolution trends");
        return;
    }

    // Extract fitness and diversity over time
    std::vector<float> times;
    std::vector<float> avgFitness;
    std::vector<float> diversity;

    times.reserve(fitnessHistory.size());
    avgFitness.reserve(fitnessHistory.size());
    diversity.reserve(fitnessHistory.size());

    for (const auto& sample : fitnessHistory) {
        times.push_back(sample.time);
        avgFitness.push_back(sample.avgFitness);
        diversity.push_back(sample.geneticDiversity);
    }

    if (ImPlot::BeginPlot("Fitness & Diversity Over Time", ImVec2(-1, 180))) {
        ImPlot::SetupAxes("Time (s)", "Value");
        ImPlot::SetupAxis(ImAxis_Y2, "Diversity");
        ImPlot::SetupAxisLimits(ImAxis_Y2, 0, 1);

        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.0f, 0.8f, 0.2f, 1.0f));
        ImPlot::PlotLine("Avg Fitness", times.data(), avgFitness.data(),
                        static_cast<int>(times.size()));
        ImPlot::PopStyleColor();

        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.8f, 0.4f, 0.8f, 1.0f));
        ImPlot::PlotLine("Diversity", times.data(), diversity.data(),
                        static_cast<int>(times.size()));
        ImPlot::PopStyleColor();

        ImPlot::EndPlot();
    }
}

// ============================================================================
// Diversity Gauge
// ============================================================================

void TraitDistributionGraphs::renderDiversityGauge(float diversity) {
    // Clamp to 0-1
    diversity = std::clamp(diversity, 0.0f, 1.0f);

    // Color based on diversity level
    ImVec4 color;
    const char* status;

    if (diversity < 0.2f) {
        color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
        status = "CRITICAL";
    } else if (diversity < 0.4f) {
        color = ImVec4(0.8f, 0.5f, 0.2f, 1.0f);
        status = "Low";
    } else if (diversity < 0.6f) {
        color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
        status = "Moderate";
    } else if (diversity < 0.8f) {
        color = ImVec4(0.4f, 0.8f, 0.2f, 1.0f);
        status = "Good";
    } else {
        color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
        status = "Excellent";
    }

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
    ImGui::ProgressBar(diversity, ImVec2(-1, 0), nullptr);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::TextColored(color, "%s (%.1f%%)", status, diversity * 100.0f);
}

} // namespace statistics
} // namespace ui
