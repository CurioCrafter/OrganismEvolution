#pragma once

// MainMenu - Main menu system with planet generator and settings
// Phase 8 Agent 8: UI Integration Owner
//
// Features:
// - Main menu flow: New Planet, Continue, Settings, Quit
// - Planet generator UI with presets and customization
// - Settings screen for graphics, performance, and simulation defaults
// - God tools toggle (default OFF)
// - Observer mode by default

#include "imgui.h"
#include "../environment/PlanetTheme.h"
#include "../environment/PlanetSeed.h"
#include <functional>
#include <string>
#include <cstdint>

struct WorldGenConfig;

namespace ui {

// ============================================================================
// World Generation Configuration
// ============================================================================

struct WorldGenConfig {
    // Seed
    uint32_t seed = 0;
    std::string seedInput;        // User input string for seed
    bool useRandomSeed = true;

    // Planet type
    PlanetPreset preset = PlanetPreset::EARTH_LIKE;
    bool isAlienWorld = false;    // Quick toggle for alien vs realistic

    // World structure
    int regionCount = 1;          // Number of islands/regions (1-7)
    float worldSize = 2000.0f;    // World dimensions (4x larger default)
    float oceanCoverage = 0.3f;   // 0-1, amount of water

    // Biome mix (weights, normalized at generation)
    float forestWeight = 1.0f;
    float grasslandWeight = 1.0f;
    float desertWeight = 0.5f;
    float tundraWeight = 0.3f;
    float wetlandWeight = 0.5f;
    float mountainWeight = 0.5f;
    float volcanicWeight = 0.1f;

    // Climate
    float temperatureBias = 0.5f; // 0=cold, 1=hot
    float moistureBias = 0.5f;    // 0=dry, 1=wet
    float seasonIntensity = 0.5f; // 0=no seasons, 1=extreme

    // Star type (affects light color and intensity)
    enum class StarType {
        YELLOW_DWARF,   // Earth-like (G-type)
        ORANGE_DWARF,   // Warmer light (K-type)
        RED_DWARF,      // Dimmer, redder (M-type)
        BLUE_GIANT,     // Bright, blue-white
        BINARY          // Two suns
    };
    StarType starType = StarType::YELLOW_DWARF;
};

// ============================================================================
// Evolution Start Preset
// ============================================================================

struct EvolutionStartPreset {
    // Initial population
    int herbivoreCount = 50;
    int carnivoreCount = 10;
    int aquaticCount = 20;
    int flyingCount = 0;          // Start with none, evolve them

    // Initial food
    int plantCount = 200;
    float plantGrowthRate = 1.0f;

    // Evolution parameters
    float mutationRate = 0.1f;
    float speciationThreshold = 0.3f;
    bool enableSexualReproduction = true;
    bool enableCoevolution = true;

    // Guidance bias (helps evolution find interesting niches)
    bool enableGuidance = true;
    float guidanceStrength = 0.3f; // 0=pure random, 1=heavily guided

    // Difficulty presets
    enum class Difficulty {
        SANDBOX,        // Abundant resources, slow death
        BALANCED,       // Default balance
        HARSH,          // Scarce resources, fast predators
        EXTINCTION      // Frequent disasters, hard survival
    };
    Difficulty difficulty = Difficulty::BALANCED;
};

// ============================================================================
// Settings Configuration
// ============================================================================

struct SettingsConfig {
    // Graphics
    enum class QualityPreset {
        LOW,
        MEDIUM,
        HIGH,
        ULTRA,
        CUSTOM
    };
    QualityPreset qualityPreset = QualityPreset::HIGH;

    int renderDistance = 500;
    int shadowQuality = 2;        // 0-3
    int grassDensity = 2;         // 0-3
    int treeLOD = 2;              // 0-3
    int creatureDetail = 2;       // 0-3
    bool enableSSAO = true;
    bool enableBloom = true;
    bool enableVolumetricFog = true;
    bool enableWaterReflections = true;
    bool enableDynamicShadows = true;

    // Performance
    int targetFPS = 60;
    bool enableVSync = true;
    bool enableFPSLimit = false;
    float renderScale = 1.0f;     // 0.5-2.0
    bool enableMultithreading = true;
    int maxCreatures = 1000;

    // Simulation defaults
    float defaultSimSpeed = 1.0f;
    bool pauseOnStart = false;
    bool autoSave = true;
    int autoSaveInterval = 300;   // seconds

    // UI
    float uiScale = 1.0f;
    bool showFPS = true;
    bool showMinimap = true;
    bool showNametags = true;
    float nametagDistance = 50.0f;
    bool showTooltips = true;

    // Camera
    float cameraSensitivity = 1.0f;
    float cameraSpeed = 20.0f;
    bool invertY = false;

    // Audio
    float masterVolume = 0.8f;
    float musicVolume = 0.5f;
    float sfxVolume = 0.7f;
    float ambientVolume = 0.6f;
    bool enableCreatureVoices = true;
};

// ============================================================================
// Main Menu State
// ============================================================================

enum class MainMenuState {
    MAIN,              // Main menu buttons
    NEW_PLANET,        // Planet generator
    SETTINGS,          // Settings screen
    LOAD_GAME,         // Load saved game (future)
    CREDITS            // Credits screen (future)
};

// ============================================================================
// MainMenu Class
// ============================================================================

class MainMenu {
public:
    MainMenu();
    ~MainMenu() = default;

    // ========================================================================
    // Main Interface
    // ========================================================================

    // Render the menu (call each frame when menu is active)
    void render();

    // Check if menu is active
    bool isActive() const { return m_active; }
    void setActive(bool active) { m_active = active; }

    // Get current state
    MainMenuState getState() const { return m_state; }
    void setState(MainMenuState state) { m_state = state; }

    // ========================================================================
    // Callbacks
    // ========================================================================

    using StartGameCallback = std::function<void(const WorldGenConfig&, const EvolutionStartPreset&, bool godMode)>;
    using QuitCallback = std::function<void()>;
    using ContinueCallback = std::function<void()>;
    using SettingsChangedCallback = std::function<void(const SettingsConfig&)>;

    void setOnStartGame(StartGameCallback cb) { m_onStartGame = cb; }
    void setOnQuit(QuitCallback cb) { m_onQuit = cb; }
    void setOnContinue(ContinueCallback cb) { m_onContinue = cb; }
    void setOnSettingsChanged(SettingsChangedCallback cb) { m_onSettingsChanged = cb; }

    // ========================================================================
    // Configuration Access
    // ========================================================================

    WorldGenConfig& getWorldGenConfig() { return m_worldGenConfig; }
    const WorldGenConfig& getWorldGenConfig() const { return m_worldGenConfig; }

    EvolutionStartPreset& getEvolutionPreset() { return m_evolutionPreset; }
    const EvolutionStartPreset& getEvolutionPreset() const { return m_evolutionPreset; }

    SettingsConfig& getSettings() { return m_settings; }
    const SettingsConfig& getSettings() const { return m_settings; }

    // God mode setting
    bool isGodModeEnabled() const { return m_godModeEnabled; }
    void setGodModeEnabled(bool enabled) { m_godModeEnabled = enabled; }

    // Can continue (has active game)
    bool canContinue() const { return m_canContinue; }
    void setCanContinue(bool can) { m_canContinue = can; }

private:
    // State
    MainMenuState m_state = MainMenuState::MAIN;
    bool m_active = true;
    bool m_godModeEnabled = false;  // Default OFF
    bool m_canContinue = false;

    // Configurations
    WorldGenConfig m_worldGenConfig;
    EvolutionStartPreset m_evolutionPreset;
    SettingsConfig m_settings;

    // Callbacks
    StartGameCallback m_onStartGame;
    QuitCallback m_onQuit;
    ContinueCallback m_onContinue;
    SettingsChangedCallback m_onSettingsChanged;

    // ========================================================================
    // Rendering Functions
    // ========================================================================

    void renderMainMenu();
    void renderNewPlanet();
    void renderSettings();
    void renderLoadGame();
    void renderCredits();

    // Sub-sections of New Planet
    void renderPlanetTypeSection();
    void renderSeedSection();
    void renderWorldStructureSection();
    void renderBiomeMixSection();
    void renderClimateSection();
    void renderEvolutionSection();
    void renderGodModeToggle();

    // Sub-sections of Settings
    void renderGraphicsSettings();
    void renderPerformanceSettings();
    void renderSimulationSettings();
    void renderUISettings();
    void renderCameraSettings();
    void renderAudioSettings();

    // ========================================================================
    // Helpers
    // ========================================================================

    void randomizeSeed();
    const char* getPresetName(PlanetPreset preset) const;
    const char* getStarTypeName(WorldGenConfig::StarType type) const;
    const char* getDifficultyName(EvolutionStartPreset::Difficulty diff) const;
    const char* getQualityName(SettingsConfig::QualityPreset preset) const;
    void applyQualityPreset(SettingsConfig::QualityPreset preset);
    ImVec4 getPresetColor(PlanetPreset preset) const;
};

// ============================================================================
// Config Translation Helper (MainMenu -> ProceduralWorld)
// ============================================================================

// Translate MainMenu config to ProceduralWorld config
::WorldGenConfig translateToProceduralWorldConfig(const ui::WorldGenConfig& menuConfig);

} // namespace ui
