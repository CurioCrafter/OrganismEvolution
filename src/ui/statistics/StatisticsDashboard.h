#pragma once

/**
 * @file StatisticsDashboard.h
 * @brief Master statistics dashboard integrating all visualization components
 *
 * Provides a unified tabbed interface for viewing:
 * - Population dynamics
 * - Genetic trait distributions
 * - Evolutionary history (phylogenetic tree)
 * - Food web visualization
 * - Ecosystem health metrics
 * - Performance metrics
 *
 * Pulls data from all simulation systems through StatisticsDataManager.
 */

#include "imgui.h"
#include "implot.h"
#include "StatisticsDataManager.h"
#include "PopulationGraphs.h"
#include "TraitDistributionGraphs.h"
#include "PhylogeneticTreeViz.h"
#include "FoodWebViz.h"
#include "EcosystemDashboard.h"
#include "PerformancePanel.h"
#include "../../core/SimulationOrchestrator.h"

namespace ui {
namespace statistics {

// ============================================================================
// Dashboard Configuration
// ============================================================================

struct DashboardConfig {
    float updateInterval = 0.5f;           // Data sampling interval
    float graphUpdateInterval = 0.1f;      // Graph refresh interval
    bool showFPSOverlay = true;            // Show FPS in corner
    bool pauseWhenMinimized = true;        // Pause data collection when minimized
    int historyDuration = 300;             // Seconds of history to keep
};

// ============================================================================
// Statistics Dashboard
// ============================================================================

/**
 * @class StatisticsDashboard
 * @brief Main statistics dashboard providing comprehensive simulation visualization
 */
class StatisticsDashboard {
public:
    StatisticsDashboard();
    ~StatisticsDashboard();

    // ========================================================================
    // Lifecycle
    // ========================================================================

    /**
     * @brief Initialize the dashboard with simulation reference
     */
    void init(SimulationOrchestrator* simulation);

    /**
     * @brief Shutdown and release resources
     */
    void shutdown();

    // ========================================================================
    // Per-Frame Updates
    // ========================================================================

    /**
     * @brief Update statistics data from simulation
     */
    void update(float deltaTime);

    /**
     * @brief Render the dashboard UI
     */
    void render();

    /**
     * @brief Render floating FPS overlay
     */
    void renderFPSOverlay();

    // ========================================================================
    // Window Control
    // ========================================================================

    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }
    void toggleVisible() { m_visible = !m_visible; }

    void setMinimized(bool minimized) { m_minimized = minimized; }
    bool isMinimized() const { return m_minimized; }

    // ========================================================================
    // Configuration
    // ========================================================================

    void setConfig(const DashboardConfig& config) { m_config = config; }
    const DashboardConfig& getConfig() const { return m_config; }

    void setPaused(bool paused) { m_dataManager.setPaused(paused); }
    bool isPaused() const { return m_dataManager.isPaused(); }

    // ========================================================================
    // Data Access
    // ========================================================================

    StatisticsDataManager& getDataManager() { return m_dataManager; }
    const StatisticsDataManager& getDataManager() const { return m_dataManager; }

private:
    // Simulation reference
    SimulationOrchestrator* m_simulation = nullptr;

    // Visibility state
    bool m_visible = true;
    bool m_minimized = false;
    bool m_initialized = false;

    // Configuration
    DashboardConfig m_config;

    // Data management
    StatisticsDataManager m_dataManager;

    // Visualization components
    PopulationGraphs m_populationGraphs;
    TraitDistributionGraphs m_traitGraphs;
    PhylogeneticTreeViz m_phylogeneticTree;
    FoodWebViz m_foodWebViz;
    EcosystemDashboard m_ecosystemDashboard;
    PerformancePanel m_performancePanel;

    // Tab state
    enum class DashboardTab {
        Overview,
        Population,
        Genetics,
        Evolution,
        FoodWeb,
        Ecosystem,
        Performance
    };
    DashboardTab m_currentTab = DashboardTab::Overview;

    // Render methods for each tab
    void renderOverviewTab();
    void renderPopulationTab();
    void renderGeneticsTab();
    void renderEvolutionTab();
    void renderFoodWebTab();
    void renderEcosystemTab();
    void renderPerformanceTab();

    // Toolbar rendering
    void renderToolbar();

    // Window state
    ImVec2 m_windowSize{600, 800};
    ImVec2 m_windowPos{10, 10};
};

} // namespace statistics
} // namespace ui
