#pragma once

/**
 * @file TraitDistributionGraphs.h
 * @brief Genetic trait distribution visualization using histograms and scatter plots
 *
 * Renders:
 * - Trait value histograms with normal distribution overlays
 * - Trait correlation heatmap
 * - Trait scatter plots (pairs)
 * - Trait evolution over time
 * - Genetic diversity metrics
 */

#include "imgui.h"
#include "implot.h"
#include "StatisticsDataManager.h"
#include <vector>
#include <array>
#include <string>

namespace ui {
namespace statistics {

// ============================================================================
// Trait Names and Labels
// ============================================================================

constexpr const char* TRAIT_NAMES[] = {
    "Size",
    "Speed",
    "Vision Range",
    "Efficiency",
    "Aggression",
    "Reproduction Rate",
    "Lifespan",
    "Mutation Rate"
};

constexpr const char* TRAIT_UNITS[] = {
    "units",
    "m/s",
    "m",
    "ratio",
    "0-1",
    "rate",
    "seconds",
    "rate"
};

// ============================================================================
// Trait Distribution Graphs Widget
// ============================================================================

/**
 * @class TraitDistributionGraphs
 * @brief Renders genetic trait distributions and correlations
 */
class TraitDistributionGraphs {
public:
    TraitDistributionGraphs() = default;
    ~TraitDistributionGraphs() = default;

    /**
     * @brief Render all trait distribution graphs
     * @param data Reference to statistics data manager
     */
    void render(const StatisticsDataManager& data);

    /**
     * @brief Render compact trait overview
     */
    void renderCompact(const StatisticsDataManager& data);

    // ========================================================================
    // Individual Components
    // ========================================================================

    /**
     * @brief Render histogram for a single trait
     */
    void renderTraitHistogram(const TraitStatistics& trait, const char* name,
                              const char* unit, const ImVec4& color);

    /**
     * @brief Render all trait histograms in a grid
     */
    void renderAllHistograms(const TraitDistributions& traits);

    /**
     * @brief Render trait correlation heatmap
     */
    void renderCorrelationHeatmap(const TraitDistributions& traits);

    /**
     * @brief Render scatter plot for two traits
     */
    void renderTraitScatter(const TraitStatistics& traitX, const TraitStatistics& traitY,
                           const char* nameX, const char* nameY);

    /**
     * @brief Render statistical summary panel
     */
    void renderStatsSummary(const TraitDistributions& traits);

    /**
     * @brief Render trait evolution sparklines
     */
    void renderTraitEvolution(const StatisticsDataManager& data);

    /**
     * @brief Render genetic diversity gauge
     */
    void renderDiversityGauge(float diversity);

    // ========================================================================
    // Configuration
    // ========================================================================

    void setHistogramHeight(float height) { m_histogramHeight = height; }
    void setShowNormalOverlay(bool show) { m_showNormalOverlay = show; }
    void setSelectedTraits(int x, int y) { m_selectedTraitX = x; m_selectedTraitY = y; }

private:
    // Configuration
    float m_histogramHeight = 150.0f;
    bool m_showNormalOverlay = true;
    int m_selectedTraitX = 0;  // Size
    int m_selectedTraitY = 1;  // Speed

    // Colors for traits (colorblind friendly)
    static constexpr ImVec4 TRAIT_COLORS[] = {
        ImVec4(0.337f, 0.706f, 0.914f, 1.0f),  // Size - Sky Blue
        ImVec4(0.902f, 0.624f, 0.000f, 1.0f),  // Speed - Orange
        ImVec4(0.000f, 0.620f, 0.451f, 1.0f),  // Vision - Bluish Green
        ImVec4(0.941f, 0.894f, 0.259f, 1.0f),  // Efficiency - Yellow
        ImVec4(0.835f, 0.369f, 0.000f, 1.0f),  // Aggression - Vermillion
        ImVec4(0.800f, 0.475f, 0.655f, 1.0f),  // Reproduction - Reddish Purple
        ImVec4(0.000f, 0.447f, 0.698f, 1.0f),  // Lifespan - Blue
        ImVec4(0.600f, 0.600f, 0.600f, 1.0f),  // Mutation - Gray
    };

    // Helper methods
    void renderNormalCurve(float mean, float stdDev, float minX, float maxX,
                          int samples, const ImVec4& color);
    const TraitStatistics* getTraitByIndex(const TraitDistributions& traits, int index) const;
    ImVec4 getCorrelationColor(float correlation) const;
};

// ============================================================================
// Inline Implementation
// ============================================================================

inline const TraitStatistics* TraitDistributionGraphs::getTraitByIndex(
    const TraitDistributions& traits, int index) const {
    switch (index) {
        case 0: return &traits.size;
        case 1: return &traits.speed;
        case 2: return &traits.visionRange;
        case 3: return &traits.efficiency;
        case 4: return &traits.aggression;
        case 5: return &traits.reproductionRate;
        case 6: return &traits.lifespan;
        case 7: return &traits.mutationRate;
        default: return nullptr;
    }
}

inline ImVec4 TraitDistributionGraphs::getCorrelationColor(float correlation) const {
    // Diverging color map: blue (negative) -> white (zero) -> red (positive)
    if (correlation >= 0) {
        float t = std::min(1.0f, correlation);
        return ImVec4(1.0f, 1.0f - t, 1.0f - t, 1.0f);
    } else {
        float t = std::min(1.0f, -correlation);
        return ImVec4(1.0f - t, 1.0f - t, 1.0f, 1.0f);
    }
}

} // namespace statistics
} // namespace ui
