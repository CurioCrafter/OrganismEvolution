#pragma once

// GodModeUI - Master integration class for all God Mode tools
// Coordinates all player interaction systems for sandbox control

#include "imgui.h"
#include "SelectionSystem.h"
#include "CreatureManipulation.h"
#include "TerraformingTools.h"
#include "SpawnTools.h"
#include "EnvironmentTools.h"
#include "MutationInjector.h"
#include "TimeControls.h"
#include "ToolWheel.h"
#include "../core/SimulationOrchestrator.h"
#include "../graphics/Camera.h"
#include <memory>

namespace ui {

// God Mode configuration
struct GodModeConfig {
    bool enableSelection = true;
    bool enableSpawning = true;
    bool enableTerraforming = true;
    bool enableEnvironment = true;
    bool enableMutations = true;
    bool enableTimeControl = true;
    bool enableToolWheel = true;

    bool showToolbar = true;
    bool showStatusBar = true;
};

class GodModeUI {
public:
    GodModeUI();
    ~GodModeUI();

    // Initialization
    void init(Forge::SimulationOrchestrator* sim);
    void shutdown();

    // Main loop
    void update(float deltaTime);
    void render();

    // Render additional visual overlays (selection circles, brush previews)
    void renderOverlays();

    // Configuration
    void setConfig(const GodModeConfig& config) { m_config = config; }
    const GodModeConfig& getConfig() const { return m_config; }

    // Enable/disable entire God Mode
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

    // Get individual tool systems
    SelectionSystem& getSelection() { return m_selection; }
    CreatureManipulation& getManipulation() { return m_manipulation; }
    TerraformingTools& getTerraforming() { return m_terraforming; }
    SpawnTools& getSpawning() { return m_spawning; }
    EnvironmentTools& getEnvironment() { return m_environment; }
    MutationInjector& getMutations() { return m_mutations; }
    TimeControls& getTimeControls() { return m_timeControls; }
    ToolWheel& getToolWheel() { return m_toolWheel; }

    // Get current active tool
    ToolCategory getActiveTool() const { return m_toolWheel.getActiveTool(); }
    void setActiveTool(ToolCategory tool) { m_toolWheel.setActiveTool(tool); }

    // Screen dimensions (for overlay rendering)
    void setScreenSize(float width, float height);
    float getScreenWidth() const { return m_screenWidth; }
    float getScreenHeight() const { return m_screenHeight; }

private:
    // Dependencies
    Forge::SimulationOrchestrator* m_simulation = nullptr;
    Camera* m_camera = nullptr;

    // Configuration
    GodModeConfig m_config;
    bool m_enabled = true;
    bool m_initialized = false;

    // Screen size
    float m_screenWidth = 1920.0f;
    float m_screenHeight = 1080.0f;

    // Tool systems
    SelectionSystem m_selection;
    CreatureManipulation m_manipulation;
    TerraformingTools m_terraforming;
    SpawnTools m_spawning;
    EnvironmentTools m_environment;
    MutationInjector m_mutations;
    TimeControls m_timeControls;
    ToolWheel m_toolWheel;

    // UI State
    bool m_showMainWindow = false;
    bool m_showToolbar = true;
    bool m_showStatusBar = true;

    // Helper methods
    void setupToolConnections();
    void handleInput(float deltaTime);
    void updateActiveTool(float deltaTime);

    void renderMainWindow();
    void renderToolbar();
    void renderStatusBar();
    void renderToolSpecificUI();

    void onToolSelected(ToolCategory tool);
    void onSelectionChanged(const SelectionChangedEvent& event);
};

// ============================================================================
// Implementation
// ============================================================================

inline GodModeUI::GodModeUI() {}

inline GodModeUI::~GodModeUI() {
    shutdown();
}

inline void GodModeUI::init(Forge::SimulationOrchestrator* sim) {
    m_simulation = sim;

    if (!sim) {
        return;
    }

    // Get camera from controller
    auto* controller = sim->getCameraController();
    if (controller) {
        // Note: CameraController should expose its camera
        // For now, we'll work without direct camera access in some cases
    }

    // Wire up tool systems
    setupToolConnections();

    m_initialized = true;
}

inline void GodModeUI::shutdown() {
    m_simulation = nullptr;
    m_camera = nullptr;
    m_initialized = false;
}

inline void GodModeUI::setupToolConnections() {
    if (!m_simulation) return;

    // Connect selection system
    m_selection.setOnSelectionChanged([this](const SelectionChangedEvent& event) {
        onSelectionChanged(event);
    });

    // Connect manipulation system
    m_manipulation.setCreatureManager(m_simulation->getCreatureManager());
    m_manipulation.setSelectionSystem(&m_selection);

    // Connect terraforming
    m_terraforming.setTerrain(m_simulation->getTerrain());

    // Connect spawning
    m_spawning.setCreatureManager(m_simulation->getCreatureManager());
    m_spawning.setTerrain(m_simulation->getTerrain());

    // Connect environment
    // Note: Would need ClimateSystem access from SimulationOrchestrator
    m_environment.setWeatherSystem(m_simulation->getWeather());

    // Connect mutations
    m_mutations.setSelectionSystem(&m_selection);

    // Connect time controls
    m_timeControls.setSimulation(m_simulation);

    // Connect tool wheel
    m_toolWheel.setOnToolSelected([this](ToolCategory tool) {
        onToolSelected(tool);
    });
}

inline void GodModeUI::setScreenSize(float width, float height) {
    m_screenWidth = width;
    m_screenHeight = height;
}

inline void GodModeUI::update(float deltaTime) {
    if (!m_enabled || !m_initialized) return;

    // Handle global input
    handleInput(deltaTime);

    // Update tool wheel
    m_toolWheel.handleHotkeys();
    m_toolWheel.update();

    // Update time controls
    m_timeControls.handleKeyboardShortcuts();

    // Update environment zones
    m_environment.update(deltaTime);

    // Update active tool
    updateActiveTool(deltaTime);
}

inline void GodModeUI::handleInput(float deltaTime) {
    ImGuiIO& io = ImGui::GetIO();

    // Global hotkeys
    // G - toggle God Mode window
    if (!io.WantCaptureKeyboard && ImGui::IsKeyPressed(ImGuiKey_G)) {
        m_showMainWindow = !m_showMainWindow;
    }

    // T - toggle toolbar
    if (!io.WantCaptureKeyboard && ImGui::IsKeyPressed(ImGuiKey_T) && io.KeyCtrl) {
        m_showToolbar = !m_showToolbar;
    }
}

inline void GodModeUI::updateActiveTool(float deltaTime) {
    // Get camera for tools that need it
    auto* controller = m_simulation ? m_simulation->getCameraController() : nullptr;

    // Update brush positions based on active tool
    switch (m_toolWheel.getActiveTool()) {
        case ToolCategory::SELECT:
            if (m_config.enableSelection && m_simulation) {
                // Update selection with camera and creatures
                // Note: This needs proper camera access
            }
            break;

        case ToolCategory::TERRAFORM:
            if (m_config.enableTerraforming && controller) {
                // Terraforming update happens in its own update call
            }
            break;

        case ToolCategory::SPAWN:
            if (m_config.enableSpawning) {
                m_spawning.update(m_screenWidth, m_screenHeight);
            }
            break;

        case ToolCategory::ENVIRONMENT:
            if (m_config.enableEnvironment) {
                m_environment.updateBrush(m_screenWidth, m_screenHeight);
            }
            break;

        default:
            break;
    }
}

inline void GodModeUI::render() {
    if (!m_enabled) return;

    // Toolbar at top
    if (m_showToolbar && m_config.showToolbar) {
        renderToolbar();
    }

    // Status bar at bottom
    if (m_showStatusBar && m_config.showStatusBar) {
        renderStatusBar();
    }

    // Main window (when toggled)
    if (m_showMainWindow) {
        renderMainWindow();
    }

    // Tool-specific UI panels
    renderToolSpecificUI();

    // Tool wheel (always available)
    if (m_config.enableToolWheel) {
        m_toolWheel.render();
    }
}

inline void GodModeUI::renderOverlays() {
    if (!m_enabled) return;

    // Selection indicators
    if (m_config.enableSelection && m_selection.hasSelection()) {
        // Note: Would need camera access for proper 3D->2D projection
        // m_selection.renderSelectionIndicators(*m_camera, m_screenWidth, m_screenHeight);
    }

    // Spawn brush preview
    if (m_toolWheel.getActiveTool() == ToolCategory::SPAWN && m_config.enableSpawning) {
        m_spawning.renderSpawnPreview(m_screenWidth, m_screenHeight);
    }

    // Environment zone visualization
    if (m_toolWheel.getActiveTool() == ToolCategory::ENVIRONMENT && m_config.enableEnvironment) {
        m_environment.renderZoneVisuals(m_screenWidth, m_screenHeight);
    }
}

inline void GodModeUI::renderMainWindow() {
    ImGui::SetNextWindowSize(ImVec2(450, 600), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("God Mode", &m_showMainWindow, ImGuiWindowFlags_MenuBar)) {
        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Tools")) {
                if (ImGui::MenuItem("Selection", "F1")) setActiveTool(ToolCategory::SELECT);
                if (ImGui::MenuItem("Spawn", "F2")) setActiveTool(ToolCategory::SPAWN);
                if (ImGui::MenuItem("Terraform", "F3")) setActiveTool(ToolCategory::TERRAFORM);
                if (ImGui::MenuItem("Environment", "F4")) setActiveTool(ToolCategory::ENVIRONMENT);
                if (ImGui::MenuItem("Mutation", "F5")) setActiveTool(ToolCategory::MUTATION);
                if (ImGui::MenuItem("Time", "F6")) setActiveTool(ToolCategory::TIME);
                if (ImGui::MenuItem("Manipulate", "F7")) setActiveTool(ToolCategory::MANIPULATE);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Toolbar", "Ctrl+T", &m_showToolbar);
                ImGui::MenuItem("Status Bar", nullptr, &m_showStatusBar);
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        // Current tool info
        ImGui::Text("Active Tool: %s", ToolWheel::getToolName(m_toolWheel.getActiveTool()));
        ImGui::Separator();

        // Render content based on active tool
        switch (m_toolWheel.getActiveTool()) {
            case ToolCategory::SELECT:
                ImGui::Text("Selection Mode");
                ImGui::TextWrapped("Click on creatures to select them. Hold Shift for multi-select. "
                                   "Drag to box-select multiple creatures.");
                if (m_selection.hasSelection()) {
                    ImGui::Separator();
                    ImGui::Text("Selected: %d creature(s)",
                        static_cast<int>(m_selection.getMultiSelection().size()));
                }
                break;

            case ToolCategory::SPAWN:
                m_spawning.renderSection();
                break;

            case ToolCategory::TERRAFORM:
                m_terraforming.renderUI();
                break;

            case ToolCategory::ENVIRONMENT:
                m_environment.renderSection();
                break;

            case ToolCategory::MUTATION:
                m_mutations.renderSection(m_selection);
                break;

            case ToolCategory::TIME:
                m_timeControls.renderSection();
                break;

            case ToolCategory::MANIPULATE:
                if (m_simulation) {
                    m_manipulation.renderSection(m_selection, *m_simulation->getCreatureManager());
                }
                break;

            default:
                ImGui::TextDisabled("Select a tool to begin.");
                break;
        }
    }
    ImGui::End();
}

inline void GodModeUI::renderToolbar() {
    ImGuiWindowFlags toolbarFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(m_screenWidth - 20, 45));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f));

    if (ImGui::Begin("##GodModeToolbar", nullptr, toolbarFlags)) {
        // Tool buttons
        const ToolCategory tools[] = {
            ToolCategory::SELECT, ToolCategory::SPAWN, ToolCategory::TERRAFORM,
            ToolCategory::ENVIRONMENT, ToolCategory::MUTATION, ToolCategory::TIME,
            ToolCategory::MANIPULATE
        };

        for (int i = 0; i < 7; ++i) {
            bool selected = (m_toolWheel.getActiveTool() == tools[i]);
            ImVec4 color = ToolWheel::getToolColor(tools[i]);

            if (selected) {
                ImGui::PushStyleColor(ImGuiCol_Button, color);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color.x * 0.5f, color.y * 0.5f, color.z * 0.5f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
            }

            if (ImGui::Button(ToolWheel::getToolName(tools[i]), ImVec2(80, 28))) {
                setActiveTool(tools[i]);
            }

            ImGui::PopStyleColor(2);

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("F%d - %s", i + 1, ToolWheel::getToolName(tools[i]));
            }

            if (i < 6) ImGui::SameLine();
        }

        // Separator
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();

        // Time controls (compact)
        m_timeControls.renderCompact();

        // God Mode toggle button (right side)
        ImGui::SameLine(m_screenWidth - 120);
        if (ImGui::Button("God Mode [G]", ImVec2(100, 28))) {
            m_showMainWindow = !m_showMainWindow;
        }
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

inline void GodModeUI::renderStatusBar() {
    ImGuiWindowFlags statusFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    float barHeight = 30.0f;
    ImGui::SetNextWindowPos(ImVec2(0, m_screenHeight - barHeight));
    ImGui::SetNextWindowSize(ImVec2(m_screenWidth, barHeight));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 6));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.08f, 0.95f));

    if (ImGui::Begin("##GodModeStatus", nullptr, statusFlags)) {
        // Selection info
        if (m_selection.hasSelection()) {
            Creature* selected = m_selection.getSelectedCreature();
            if (selected) {
                ImGui::Text("Selected: %s #%d | Energy: %.0f | Gen: %d",
                    getCreatureTypeName(selected->getType()),
                    selected->getID(),
                    selected->getEnergy(),
                    selected->getGeneration());
            }
        } else {
            ImGui::TextDisabled("No selection");
        }

        // Simulation stats (right side)
        if (m_simulation) {
            const auto& stats = m_simulation->getStats();

            ImGui::SameLine(m_screenWidth - 450);
            ImGui::Text("Day: %d | Population: %d | Max Gen: %d | Time: %.1fx",
                stats.dayCount,
                stats.totalCreatures,
                stats.maxGeneration,
                m_simulation->getTimeScale());
        }
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

inline void GodModeUI::renderToolSpecificUI() {
    // Render individual tool windows if they're visible
    // These are independent windows that can be toggled

    // Note: Most tools render through the main window
    // Standalone windows can be enabled per-tool if desired
}

inline void GodModeUI::onToolSelected(ToolCategory tool) {
    // Handle tool selection events
    // Could show/hide specific UI elements based on tool
}

inline void GodModeUI::onSelectionChanged(const SelectionChangedEvent& event) {
    // Update manipulation panel with new selection
    m_manipulation.setSelectionSystem(&m_selection);
}

} // namespace ui
