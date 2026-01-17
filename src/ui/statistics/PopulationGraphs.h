#pragma once

/**
 * @file PopulationGraphs.h
 * @brief Real-time population dynamics visualization using ImPlot
 *
 * Renders interactive graphs showing:
 * - Total population over time
 * - Population by ecological role (herbivore, carnivore, omnivore)
 * - Population by domain (land, water, air)
 * - Per-species population trends
 * - Birth/death rate visualization
 */

#include "imgui.h"
#include "implot.h"
#include "StatisticsDataManager.h"
#include "../../entities/genetics/Species.h"
#include <vector>
#include <map>
#include <string>

namespace ui {
namespace statistics {

// ============================================================================
// Color Blind Friendly Palette
// ============================================================================

namespace Colors {
    // Okabe-Ito color palette (colorblind friendly)
    constexpr ImVec4 Orange      = ImVec4(0.902f, 0.624f, 0.000f, 1.0f);
    constexpr ImVec4 SkyBlue     = ImVec4(0.337f, 0.706f, 0.914f, 1.0f);
    constexpr ImVec4 BluishGreen = ImVec4(0.000f, 0.620f, 0.451f, 1.0f);
    constexpr ImVec4 Yellow      = ImVec4(0.941f, 0.894f, 0.259f, 1.0f);
    constexpr ImVec4 Blue        = ImVec4(0.000f, 0.447f, 0.698f, 1.0f);
    constexpr ImVec4 Vermillion  = ImVec4(0.835f, 0.369f, 0.000f, 1.0f);
    constexpr ImVec4 ReddishPurple = ImVec4(0.800f, 0.475f, 0.655f, 1.0f);
    constexpr ImVec4 Gray        = ImVec4(0.600f, 0.600f, 0.600f, 1.0f);

    // Role colors
    constexpr ImVec4 Herbivore   = BluishGreen;
    constexpr ImVec4 Carnivore   = Vermillion;
    constexpr ImVec4 Omnivore    = Orange;
    constexpr ImVec4 Total       = Blue;

    // Domain colors
    constexpr ImVec4 Land        = BluishGreen;
    constexpr ImVec4 Water       = SkyBlue;
    constexpr ImVec4 Air         = Yellow;
}

// ============================================================================
// Population Graphs Widget
// ============================================================================

/**
 * @class PopulationGraphs
 * @brief Renders population dynamics graphs using ImPlot
 */
class PopulationGraphs {
public:
    PopulationGraphs() = default;
    ~PopulationGraphs() = default;

    /**
     * @brief Render all population graphs
     * @param data Reference to statistics data manager
     */
    void render(const StatisticsDataManager& data);

    /**
     * @brief Render compact population overview
     * @param data Reference to statistics data manager
     */
    void renderCompact(const StatisticsDataManager& data);

    // ========================================================================
    // Individual Graph Components
    // ========================================================================

    /**
     * @brief Render total population over time
     */
    void renderTotalPopulation(const StatisticsDataManager& data);

    /**
     * @brief Render population breakdown by ecological role
     */
    void renderPopulationByRole(const StatisticsDataManager& data);

    /**
     * @brief Render stacked area chart of populations
     */
    void renderStackedPopulation(const StatisticsDataManager& data);

    /**
     * @brief Render population by domain (land/water/air)
     */
    void renderPopulationByDomain(const StatisticsDataManager& data);

    /**
     * @brief Render species-specific population trends
     */
    void renderSpeciesPopulations(const StatisticsDataManager& data,
                                   const genetics::SpeciationTracker* tracker);

    /**
     * @brief Render population rate of change
     */
    void renderPopulationRates(const StatisticsDataManager& data);

    /**
     * @brief Render predator-prey phase plot
     */
    void renderPredatorPreyPhase(const StatisticsDataManager& data);

    // ========================================================================
    // Configuration
    // ========================================================================

    void setGraphHeight(float height) { m_graphHeight = height; }
    void setShowLegend(bool show) { m_showLegend = show; }
    void setTimeWindow(float seconds) { m_timeWindow = seconds; }
    void setSmoothing(int samples) { m_smoothingSamples = samples; }

private:
    // Configuration
    float m_graphHeight = 200.0f;
    bool m_showLegend = true;
    float m_timeWindow = 300.0f;  // Show last 5 minutes by default
    int m_smoothingSamples = 3;

    // Cached data for rendering (to avoid allocations per frame)
    mutable std::vector<float> m_times;
    mutable std::vector<float> m_totals;
    mutable std::vector<float> m_herbivores;
    mutable std::vector<float> m_carnivores;
    mutable std::vector<float> m_omnivores;
    mutable std::vector<float> m_aquatic;
    mutable std::vector<float> m_flying;
    mutable std::vector<float> m_land;

    // Per-species tracking
    mutable std::map<genetics::SpeciesId, std::vector<float>> m_speciesData;

    // Rate calculation buffer
    mutable std::vector<float> m_rateBuffer;

    // Helper methods
    void prepareData(const StatisticsDataManager& data) const;
    float smoothValue(const std::vector<float>& data, size_t index) const;
    void calculateRates(const std::vector<float>& values) const;

    // ImPlot styling helpers
    void setupPlotStyle() const;
    void resetPlotStyle() const;
};

// ============================================================================
// Inline Implementation
// ============================================================================

inline void PopulationGraphs::setupPlotStyle() const {
    ImPlot::GetStyle().LineWeight = 2.0f;
    ImPlot::GetStyle().FillAlpha = 0.5f;
}

inline void PopulationGraphs::resetPlotStyle() const {
    ImPlot::GetStyle().LineWeight = 1.0f;
    ImPlot::GetStyle().FillAlpha = 1.0f;
}

inline float PopulationGraphs::smoothValue(const std::vector<float>& data, size_t index) const {
    if (m_smoothingSamples <= 1 || data.empty()) {
        return index < data.size() ? data[index] : 0.0f;
    }

    float sum = 0.0f;
    int count = 0;
    int halfWindow = m_smoothingSamples / 2;

    for (int i = -halfWindow; i <= halfWindow; ++i) {
        int idx = static_cast<int>(index) + i;
        if (idx >= 0 && idx < static_cast<int>(data.size())) {
            sum += data[idx];
            count++;
        }
    }

    return count > 0 ? sum / static_cast<float>(count) : 0.0f;
}

} // namespace statistics
} // namespace ui
