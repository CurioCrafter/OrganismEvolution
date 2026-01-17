#include "SimulationDashboard.h"
#include "../core/DayNightCycle.h"
#include "../core/CreatureManager.h"
#include "../environment/Food.h"
#include "../audio/AudioManager.h"
#include "../audio/CreatureVoiceGenerator.h"
#include "../audio/AmbientSoundscape.h"
#include <algorithm>
#include <cmath>

namespace ui {

// ============================================================================
// Constructor / Destructor
// ============================================================================

SimulationDashboard::SimulationDashboard() = default;

SimulationDashboard::~SimulationDashboard() {
    shutdown();
}

bool SimulationDashboard::initialize() {
    if (m_initialized) return true;

    // Create ImPlot context
    ImPlot::CreateContext();

    // Configure ImPlot style
    ImPlot::GetStyle().LineWeight = 1.5f;

    m_initialized = true;
    return true;
}

void SimulationDashboard::shutdown() {
    if (!m_initialized) return;

    ImPlot::DestroyContext();
    m_initialized = false;
}

// ============================================================================
// Main Render
// ============================================================================

void SimulationDashboard::render(
    std::vector<std::unique_ptr<Creature>>& creatures,
    std::vector<std::unique_ptr<Food>>& food,
    DayNightCycle& dayNight,
    Camera& camera,
    float simulationTime,
    uint32_t generation,
    float deltaTime
) {
    if (!m_showDebugPanel) return;

    // Update metrics
    std::vector<Creature*> creaturePtrs;
    creaturePtrs.reserve(creatures.size());
    for (auto& c : creatures) {
        creaturePtrs.push_back(c.get());
    }

    std::vector<Food*> foodPtrs;
    foodPtrs.reserve(food.size());
    for (auto& f : food) {
        foodPtrs.push_back(f.get());
    }

    m_metrics.update(creaturePtrs, foodPtrs, deltaTime);
    m_metrics.recordFrameTime(deltaTime * 1000.0f);

    // Validate selected creature is still alive
    if (m_selectedCreature && !m_selectedCreature->isAlive()) {
        m_selectedCreature = nullptr;
        m_selectedCreatureId = -1;
    }

    // Try to find selected creature if we have an ID but no pointer
    if (m_selectedCreatureId >= 0 && m_selectedCreature == nullptr) {
        m_selectedCreature = findCreatureById(creatures, m_selectedCreatureId);
    }

    // Render panels
    renderLeftPanel(creatures, food, dayNight, camera, simulationTime, generation);
    renderRightPanel(creatures, camera);

    if (m_showStatusBar) {
        renderStatusBar(simulationTime, generation);
    }

    if (m_showHelp) {
        renderHelpWindow();
    }
}

// ============================================================================
// Input Handling
// ============================================================================

void SimulationDashboard::handleInput() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) return;

    // Space to toggle pause
    if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
        m_paused = !m_paused;
    }

    // F1 for help
    if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
        m_showHelp = !m_showHelp;
    }

    // F3 to toggle debug panel
    if (ImGui::IsKeyPressed(ImGuiKey_F3)) {
        m_showDebugPanel = !m_showDebugPanel;
    }

    // Escape to deselect creature
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_selectedCreature = nullptr;
        m_selectedCreatureId = -1;
    }

    // P to toggle pause (alternative)
    if (ImGui::IsKeyPressed(ImGuiKey_P)) {
        m_paused = !m_paused;
    }

    // Number keys for speed
    if (ImGui::IsKeyPressed(ImGuiKey_1)) m_simulationSpeed = 0.5f;
    if (ImGui::IsKeyPressed(ImGuiKey_2)) m_simulationSpeed = 1.0f;
    if (ImGui::IsKeyPressed(ImGuiKey_3)) m_simulationSpeed = 2.0f;
    if (ImGui::IsKeyPressed(ImGuiKey_4)) m_simulationSpeed = 4.0f;
    if (ImGui::IsKeyPressed(ImGuiKey_5)) m_simulationSpeed = 8.0f;
}

// ============================================================================
// Left Panel (Controls)
// ============================================================================

void SimulationDashboard::renderLeftPanel(
    std::vector<std::unique_ptr<Creature>>& creatures,
    std::vector<std::unique_ptr<Food>>& food,
    DayNightCycle& dayNight,
    Camera& camera,
    float simulationTime,
    uint32_t generation
) {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320, 700), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Simulation Dashboard", nullptr, flags)) {
        // Tab bar for different views
        if (ImGui::BeginTabBar("DashboardTabs")) {
            if (ImGui::BeginTabItem("Overview")) {
                m_currentTab = DashboardTab::Overview;
                renderOverviewTab(creatures, food, dayNight, simulationTime, generation);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Genetics")) {
                m_currentTab = DashboardTab::Genetics;
                renderGeneticsTab(creatures);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Species")) {
                m_currentTab = DashboardTab::Species;
                m_speciesPanel.render();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Neural")) {
                m_currentTab = DashboardTab::Neural;
                renderNeuralTab(creatures);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("World")) {
                m_currentTab = DashboardTab::World;
                renderWorldTab(dayNight, camera);
                ImGui::EndTabItem();
            }

            // Phase 8: Inspect tab (shows when creature is selected)
            bool hasInspection = m_inspectionPanel.hasInspectedCreature();
            if (hasInspection) {
                ImGuiTabItemFlags inspectFlags = ImGuiTabItemFlags_None;
                // Auto-select inspect tab when creature is newly selected
                if (m_currentTab != DashboardTab::Inspect && m_selectionSystem.getSelectedCreature() != nullptr) {
                    // Only auto-switch if not already viewing inspection
                }

                if (ImGui::BeginTabItem("Inspect", nullptr, inspectFlags)) {
                    m_currentTab = DashboardTab::Inspect;
                    renderInspectTab();
                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

// ============================================================================
// Right Panel (Inspector)
// ============================================================================

void SimulationDashboard::renderRightPanel(
    std::vector<std::unique_ptr<Creature>>& creatures,
    Camera& camera
) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float panelWidth = 350.0f;  // Increased from 300 for better visibility

    ImGui::SetNextWindowPos(
        ImVec2(viewport->Pos.x + viewport->Size.x - panelWidth - 10, viewport->Pos.y + 10),
        ImGuiCond_FirstUseEver
    );
    ImGui::SetNextWindowSize(ImVec2(panelWidth, 700), ImGuiCond_FirstUseEver);  // Increased from 500

    if (ImGui::Begin("Creature List", nullptr, ImGuiWindowFlags_NoCollapse)) {
        // Phase 11: Always show creature list, selection shown in detail if clicked
        renderCreatureList(creatures);
    }
    ImGui::End();
}

// ============================================================================
// Status Bar
// ============================================================================

void SimulationDashboard::renderStatusBar(
    float simulationTime,
    uint32_t generation
) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float barHeight = 25.0f;

    ImGui::SetNextWindowPos(
        ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - barHeight)
    );
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, barHeight));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 4));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.10f, 0.95f));

    if (ImGui::Begin("##StatusBar", nullptr, flags)) {
        // FPS
        ImGui::Text("FPS: %.0f", m_metrics.fps);
        ImGui::SameLine(0, 20);

        // Population
        ImGui::Text("Creatures: %d", m_metrics.totalCreatures);
        ImGui::SameLine(0, 20);

        // Generation
        ImGui::Text("Gen: %u", generation);
        ImGui::SameLine(0, 20);

        // Time
        int minutes = (int)(simulationTime / 60.0f);
        int seconds = (int)(simulationTime) % 60;
        ImGui::Text("Time: %02d:%02d", minutes, seconds);
        ImGui::SameLine(0, 20);

        // Ecosystem health
        ImVec4 healthColor;
        if (m_metrics.ecosystemHealth >= 70) {
            healthColor = ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
        } else if (m_metrics.ecosystemHealth >= 40) {
            healthColor = ImVec4(0.8f, 0.8f, 0.3f, 1.0f);
        } else {
            healthColor = ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
        }
        ImGui::TextColored(healthColor, "Health: %.0f%%", m_metrics.ecosystemHealth);
        ImGui::SameLine(0, 20);

        // Pause state
        if (m_paused) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "PAUSED");
        } else {
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "RUNNING (%.1fx)", m_simulationSpeed);
        }

        // Right-aligned help hint
        float helpWidth = ImGui::CalcTextSize("F1 = Help | F3 = Toggle UI").x;
        ImGui::SameLine(viewport->Size.x - helpWidth - 20);
        ImGui::TextDisabled("F1 = Help | F3 = Toggle UI");
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

// ============================================================================
// Help Window
// ============================================================================

void SimulationDashboard::renderHelpWindow() {
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Help", &m_showHelp)) {
        ImGui::Text("Keyboard Shortcuts");
        ImGui::Separator();

        ImGui::BulletText("Space / P - Toggle pause");
        ImGui::BulletText("1-5 - Set simulation speed (0.5x to 8x)");
        ImGui::BulletText("F1 - Toggle this help window");
        ImGui::BulletText("F3 - Toggle UI panels");
        ImGui::BulletText("Escape - Deselect creature");

        ImGui::Spacing();
        ImGui::Text("Camera Controls");
        ImGui::Separator();

        ImGui::BulletText("WASD - Move camera");
        ImGui::BulletText("Q/E or Space/C - Move up/down");
        ImGui::BulletText("Shift - Move faster");
        ImGui::BulletText("Mouse - Look around");
        ImGui::BulletText("Click - Capture/release mouse");

        ImGui::Spacing();
        ImGui::Text("Tips");
        ImGui::Separator();

        ImGui::TextWrapped("Click on a creature in the creature list to inspect it. "
                          "Use the 'Follow' button to track a creature with the camera.");
    }
    ImGui::End();
}

// ============================================================================
// Overview Tab
// ============================================================================

void SimulationDashboard::renderOverviewTab(
    std::vector<std::unique_ptr<Creature>>& creatures,
    std::vector<std::unique_ptr<Food>>& food,
    DayNightCycle& dayNight,
    float simulationTime,
    uint32_t generation
) {
    // Performance section
    if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("FPS: %.1f", m_metrics.fps);
        ImGui::Text("Frame Time: %.2f ms (avg: %.2f)",
                   1000.0f / std::max(m_metrics.fps, 1.0f), m_metrics.avgFrameTime);

        // Frame time mini-graph
        if (m_showPerformance) {
            float frameTimeHistory[DashboardMetrics::FRAME_TIME_HISTORY_SIZE];
            for (int i = 0; i < DashboardMetrics::FRAME_TIME_HISTORY_SIZE; ++i) {
                frameTimeHistory[i] = m_metrics.frameTimeHistory[i];
            }
            ImGui::PlotLines("##FrameTime", frameTimeHistory,
                           DashboardMetrics::FRAME_TIME_HISTORY_SIZE,
                           m_metrics.frameTimeIndex, nullptr, 0.0f, 50.0f, ImVec2(-1, 40));
        }
    }

    // Simulation controls
    if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderSimulationControls();
    }

    // Population statistics
    if (ImGui::CollapsingHeader("Population", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderPopulationStats();
    }

    // Population graphs
    if (ImGui::CollapsingHeader("Population Graphs", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderPopulationGraphs();
    }

    // Ecosystem health
    if (ImGui::CollapsingHeader("Ecosystem Health")) {
        renderEcosystemHealth();
    }

    // Spawn controls
    if (ImGui::CollapsingHeader("Spawn Controls")) {
        renderSpawnControls();
    }

    // Chaos buttons
    if (ImGui::CollapsingHeader("Chaos Controls")) {
        renderChaosButtons();
    }
}

// ============================================================================
// Genetics Tab
// ============================================================================

void SimulationDashboard::renderGeneticsTab(
    std::vector<std::unique_ptr<Creature>>& creatures
) {
    // Genetic diversity overview
    if (ImGui::CollapsingHeader("Genetic Diversity", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderGeneticDiversityPanel();
    }

    // Trait distributions
    if (ImGui::CollapsingHeader("Trait Distributions", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderTraitDistribution(creatures);
    }

    // Fitness graphs
    if (ImGui::CollapsingHeader("Fitness Evolution", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderFitnessGraphs();
    }

    // Species breakdown
    if (ImGui::CollapsingHeader("Species Breakdown")) {
        renderSpeciesBreakdown();
    }
}

// ============================================================================
// Neural Tab
// ============================================================================

void SimulationDashboard::renderNeuralTab(
    std::vector<std::unique_ptr<Creature>>& creatures
) {
    renderNeuralNetworkPanel(creatures);
}

// ============================================================================
// World Tab
// ============================================================================

void SimulationDashboard::renderWorldTab(
    DayNightCycle& dayNight,
    Camera& camera
) {
    if (ImGui::CollapsingHeader("Day/Night Cycle", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderDayNightControls(dayNight);
    }

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderCameraControls(camera);
    }

    if (ImGui::CollapsingHeader("Audio", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderAudioSettings();
    }

    if (ImGui::CollapsingHeader("Graphics")) {
        renderGraphicsSettings();
    }
}

// ============================================================================
// Widget Implementations
// ============================================================================

void SimulationDashboard::renderSimulationControls() {
    // Pause control
    if (ImGui::Checkbox("Paused", &m_paused)) {
        // State changed
    }
    ImGui::SameLine();
    if (ImGui::Button("Step")) {
        m_stepOneFrame = true;
    }

    ImGui::Separator();

    // Speed controls
    ImGui::Text("Simulation Speed:");

    if (ImGui::Button("0.5x")) m_simulationSpeed = 0.5f;
    ImGui::SameLine();
    if (ImGui::Button("1x")) m_simulationSpeed = 1.0f;
    ImGui::SameLine();
    if (ImGui::Button("2x")) m_simulationSpeed = 2.0f;
    ImGui::SameLine();
    if (ImGui::Button("4x")) m_simulationSpeed = 4.0f;
    ImGui::SameLine();
    if (ImGui::Button("8x")) m_simulationSpeed = 8.0f;

    ImGui::SliderFloat("##Speed", &m_simulationSpeed, 0.1f, 10.0f, "%.1fx");
}

void SimulationDashboard::renderPopulationStats() {
    // Current counts with colored text
    ImGui::Text("Total Creatures: %d", m_metrics.totalCreatures);

    ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f),
                      "  Herbivores: %d", m_metrics.herbivoreCount);
    ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f),
                      "  Carnivores: %d", m_metrics.carnivoreCount);
    if (m_metrics.aquaticCount > 0) {
        ImGui::TextColored(ImVec4(0.3f, 0.6f, 0.9f, 1.0f),
                          "  Aquatic: %d", m_metrics.aquaticCount);
    }
    if (m_metrics.flyingCount > 0) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1.0f),
                          "  Flying: %d", m_metrics.flyingCount);
    }

    ImGui::Text("Food Sources: %d", m_metrics.foodCount);

    ImGui::Separator();

    // Herbivore/Carnivore ratio bar
    if (m_metrics.totalCreatures > 0) {
        float herbRatio = (float)m_metrics.herbivoreCount / m_metrics.totalCreatures;
        ImGui::ProgressBar(herbRatio, ImVec2(-1, 0), "H/C Ratio");
    }

    ImGui::Separator();

    // Birth/Death statistics
    ImGui::Text("Births/min: %d", m_metrics.birthsThisMinute);
    ImGui::Text("Deaths/min: %d", m_metrics.deathsThisMinute);
    ImGui::Text("Total Births: %d", m_metrics.totalBirths);
    ImGui::Text("Total Deaths: %d", m_metrics.totalDeaths);

    ImGui::Separator();

    // Generation stats
    ImGui::Text("Max Generation: %d", m_metrics.maxGeneration);
    ImGui::Text("Avg Generation: %.1f", m_metrics.avgGeneration);
}

void SimulationDashboard::renderEcosystemHealth() {
    // Health score with color
    ImVec4 healthColor;
    const char* healthStatus;
    if (m_metrics.ecosystemHealth >= 70) {
        healthColor = ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
        healthStatus = "Healthy";
    } else if (m_metrics.ecosystemHealth >= 40) {
        healthColor = ImVec4(0.8f, 0.8f, 0.3f, 1.0f);
        healthStatus = "Stressed";
    } else {
        healthColor = ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
        healthStatus = "Critical";
    }

    ImGui::TextColored(healthColor, "Ecosystem Health: %.0f%% (%s)",
                      m_metrics.ecosystemHealth, healthStatus);

    // Health bar
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, healthColor);
    ImGui::ProgressBar(m_metrics.ecosystemHealth / 100.0f, ImVec2(-1, 0));
    ImGui::PopStyleColor();

    ImGui::Separator();

    // Detailed indicators
    ImGui::Text("Predator-Prey Ratio: %.2f", m_metrics.predatorPreyRatio);
    ImGui::SameLine();
    if (m_metrics.predatorPreyRatio >= 0.2f && m_metrics.predatorPreyRatio <= 0.3f) {
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "(ideal)");
    } else if (m_metrics.predatorPreyRatio < 0.1f || m_metrics.predatorPreyRatio > 0.5f) {
        ImGui::TextColored(ImVec4(0.8f, 0.3f, 0.3f, 1.0f), "(poor)");
    }

    ImGui::Text("Food Availability: %.2f", m_metrics.foodAvailabilityRatio);
    ImGui::Text("Genetic Diversity: %.2f", m_metrics.geneticDiversity);
    ImGui::Text("Avg Creature Energy: %.1f", m_metrics.avgCreatureEnergy);
    ImGui::Text("Avg Creature Age: %.1fs", m_metrics.avgCreatureAge);
}

void SimulationDashboard::renderSpawnControls() {
    if (ImGui::Button("Spawn 10 Herbivores")) {
        if (m_spawnCreatureCallback) {
            m_spawnCreatureCallback(CreatureType::HERBIVORE, 10);
        }
    }

    if (ImGui::Button("Spawn 5 Carnivores")) {
        if (m_spawnCreatureCallback) {
            m_spawnCreatureCallback(CreatureType::CARNIVORE, 5);
        }
    }

    if (ImGui::Button("Spawn 20 Food")) {
        if (m_spawnFoodCallback) {
            m_spawnFoodCallback(20);
        }
    }
}

void SimulationDashboard::renderChaosButtons() {
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Warning: Destructive actions!");
    ImGui::Separator();

    if (ImGui::Button("Kill All Carnivores")) {
        if (m_killCallback) {
            m_killCallback(CreatureType::CARNIVORE);
        }
    }

    if (ImGui::Button("Kill All Herbivores")) {
        if (m_killCallback) {
            m_killCallback(CreatureType::HERBIVORE);
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Mass Extinction (50%)")) {
        if (m_massExtinctionCallback) {
            m_massExtinctionCallback(0.5f);
        }
    }

    if (ImGui::Button("Mass Extinction (90%)")) {
        if (m_massExtinctionCallback) {
            m_massExtinctionCallback(0.9f);
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Food Boom (x100)")) {
        if (m_spawnFoodCallback) {
            m_spawnFoodCallback(100);
        }
    }
}

void SimulationDashboard::renderPopulationGraphs() {
    if (!m_metrics.herbivoreHistory.empty() && ImPlot::BeginPlot("Population Over Time", ImVec2(-1, 200))) {
        ImPlot::SetupAxes("Time (s)", "Count");
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 200, ImPlotCond_Once);

        // Generate x-axis data (time in seconds)
        std::vector<float> xData(m_metrics.herbivoreHistory.size());
        for (size_t i = 0; i < xData.size(); ++i) {
            xData[i] = (float)i;
        }

        ImPlot::SetNextLineStyle(ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
        ImPlot::PlotLine("Herbivores", xData.data(), m_metrics.herbivoreHistory.data(),
                        (int)m_metrics.herbivoreHistory.size());

        ImPlot::SetNextLineStyle(ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImPlot::PlotLine("Carnivores", xData.data(), m_metrics.carnivoreHistory.data(),
                        (int)m_metrics.carnivoreHistory.size());

        ImPlot::SetNextLineStyle(ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
        ImPlot::PlotLine("Food", xData.data(), m_metrics.foodHistory.data(),
                        (int)m_metrics.foodHistory.size());

        ImPlot::EndPlot();
    }
}

void SimulationDashboard::renderFitnessGraphs() {
    if (!m_metrics.fitnessHistory.empty() && ImPlot::BeginPlot("Ecosystem Health Over Time", ImVec2(-1, 150))) {
        ImPlot::SetupAxes("Time (s)", "Health");
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1, ImPlotCond_Once);

        std::vector<float> xData(m_metrics.fitnessHistory.size());
        for (size_t i = 0; i < xData.size(); ++i) {
            xData[i] = (float)i;
        }

        ImPlot::SetNextLineStyle(ImVec4(0.5f, 0.8f, 0.5f, 1.0f));
        ImPlot::PlotLine("Health", xData.data(), m_metrics.fitnessHistory.data(),
                        (int)m_metrics.fitnessHistory.size());

        ImPlot::EndPlot();
    }
}

void SimulationDashboard::renderTraitDistribution(std::vector<std::unique_ptr<Creature>>& creatures) {
    if (creatures.empty()) {
        ImGui::Text("No creatures to analyze");
        return;
    }

    // Collect trait data
    std::vector<float> sizes, speeds;
    for (auto& c : creatures) {
        if (c && c->isAlive()) {
            sizes.push_back(c->getGenome().size);
            speeds.push_back(c->getGenome().speed);
        }
    }

    if (sizes.empty()) {
        ImGui::Text("No alive creatures");
        return;
    }

    // Size distribution histogram
    if (ImPlot::BeginPlot("Size Distribution", ImVec2(-1, 120))) {
        ImPlot::SetupAxes("Size", "Count");
        ImPlot::PlotHistogram("Size", sizes.data(), (int)sizes.size(), 15);
        ImPlot::EndPlot();
    }

    // Speed distribution histogram
    if (ImPlot::BeginPlot("Speed Distribution", ImVec2(-1, 120))) {
        ImPlot::SetupAxes("Speed", "Count");
        ImPlot::PlotHistogram("Speed", speeds.data(), (int)speeds.size(), 15);
        ImPlot::EndPlot();
    }
}

void SimulationDashboard::renderGeneticDiversityPanel() {
    ImGui::Text("Trait Statistics:");
    ImGui::Separator();

    ImGui::Text("Size:      avg=%.2f, std=%.2f", m_metrics.avgSize, m_metrics.stdSize);
    ImGui::Text("Speed:     avg=%.2f, std=%.2f", m_metrics.avgSpeed, m_metrics.stdSpeed);
    ImGui::Text("Vision:    avg=%.2f, std=%.2f", m_metrics.avgVision, m_metrics.stdVision);
    ImGui::Text("Efficiency: avg=%.2f, std=%.2f", m_metrics.avgEfficiency, m_metrics.stdEfficiency);

    ImGui::Separator();

    // Diversity score bar
    ImGui::Text("Genetic Diversity Score: %.2f", m_metrics.geneticDiversity);
    ImGui::ProgressBar(m_metrics.geneticDiversity, ImVec2(-1, 0));

    ImGui::TextWrapped("Higher diversity = more variation in traits = more evolutionary potential");
}

void SimulationDashboard::renderSpeciesBreakdown() {
    if (m_metrics.speciesList.empty()) {
        ImGui::Text("No species data available");
        return;
    }

    ImGui::Text("Species Count: %d", m_metrics.totalSpeciesCount);
    ImGui::Separator();

    for (const auto& species : m_metrics.speciesList) {
        ImGui::PushID(species.id);

        // Species color indicator
        ImGui::ColorButton("##color", ImVec4(species.avgColor.x, species.avgColor.y, species.avgColor.z, 1.0f),
                          ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(20, 20));
        ImGui::SameLine();

        // Species info
        ImGui::Text("%s: %d members", species.name.c_str(), species.memberCount);
        ImGui::Text("  Avg Fitness: %.2f, Gen: %d", species.avgFitness, species.generation);
        ImGui::Text("  Avg Size: %.2f, Speed: %.2f", species.avgSize, species.avgSpeed);

        ImGui::Separator();
        ImGui::PopID();
    }
}

void SimulationDashboard::renderNeuralNetworkPanel(std::vector<std::unique_ptr<Creature>>& creatures) {
    // Find best creature for neural network display
    Creature* bestCreature = nullptr;
    float bestFitness = -1.0f;

    for (auto& c : creatures) {
        if (c && c->isAlive() && c->getFitness() > bestFitness) {
            bestFitness = c->getFitness();
            bestCreature = c.get();
        }
    }

    if (bestCreature == nullptr) {
        ImGui::Text("No creatures available");
        return;
    }

    const char* typeStr = bestCreature->getType() == CreatureType::HERBIVORE ? "Herbivore" : "Carnivore";
    ImGui::Text("Best Creature: %s #%d", typeStr, bestCreature->getID());
    ImGui::Text("Generation: %d, Fitness: %.2f", bestCreature->getGeneration(), bestFitness);

    ImGui::Separator();

    // Neural network inputs/outputs
    if (ImGui::CollapsingHeader("Brain Inputs/Outputs", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderBrainInputsOutputs(bestCreature);
    }

    // Simple brain stats (without full neural network access)
    if (ImGui::CollapsingHeader("Brain Statistics")) {
        ImGui::Text("Brain complexity information requires");
        ImGui::Text("direct neural network access.");
        ImGui::Text("See creature's behavior for AI insights.");
    }
}

void SimulationDashboard::renderBrainInputsOutputs(Creature* creature) {
    if (!creature) return;

    const auto& sensory = creature->getSensory();
    const auto& genome = creature->getGenome();

    ImGui::Text("Sensory Inputs:");
    ImGui::Text("  Vision Range: %.1f", genome.visionRange);
    ImGui::Text("  Current Fear: %.2f", creature->getFear());
    ImGui::Text("  Energy: %.1f / 200", creature->getEnergy());

    ImGui::Separator();

    ImGui::Text("Current State:");
    ImGui::Text("  Speed: %.2f", glm::length(creature->getVelocity()));
    ImGui::Text("  Age: %.1fs", creature->getAge());
    ImGui::Text("  Kill Count: %d", creature->getKillCount());

    if (creature->isBeingHunted()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "  Being Hunted!");
    }
}

void SimulationDashboard::renderCreatureInspector(std::vector<std::unique_ptr<Creature>>& creatures) {
    if (m_selectedCreature == nullptr) {
        ImGui::TextWrapped("Select a creature from the list below to inspect it.");
        ImGui::Separator();

        // Show top 10 creatures by fitness
        renderCreatureList(creatures);
        return;
    }

    // Check if creature is still alive
    if (!m_selectedCreature->isAlive()) {
        ImGui::TextColored(ImVec4(0.8f, 0.3f, 0.3f, 1.0f), "Selected creature has died");
        if (ImGui::Button("Clear Selection")) {
            m_selectedCreature = nullptr;
            m_selectedCreatureId = -1;
        }
        ImGui::Separator();
        renderCreatureList(creatures);
        return;
    }

    Creature* c = m_selectedCreature;

    // Basic info
    const char* typeStr = c->getType() == CreatureType::HERBIVORE ? "Herbivore" :
                         c->getType() == CreatureType::CARNIVORE ? "Carnivore" :
                         c->getType() == CreatureType::AQUATIC ? "Aquatic" : "Flying";

    ImGui::Text("ID: %d", c->getID());
    ImGui::SameLine();
    ImGui::Text("Type: %s", typeStr);
    ImGui::Text("Generation: %d", c->getGeneration());

    ImGui::Separator();

    // Vital stats with progress bars
    float energyRatio = c->getEnergy() / 200.0f;
    ImGui::Text("Energy:");
    ImGui::SameLine();
    ImGui::ProgressBar(energyRatio, ImVec2(-1, 0), nullptr);

    ImGui::Text("Age: %.1f seconds", c->getAge());
    ImGui::Text("Fitness: %.2f", c->getFitness());

    ImGui::Separator();

    // Position and movement
    const glm::vec3& pos = c->getPosition();
    ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
    ImGui::Text("Speed: %.2f", glm::length(c->getVelocity()));

    ImGui::Separator();

    // Genome info
    if (ImGui::CollapsingHeader("Genome", ImGuiTreeNodeFlags_DefaultOpen)) {
        const Genome& g = c->getGenome();
        ImGui::Text("Size: %.2f", g.size);
        ImGui::Text("Speed Gene: %.2f", g.speed);
        ImGui::Text("Vision Range: %.2f", g.visionRange);
        ImGui::Text("Efficiency: %.2f", g.efficiency);

        ImGui::ColorEdit3("Color", (float*)&g.color, ImGuiColorEditFlags_NoInputs);
    }

    // Combat stats (for carnivores)
    if (c->getType() == CreatureType::CARNIVORE) {
        if (ImGui::CollapsingHeader("Combat")) {
            ImGui::Text("Kill Count: %d", c->getKillCount());
            ImGui::Text("Fear Level: %.2f", c->getFear());
        }
    }

    ImGui::Separator();

    // Action buttons
    if (ImGui::Button("Follow Camera")) {
        if (m_followCreatureCallback) {
            m_followCreatureCallback(c);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Deselect")) {
        m_selectedCreature = nullptr;
        m_selectedCreatureId = -1;
    }

    ImGui::Separator();

    // Creature list
    if (ImGui::CollapsingHeader("Other Creatures")) {
        renderCreatureList(creatures);
    }
}

void SimulationDashboard::renderCreatureList(std::vector<std::unique_ptr<Creature>>& creatures) {
    // Phase 11: Enhanced creature list with filtering, search, and better display

    // Header with stats
    int totalCount = 0;
    for (auto& c : creatures) {
        if (c && c->isAlive()) totalCount++;
    }

    ImGui::Text("Creatures: %d", totalCount);
    ImGui::Separator();

    // Search box
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##search", "Search by name...", m_creatureSearchBuffer, sizeof(m_creatureSearchBuffer));

    // Filter toggles (compact horizontal layout)
    ImGui::Text("Filter:");
    ImGui::SameLine();
    ImGui::Checkbox("H##herb", &m_filterByHerbivore);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Herbivores");
    ImGui::SameLine();
    ImGui::Checkbox("C##carn", &m_filterByCarnivore);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Carnivores");
    ImGui::SameLine();
    ImGui::Checkbox("A##aqua", &m_filterByAquatic);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Aquatic");
    ImGui::SameLine();
    ImGui::Checkbox("F##fly", &m_filterByFlying);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Flying");

    // Sort mode
    ImGui::Text("Sort:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    const char* sortModes[] = { "Fitness", "Name", "Distance", "Energy", "Age" };
    ImGui::Combo("##sort", &m_sortMode, sortModes, 5);

    ImGui::Separator();

    // Filter and collect creatures
    std::vector<Creature*> filtered;
    std::string searchStr = m_creatureSearchBuffer;
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

    for (auto& c : creatures) {
        if (!c || !c->isAlive()) continue;

        // Type filter
        CreatureType type = c->getType();
        if (type == CreatureType::HERBIVORE && !m_filterByHerbivore) continue;
        if (type == CreatureType::CARNIVORE && !m_filterByCarnivore) continue;
        if (type == CreatureType::AQUATIC && !m_filterByAquatic) continue;
        if (type == CreatureType::FLYING && !m_filterByFlying) continue;

        // Search filter
        if (searchStr.length() > 0) {
            std::string name = c->getSpeciesDisplayName();
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name.find(searchStr) == std::string::npos) continue;
        }

        filtered.push_back(c.get());
    }

    // Sort filtered list
    if (m_sortMode == 0) {
        // Fitness (descending)
        std::sort(filtered.begin(), filtered.end(),
                 [](Creature* a, Creature* b) { return a->getFitness() > b->getFitness(); });
    } else if (m_sortMode == 1) {
        // Name (ascending)
        std::sort(filtered.begin(), filtered.end(),
                 [](Creature* a, Creature* b) { return a->getSpeciesDisplayName() < b->getSpeciesDisplayName(); });
    } else if (m_sortMode == 2) {
        // Distance (ascending, requires camera - placeholder for now)
        std::sort(filtered.begin(), filtered.end(),
                 [](Creature* a, Creature* b) { return a->getID() < b->getID(); });
    } else if (m_sortMode == 3) {
        // Energy (descending)
        std::sort(filtered.begin(), filtered.end(),
                 [](Creature* a, Creature* b) { return a->getEnergy() > b->getEnergy(); });
    } else if (m_sortMode == 4) {
        // Age (descending)
        std::sort(filtered.begin(), filtered.end(),
                 [](Creature* a, Creature* b) { return a->getAge() > b->getAge(); });
    }

    // Display count
    ImGui::Text("Showing: %d / %d", (int)filtered.size(), totalCount);

    // Scrollable list (fill remaining space)
    ImGui::BeginChild("CreatureListScroll", ImVec2(-1, -1), true);

    for (Creature* c : filtered) {
        ImGui::PushID(c->getID());

        bool isSelected = (c == m_selectedCreature);

        // Type icon with color
        const char* typeIcon;
        ImVec4 typeColor;
        switch (c->getType()) {
            case CreatureType::HERBIVORE:
                typeIcon = "[H]";
                typeColor = ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
                break;
            case CreatureType::CARNIVORE:
                typeIcon = "[C]";
                typeColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
                break;
            case CreatureType::AQUATIC:
                typeIcon = "[A]";
                typeColor = ImVec4(0.3f, 0.6f, 0.9f, 1.0f);
                break;
            case CreatureType::FLYING:
                typeIcon = "[F]";
                typeColor = ImVec4(0.7f, 0.7f, 0.3f, 1.0f);
                break;
            default:
                typeIcon = "[?]";
                typeColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        }

        // Species name (or ID if no name)
        std::string displayName = c->getSpeciesDisplayName();
        if (displayName.empty()) {
            displayName = "Creature #" + std::to_string(c->getID());
        }

        // Build label with more info
        char label[256];
        snprintf(label, sizeof(label), "%s %s##%d", typeIcon, displayName.c_str(), c->getID());

        // Selectable item
        if (ImGui::Selectable(label, isSelected)) {
            m_selectedCreature = c;
            m_selectedCreatureId = c->getID();

            // Update inspection panel and selection system
            m_inspectionPanel.setInspectedCreature(c);
            m_selectionSystem.setSelectedCreature(c);
        }

        // Show tooltip with detailed info on hover
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("ID: %d", c->getID());
            ImGui::TextColored(typeColor, "Type: %s", typeIcon);
            ImGui::Text("Energy: %.0f / %.0f", c->getEnergy(), c->getMaxEnergy());
            ImGui::Text("Age: %.1fs", c->getAge());
            ImGui::Text("Fitness: %.2f", c->getFitness());
            ImGui::Text("Generation: %d", c->getGeneration());
            ImGui::Separator();
            ImGui::Text("Click to select and inspect");
            ImGui::EndTooltip();
        }

        // Additional stats on same line (compact)
        if (isSelected) {
            ImGui::Indent(20);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "ID: %d | Gen: %d", c->getID(), c->getGeneration());
            float energyRatio = c->getEnergy() / c->getMaxEnergy();
            ImVec4 energyColor = energyRatio > 0.6f ? ImVec4(0.3f, 0.8f, 0.3f, 1.0f) :
                                energyRatio > 0.3f ? ImVec4(0.8f, 0.8f, 0.3f, 1.0f) :
                                                     ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
            ImGui::TextColored(energyColor, "Energy: %.0f", c->getEnergy());
            ImGui::SameLine();
            ImGui::Text("| Fit: %.1f", c->getFitness());

            // Action buttons for selected creature
            if (ImGui::Button("Focus Camera", ImVec2(-1, 0))) {
                if (m_cameraController) {
                    m_cameraController->startInspect(c);
                }
            }

            ImGui::Unindent(20);
            ImGui::Separator();
        }

        ImGui::PopID();
    }

    ImGui::EndChild();
}

void SimulationDashboard::renderDayNightControls(DayNightCycle& dayNight) {
    ImGui::Text("Time: %s", dayNight.GetTimeOfDayString());

    ImGui::SliderFloat("Time of Day", &dayNight.dayTime, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Day Length (s)", &dayNight.dayLengthSeconds, 30.0f, 600.0f, "%.0f");
    ImGui::Checkbox("Pause Cycle", &dayNight.paused);

    ImGui::Separator();

    ImGui::Text("Quick Set:");
    if (ImGui::Button("Dawn")) dayNight.dayTime = 0.22f;
    ImGui::SameLine();
    if (ImGui::Button("Noon")) dayNight.dayTime = 0.5f;
    ImGui::SameLine();
    if (ImGui::Button("Dusk")) dayNight.dayTime = 0.75f;
    ImGui::SameLine();
    if (ImGui::Button("Night")) dayNight.dayTime = 0.0f;

    ImGui::Separator();

    // Sky colors preview
    SkyColors sky = dayNight.GetSkyColors();
    ImGui::Text("Sky Colors:");
    ImGui::ColorButton("##skyTop", ImVec4(sky.skyTop.x, sky.skyTop.y, sky.skyTop.z, 1.0f), 0, ImVec2(30, 20));
    ImGui::SameLine();
    ImGui::Text("Top");
    ImGui::SameLine();
    ImGui::ColorButton("##skyHorizon", ImVec4(sky.skyHorizon.x, sky.skyHorizon.y, sky.skyHorizon.z, 1.0f), 0, ImVec2(30, 20));
    ImGui::SameLine();
    ImGui::Text("Horizon");

    ImGui::Text("Sun Intensity: %.2f", sky.sunIntensity);
    ImGui::Text("Star Visibility: %.2f", dayNight.GetStarVisibility());
}

void SimulationDashboard::renderCameraControls(Camera& camera) {
    ImGui::Text("Position: (%.1f, %.1f, %.1f)",
               camera.Position.x, camera.Position.y, camera.Position.z);
    ImGui::Text("Yaw: %.2f deg", camera.Yaw);
    ImGui::Text("Pitch: %.2f deg", camera.Pitch);
    ImGui::Text("FOV: %.1f deg", camera.Zoom);

    ImGui::Separator();

    ImGui::SliderFloat("Sensitivity", &camera.MouseSensitivity, 0.01f, 1.0f, "%.2f");
    ImGui::SliderFloat("Move Speed", &camera.MovementSpeed, 10.0f, 200.0f, "%.0f");

    if (ImGui::Button("Reset Camera")) {
        camera.Position = glm::vec3(0.0f, 50.0f, 100.0f);
        camera.Yaw = YAW;
        camera.Pitch = PITCH;
    }
}

void SimulationDashboard::renderGraphicsSettings() {
    ImGui::Checkbox("Show Nametags", &showNametags);
    ImGui::Checkbox("Show Trees", &showTrees);
    ImGui::SliderFloat("Nametag Distance", &nametagMaxDistance, 10.0f, 100.0f, "%.0f");
}

void SimulationDashboard::renderAudioSettings() {
    if (!m_audioManager) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Audio system not available");
        return;
    }

    // Master volume control
    ImGui::Text("Volume Controls");
    ImGui::Separator();

    float masterVolume = m_audioManager->getMasterVolume();
    if (ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 1.0f, "%.2f")) {
        m_audioManager->setMasterVolume(masterVolume);
    }

    ImGui::Spacing();

    // Category-specific volume controls
    ImGui::Text("Category Volumes:");

    // Creature sounds
    float creatureVolume = m_audioManager->getCategoryVolume(Forge::SoundCategory::CREATURES);
    if (ImGui::SliderFloat("Creatures##vol", &creatureVolume, 0.0f, 1.0f, "%.2f")) {
        m_audioManager->setCategoryVolume(Forge::SoundCategory::CREATURES, creatureVolume);
        if (m_creatureVoices) {
            m_creatureVoices->setCreatureVolume(creatureVolume);
        }
    }

    // Ambient sounds
    float ambientVolume = m_audioManager->getCategoryVolume(Forge::SoundCategory::AMBIENT);
    if (ImGui::SliderFloat("Ambient##vol", &ambientVolume, 0.0f, 1.0f, "%.2f")) {
        m_audioManager->setCategoryVolume(Forge::SoundCategory::AMBIENT, ambientVolume);
        if (m_ambientSoundscape) {
            m_ambientSoundscape->setAmbientVolume(ambientVolume);
        }
    }

    // Weather sounds
    float weatherVolume = m_audioManager->getCategoryVolume(Forge::SoundCategory::WEATHER);
    if (ImGui::SliderFloat("Weather##vol", &weatherVolume, 0.0f, 1.0f, "%.2f")) {
        m_audioManager->setCategoryVolume(Forge::SoundCategory::WEATHER, weatherVolume);
        if (m_ambientSoundscape) {
            m_ambientSoundscape->setWeatherVolume(weatherVolume);
        }
    }

    // UI sounds
    float uiVolume = m_audioManager->getCategoryVolume(Forge::SoundCategory::UI);
    if (ImGui::SliderFloat("UI##vol", &uiVolume, 0.0f, 1.0f, "%.2f")) {
        m_audioManager->setCategoryVolume(Forge::SoundCategory::UI, uiVolume);
    }

    ImGui::Separator();

    // Enable/disable toggles
    ImGui::Text("Enable/Disable:");

    if (m_creatureVoices) {
        bool creaturesEnabled = m_creatureVoices->isEnabled();
        if (ImGui::Checkbox("Creature Sounds", &creaturesEnabled)) {
            m_creatureVoices->setEnabled(creaturesEnabled);
        }
    }

    if (m_ambientSoundscape) {
        bool ambientEnabled = m_ambientSoundscape->isEnabled();
        if (ImGui::Checkbox("Ambient Sounds", &ambientEnabled)) {
            m_ambientSoundscape->setEnabled(ambientEnabled);
        }
    }

    ImGui::Separator();

    // Audio status display
    ImGui::Text("Audio Status:");

    int activeVoices = m_audioManager->getActiveVoiceCount();
    int maxVoices = m_audioManager->getMaxVoices();
    ImGui::Text("Active Voices: %d / %d", activeVoices, maxVoices);

    // Voice usage bar
    float voiceUsage = maxVoices > 0 ? (float)activeVoices / maxVoices : 0.0f;
    ImVec4 voiceColor = voiceUsage < 0.7f ? ImVec4(0.3f, 0.8f, 0.3f, 1.0f) :
                        voiceUsage < 0.9f ? ImVec4(0.8f, 0.8f, 0.3f, 1.0f) :
                                           ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, voiceColor);
    ImGui::ProgressBar(voiceUsage, ImVec2(-1, 0), "Voice Pool");
    ImGui::PopStyleColor();

    // Underwater indicator
    if (m_audioManager->isUnderwaterMode()) {
        ImGui::TextColored(ImVec4(0.3f, 0.6f, 0.9f, 1.0f), "Underwater Audio Active");
    }

    // Current biome ambient
    if (m_ambientSoundscape) {
        ImGui::Text("Current Biome: %s", m_ambientSoundscape->getCurrentBiomeName());

        // Show active ambient layers
        auto layers = m_ambientSoundscape->getActiveLayers();
        if (!layers.empty()) {
            ImGui::Text("Active Layers:");
            for (const auto& layer : layers) {
                if (layer.active) {
                    ImGui::Text("  - %s (%.0f%%)", layer.name, layer.volume * 100.0f);
                }
            }
        }
    }
}

// ============================================================================
// Helper Functions
// ============================================================================

Creature* SimulationDashboard::findCreatureById(std::vector<std::unique_ptr<Creature>>& creatures, int id) {
    for (auto& c : creatures) {
        if (c && c->getID() == id && c->isAlive()) {
            return c.get();
        }
    }
    return nullptr;
}

// ============================================================================
// Phase 8: Inspect Tab and Selection Integration
// ============================================================================

void SimulationDashboard::renderInspectTab() {
    // The inspection panel handles its own rendering
    // Here we just provide some context in the tab
    if (!m_inspectionPanel.hasInspectedCreature()) {
        ImGui::TextWrapped("No creature selected for inspection.");
        ImGui::TextWrapped("Click on a creature in the world to inspect it.");
        return;
    }

    Creature* c = m_inspectionPanel.getInspectedCreature();
    if (!c) return;

    // Show a summary in the tab
    ImGui::Text("Currently Inspecting:");
    ImGui::Separator();

    // Species name
    const std::string& name = c->getSpeciesDisplayName();
    ImGui::Text("Species: %s", name.empty() ? "Unknown" : name.c_str());

    // Type and ID
    const char* typeStr = c->getType() == CreatureType::HERBIVORE ? "Herbivore" :
                          c->getType() == CreatureType::CARNIVORE ? "Carnivore" :
                          c->getType() == CreatureType::AQUATIC ? "Aquatic" : "Flying";
    ImGui::Text("Type: %s  ID: #%d", typeStr, c->getID());

    // Quick stats
    ImGui::Separator();
    ImGui::Text("Energy: %.0f / %.0f", c->getEnergy(), c->getMaxEnergy());
    ImGui::Text("Fitness: %.2f", c->getFitness());
    ImGui::Text("Generation: %d", c->getGeneration());

    // Camera controls
    ImGui::Separator();
    ImGui::Text("Camera:");
    ImGui::SameLine();
    if (ImGui::Button("Focus")) {
        if (m_cameraController) {
            m_cameraController->startInspect(c);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Track")) {
        if (m_cameraController) {
            m_cameraController->startFollowTarget(c);
            m_cameraController->lockTarget(true);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Release")) {
        if (m_cameraController) {
            m_cameraController->exitInspect();
            m_cameraController->lockTarget(false);
            m_cameraController->setMode(Forge::CameraMode::FREE, true);
        }
    }

    ImGui::Separator();

    // Button to open full inspection panel
    if (ImGui::Button("Open Detail Panel")) {
        m_inspectionPanel.setVisible(true);
    }

    ImGui::SameLine();
    if (ImGui::Button("Stop Inspecting")) {
        m_inspectionPanel.clearInspection();
        m_selectionSystem.clearSelection();
    }

    // Show position info
    ImGui::Separator();
    const glm::vec3& pos = c->getPosition();
    ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
    float speed = glm::length(c->getVelocity());
    ImGui::Text("Speed: %.2f", speed);
}

void SimulationDashboard::updateSelection(const Camera& camera, Forge::CreatureManager& creatures,
                                          float screenWidth, float screenHeight) {
    // Update selection system
    bool clicked = m_selectionSystem.update(camera, creatures, screenWidth, screenHeight);

    // If a creature was clicked, update the inspection panel
    if (clicked) {
        Creature* selected = m_selectionSystem.getSelectedCreature();
        if (selected) {
            m_inspectionPanel.setInspectedCreature(selected);
            m_selectedCreature = selected;
            m_selectedCreatureId = selected->getID();

            // Optionally auto-focus camera
            if (m_cameraController) {
                m_cameraController->startInspect(selected);
            }
        }
    }

    // Handle selection changed callback integration
    // (This is already set up via setOnSelectionChanged if needed externally)

    // Validate current inspection target
    if (m_inspectionPanel.hasInspectedCreature()) {
        Creature* inspected = m_inspectionPanel.getInspectedCreature();
        if (!inspected || !inspected->isAlive()) {
            // Creature died - clear selection
            m_inspectionPanel.clearInspection();
            m_selectionSystem.clearSelection();
            m_selectedCreature = nullptr;
            m_selectedCreatureId = -1;
        }
    }
}

} // namespace ui
