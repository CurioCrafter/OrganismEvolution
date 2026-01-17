#pragma once

#include "imgui.h"
#include "CreatureSpawnPanel.h"
#include "EvolutionControlPanel.h"
#include "EnvironmentControlPanel.h"
#include "ScenarioPresets.h"
#include "StatisticsPanel.h"
#include "../entities/Creature.h"
#include "../graphics/Camera.h"
#include <functional>
#include <memory>
#include <vector>

// Forward declarations
class DayNightCycle;
class Food;

namespace ui {

// ============================================================================
// Master Control Panel
// ============================================================================
// Central UI hub that integrates all control panels and provides:
// - Play/Pause/Step controls
// - Speed multiplier
// - Quick access to all sub-panels
// - Unified callback management
// - Dockable window organization

class MasterControlPanel {
public:
    MasterControlPanel();
    ~MasterControlPanel() = default;

    // Initialize all sub-panels
    bool initialize();
    void shutdown();

    // Main render function - renders all visible panels
    void render(
        std::vector<std::unique_ptr<Creature>>& creatures,
        std::vector<std::unique_ptr<Food>>& food,
        DayNightCycle& dayNight,
        Camera& camera,
        float simulationTime,
        uint32_t generation,
        float deltaTime
    );

    // Handle keyboard input for all panels
    void handleInput();

    // Master simulation controls
    bool isPaused() const { return m_paused; }
    void setPaused(bool paused) { m_paused = paused; }
    void togglePause() { m_paused = !m_paused; }
    bool shouldStepFrame() { bool step = m_stepOneFrame; m_stepOneFrame = false; return step; }

    float getSimulationSpeed() const { return m_simulationSpeed; }
    void setSimulationSpeed(float speed) { m_simulationSpeed = speed; }

    // Panel visibility toggles
    void toggleMainPanel() { m_showMainPanel = !m_showMainPanel; }
    void toggleSpawnPanel() { m_spawnPanel.toggleVisible(); }
    void toggleEvolutionPanel() { m_evolutionPanel.toggleVisible(); }
    void toggleEnvironmentPanel() { m_environmentPanel.toggleVisible(); }
    void toggleScenarioPanel() { m_scenarioPanel.toggleVisible(); }
    void toggleStatisticsPanel() { m_statisticsPanel.toggleVisible(); }
    void toggleDebugPanel() { m_showDebugPanel = !m_showDebugPanel; }

    // Access sub-panels for external configuration
    CreatureSpawnPanel& getSpawnPanel() { return m_spawnPanel; }
    EvolutionControlPanel& getEvolutionPanel() { return m_evolutionPanel; }
    EnvironmentControlPanel& getEnvironmentPanel() { return m_environmentPanel; }
    ScenarioPresetsPanel& getScenarioPanel() { return m_scenarioPanel; }
    StatisticsPanel& getStatisticsPanel() { return m_statisticsPanel; }

    // Callbacks
    using SaveCallback = std::function<void()>;
    using LoadCallback = std::function<void()>;
    using ResetCallback = std::function<void()>;
    using FollowCreatureCallback = std::function<void(Creature*)>;

    void setSaveCallback(SaveCallback cb) { m_saveCallback = cb; }
    void setLoadCallback(LoadCallback cb) { m_loadCallback = cb; }
    void setResetCallback(ResetCallback cb) { m_resetCallback = cb; }
    void setFollowCreatureCallback(FollowCreatureCallback cb) { m_followCreatureCallback = cb; }

    // Selected creature management
    void setSelectedCreature(Creature* creature);
    Creature* getSelectedCreature() const { return m_selectedCreature; }

    // Debug visualization flags
    bool showCreatureVision = false;
    bool showSpatialGrid = false;
    bool showPathfinding = false;
    bool showCreatureStats = false;
    bool showTerrainWireframe = false;
    bool showBoundingBoxes = false;
    bool showNametags = true;
    float nametagMaxDistance = 50.0f;

    // Camera mode
    enum class CameraMode {
        Free,
        FollowCreature,
        BirdsEye,
        Underwater
    };
    CameraMode cameraMode = CameraMode::Free;
    float cameraSpeed = 50.0f;

private:
    bool m_initialized = false;

    // Simulation state
    bool m_paused = false;
    float m_simulationSpeed = 1.0f;
    bool m_stepOneFrame = false;

    // Panel visibility
    bool m_showMainPanel = true;
    bool m_showDebugPanel = false;
    bool m_showHelp = false;
    bool m_showStatusBar = true;

    // Sub-panels
    CreatureSpawnPanel m_spawnPanel;
    EvolutionControlPanel m_evolutionPanel;
    EnvironmentControlPanel m_environmentPanel;
    ScenarioPresetsPanel m_scenarioPanel;
    StatisticsPanel m_statisticsPanel;

    // Selected creature
    Creature* m_selectedCreature = nullptr;

    // Callbacks
    SaveCallback m_saveCallback;
    LoadCallback m_loadCallback;
    ResetCallback m_resetCallback;
    FollowCreatureCallback m_followCreatureCallback;

    // Render functions
    void renderMainPanel();
    void renderSimulationControls();
    void renderPanelToggles();
    void renderQuickStats(int totalCreatures, int generation);
    void renderDebugPanel(Camera& camera);
    void renderCameraControls(Camera& camera);
    void renderVisualizationToggles();
    void renderStatusBar(float simulationTime, uint32_t generation, int totalCreatures);
    void renderHelpWindow();
    void renderMenuBar();
};

// ============================================================================
// Implementation
// ============================================================================

inline MasterControlPanel::MasterControlPanel() = default;

inline bool MasterControlPanel::initialize() {
    if (m_initialized) return true;

    // Initialize sub-panels (they handle their own initialization)
    m_initialized = true;
    return true;
}

inline void MasterControlPanel::shutdown() {
    m_initialized = false;
}

inline void MasterControlPanel::render(
    std::vector<std::unique_ptr<Creature>>& creatures,
    std::vector<std::unique_ptr<Food>>& food,
    DayNightCycle& dayNight,
    Camera& camera,
    float simulationTime,
    uint32_t generation,
    float deltaTime
) {
    if (!m_initialized) return;

    // Update statistics
    m_statisticsPanel.update(creatures, static_cast<int>(food.size()), simulationTime, deltaTime);

    // Update environment panel display
    m_environmentPanel.setCurrentTimeOfDay(dayNight.dayTime);

    // Render menu bar
    renderMenuBar();

    // Render main control panel
    if (m_showMainPanel) {
        renderMainPanel();
    }

    // Render sub-panels
    m_spawnPanel.render();
    m_evolutionPanel.render();
    m_environmentPanel.render();
    m_scenarioPanel.render();
    m_statisticsPanel.render();

    // Render debug panel
    if (m_showDebugPanel) {
        renderDebugPanel(camera);
    }

    // Render status bar
    if (m_showStatusBar) {
        renderStatusBar(simulationTime, generation, static_cast<int>(creatures.size()));
    }

    // Render help window
    if (m_showHelp) {
        renderHelpWindow();
    }
}

inline void MasterControlPanel::handleInput() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) return;

    // Space or P to toggle pause
    if (ImGui::IsKeyPressed(ImGuiKey_Space) || ImGui::IsKeyPressed(ImGuiKey_P)) {
        m_paused = !m_paused;
    }

    // Period for step
    if (ImGui::IsKeyPressed(ImGuiKey_Period)) {
        m_stepOneFrame = true;
    }

    // F1 for help
    if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
        m_showHelp = !m_showHelp;
    }

    // F2 for main panel
    if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
        m_showMainPanel = !m_showMainPanel;
    }

    // F3 for debug
    if (ImGui::IsKeyPressed(ImGuiKey_F3)) {
        m_showDebugPanel = !m_showDebugPanel;
    }

    // F4 for statistics
    if (ImGui::IsKeyPressed(ImGuiKey_F4)) {
        m_statisticsPanel.toggleVisible();
    }

    // Number keys for speed
    if (ImGui::IsKeyPressed(ImGuiKey_1)) m_simulationSpeed = 0.25f;
    if (ImGui::IsKeyPressed(ImGuiKey_2)) m_simulationSpeed = 0.5f;
    if (ImGui::IsKeyPressed(ImGuiKey_3)) m_simulationSpeed = 1.0f;
    if (ImGui::IsKeyPressed(ImGuiKey_4)) m_simulationSpeed = 2.0f;
    if (ImGui::IsKeyPressed(ImGuiKey_5)) m_simulationSpeed = 4.0f;
    if (ImGui::IsKeyPressed(ImGuiKey_6)) m_simulationSpeed = 8.0f;

    // Escape to deselect
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_selectedCreature = nullptr;
        m_spawnPanel.setSelectedCreature(nullptr);
    }
}

inline void MasterControlPanel::setSelectedCreature(Creature* creature) {
    m_selectedCreature = creature;
    m_spawnPanel.setSelectedCreature(creature);
}

inline void MasterControlPanel::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Simulation", "Ctrl+N")) {
                if (m_resetCallback) m_resetCallback();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                if (m_saveCallback) m_saveCallback();
            }
            if (ImGui::MenuItem("Load", "Ctrl+L")) {
                if (m_loadCallback) m_loadCallback();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                // Handle exit
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Main Panel", "F2", &m_showMainPanel);
            ImGui::MenuItem("Debug Panel", "F3", &m_showDebugPanel);

            bool statsVisible = m_statisticsPanel.isVisible();
            if (ImGui::MenuItem("Statistics", "F4", &statsVisible)) {
                m_statisticsPanel.setVisible(statsVisible);
            }

            bool spawnVisible = m_spawnPanel.isVisible();
            if (ImGui::MenuItem("Spawn Panel", nullptr, &spawnVisible)) {
                m_spawnPanel.setVisible(spawnVisible);
            }

            bool evolVisible = m_evolutionPanel.isVisible();
            if (ImGui::MenuItem("Evolution Panel", nullptr, &evolVisible)) {
                m_evolutionPanel.setVisible(evolVisible);
            }

            bool envVisible = m_environmentPanel.isVisible();
            if (ImGui::MenuItem("Environment Panel", nullptr, &envVisible)) {
                m_environmentPanel.setVisible(envVisible);
            }

            bool scenarioVisible = m_scenarioPanel.isVisible();
            if (ImGui::MenuItem("Scenarios", nullptr, &scenarioVisible)) {
                m_scenarioPanel.setVisible(scenarioVisible);
            }

            ImGui::Separator();
            ImGui::MenuItem("Status Bar", nullptr, &m_showStatusBar);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Simulation")) {
            if (ImGui::MenuItem(m_paused ? "Resume" : "Pause", "Space")) {
                m_paused = !m_paused;
            }
            if (ImGui::MenuItem("Step Frame", ".")) {
                m_stepOneFrame = true;
            }
            ImGui::Separator();

            if (ImGui::BeginMenu("Speed")) {
                if (ImGui::MenuItem("0.25x", "1")) m_simulationSpeed = 0.25f;
                if (ImGui::MenuItem("0.5x", "2")) m_simulationSpeed = 0.5f;
                if (ImGui::MenuItem("1x", "3")) m_simulationSpeed = 1.0f;
                if (ImGui::MenuItem("2x", "4")) m_simulationSpeed = 2.0f;
                if (ImGui::MenuItem("4x", "5")) m_simulationSpeed = 4.0f;
                if (ImGui::MenuItem("8x", "6")) m_simulationSpeed = 8.0f;
                ImGui::EndMenu();
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Reset Simulation")) {
                if (m_resetCallback) m_resetCallback();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("Show Help", "F1", &m_showHelp);
            ImGui::Separator();
            if (ImGui::MenuItem("About")) {
                // Show about dialog
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

inline void MasterControlPanel::renderMainPanel() {
    ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Simulation Control", &m_showMainPanel)) {
        // Simulation controls
        if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderSimulationControls();
        }

        // Panel toggles
        if (ImGui::CollapsingHeader("Panels", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderPanelToggles();
        }
    }
    ImGui::End();
}

inline void MasterControlPanel::renderSimulationControls() {
    // Play/Pause buttons
    ImGui::PushStyleColor(ImGuiCol_Button,
        m_paused ? ImVec4(0.2f, 0.6f, 0.2f, 1.0f) : ImVec4(0.6f, 0.2f, 0.2f, 1.0f));

    if (ImGui::Button(m_paused ? "PLAY" : "PAUSE", ImVec2(80, 30))) {
        m_paused = !m_paused;
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    if (ImGui::Button("STEP", ImVec2(60, 30))) {
        m_stepOneFrame = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("RESET", ImVec2(60, 30))) {
        if (m_resetCallback) m_resetCallback();
    }

    // Speed controls
    ImGui::Separator();
    ImGui::Text("Simulation Speed: %.2fx", m_simulationSpeed);

    if (ImGui::Button("0.25x")) m_simulationSpeed = 0.25f;
    ImGui::SameLine();
    if (ImGui::Button("0.5x")) m_simulationSpeed = 0.5f;
    ImGui::SameLine();
    if (ImGui::Button("1x")) m_simulationSpeed = 1.0f;
    ImGui::SameLine();
    if (ImGui::Button("2x")) m_simulationSpeed = 2.0f;
    ImGui::SameLine();
    if (ImGui::Button("4x")) m_simulationSpeed = 4.0f;

    ImGui::SliderFloat("##SpeedSlider", &m_simulationSpeed, 0.1f, 10.0f, "%.2fx");
}

inline void MasterControlPanel::renderPanelToggles() {
    bool spawnVisible = m_spawnPanel.isVisible();
    if (ImGui::Checkbox("Creature Spawner", &spawnVisible)) {
        m_spawnPanel.setVisible(spawnVisible);
    }

    bool evolVisible = m_evolutionPanel.isVisible();
    if (ImGui::Checkbox("Evolution Controls", &evolVisible)) {
        m_evolutionPanel.setVisible(evolVisible);
    }

    bool envVisible = m_environmentPanel.isVisible();
    if (ImGui::Checkbox("Environment Controls", &envVisible)) {
        m_environmentPanel.setVisible(envVisible);
    }

    bool scenarioVisible = m_scenarioPanel.isVisible();
    if (ImGui::Checkbox("Scenario Presets", &scenarioVisible)) {
        m_scenarioPanel.setVisible(scenarioVisible);
    }

    bool statsVisible = m_statisticsPanel.isVisible();
    if (ImGui::Checkbox("Statistics & Graphs", &statsVisible)) {
        m_statisticsPanel.setVisible(statsVisible);
    }

    ImGui::Checkbox("Debug Panel", &m_showDebugPanel);
}

inline void MasterControlPanel::renderDebugPanel(Camera& camera) {
    ImGui::SetNextWindowSize(ImVec2(320, 450), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Debug & Visualization", &m_showDebugPanel)) {
        // Visualization toggles
        if (ImGui::CollapsingHeader("Visualization", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderVisualizationToggles();
        }

        // Camera controls
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderCameraControls(camera);
        }
    }
    ImGui::End();
}

inline void MasterControlPanel::renderVisualizationToggles() {
    ImGui::Text("Debug Overlays:");
    ImGui::Checkbox("Show Vision Cones", &showCreatureVision);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Display creature vision ranges and fields of view");

    ImGui::Checkbox("Show Spatial Grid", &showSpatialGrid);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Display the spatial partitioning grid");

    ImGui::Checkbox("Show Pathfinding", &showPathfinding);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Display creature pathfinding debug info");

    ImGui::Checkbox("Show Creature Stats", &showCreatureStats);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Display energy/health bars above creatures");

    ImGui::Checkbox("Show Bounding Boxes", &showBoundingBoxes);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Display collision bounding boxes");

    ImGui::Checkbox("Show Terrain Wireframe", &showTerrainWireframe);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Display terrain as wireframe");

    ImGui::Separator();

    ImGui::Text("Creature Labels:");
    ImGui::Checkbox("Show Nametags", &showNametags);
    ImGui::SliderFloat("Nametag Distance", &nametagMaxDistance, 10.0f, 200.0f, "%.0f");
}

inline void MasterControlPanel::renderCameraControls(Camera& camera) {
    // Camera mode selection
    ImGui::Text("Camera Mode:");

    const char* modeNames[] = { "Free", "Follow Creature", "Bird's Eye", "Underwater" };
    int currentMode = static_cast<int>(cameraMode);
    if (ImGui::Combo("##CameraMode", &currentMode, modeNames, 4)) {
        cameraMode = static_cast<CameraMode>(currentMode);
    }

    // Mode-specific controls
    if (cameraMode == CameraMode::FollowCreature) {
        if (m_selectedCreature) {
            ImGui::Text("Following: %s #%d",
                getCreatureTypeName(m_selectedCreature->getType()),
                m_selectedCreature->getID());
            if (ImGui::Button("Stop Following")) {
                cameraMode = CameraMode::Free;
            }
        } else {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "Select a creature to follow");
        }
    }

    ImGui::Separator();

    // Camera properties
    ImGui::Text("Position: (%.1f, %.1f, %.1f)", camera.Position.x, camera.Position.y, camera.Position.z);
    ImGui::Text("Yaw: %.1f, Pitch: %.1f", camera.Yaw, camera.Pitch);

    ImGui::SliderFloat("Move Speed", &camera.MovementSpeed, 10.0f, 200.0f, "%.0f");
    ImGui::SliderFloat("Sensitivity", &camera.MouseSensitivity, 0.01f, 1.0f, "%.2f");
    ImGui::SliderFloat("FOV", &camera.Zoom, 30.0f, 120.0f, "%.0f");

    // Quick positions
    ImGui::Separator();
    ImGui::Text("Quick Positions:");

    if (ImGui::Button("Overview", ImVec2(80, 0))) {
        camera.Position = glm::vec3(0.0f, 100.0f, 100.0f);
        camera.Yaw = -90.0f;
        camera.Pitch = -30.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Top Down", ImVec2(80, 0))) {
        camera.Position = glm::vec3(0.0f, 150.0f, 0.0f);
        camera.Pitch = -89.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Ground", ImVec2(80, 0))) {
        camera.Position = glm::vec3(0.0f, 5.0f, 50.0f);
        camera.Pitch = 0.0f;
    }
}

inline void MasterControlPanel::renderStatusBar(float simulationTime, uint32_t generation, int totalCreatures) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float barHeight = 25.0f;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - barHeight));
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
        // Simulation state
        if (m_paused) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "PAUSED");
        } else {
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "%.2fx", m_simulationSpeed);
        }

        ImGui::SameLine(0, 20);
        ImGui::Text("Creatures: %d", totalCreatures);

        ImGui::SameLine(0, 20);
        ImGui::Text("Gen: %u", generation);

        ImGui::SameLine(0, 20);
        int minutes = static_cast<int>(simulationTime / 60.0f);
        int seconds = static_cast<int>(simulationTime) % 60;
        ImGui::Text("Time: %02d:%02d", minutes, seconds);

        ImGui::SameLine(0, 20);
        ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);

        // Right-aligned help hint
        float helpWidth = ImGui::CalcTextSize("F1=Help F2=Panel F3=Debug").x;
        ImGui::SameLine(viewport->Size.x - helpWidth - 20);
        ImGui::TextDisabled("F1=Help F2=Panel F3=Debug");
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

inline void MasterControlPanel::renderHelpWindow() {
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(450, 500), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Help & Keyboard Shortcuts", &m_showHelp)) {
        ImGui::Text("KEYBOARD SHORTCUTS");
        ImGui::Separator();

        ImGui::Text("Simulation Control:");
        ImGui::BulletText("Space / P - Toggle pause");
        ImGui::BulletText(". (Period) - Step one frame");
        ImGui::BulletText("1-6 - Set simulation speed (0.25x to 8x)");

        ImGui::Spacing();
        ImGui::Text("Panels:");
        ImGui::BulletText("F1 - Toggle this help window");
        ImGui::BulletText("F2 - Toggle main control panel");
        ImGui::BulletText("F3 - Toggle debug panel");
        ImGui::BulletText("F4 - Toggle statistics panel");
        ImGui::BulletText("Escape - Deselect creature / close dialogs");

        ImGui::Spacing();
        ImGui::Text("Camera Controls:");
        ImGui::BulletText("WASD - Move camera");
        ImGui::BulletText("Q/E or Space/Ctrl - Move up/down");
        ImGui::BulletText("Shift - Move faster");
        ImGui::BulletText("Mouse - Look around (when captured)");
        ImGui::BulletText("Right Click - Capture/release mouse");

        ImGui::Separator();
        ImGui::Text("SCENARIO PRESETS");
        ImGui::TextWrapped("Use the Scenario Presets panel to quickly set up different simulation conditions:");
        ImGui::BulletText("Cambrian Explosion - High mutation, rapid speciation");
        ImGui::BulletText("Ice Age - Harsh survival conditions");
        ImGui::BulletText("Paradise Island - Abundant resources, low pressure");
        ImGui::BulletText("Predator Hell - High predator pressure");
        ImGui::BulletText("And more...");

        ImGui::Separator();
        ImGui::Text("TIPS");
        ImGui::TextWrapped("- Use the Statistics panel (F4) to monitor population trends and genetic diversity");
        ImGui::TextWrapped("- Adjust Evolution Controls to change mutation rates and selection pressure");
        ImGui::TextWrapped("- Environment Controls let you modify climate and food abundance in real-time");
        ImGui::TextWrapped("- Use Chaos Controls in the Spawn panel for mass extinction events");
    }
    ImGui::End();
}

} // namespace ui
