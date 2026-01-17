#include "EcosystemDashboard.h"
#include <cmath>
#include <algorithm>

namespace ui {
namespace statistics {

// ============================================================================
// Main Render
// ============================================================================

void EcosystemDashboard::render(const StatisticsDataManager& data,
                                 const EcosystemMetrics* metrics,
                                 const genetics::NicheManager* niches,
                                 const genetics::SelectionPressureCalculator* pressures) {
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.12f, 1.0f));

    // Top row - Key gauges
    if (ImGui::CollapsingHeader("Ecosystem Health", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Columns(3, nullptr, false);

        // Health gauge
        renderHealthGauge(data.getEcosystemHealth());
        ImGui::NextColumn();

        // Diversity gauge
        const auto& pop = data.getCurrentPopulation();
        renderDiversityIndicator(data.getSpeciesDiversity(), pop.speciesCount);
        ImGui::NextColumn();

        // Trophic balance gauge
        renderTrophicBalance(data.getTrophicBalance(), pop.herbivoreCount, pop.carnivoreCount);
        ImGui::Columns(1);
    }

    // Selection pressures
    if (ImGui::CollapsingHeader("Selection Pressures")) {
        renderSelectionPressureRadar(data);
    }

    // Niche occupancy
    if (ImGui::CollapsingHeader("Niche Occupancy")) {
        renderNicheOccupancy(data);
    }

    // Energy flow
    if (ImGui::CollapsingHeader("Energy Flow")) {
        renderEnergyMetrics(data);
    }

    // Aquatic ecosystem - depth distribution
    if (ImGui::CollapsingHeader("Aquatic Depth Distribution")) {
        renderAquaticDepthHistogram(data);
    }

    // Warnings
    if (metrics) {
        if (ImGui::CollapsingHeader("Ecosystem Warnings")) {
            renderWarnings(metrics);
        }
    }

    ImGui::PopStyleColor();
}

void EcosystemDashboard::renderCompact(const StatisticsDataManager& data) {
    float health = data.getEcosystemHealth();
    float diversity = data.getSpeciesDiversity();
    float balance = data.getTrophicBalance();

    // Compact row of mini gauges
    ImGui::BeginGroup();

    // Health
    ImU32 healthColor = getHealthColor(health);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImGui::ColorConvertU32ToFloat4(healthColor));
    ImGui::ProgressBar(health / 100.0f, ImVec2(100, 0), nullptr);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Text("Health: %.0f%%", health);

    // Diversity
    ImU32 divColor = getDiversityColor(diversity);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImGui::ColorConvertU32ToFloat4(divColor));
    ImGui::ProgressBar(diversity, ImVec2(100, 0), nullptr);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Text("Diversity: %.0f%%", diversity * 100.0f);

    ImGui::EndGroup();
}

// ============================================================================
// Health Gauge
// ============================================================================

void EcosystemDashboard::renderHealthGauge(float health) {
    ImGui::BeginGroup();
    ImGui::Text("Ecosystem Health");

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size(100, 100);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImVec2 center(pos.x + size.x / 2.0f, pos.y + size.y / 2.0f);
    float radius = 40.0f;

    // Background arc
    drawRadialBar(drawList, center, radius - 8, radius, -2.4f, 2.4f, IM_COL32(40, 40, 45, 255));

    // Health arc
    float healthNorm = std::clamp(health / 100.0f, 0.0f, 1.0f);
    float endAngle = -2.4f + 4.8f * healthNorm;
    ImU32 healthColor = getHealthColor(health);
    drawRadialBar(drawList, center, radius - 8, radius, -2.4f, endAngle, healthColor);

    // Center text
    char buf[32];
    snprintf(buf, sizeof(buf), "%.0f", health);
    ImVec2 textSize = ImGui::CalcTextSize(buf);
    drawList->AddText(ImVec2(center.x - textSize.x / 2.0f, center.y - textSize.y / 2.0f - 5),
                     IM_COL32(255, 255, 255, 255), buf);
    drawList->AddText(ImVec2(center.x - 10, center.y + 5), IM_COL32(150, 150, 150, 255), "%");

    ImGui::Dummy(size);
    ImGui::EndGroup();
}

// ============================================================================
// Diversity Indicator
// ============================================================================

void EcosystemDashboard::renderDiversityIndicator(float diversity, int speciesCount) {
    ImGui::BeginGroup();
    ImGui::Text("Species Diversity");

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size(100, 100);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImVec2 center(pos.x + size.x / 2.0f, pos.y + size.y / 2.0f);
    float radius = 40.0f;

    // Background circle
    drawList->AddCircle(center, radius, IM_COL32(40, 40, 45, 255), 0, 3.0f);

    // Progress circle
    ImU32 color = getDiversityColor(diversity);
    drawCircularProgress(drawList, center, radius, diversity, IM_COL32(40, 40, 45, 255), color, 6.0f);

    // Center text
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", speciesCount);
    ImVec2 textSize = ImGui::CalcTextSize(buf);
    drawList->AddText(ImVec2(center.x - textSize.x / 2.0f, center.y - textSize.y / 2.0f - 5),
                     IM_COL32(255, 255, 255, 255), buf);
    drawList->AddText(ImVec2(center.x - 20, center.y + 5), IM_COL32(150, 150, 150, 255), "species");

    ImGui::Dummy(size);
    ImGui::EndGroup();
}

// ============================================================================
// Trophic Balance
// ============================================================================

void EcosystemDashboard::renderTrophicBalance(float balance, int herbivores, int carnivores) {
    ImGui::BeginGroup();
    ImGui::Text("Trophic Balance");

    // Calculate actual ratio
    float ratio = carnivores > 0 ? static_cast<float>(herbivores) / carnivores : 0.0f;
    float idealRatio = 10.0f;  // Ideal herbivore:carnivore ratio

    // Deviation from ideal
    float deviation = std::abs(ratio - idealRatio) / idealRatio;
    deviation = std::clamp(deviation, 0.0f, 1.0f);
    float balanceScore = 1.0f - deviation;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size(100, 100);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImVec2 center(pos.x + size.x / 2.0f, pos.y + size.y / 2.0f);

    // Balance scale visualization
    float scaleWidth = 80.0f;
    float scaleHeight = 10.0f;
    float scaleY = center.y;

    // Scale bar
    drawList->AddRectFilled(ImVec2(center.x - scaleWidth / 2, scaleY - scaleHeight / 2),
                           ImVec2(center.x + scaleWidth / 2, scaleY + scaleHeight / 2),
                           IM_COL32(40, 40, 45, 255), 5.0f);

    // Balance indicator position (-1 to 1 based on ratio)
    float normalizedBalance = std::clamp((ratio - idealRatio) / idealRatio, -1.0f, 1.0f);
    float indicatorX = center.x + normalizedBalance * (scaleWidth / 2.0f - 10);

    // Ideal zone (green)
    drawList->AddRectFilled(ImVec2(center.x - 10, scaleY - scaleHeight / 2),
                           ImVec2(center.x + 10, scaleY + scaleHeight / 2),
                           IM_COL32(50, 150, 50, 100), 3.0f);

    // Indicator
    ImU32 indicatorColor = getBalanceColor(balanceScore);
    drawList->AddCircleFilled(ImVec2(indicatorX, scaleY), 8.0f, indicatorColor);
    drawList->AddCircle(ImVec2(indicatorX, scaleY), 8.0f, IM_COL32(255, 255, 255, 150), 0, 2.0f);

    // Labels
    drawList->AddText(ImVec2(pos.x + 5, scaleY + 15), IM_COL32(100, 180, 100, 255), "H");
    drawList->AddText(ImVec2(pos.x + size.x - 15, scaleY + 15), IM_COL32(180, 100, 100, 255), "C");

    // Ratio text
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f:1", ratio);
    ImVec2 textSize = ImGui::CalcTextSize(buf);
    drawList->AddText(ImVec2(center.x - textSize.x / 2, scaleY - 30), IM_COL32(200, 200, 200, 255), buf);

    // Population counts
    snprintf(buf, sizeof(buf), "H:%d C:%d", herbivores, carnivores);
    textSize = ImGui::CalcTextSize(buf);
    drawList->AddText(ImVec2(center.x - textSize.x / 2, scaleY + 35), IM_COL32(150, 150, 150, 255), buf);

    ImGui::Dummy(size);
    ImGui::EndGroup();
}

// ============================================================================
// Selection Pressure Radar
// ============================================================================

void EcosystemDashboard::renderSelectionPressureRadar(const StatisticsDataManager& data) {
    const auto& pressure = data.getCurrentSelectionPressures();

    // Labels and values for radar chart
    const char* labels[] = {"Predation", "Competition", "Climate", "Food", "Disease", "Sexual"};
    float values[] = {
        pressure.predationPressure,
        pressure.competitionPressure,
        pressure.climatePressure,
        pressure.foodPressure,
        pressure.diseasePressure,
        pressure.sexualSelectionPressure
    };
    constexpr int n = 6;

    // Normalize values to 0-1
    for (int i = 0; i < n; ++i) {
        values[i] = std::clamp(values[i], 0.0f, 1.0f);
    }

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size(200, 200);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImVec2 center(pos.x + size.x / 2.0f, pos.y + size.y / 2.0f);
    float radius = 80.0f;

    // Draw background rings
    for (int r = 1; r <= 4; ++r) {
        float ringRadius = radius * r / 4.0f;
        drawList->AddCircle(center, ringRadius, IM_COL32(50, 50, 55, 100), 6);
    }

    // Draw axes
    for (int i = 0; i < n; ++i) {
        float angle = -3.14159f / 2.0f + 2.0f * 3.14159f * i / n;
        ImVec2 end(center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius);
        drawList->AddLine(center, end, IM_COL32(60, 60, 65, 150), 1.0f);

        // Label
        ImVec2 labelPos(center.x + std::cos(angle) * (radius + 15) - 20,
                       center.y + std::sin(angle) * (radius + 15) - 8);
        drawList->AddText(labelPos, IM_COL32(150, 150, 150, 255), labels[i]);
    }

    // Draw filled polygon for values
    std::vector<ImVec2> points;
    for (int i = 0; i < n; ++i) {
        float angle = -3.14159f / 2.0f + 2.0f * 3.14159f * i / n;
        float r = values[i] * radius;
        points.push_back(ImVec2(center.x + std::cos(angle) * r, center.y + std::sin(angle) * r));
    }

    // Fill
    if (points.size() >= 3) {
        // Note: AddConvexPolyFilled requires clockwise points
        drawList->AddConvexPolyFilled(points.data(), static_cast<int>(points.size()),
                                      IM_COL32(100, 150, 200, 80));
    }

    // Outline
    for (int i = 0; i < n; ++i) {
        int next = (i + 1) % n;
        drawList->AddLine(points[i], points[next], IM_COL32(100, 180, 230, 200), 2.0f);
    }

    // Points at vertices
    for (const auto& pt : points) {
        drawList->AddCircleFilled(pt, 4.0f, IM_COL32(100, 200, 255, 255));
    }

    ImGui::Dummy(size);

    // Value list below
    ImGui::Columns(3, nullptr, false);
    for (int i = 0; i < n; ++i) {
        ImGui::Text("%s: %.0f%%", labels[i], values[i] * 100.0f);
        if ((i + 1) % 2 == 0) ImGui::NextColumn();
    }
    ImGui::Columns(1);
}

// ============================================================================
// Niche Occupancy
// ============================================================================

void EcosystemDashboard::renderNicheOccupancy(const StatisticsDataManager& data) {
    const auto& niche = data.getCurrentNicheOccupancy();

    ImGui::Text("Occupied Niches: %d / %d", niche.occupiedNiches, niche.occupiedNiches + niche.emptyNiches);

    // Progress bar for occupancy
    float occupancyRate = (niche.occupiedNiches + niche.emptyNiches) > 0 ?
        static_cast<float>(niche.occupiedNiches) / (niche.occupiedNiches + niche.emptyNiches) : 0.0f;

    ImGui::ProgressBar(occupancyRate, ImVec2(-1, 0), nullptr);

    // Niche overlap index
    ImGui::Text("Niche Overlap Index: %.2f", niche.nicheOverlapIndex);

    if (niche.nicheOverlapIndex > 0.5f) {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                          "Warning: High niche overlap - competition pressure elevated");
    }

    // Niche grid visualization
    if (!niche.occupancy.empty()) {
        ImGui::Separator();
        ImGui::Text("Niche Population Grid:");

        ImGui::Columns(4, nullptr, true);
        for (const auto& [nicheType, population] : niche.occupancy) {
            ImVec4 color;
            if (population == 0) {
                color = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
            } else if (population < 10) {
                color = ImVec4(0.8f, 0.4f, 0.2f, 1.0f);
            } else if (population < 50) {
                color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
            } else {
                color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
            }

            ImGui::TextColored(color, "N%d: %d", static_cast<int>(nicheType), population);
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
    }
}

// ============================================================================
// Warnings
// ============================================================================

void EcosystemDashboard::renderWarnings(const EcosystemMetrics* metrics) {
    if (!metrics) return;

    const auto& warnings = metrics->getWarnings();

    if (warnings.empty()) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "No ecosystem warnings");
        return;
    }

    for (const auto& warning : warnings) {
        ImVec4 color;
        const char* icon;

        switch (warning.severity) {
            case EcosystemWarning::Severity::INFO:
                color = ImVec4(0.3f, 0.6f, 0.9f, 1.0f);
                icon = "[i]";
                break;
            case EcosystemWarning::Severity::WARNING:
                color = ImVec4(0.9f, 0.7f, 0.2f, 1.0f);
                icon = "[!]";
                break;
            case EcosystemWarning::Severity::CRITICAL:
                color = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
                icon = "[X]";
                break;
        }

        ImGui::TextColored(color, "%s %s", icon, warning.message.c_str());
        ImGui::Text("   Value: %.1f (Threshold: %.1f)", warning.value, warning.threshold);
    }

    if (metrics->hasCriticalWarnings()) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f),
                          "CRITICAL WARNINGS PRESENT - Ecosystem stability at risk!");
    }
}

// ============================================================================
// Energy Metrics
// ============================================================================

void EcosystemDashboard::renderEnergyMetrics(const StatisticsDataManager& data) {
    const auto& energy = data.getCurrentEnergyFlow();

    // Energy flow chart over time
    const auto& history = data.getEnergyFlowHistory();

    if (history.size() > 1) {
        std::vector<float> times, producers, herbivores, carnivores;
        for (const auto& sample : history) {
            times.push_back(sample.time);
            producers.push_back(sample.producerEnergy);
            herbivores.push_back(sample.herbivoreEnergy);
            carnivores.push_back(sample.carnivoreEnergy);
        }

        if (ImPlot::BeginPlot("Energy Distribution", ImVec2(-1, 150))) {
            ImPlot::SetupAxes("Time (s)", "Energy");
            ImPlot::SetupLegend(ImPlotLocation_NorthEast);

            ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.2f, 0.8f, 0.2f, 0.5f));
            ImPlot::PlotShaded("Producers", times.data(), producers.data(),
                              static_cast<int>(times.size()), 0);
            ImPlot::PopStyleColor();

            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
            ImPlot::PlotLine("Herbivores", times.data(), herbivores.data(),
                            static_cast<int>(times.size()));
            ImPlot::PopStyleColor();

            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImPlot::PlotLine("Carnivores", times.data(), carnivores.data(),
                            static_cast<int>(times.size()));
            ImPlot::PopStyleColor();

            ImPlot::EndPlot();
        }
    }

    // Current values
    ImGui::Columns(2, nullptr, false);
    ImGui::Text("Producer Energy: %.1f", energy.producerEnergy);
    ImGui::Text("Herbivore Energy: %.1f", energy.herbivoreEnergy);
    ImGui::NextColumn();
    ImGui::Text("Carnivore Energy: %.1f", energy.carnivoreEnergy);
    ImGui::Text("Transfer Efficiency: %.1f%%", energy.transferEfficiency * 100.0f);
    ImGui::Columns(1);
}

// ============================================================================
// Aquatic Depth Histogram
// ============================================================================

void EcosystemDashboard::renderAquaticDepthHistogram(const StatisticsDataManager& data) {
    // Get aquatic population by depth band
    // Depth bands: Surface (0-2m), Shallow (2-5m), Mid-Water (5-25m), Deep (25-50m), Abyss (50m+)
    const char* depthBandNames[] = { "Surface\n(0-2m)", "Shallow\n(2-5m)", "Mid-Water\n(5-25m)", "Deep\n(25-50m)", "Abyss\n(50m+)" };
    const float depthBandPositions[] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };

    // Get creature counts from data manager
    const auto& depthCounts = data.getAquaticDepthCounts();
    int totalAquatic = 0;
    for (int count : depthCounts) {
        totalAquatic += count;
    }

    ImGui::Text("Total Aquatic Creatures: %d", totalAquatic);
    ImGui::Separator();

    if (totalAquatic > 0) {
        // Depth distribution bar chart
        float counts[5] = {
            static_cast<float>(depthCounts[0]),  // Surface
            static_cast<float>(depthCounts[1]),  // Shallow
            static_cast<float>(depthCounts[2]),  // Mid-Water
            static_cast<float>(depthCounts[3]),  // Deep
            static_cast<float>(depthCounts[4])   // Abyss
        };

        if (ImPlot::BeginPlot("Depth Distribution", ImVec2(-1, 200))) {
            ImPlot::SetupAxes("Depth Band", "Creature Count");
            ImPlot::SetupAxisTicks(ImAxis_X1, depthBandPositions, 5, depthBandNames);
            ImPlot::SetupAxisLimits(ImAxis_X1, -0.5, 4.5);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, std::max(10.0f, *std::max_element(counts, counts + 5) * 1.2f));

            // Color gradient from light blue (surface) to dark blue (deep)
            ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.3f, 0.6f, 0.9f, 0.7f));
            ImPlot::PlotBars("##depthbars", depthBandPositions, counts, 5, 0.6f);
            ImPlot::PopStyleColor();

            ImPlot::EndPlot();
        }

        // Text breakdown by species type
        ImGui::Separator();
        ImGui::Text("Distribution by Depth Band:");
        ImGui::Columns(5, nullptr, true);
        for (int i = 0; i < 5; ++i) {
            float percentage = (totalAquatic > 0) ? (depthCounts[i] * 100.0f / totalAquatic) : 0.0f;
            ImVec4 color = ImVec4(0.3f + (4 - i) * 0.15f, 0.5f + (4 - i) * 0.1f, 0.9f - i * 0.1f, 1.0f);
            ImGui::TextColored(color, "%d", depthCounts[i]);
            ImGui::Text("%.1f%%", percentage);
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
    } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1.0f), "No aquatic creatures detected");
        ImGui::Text("Spawn aquatic creatures to see depth distribution");
    }
}

// ============================================================================
// Drawing Helpers
// ============================================================================

void EcosystemDashboard::drawRadialBar(ImDrawList* drawList, ImVec2 center,
                                        float innerRadius, float outerRadius,
                                        float startAngle, float endAngle, ImU32 color) {
    int segments = 32;
    float angleStep = (endAngle - startAngle) / segments;

    for (int i = 0; i < segments; ++i) {
        float a1 = startAngle + i * angleStep;
        float a2 = startAngle + (i + 1) * angleStep;

        ImVec2 p1(center.x + std::cos(a1) * innerRadius, center.y + std::sin(a1) * innerRadius);
        ImVec2 p2(center.x + std::cos(a1) * outerRadius, center.y + std::sin(a1) * outerRadius);
        ImVec2 p3(center.x + std::cos(a2) * outerRadius, center.y + std::sin(a2) * outerRadius);
        ImVec2 p4(center.x + std::cos(a2) * innerRadius, center.y + std::sin(a2) * innerRadius);

        drawList->AddQuadFilled(p1, p2, p3, p4, color);
    }
}

void EcosystemDashboard::drawCircularProgress(ImDrawList* drawList, ImVec2 center, float radius,
                                               float progress, ImU32 bgColor, ImU32 fgColor,
                                               float thickness) {
    // Background circle
    drawList->AddCircle(center, radius, bgColor, 0, thickness);

    // Progress arc
    float startAngle = -3.14159f / 2.0f;
    float endAngle = startAngle + 2.0f * 3.14159f * progress;

    ImVec2 prevPt(center.x + std::cos(startAngle) * radius, center.y + std::sin(startAngle) * radius);

    int segments = static_cast<int>(32 * progress);
    for (int i = 1; i <= segments; ++i) {
        float angle = startAngle + (endAngle - startAngle) * i / segments;
        ImVec2 pt(center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius);
        drawList->AddLine(prevPt, pt, fgColor, thickness);
        prevPt = pt;
    }
}

void EcosystemDashboard::drawGauge(ImDrawList* drawList, ImVec2 center, float radius,
                                    float value, float min, float max, ImU32 color,
                                    const char* label) {
    // Simplified gauge drawing
    float normalized = (value - min) / (max - min);
    normalized = std::clamp(normalized, 0.0f, 1.0f);

    drawRadialBar(drawList, center, radius - 10, radius, -2.4f, 2.4f, IM_COL32(40, 40, 45, 255));
    drawRadialBar(drawList, center, radius - 10, radius, -2.4f, -2.4f + 4.8f * normalized, color);

    // Label
    ImVec2 textSize = ImGui::CalcTextSize(label);
    drawList->AddText(ImVec2(center.x - textSize.x / 2, center.y + radius + 5),
                     IM_COL32(200, 200, 200, 255), label);
}

} // namespace statistics
} // namespace ui
