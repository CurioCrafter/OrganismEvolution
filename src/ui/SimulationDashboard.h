#pragma once

#include "imgui.h"
#include "implot.h"
#include "DashboardMetrics.h"
#include "NEATVisualizer.h"
#include "PhylogeneticTreeVisualizer.h"
#include "CreatureInspectionPanel.h"
#include "SelectionSystem.h"
#include "../entities/Creature.h"
#include "../entities/genetics/Species.h"
#include "../graphics/Camera.h"
#include "../graphics/CameraController.h"
#include <vector>
#include <string>
#include <functional>

// Forward declarations
class DayNightCycle;
namespace Forge {
    class AudioManager;
    class CreatureVoiceGenerator;
    class AmbientSoundscape;
}
class Terrain;
class Food;
class SpatialGrid;

namespace ui {

// ============================================================================
// Simulation Dashboard - Comprehensive UI for evolution simulator
// ============================================================================
//
// Features:
// - Tabbed interface with Overview, Genetics, Neural, and World panels
// - Real-time population and fitness graphs using ImPlot
// - Creature inspector with detailed genome and brain visualization
// - Simulation controls (pause, speed, spawning)
// - Ecosystem health indicators
// - Neural network topology visualization
// - Keyboard shortcuts and status bar
//
// Layout:
// +------------------+------------------------+------------------+
// |  Left Panel      |    Main Viewport       |  Right Panel     |
// |  (Controls)      |                        |  (Inspector)     |
// +------------------+------------------------+------------------+
// |                      Status Bar                              |
// +-------------------------------------------------------------+

class SimulationDashboard {
public:
    SimulationDashboard();
    ~SimulationDashboard();

    // Initialize ImPlot context
    bool initialize();
    void shutdown();

    // Main render function - call each frame
    void render(
        std::vector<std::unique_ptr<Creature>>& creatures,
        std::vector<std::unique_ptr<Food>>& food,
        DayNightCycle& dayNight,
        Camera& camera,
        float simulationTime,
        uint32_t generation,
        float deltaTime
    );

    // Handle keyboard shortcuts
    void handleInput();

    // Control state accessors
    bool isPaused() const { return m_paused; }
    float getSimulationSpeed() const { return m_simulationSpeed; }
    bool isDebugPanelVisible() const { return m_showDebugPanel; }

    // Setters for external state
    void setPaused(bool paused) { m_paused = paused; }
    void setSimulationSpeed(float speed) { m_simulationSpeed = speed; }
    void toggleDebugPanel() { m_showDebugPanel = !m_showDebugPanel; }

    // Spawn callbacks
    using SpawnCallback = std::function<void(CreatureType, int)>;
    using SpawnFoodCallback = std::function<void(int)>;
    void setSpawnCreatureCallback(SpawnCallback cb) { m_spawnCreatureCallback = cb; }
    void setSpawnFoodCallback(SpawnFoodCallback cb) { m_spawnFoodCallback = cb; }

    // Kill callbacks for chaos buttons
    using KillCallback = std::function<void(CreatureType)>;
    using MassExtinctionCallback = std::function<void(float)>;
    void setKillCallback(KillCallback cb) { m_killCallback = cb; }
    void setMassExtinctionCallback(MassExtinctionCallback cb) { m_massExtinctionCallback = cb; }

    // Camera follow callback
    using FollowCreatureCallback = std::function<void(Creature*)>;
    void setFollowCreatureCallback(FollowCreatureCallback cb) { m_followCreatureCallback = cb; }

    // Graphics settings
    bool showNametags = true;
    bool showTrees = true;
    float nametagMaxDistance = 50.0f;

    // Species tracker integration
    void setSpeciationTracker(genetics::SpeciationTracker* tracker) {
        m_speciesPanel.setSpeciationTracker(tracker);
    }

    // Audio system integration
    void setAudioSystem(Forge::AudioManager* audio,
                       Forge::CreatureVoiceGenerator* voices,
                       Forge::AmbientSoundscape* ambient) {
        m_audioManager = audio;
        m_creatureVoices = voices;
        m_ambientSoundscape = ambient;
    }

    // Phase 8: Camera controller integration for creature inspection
    void setCameraController(Forge::CameraController* controller) {
        m_cameraController = controller;
        m_inspectionPanel.setCameraController(controller);
    }

    // Phase 8: Get selection system for external integration
    SelectionSystem& getSelectionSystem() { return m_selectionSystem; }
    const SelectionSystem& getSelectionSystem() const { return m_selectionSystem; }

    // Phase 8: Get inspection panel for external access
    CreatureInspectionPanel& getInspectionPanel() { return m_inspectionPanel; }
    const CreatureInspectionPanel& getInspectionPanel() const { return m_inspectionPanel; }

    // Phase 8: Set biome system for environment info in inspection panel
    void setBiomeSystem(const BiomeSystem* biomes) {
        m_inspectionPanel.setBiomeSystem(biomes);
    }

    // Phase 8: Process selection (call each frame with creature manager)
    void updateSelection(const Camera& camera, Forge::CreatureManager& creatures,
                        float screenWidth, float screenHeight);

private:
    // Dashboard state
    bool m_initialized = false;
    bool m_showDebugPanel = true;
    bool m_paused = false;
    float m_simulationSpeed = 1.0f;
    bool m_stepOneFrame = false;

    // Tab state
    enum class DashboardTab {
        Overview,
        Genetics,
        Species,
        Neural,
        World,
        Inspect  // Phase 8: Creature inspection tab
    };
    DashboardTab m_currentTab = DashboardTab::Overview;

    // Metrics tracking
    DashboardMetrics m_metrics;

    // Neural network visualizer
    NeuralNetworkVisualizer m_neuralVisualizer;
    NEATEvolutionPanel m_neatPanel;

    // Species evolution panel
    SpeciesEvolutionPanel m_speciesPanel;

    // Phase 8: Creature inspection panel and selection system
    CreatureInspectionPanel m_inspectionPanel;
    SelectionSystem m_selectionSystem;
    Forge::CameraController* m_cameraController = nullptr;

    // Selected creature for inspection
    Creature* m_selectedCreature = nullptr;
    int m_selectedCreatureId = -1;

    // UI state
    bool m_showHelp = false;
    bool m_showPerformance = true;
    bool m_showGraphs = true;
    bool m_showStatusBar = true;

    // Callbacks
    SpawnCallback m_spawnCreatureCallback;
    SpawnFoodCallback m_spawnFoodCallback;
    KillCallback m_killCallback;
    MassExtinctionCallback m_massExtinctionCallback;
    FollowCreatureCallback m_followCreatureCallback;

    // Audio system pointers
    Forge::AudioManager* m_audioManager = nullptr;
    Forge::CreatureVoiceGenerator* m_creatureVoices = nullptr;
    Forge::AmbientSoundscape* m_ambientSoundscape = nullptr;

    // Helper to find creature by ID
    Creature* findCreatureById(std::vector<std::unique_ptr<Creature>>& creatures, int id);

    // Panel rendering functions
    void renderLeftPanel(
        std::vector<std::unique_ptr<Creature>>& creatures,
        std::vector<std::unique_ptr<Food>>& food,
        DayNightCycle& dayNight,
        Camera& camera,
        float simulationTime,
        uint32_t generation
    );

    void renderRightPanel(
        std::vector<std::unique_ptr<Creature>>& creatures,
        Camera& camera
    );

    void renderStatusBar(
        float simulationTime,
        uint32_t generation
    );

    void renderHelpWindow();

    // Tab content rendering
    void renderOverviewTab(
        std::vector<std::unique_ptr<Creature>>& creatures,
        std::vector<std::unique_ptr<Food>>& food,
        DayNightCycle& dayNight,
        float simulationTime,
        uint32_t generation
    );

    void renderGeneticsTab(
        std::vector<std::unique_ptr<Creature>>& creatures
    );

    void renderNeuralTab(
        std::vector<std::unique_ptr<Creature>>& creatures
    );

    void renderWorldTab(
        DayNightCycle& dayNight,
        Camera& camera
    );

    // Phase 8: Inspection tab (uses CreatureInspectionPanel)
    void renderInspectTab();

    // Widget helpers
    void renderSimulationControls();
    void renderPopulationStats();
    void renderEcosystemHealth();
    void renderSpawnControls();
    void renderChaosButtons();

    void renderPopulationGraphs();
    void renderFitnessGraphs();
    void renderTraitDistribution(std::vector<std::unique_ptr<Creature>>& creatures);
    void renderGeneticDiversityPanel();
    void renderSpeciesBreakdown();

    void renderNeuralNetworkPanel(std::vector<std::unique_ptr<Creature>>& creatures);
    void renderBrainInputsOutputs(Creature* creature);

    void renderCreatureInspector(std::vector<std::unique_ptr<Creature>>& creatures);
    void renderCreatureList(std::vector<std::unique_ptr<Creature>>& creatures);

    void renderDayNightControls(DayNightCycle& dayNight);
    void renderCameraControls(Camera& camera);
    void renderGraphicsSettings();
    void renderAudioSettings();
};

} // namespace ui
