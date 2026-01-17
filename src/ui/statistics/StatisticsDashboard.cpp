#include "StatisticsDashboard.h"

namespace ui {
namespace statistics {

// ============================================================================
// Constructor / Destructor
// ============================================================================

StatisticsDashboard::StatisticsDashboard() = default;
StatisticsDashboard::~StatisticsDashboard() = default;

// ============================================================================
// Lifecycle
// ============================================================================

void StatisticsDashboard::init(SimulationOrchestrator* simulation) {
    m_simulation = simulation;
    m_initialized = true;

    // Configure data manager
    m_dataManager.setSampleInterval(m_config.updateInterval);
}

void StatisticsDashboard::shutdown() {
    m_simulation = nullptr;
    m_initialized = false;
    m_dataManager.clear();
}

// ============================================================================
// Update
// ============================================================================

void StatisticsDashboard::update(float deltaTime) {
    if (!m_initialized || !m_simulation) return;
    if (m_minimized && m_config.pauseWhenMinimized) return;

    // Update data manager from simulation
    m_dataManager.update(deltaTime, *m_simulation);

    // Update performance panel
    m_performancePanel.update(m_simulation->getPerformanceManager(), deltaTime);

    // Update phylogenetic tree periodically
    static float treeUpdateTimer = 0.0f;
    treeUpdateTimer += deltaTime;
    if (treeUpdateTimer >= 2.0f) {
        treeUpdateTimer = 0.0f;

        if (m_simulation->getEvolutionaryHistory() && m_simulation->getSpeciationTracker()) {
            m_phylogeneticTree.updateFromHistory(*m_simulation->getEvolutionaryHistory(),
                                                 *m_simulation->getSpeciationTracker());
        }
    }

    // Update food web
    if (m_simulation->getFoodChainManager() && m_simulation->getCreatureManager()) {
        m_foodWebViz.update(*m_simulation->getFoodChainManager(),
                           *m_simulation->getCreatureManager());
    }
}

// ============================================================================
// Render
// ============================================================================

void StatisticsDashboard::render() {
    if (!m_visible) return;

    // Set window style
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.10f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.12f, 0.12f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar;

    ImGui::SetNextWindowSize(m_windowSize, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(m_windowPos, ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Statistics Dashboard", &m_visible, flags)) {
        m_windowSize = ImGui::GetWindowSize();
        m_windowPos = ImGui::GetWindowPos();

        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("FPS Overlay", nullptr, &m_config.showFPSOverlay);
                ImGui::Separator();
                if (ImGui::MenuItem("Pause Data Collection", nullptr, m_dataManager.isPaused())) {
                    m_dataManager.setPaused(!m_dataManager.isPaused());
                }
                if (ImGui::MenuItem("Clear History")) {
                    m_dataManager.clear();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Options")) {
                ImGui::SliderFloat("Sample Rate", &m_config.updateInterval, 0.1f, 2.0f, "%.1f s");
                ImGui::SliderInt("History", &m_config.historyDuration, 60, 600, "%d s");
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        // Toolbar
        renderToolbar();

        ImGui::Separator();

        // Tab bar
        if (ImGui::BeginTabBar("DashboardTabs")) {
            if (ImGui::BeginTabItem("Overview")) {
                m_currentTab = DashboardTab::Overview;
                renderOverviewTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Population")) {
                m_currentTab = DashboardTab::Population;
                renderPopulationTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Genetics")) {
                m_currentTab = DashboardTab::Genetics;
                renderGeneticsTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Evolution")) {
                m_currentTab = DashboardTab::Evolution;
                renderEvolutionTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Food Web")) {
                m_currentTab = DashboardTab::FoodWeb;
                renderFoodWebTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Ecosystem")) {
                m_currentTab = DashboardTab::Ecosystem;
                renderEcosystemTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Performance")) {
                m_currentTab = DashboardTab::Performance;
                renderPerformanceTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
}

void StatisticsDashboard::renderFPSOverlay() {
    if (!m_config.showFPSOverlay) return;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav |
                             ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.6f);

    if (ImGui::Begin("FPS Overlay", nullptr, flags)) {
        m_performancePanel.renderFPSCounter(m_dataManager.getCurrentFPS());
    }
    ImGui::End();
}

// ============================================================================
// Toolbar
// ============================================================================

void StatisticsDashboard::renderToolbar() {
    // Status indicators
    const auto& pop = m_dataManager.getCurrentPopulation();

    ImGui::Text("Time: %.1fs | Gen: %d | Pop: %d | Species: %d",
                m_dataManager.getSimulationTime(),
                m_dataManager.getTotalGenerations(),
                pop.totalCreatures,
                pop.speciesCount);

    ImGui::SameLine(ImGui::GetWindowWidth() - 200);

    // Health indicator
    float health = m_dataManager.getEcosystemHealth();
    ImVec4 healthColor;
    if (health >= 70) healthColor = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
    else if (health >= 40) healthColor = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
    else healthColor = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);

    ImGui::TextColored(healthColor, "Health: %.0f%%", health);

    ImGui::SameLine();

    // Pause indicator
    if (m_dataManager.isPaused()) {
        ImGui::TextColored(ImVec4(0.8f, 0.5f, 0.2f, 1.0f), "[PAUSED]");
    }
}

// ============================================================================
// Tab Renderers
// ============================================================================

void StatisticsDashboard::renderOverviewTab() {
    ImGui::BeginChild("OverviewContent", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    // Top row - key metrics
    ImGui::Columns(3, nullptr, false);

    // Population overview
    ImGui::Text("Population");
    m_populationGraphs.renderCompact(m_dataManager);
    ImGui::NextColumn();

    // Ecosystem health
    ImGui::Text("Ecosystem Health");
    m_ecosystemDashboard.renderCompact(m_dataManager);
    ImGui::NextColumn();

    // Performance
    ImGui::Text("Performance");
    m_performancePanel.renderCompact(m_dataManager);
    ImGui::Columns(1);

    ImGui::Separator();

    // Main population graph
    ImGui::Text("Population Dynamics");
    m_populationGraphs.renderTotalPopulation(m_dataManager);

    ImGui::Separator();

    // Diversity overview
    ImGui::Text("Genetic Diversity");
    m_traitGraphs.renderCompact(m_dataManager);

    ImGui::EndChild();
}

void StatisticsDashboard::renderPopulationTab() {
    ImGui::BeginChild("PopulationContent", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    m_populationGraphs.render(m_dataManager);
    ImGui::EndChild();
}

void StatisticsDashboard::renderGeneticsTab() {
    ImGui::BeginChild("GeneticsContent", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    m_traitGraphs.render(m_dataManager);
    ImGui::EndChild();
}

void StatisticsDashboard::renderEvolutionTab() {
    ImGui::BeginChild("EvolutionContent", ImVec2(0, 0), false);

    // Layout controls
    ImGui::Text("Layout:");
    ImGui::SameLine();
    if (ImGui::RadioButton("Vertical", m_phylogeneticTree.getLayoutStyle() == TreeLayoutStyle::VERTICAL)) {
        m_phylogeneticTree.setLayoutStyle(TreeLayoutStyle::VERTICAL);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Horizontal", m_phylogeneticTree.getLayoutStyle() == TreeLayoutStyle::HORIZONTAL)) {
        m_phylogeneticTree.setLayoutStyle(TreeLayoutStyle::HORIZONTAL);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Radial", m_phylogeneticTree.getLayoutStyle() == TreeLayoutStyle::RADIAL)) {
        m_phylogeneticTree.setLayoutStyle(TreeLayoutStyle::RADIAL);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Timeline", m_phylogeneticTree.getLayoutStyle() == TreeLayoutStyle::TIMELINE)) {
        m_phylogeneticTree.setLayoutStyle(TreeLayoutStyle::TIMELINE);
    }

    ImGui::SameLine();
    bool showExtinct = m_phylogeneticTree.getShowExtinct();
    if (ImGui::Checkbox("Show Extinct", &showExtinct)) {
        m_phylogeneticTree.setShowExtinct(showExtinct);
    }

    ImGui::SameLine();
    if (ImGui::Button("Fit to View")) {
        m_phylogeneticTree.fitToCanvas(ImVec2(ImGui::GetContentRegionAvail().x,
                                              ImGui::GetContentRegionAvail().y - 200));
    }

    // Phylogenetic tree
    ImVec2 treeSize(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 180);
    m_phylogeneticTree.render(treeSize);

    ImGui::Separator();

    // Evolutionary timeline
    ImGui::Text("Evolutionary Events Timeline");
    m_phylogeneticTree.renderTimeline(m_dataManager);

    ImGui::EndChild();
}

void StatisticsDashboard::renderFoodWebTab() {
    ImGui::BeginChild("FoodWebContent", ImVec2(0, 0), false);

    // Controls
    bool useForce = m_foodWebViz.getShowEnergyFlow();  // Placeholder - add actual method if needed
    if (ImGui::Checkbox("Show Energy Flow Labels", &useForce)) {
        m_foodWebViz.setShowEnergyFlow(useForce);
    }

    ImGui::SameLine();
    bool showLabels = true;
    if (ImGui::Checkbox("Show Labels", &showLabels)) {
        m_foodWebViz.setShowLabels(showLabels);
    }

    // Food web visualization
    ImVec2 webSize(ImGui::GetContentRegionAvail().x, 350);
    m_foodWebViz.render(webSize);

    ImGui::Separator();

    // Energy pyramid
    ImGui::Text("Ecological Pyramid");
    ImVec2 pyramidSize(ImGui::GetContentRegionAvail().x, 150);
    m_foodWebViz.renderPyramid(pyramidSize);

    ImGui::Separator();

    // Energy flow stats
    m_foodWebViz.renderEnergyFlow();

    ImGui::EndChild();
}

void StatisticsDashboard::renderEcosystemTab() {
    ImGui::BeginChild("EcosystemContent", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    if (m_simulation) {
        m_ecosystemDashboard.render(m_dataManager,
                                    m_simulation->getEcosystemMetrics(),
                                    m_simulation->getNicheManager(),
                                    m_simulation->getSelectionPressureCalculator());
    } else {
        m_ecosystemDashboard.render(m_dataManager);
    }

    ImGui::EndChild();
}

void StatisticsDashboard::renderPerformanceTab() {
    ImGui::BeginChild("PerformanceContent", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    if (m_simulation) {
        m_performancePanel.render(m_dataManager, m_simulation->getPerformanceManager());
    } else {
        m_performancePanel.render(m_dataManager);
    }

    ImGui::EndChild();
}

} // namespace statistics
} // namespace ui
