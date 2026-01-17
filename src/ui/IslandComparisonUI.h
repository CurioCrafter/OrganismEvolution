#pragma once

// IslandComparisonUI - UI panel for comparing statistics and evolution between islands
// Displays side-by-side comparisons, gene flow diagrams, and divergence metrics

#include "../core/MultiIslandManager.h"
#include "../entities/behaviors/InterIslandMigration.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <array>

namespace Forge {

// ============================================================================
// Comparison Mode
// ============================================================================

enum class ComparisonMode {
    SIDE_BY_SIDE,       // Two islands compared directly
    OVERVIEW,           // All islands overview
    GENE_FLOW,          // Gene flow diagram between islands
    DIVERGENCE_TREE,    // Phylogenetic-style divergence visualization
    MIGRATION_MAP       // Active migrations visualization
};

// ============================================================================
// Chart Data Types
// ============================================================================

struct PopulationChartData {
    std::vector<float> timestamps;
    std::vector<int> populations;
    int maxPopulation = 0;
    float timeRange = 100.0f;
};

struct GeneticDistanceData {
    uint32_t islandA;
    uint32_t islandB;
    float distance;
    float trend;  // Positive = diverging, negative = converging
    int sampleCount;
};

struct GeneFlowEdge {
    uint32_t fromIsland;
    uint32_t toIsland;
    int migrationCount;
    float flowStrength;  // 0-1
    glm::vec3 color;
};

// ============================================================================
// UI Configuration
// ============================================================================

struct IslandComparisonUIConfig {
    // Layout
    float panelWidth = 400.0f;
    float panelHeight = 600.0f;
    float chartHeight = 150.0f;
    float barChartWidth = 200.0f;

    // Colors
    glm::vec4 backgroundColor = glm::vec4(0.1f, 0.1f, 0.12f, 0.95f);
    glm::vec4 headerColor = glm::vec4(0.2f, 0.4f, 0.6f, 1.0f);
    glm::vec4 textColor = glm::vec4(0.9f, 0.9f, 0.9f, 1.0f);
    glm::vec4 highlightColor = glm::vec4(0.3f, 0.7f, 0.3f, 1.0f);
    glm::vec4 warningColor = glm::vec4(0.9f, 0.6f, 0.2f, 1.0f);
    glm::vec4 dangerColor = glm::vec4(0.9f, 0.3f, 0.3f, 1.0f);

    // Chart colors for different islands
    std::array<glm::vec4, 8> islandColors = {{
        glm::vec4(0.2f, 0.6f, 0.9f, 1.0f),  // Blue
        glm::vec4(0.9f, 0.4f, 0.2f, 1.0f),  // Orange
        glm::vec4(0.3f, 0.8f, 0.4f, 1.0f),  // Green
        glm::vec4(0.8f, 0.3f, 0.7f, 1.0f),  // Purple
        glm::vec4(0.9f, 0.8f, 0.2f, 1.0f),  // Yellow
        glm::vec4(0.4f, 0.9f, 0.9f, 1.0f),  // Cyan
        glm::vec4(0.9f, 0.5f, 0.5f, 1.0f),  // Pink
        glm::vec4(0.6f, 0.6f, 0.6f, 1.0f)   // Gray
    }};

    // Update rate
    float updateInterval = 0.5f;  // Seconds between UI updates
};

// ============================================================================
// Island Comparison UI
// ============================================================================

class IslandComparisonUI {
public:
    IslandComparisonUI();
    ~IslandComparisonUI() = default;

    // ========================================================================
    // Main Interface
    // ========================================================================

    // Render the comparison UI (call in ImGui context)
    void render(const MultiIslandManager& islands);

    // Render with migration data
    void render(const MultiIslandManager& islands, const InterIslandMigration* migration);

    // ========================================================================
    // Mode Control
    // ========================================================================

    void setMode(ComparisonMode mode) { m_mode = mode; }
    ComparisonMode getMode() const { return m_mode; }

    // For side-by-side mode
    void selectIslandA(uint32_t index) { m_selectedIslandA = index; }
    void selectIslandB(uint32_t index) { m_selectedIslandB = index; }
    uint32_t getSelectedIslandA() const { return m_selectedIslandA; }
    uint32_t getSelectedIslandB() const { return m_selectedIslandB; }

    // ========================================================================
    // Configuration
    // ========================================================================

    void setConfig(const IslandComparisonUIConfig& config) { m_config = config; }
    const IslandComparisonUIConfig& getConfig() const { return m_config; }

    // Visibility
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }

    // ========================================================================
    // Data Recording
    // ========================================================================

    // Call periodically to record history for charts
    void recordHistory(const MultiIslandManager& islands);

private:
    // Configuration
    IslandComparisonUIConfig m_config;
    ComparisonMode m_mode = ComparisonMode::OVERVIEW;
    bool m_visible = true;

    // Selection state
    uint32_t m_selectedIslandA = 0;
    uint32_t m_selectedIslandB = 1;

    // History for charts
    static constexpr size_t MAX_HISTORY = 200;
    std::vector<PopulationChartData> m_populationHistory;
    std::vector<GeneticDistanceData> m_distanceHistory;

    // Update timing
    float m_lastUpdateTime = 0.0f;
    float m_accumulatedTime = 0.0f;

    // Render methods for each mode
    void renderSideBySide(const MultiIslandManager& islands);
    void renderOverview(const MultiIslandManager& islands);
    void renderGeneFlowDiagram(const MultiIslandManager& islands, const InterIslandMigration* migration);
    void renderDivergenceTree(const MultiIslandManager& islands);
    void renderMigrationMap(const MultiIslandManager& islands, const InterIslandMigration* migration);

    // Component renderers
    void renderIslandCard(const Island& island, uint32_t index);
    void renderIslandStats(const IslandStats& stats, const char* label);
    void renderPopulationChart(const std::vector<PopulationChartData>& data);
    void renderComparisonBars(const IslandStats& statsA, const IslandStats& statsB);
    void renderGeneticDistanceMatrix(const MultiIslandManager& islands);
    void renderMigrationArrows(const std::vector<GeneFlowEdge>& edges);
    void renderIslandNode(const Island& island, uint32_t index, const glm::vec2& position, float radius);

    // Utility methods
    glm::vec4 getColorForStat(float value, float min, float max) const;
    glm::vec4 getIslandColor(uint32_t index) const;
    std::string formatNumber(int number) const;
    std::string formatPercent(float value) const;

    // Data collection
    std::vector<GeneFlowEdge> collectGeneFlowData(const MultiIslandManager& islands,
                                                   const InterIslandMigration* migration) const;
};

} // namespace Forge
