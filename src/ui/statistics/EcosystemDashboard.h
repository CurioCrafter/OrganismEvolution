#pragma once

/**
 * @file EcosystemDashboard.h
 * @brief Ecosystem health dashboard with gauges and indicators
 *
 * Displays:
 * - Overall ecosystem health score
 * - Species diversity index
 * - Trophic balance indicator
 * - Selection pressure visualization
 * - Niche occupancy heatmap
 * - Warning indicators
 */

#include "imgui.h"
#include "implot.h"
#include "StatisticsDataManager.h"
#include "../../environment/EcosystemMetrics.h"
#include "../../entities/genetics/NicheSystem.h"
#include "../../entities/genetics/SelectionPressures.h"
#include <vector>
#include <string>

namespace ui {
namespace statistics {

// ============================================================================
// Ecosystem Dashboard Widget
// ============================================================================

/**
 * @class EcosystemDashboard
 * @brief Renders ecosystem health metrics and indicators
 */
class EcosystemDashboard {
public:
    EcosystemDashboard() = default;
    ~EcosystemDashboard() = default;

    /**
     * @brief Render full ecosystem dashboard
     */
    void render(const StatisticsDataManager& data,
               const EcosystemMetrics* metrics = nullptr,
               const genetics::NicheManager* niches = nullptr,
               const genetics::SelectionPressureCalculator* pressures = nullptr);

    /**
     * @brief Render compact health overview
     */
    void renderCompact(const StatisticsDataManager& data);

    // ========================================================================
    // Individual Components
    // ========================================================================

    /**
     * @brief Render ecosystem health gauge
     */
    void renderHealthGauge(float health);

    /**
     * @brief Render species diversity indicator
     */
    void renderDiversityIndicator(float diversity, int speciesCount);

    /**
     * @brief Render trophic balance meter
     */
    void renderTrophicBalance(float balance, int herbivores, int carnivores);

    /**
     * @brief Render selection pressure radar chart
     */
    void renderSelectionPressureRadar(const StatisticsDataManager& data);

    /**
     * @brief Render niche occupancy visualization
     */
    void renderNicheOccupancy(const StatisticsDataManager& data);

    /**
     * @brief Render ecosystem warnings panel
     */
    void renderWarnings(const EcosystemMetrics* metrics);

    /**
     * @brief Render energy flow metrics
     */
    void renderEnergyMetrics(const StatisticsDataManager& data);

    /**
     * @brief Render aquatic depth distribution histogram
     * Shows creature counts at each depth band
     */
    void renderAquaticDepthHistogram(const StatisticsDataManager& data);

private:
    // Gauge rendering helpers
    void drawGauge(ImDrawList* drawList, ImVec2 center, float radius,
                  float value, float min, float max, ImU32 color,
                  const char* label);
    void drawRadialBar(ImDrawList* drawList, ImVec2 center, float innerRadius,
                      float outerRadius, float startAngle, float endAngle,
                      ImU32 color);
    void drawCircularProgress(ImDrawList* drawList, ImVec2 center, float radius,
                             float progress, ImU32 bgColor, ImU32 fgColor,
                             float thickness);

    // Color helpers
    ImU32 getHealthColor(float health) const;
    ImU32 getDiversityColor(float diversity) const;
    ImU32 getBalanceColor(float balance) const;
};

// ============================================================================
// Inline Implementation
// ============================================================================

inline ImU32 EcosystemDashboard::getHealthColor(float health) const {
    if (health < 30.0f) return IM_COL32(220, 50, 50, 255);   // Red
    if (health < 50.0f) return IM_COL32(220, 150, 50, 255);  // Orange
    if (health < 70.0f) return IM_COL32(220, 220, 50, 255);  // Yellow
    if (health < 85.0f) return IM_COL32(150, 220, 50, 255);  // Light green
    return IM_COL32(50, 220, 50, 255);  // Green
}

inline ImU32 EcosystemDashboard::getDiversityColor(float diversity) const {
    if (diversity < 0.2f) return IM_COL32(220, 50, 50, 255);
    if (diversity < 0.4f) return IM_COL32(220, 150, 50, 255);
    if (diversity < 0.6f) return IM_COL32(220, 220, 50, 255);
    return IM_COL32(50, 220, 50, 255);
}

inline ImU32 EcosystemDashboard::getBalanceColor(float balance) const {
    // Balance is best near 1.0 (ideal ratio)
    float deviation = std::abs(balance - 1.0f);
    if (deviation < 0.2f) return IM_COL32(50, 220, 50, 255);
    if (deviation < 0.4f) return IM_COL32(150, 220, 50, 255);
    if (deviation < 0.6f) return IM_COL32(220, 220, 50, 255);
    return IM_COL32(220, 100, 50, 255);
}

} // namespace statistics
} // namespace ui
