#include "MainMenu.h"
#include <cstdlib>
#include <ctime>
#include <random>

namespace ui {

// ============================================================================
// Constructor
// ============================================================================

MainMenu::MainMenu() {
    // Initialize random seed
    randomizeSeed();
}

// ============================================================================
// Main Render
// ============================================================================

void MainMenu::render() {
    if (!m_active) return;

    // Full screen overlay
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.07f, 0.10f, 0.98f));

    if (ImGui::Begin("##MainMenu", nullptr, flags)) {
        switch (m_state) {
            case MainMenuState::MAIN:
                renderMainMenu();
                break;
            case MainMenuState::NEW_PLANET:
                renderNewPlanet();
                break;
            case MainMenuState::SETTINGS:
                renderSettings();
                break;
            case MainMenuState::LOAD_GAME:
                renderLoadGame();
                break;
            case MainMenuState::CREDITS:
                renderCredits();
                break;
        }
    }
    ImGui::End();

    ImGui::PopStyleColor();
}

// ============================================================================
// Main Menu Screen
// ============================================================================

void MainMenu::renderMainMenu() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float centerX = viewport->Size.x * 0.5f;
    float centerY = viewport->Size.y * 0.5f;

    // Title
    ImGui::SetCursorPos(ImVec2(centerX - 200, centerY - 200));
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);  // Use default font, would use larger if available
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.95f, 1.0f, 1.0f));
    ImGui::SetWindowFontScale(2.5f);
    ImGui::Text("Organism Evolution");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    ImGui::PopFont();

    // Subtitle
    ImGui::SetCursorPos(ImVec2(centerX - 150, centerY - 150));
    ImGui::TextColored(ImVec4(0.6f, 0.7f, 0.8f, 1.0f), "Procedural Life Simulation");

    // Menu buttons
    float buttonWidth = 250.0f;
    float buttonHeight = 50.0f;
    float buttonX = centerX - buttonWidth * 0.5f;
    float startY = centerY - 50;
    float spacing = 60.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 12));

    // New Planet button
    ImGui::SetCursorPos(ImVec2(buttonX, startY));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.4f, 1.0f));
    if (ImGui::Button("New Planet", ImVec2(buttonWidth, buttonHeight))) {
        m_state = MainMenuState::NEW_PLANET;
    }
    ImGui::PopStyleColor(2);

    // Continue button (if game in progress)
    ImGui::SetCursorPos(ImVec2(buttonX, startY + spacing));
    if (m_canContinue) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.4f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.5f, 0.7f, 1.0f));
        if (ImGui::Button("Continue", ImVec2(buttonWidth, buttonHeight))) {
            if (m_onContinue) {
                m_onContinue();
            }
            m_active = false;
        }
        ImGui::PopStyleColor(2);
    } else {
        ImGui::BeginDisabled();
        ImGui::Button("Continue", ImVec2(buttonWidth, buttonHeight));
        ImGui::EndDisabled();
    }

    // Settings button
    ImGui::SetCursorPos(ImVec2(buttonX, startY + spacing * 2));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.35f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.45f, 0.5f, 1.0f));
    if (ImGui::Button("Settings", ImVec2(buttonWidth, buttonHeight))) {
        m_state = MainMenuState::SETTINGS;
    }
    ImGui::PopStyleColor(2);

    // Quit button
    ImGui::SetCursorPos(ImVec2(buttonX, startY + spacing * 3));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));
    if (ImGui::Button("Quit", ImVec2(buttonWidth, buttonHeight))) {
        if (m_onQuit) {
            m_onQuit();
        }
    }
    ImGui::PopStyleColor(2);

    ImGui::PopStyleVar(2);

    // Version info
    ImGui::SetCursorPos(ImVec2(20, viewport->Size.y - 30));
    ImGui::TextDisabled("Version 1.0 - Phase 8");
}

// ============================================================================
// New Planet Screen
// ============================================================================

void MainMenu::renderNewPlanet() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    // Header with back button
    ImGui::SetCursorPos(ImVec2(20, 20));
    if (ImGui::Button("< Back", ImVec2(80, 30))) {
        m_state = MainMenuState::MAIN;
    }

    ImGui::SameLine(viewport->Size.x * 0.5f - 100);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("Create New Planet");
    ImGui::SetWindowFontScale(1.0f);

    // Content area - two columns
    float leftColumnX = 50.0f;
    float rightColumnX = viewport->Size.x * 0.5f + 50.0f;
    float columnWidth = viewport->Size.x * 0.5f - 100.0f;
    float contentY = 80.0f;

    // Left column - Planet configuration
    ImGui::SetCursorPos(ImVec2(leftColumnX, contentY));
    ImGui::BeginChild("LeftColumn", ImVec2(columnWidth, viewport->Size.y - 180), true);

    renderPlanetTypeSection();
    ImGui::Separator();
    renderSeedSection();
    ImGui::Separator();
    renderWorldStructureSection();
    ImGui::Separator();
    renderBiomeMixSection();
    ImGui::Separator();
    renderClimateSection();

    ImGui::EndChild();

    // Right column - Evolution configuration
    ImGui::SetCursorPos(ImVec2(rightColumnX, contentY));
    ImGui::BeginChild("RightColumn", ImVec2(columnWidth, viewport->Size.y - 180), true);

    renderEvolutionSection();
    ImGui::Separator();
    renderGodModeToggle();

    ImGui::EndChild();

    // Start button at bottom
    float startButtonWidth = 300.0f;
    float startButtonHeight = 60.0f;
    ImGui::SetCursorPos(ImVec2(
        viewport->Size.x * 0.5f - startButtonWidth * 0.5f,
        viewport->Size.y - 90
    ));

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.4f, 1.0f));

    if (ImGui::Button("Generate Planet & Start", ImVec2(startButtonWidth, startButtonHeight))) {
        if (m_onStartGame) {
            m_onStartGame(m_worldGenConfig, m_evolutionPreset, m_godModeEnabled);
        }
        m_active = false;
        m_canContinue = true;
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}

void MainMenu::renderPlanetTypeSection() {
    ImGui::Text("Planet Type");
    ImGui::Spacing();

    // Quick toggle: Realistic vs Alien
    ImGui::Checkbox("Alien World", &m_worldGenConfig.isAlienWorld);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Enable for strange alien colors and atmospheres");
    }

    ImGui::Spacing();

    // Preset selection
    ImGui::Text("Theme Preset:");
    const PlanetPreset presets[] = {
        PlanetPreset::EARTH_LIKE, PlanetPreset::ALIEN_PURPLE, PlanetPreset::ALIEN_RED,
        PlanetPreset::ALIEN_BLUE, PlanetPreset::FROZEN_WORLD, PlanetPreset::DESERT_WORLD,
        PlanetPreset::OCEAN_WORLD, PlanetPreset::VOLCANIC_WORLD, PlanetPreset::BIOLUMINESCENT
    };

    int currentPreset = static_cast<int>(m_worldGenConfig.preset);
    ImGui::PushItemWidth(-1);

    if (ImGui::BeginCombo("##PresetCombo", getPresetName(m_worldGenConfig.preset))) {
        for (int i = 0; i < 9; ++i) {
            bool selected = (presets[i] == m_worldGenConfig.preset);
            ImVec4 color = getPresetColor(presets[i]);

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            if (ImGui::Selectable(getPresetName(presets[i]), selected)) {
                m_worldGenConfig.preset = presets[i];
                m_worldGenConfig.isAlienWorld = (i >= 1 && i <= 4) || (i == 8);
            }
            ImGui::PopStyleColor();

            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::PopItemWidth();
}

void MainMenu::renderSeedSection() {
    ImGui::Text("World Seed");
    ImGui::Spacing();

    ImGui::Checkbox("Random Seed", &m_worldGenConfig.useRandomSeed);

    if (!m_worldGenConfig.useRandomSeed) {
        // Manual seed entry
        char seedBuf[32];
        snprintf(seedBuf, sizeof(seedBuf), "%u", m_worldGenConfig.seed);
        ImGui::PushItemWidth(200);
        if (ImGui::InputText("Seed", seedBuf, sizeof(seedBuf), ImGuiInputTextFlags_CharsDecimal)) {
            m_worldGenConfig.seed = static_cast<uint32_t>(std::strtoul(seedBuf, nullptr, 10));
        }
        ImGui::PopItemWidth();
    }

    ImGui::SameLine();
    if (ImGui::Button("Randomize")) {
        randomizeSeed();
    }

    // Show seed fingerprint
    PlanetSeed ps(m_worldGenConfig.seed);
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Fingerprint: %s", ps.fingerprint.c_str());
}

void MainMenu::renderWorldStructureSection() {
    ImGui::Text("World Structure");
    ImGui::Spacing();

    // Region count (islands)
    ImGui::SliderInt("Islands/Regions", &m_worldGenConfig.regionCount, 1, 7);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Number of separate land masses. More islands = more isolated evolution.");
    }

    // World size
    ImGui::SliderFloat("World Size", &m_worldGenConfig.worldSize, 200.0f, 1000.0f, "%.0f");

    // Ocean coverage
    ImGui::SliderFloat("Ocean Coverage", &m_worldGenConfig.oceanCoverage, 0.0f, 0.8f, "%.0f%%");
}

void MainMenu::renderBiomeMixSection() {
    ImGui::Text("Biome Mix");
    ImGui::Spacing();

    ImGui::TextDisabled("Adjust weights for biome distribution:");

    ImGui::SliderFloat("Forest", &m_worldGenConfig.forestWeight, 0.0f, 2.0f);
    ImGui::SliderFloat("Grassland", &m_worldGenConfig.grasslandWeight, 0.0f, 2.0f);
    ImGui::SliderFloat("Desert", &m_worldGenConfig.desertWeight, 0.0f, 2.0f);
    ImGui::SliderFloat("Tundra", &m_worldGenConfig.tundraWeight, 0.0f, 2.0f);
    ImGui::SliderFloat("Wetland", &m_worldGenConfig.wetlandWeight, 0.0f, 2.0f);
    ImGui::SliderFloat("Mountain", &m_worldGenConfig.mountainWeight, 0.0f, 2.0f);
    ImGui::SliderFloat("Volcanic", &m_worldGenConfig.volcanicWeight, 0.0f, 2.0f);
}

void MainMenu::renderClimateSection() {
    ImGui::Text("Climate & Star");
    ImGui::Spacing();

    // Star type
    ImGui::Text("Star Type:");
    const WorldGenConfig::StarType starTypes[] = {
        WorldGenConfig::StarType::YELLOW_DWARF,
        WorldGenConfig::StarType::ORANGE_DWARF,
        WorldGenConfig::StarType::RED_DWARF,
        WorldGenConfig::StarType::BLUE_GIANT,
        WorldGenConfig::StarType::BINARY
    };

    if (ImGui::BeginCombo("##StarType", getStarTypeName(m_worldGenConfig.starType))) {
        for (int i = 0; i < 5; ++i) {
            bool selected = (starTypes[i] == m_worldGenConfig.starType);
            if (ImGui::Selectable(getStarTypeName(starTypes[i]), selected)) {
                m_worldGenConfig.starType = starTypes[i];
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Spacing();

    // Climate sliders
    ImGui::SliderFloat("Temperature", &m_worldGenConfig.temperatureBias, 0.0f, 1.0f, "Cold <- %.2f -> Hot");
    ImGui::SliderFloat("Moisture", &m_worldGenConfig.moistureBias, 0.0f, 1.0f, "Dry <- %.2f -> Wet");
    ImGui::SliderFloat("Seasons", &m_worldGenConfig.seasonIntensity, 0.0f, 1.0f, "%.2f");
}

void MainMenu::renderEvolutionSection() {
    ImGui::Text("Evolution Settings");
    ImGui::Spacing();

    // Difficulty preset
    ImGui::Text("Difficulty:");
    const EvolutionStartPreset::Difficulty difficulties[] = {
        EvolutionStartPreset::Difficulty::SANDBOX,
        EvolutionStartPreset::Difficulty::BALANCED,
        EvolutionStartPreset::Difficulty::HARSH,
        EvolutionStartPreset::Difficulty::EXTINCTION
    };

    if (ImGui::BeginCombo("##Difficulty", getDifficultyName(m_evolutionPreset.difficulty))) {
        for (int i = 0; i < 4; ++i) {
            bool selected = (difficulties[i] == m_evolutionPreset.difficulty);
            if (ImGui::Selectable(getDifficultyName(difficulties[i]), selected)) {
                m_evolutionPreset.difficulty = difficulties[i];
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Starting populations
    ImGui::Text("Starting Life:");
    ImGui::SliderInt("Herbivores", &m_evolutionPreset.herbivoreCount, 0, 200);
    ImGui::SliderInt("Carnivores", &m_evolutionPreset.carnivoreCount, 0, 50);
    ImGui::SliderInt("Aquatic", &m_evolutionPreset.aquaticCount, 0, 100);
    ImGui::SliderInt("Flying", &m_evolutionPreset.flyingCount, 0, 50);
    ImGui::SliderInt("Plants", &m_evolutionPreset.plantCount, 50, 500);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Evolution parameters
    ImGui::Text("Evolution Parameters:");
    ImGui::SliderFloat("Mutation Rate", &m_evolutionPreset.mutationRate, 0.01f, 0.5f, "%.2f");
    ImGui::SliderFloat("Speciation Threshold", &m_evolutionPreset.speciationThreshold, 0.1f, 0.8f, "%.2f");

    ImGui::Checkbox("Sexual Reproduction", &m_evolutionPreset.enableSexualReproduction);
    ImGui::Checkbox("Coevolution", &m_evolutionPreset.enableCoevolution);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Guidance
    ImGui::Checkbox("Evolution Guidance", &m_evolutionPreset.enableGuidance);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Subtle nudges to help evolution discover interesting niches.\n"
                          "Disable for pure random evolution.");
    }

    if (m_evolutionPreset.enableGuidance) {
        ImGui::SliderFloat("Guidance Strength", &m_evolutionPreset.guidanceStrength, 0.0f, 1.0f, "%.2f");
    }
}

void MainMenu::renderGodModeToggle() {
    ImGui::Text("Game Mode");
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.5f, 1.0f), "Observer Mode (Default):");
    ImGui::TextWrapped("Watch evolution unfold naturally. Inspect creatures, follow them with the camera, "
                       "and observe the ecosystem develop.");

    ImGui::Spacing();

    ImGui::Checkbox("Enable God Tools", &m_godModeEnabled);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Unlock spawning, terraforming, mutations, and other sandbox tools.\n"
                          "This changes the experience from observation to active manipulation.");
    }

    if (m_godModeEnabled) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f),
            "God Mode enabled - you can modify the simulation.");
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f),
            "Pure observer mode - watch life evolve naturally.");
    }
}

// ============================================================================
// Settings Screen
// ============================================================================

void MainMenu::renderSettings() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    // Header with back button
    ImGui::SetCursorPos(ImVec2(20, 20));
    if (ImGui::Button("< Back", ImVec2(80, 30))) {
        m_state = MainMenuState::MAIN;
        if (m_onSettingsChanged) {
            m_onSettingsChanged(m_settings);
        }
    }

    ImGui::SameLine(viewport->Size.x * 0.5f - 60);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("Settings");
    ImGui::SetWindowFontScale(1.0f);

    // Tab bar for different settings categories
    ImGui::SetCursorPos(ImVec2(50, 80));
    ImGui::BeginChild("SettingsContent", ImVec2(viewport->Size.x - 100, viewport->Size.y - 140), true);

    if (ImGui::BeginTabBar("SettingsTabs")) {
        if (ImGui::BeginTabItem("Graphics")) {
            renderGraphicsSettings();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Performance")) {
            renderPerformanceSettings();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Simulation")) {
            renderSimulationSettings();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Interface")) {
            renderUISettings();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Camera")) {
            renderCameraSettings();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Audio")) {
            renderAudioSettings();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::EndChild();
}

void MainMenu::renderGraphicsSettings() {
    ImGui::Text("Graphics Quality");
    ImGui::Spacing();

    // Quality preset
    const SettingsConfig::QualityPreset presets[] = {
        SettingsConfig::QualityPreset::LOW,
        SettingsConfig::QualityPreset::MEDIUM,
        SettingsConfig::QualityPreset::HIGH,
        SettingsConfig::QualityPreset::ULTRA,
        SettingsConfig::QualityPreset::CUSTOM
    };

    if (ImGui::BeginCombo("Quality Preset", getQualityName(m_settings.qualityPreset))) {
        for (int i = 0; i < 5; ++i) {
            bool selected = (presets[i] == m_settings.qualityPreset);
            if (ImGui::Selectable(getQualityName(presets[i]), selected)) {
                m_settings.qualityPreset = presets[i];
                if (presets[i] != SettingsConfig::QualityPreset::CUSTOM) {
                    applyQualityPreset(presets[i]);
                }
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Individual settings
    if (ImGui::SliderInt("Render Distance", &m_settings.renderDistance, 100, 1000)) {
        m_settings.qualityPreset = SettingsConfig::QualityPreset::CUSTOM;
    }

    if (ImGui::SliderInt("Shadow Quality", &m_settings.shadowQuality, 0, 3, "%d (0=Off, 3=Ultra)")) {
        m_settings.qualityPreset = SettingsConfig::QualityPreset::CUSTOM;
    }

    if (ImGui::SliderInt("Grass Density", &m_settings.grassDensity, 0, 3)) {
        m_settings.qualityPreset = SettingsConfig::QualityPreset::CUSTOM;
    }

    if (ImGui::SliderInt("Tree LOD", &m_settings.treeLOD, 0, 3)) {
        m_settings.qualityPreset = SettingsConfig::QualityPreset::CUSTOM;
    }

    if (ImGui::SliderInt("Creature Detail", &m_settings.creatureDetail, 0, 3)) {
        m_settings.qualityPreset = SettingsConfig::QualityPreset::CUSTOM;
    }

    ImGui::Spacing();

    // Effect toggles
    if (ImGui::Checkbox("SSAO", &m_settings.enableSSAO)) {
        m_settings.qualityPreset = SettingsConfig::QualityPreset::CUSTOM;
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Bloom", &m_settings.enableBloom)) {
        m_settings.qualityPreset = SettingsConfig::QualityPreset::CUSTOM;
    }

    if (ImGui::Checkbox("Volumetric Fog", &m_settings.enableVolumetricFog)) {
        m_settings.qualityPreset = SettingsConfig::QualityPreset::CUSTOM;
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Water Reflections", &m_settings.enableWaterReflections)) {
        m_settings.qualityPreset = SettingsConfig::QualityPreset::CUSTOM;
    }

    if (ImGui::Checkbox("Dynamic Shadows", &m_settings.enableDynamicShadows)) {
        m_settings.qualityPreset = SettingsConfig::QualityPreset::CUSTOM;
    }
}

void MainMenu::renderPerformanceSettings() {
    ImGui::Text("Performance Settings");
    ImGui::Spacing();

    ImGui::SliderInt("Target FPS", &m_settings.targetFPS, 30, 144);
    ImGui::Checkbox("V-Sync", &m_settings.enableVSync);
    ImGui::Checkbox("FPS Limit", &m_settings.enableFPSLimit);

    ImGui::Spacing();

    ImGui::SliderFloat("Render Scale", &m_settings.renderScale, 0.5f, 2.0f, "%.1fx");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Lower values improve performance, higher values improve quality");
    }

    ImGui::Checkbox("Multithreading", &m_settings.enableMultithreading);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Simulation Limits");
    ImGui::SliderInt("Max Creatures", &m_settings.maxCreatures, 100, 5000);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Higher values allow larger populations but may impact performance");
    }
}

void MainMenu::renderSimulationSettings() {
    ImGui::Text("Simulation Defaults");
    ImGui::Spacing();

    ImGui::SliderFloat("Default Sim Speed", &m_settings.defaultSimSpeed, 0.25f, 8.0f, "%.2fx");
    ImGui::Checkbox("Pause on Start", &m_settings.pauseOnStart);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Auto-Save");
    ImGui::Checkbox("Enable Auto-Save", &m_settings.autoSave);

    if (m_settings.autoSave) {
        int intervalMinutes = m_settings.autoSaveInterval / 60;
        if (ImGui::SliderInt("Interval (minutes)", &intervalMinutes, 1, 30)) {
            m_settings.autoSaveInterval = intervalMinutes * 60;
        }
    }
}

void MainMenu::renderUISettings() {
    ImGui::Text("Interface Settings");
    ImGui::Spacing();

    ImGui::SliderFloat("UI Scale", &m_settings.uiScale, 0.75f, 1.5f, "%.2fx");

    ImGui::Spacing();

    ImGui::Checkbox("Show FPS Counter", &m_settings.showFPS);
    ImGui::Checkbox("Show Minimap", &m_settings.showMinimap);
    ImGui::Checkbox("Show Nametags", &m_settings.showNametags);

    if (m_settings.showNametags) {
        ImGui::SliderFloat("Nametag Distance", &m_settings.nametagDistance, 10.0f, 200.0f, "%.0f");
    }

    ImGui::Checkbox("Show Tooltips", &m_settings.showTooltips);
}

void MainMenu::renderCameraSettings() {
    ImGui::Text("Camera Controls");
    ImGui::Spacing();

    ImGui::SliderFloat("Mouse Sensitivity", &m_settings.cameraSensitivity, 0.25f, 2.0f, "%.2f");
    ImGui::SliderFloat("Movement Speed", &m_settings.cameraSpeed, 5.0f, 100.0f, "%.0f");
    ImGui::Checkbox("Invert Y Axis", &m_settings.invertY);
}

void MainMenu::renderAudioSettings() {
    ImGui::Text("Audio Levels");
    ImGui::Spacing();

    ImGui::SliderFloat("Master Volume", &m_settings.masterVolume, 0.0f, 1.0f, "%.0f%%");
    ImGui::SliderFloat("Music Volume", &m_settings.musicVolume, 0.0f, 1.0f, "%.0f%%");
    ImGui::SliderFloat("Effects Volume", &m_settings.sfxVolume, 0.0f, 1.0f, "%.0f%%");
    ImGui::SliderFloat("Ambient Volume", &m_settings.ambientVolume, 0.0f, 1.0f, "%.0f%%");

    ImGui::Spacing();

    ImGui::Checkbox("Creature Voices", &m_settings.enableCreatureVoices);
}

// ============================================================================
// Load Game Screen
// ============================================================================

void MainMenu::renderLoadGame() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetCursorPos(ImVec2(20, 20));
    if (ImGui::Button("< Back", ImVec2(80, 30))) {
        m_state = MainMenuState::MAIN;
    }

    ImGui::SetCursorPos(ImVec2(viewport->Size.x * 0.5f - 100, viewport->Size.y * 0.5f));
    ImGui::Text("Load Game - Coming Soon");
}

// ============================================================================
// Credits Screen
// ============================================================================

void MainMenu::renderCredits() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetCursorPos(ImVec2(20, 20));
    if (ImGui::Button("< Back", ImVec2(80, 30))) {
        m_state = MainMenuState::MAIN;
    }

    ImGui::SetCursorPos(ImVec2(viewport->Size.x * 0.5f - 100, viewport->Size.y * 0.5f - 50));
    ImGui::Text("Organism Evolution");
    ImGui::SetCursorPos(ImVec2(viewport->Size.x * 0.5f - 100, viewport->Size.y * 0.5f));
    ImGui::Text("A Procedural Life Simulation");
}

// ============================================================================
// Helpers
// ============================================================================

void MainMenu::randomizeSeed() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis;
    m_worldGenConfig.seed = dis(gen);
}

const char* MainMenu::getPresetName(PlanetPreset preset) const {
    switch (preset) {
        case PlanetPreset::EARTH_LIKE: return "Earth-Like";
        case PlanetPreset::ALIEN_PURPLE: return "Alien Purple";
        case PlanetPreset::ALIEN_RED: return "Alien Red";
        case PlanetPreset::ALIEN_BLUE: return "Alien Blue";
        case PlanetPreset::FROZEN_WORLD: return "Frozen World";
        case PlanetPreset::DESERT_WORLD: return "Desert World";
        case PlanetPreset::OCEAN_WORLD: return "Ocean World";
        case PlanetPreset::VOLCANIC_WORLD: return "Volcanic World";
        case PlanetPreset::BIOLUMINESCENT: return "Bioluminescent";
        case PlanetPreset::CRYSTAL_WORLD: return "Crystal World";
        case PlanetPreset::TOXIC_WORLD: return "Toxic World";
        case PlanetPreset::ANCIENT_WORLD: return "Ancient World";
        case PlanetPreset::CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

const char* MainMenu::getStarTypeName(WorldGenConfig::StarType type) const {
    switch (type) {
        case WorldGenConfig::StarType::YELLOW_DWARF: return "Yellow Dwarf (Earth-like)";
        case WorldGenConfig::StarType::ORANGE_DWARF: return "Orange Dwarf (Warmer)";
        case WorldGenConfig::StarType::RED_DWARF: return "Red Dwarf (Dim, Red)";
        case WorldGenConfig::StarType::BLUE_GIANT: return "Blue Giant (Bright, Blue)";
        case WorldGenConfig::StarType::BINARY: return "Binary Stars";
        default: return "Unknown";
    }
}

const char* MainMenu::getDifficultyName(EvolutionStartPreset::Difficulty diff) const {
    switch (diff) {
        case EvolutionStartPreset::Difficulty::SANDBOX: return "Sandbox (Easy)";
        case EvolutionStartPreset::Difficulty::BALANCED: return "Balanced (Normal)";
        case EvolutionStartPreset::Difficulty::HARSH: return "Harsh (Hard)";
        case EvolutionStartPreset::Difficulty::EXTINCTION: return "Extinction (Extreme)";
        default: return "Unknown";
    }
}

const char* MainMenu::getQualityName(SettingsConfig::QualityPreset preset) const {
    switch (preset) {
        case SettingsConfig::QualityPreset::LOW: return "Low";
        case SettingsConfig::QualityPreset::MEDIUM: return "Medium";
        case SettingsConfig::QualityPreset::HIGH: return "High";
        case SettingsConfig::QualityPreset::ULTRA: return "Ultra";
        case SettingsConfig::QualityPreset::CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

void MainMenu::applyQualityPreset(SettingsConfig::QualityPreset preset) {
    switch (preset) {
        case SettingsConfig::QualityPreset::LOW:
            m_settings.renderDistance = 200;
            m_settings.shadowQuality = 0;
            m_settings.grassDensity = 0;
            m_settings.treeLOD = 0;
            m_settings.creatureDetail = 0;
            m_settings.enableSSAO = false;
            m_settings.enableBloom = false;
            m_settings.enableVolumetricFog = false;
            m_settings.enableWaterReflections = false;
            m_settings.enableDynamicShadows = false;
            break;

        case SettingsConfig::QualityPreset::MEDIUM:
            m_settings.renderDistance = 350;
            m_settings.shadowQuality = 1;
            m_settings.grassDensity = 1;
            m_settings.treeLOD = 1;
            m_settings.creatureDetail = 1;
            m_settings.enableSSAO = false;
            m_settings.enableBloom = true;
            m_settings.enableVolumetricFog = false;
            m_settings.enableWaterReflections = true;
            m_settings.enableDynamicShadows = true;
            break;

        case SettingsConfig::QualityPreset::HIGH:
            m_settings.renderDistance = 500;
            m_settings.shadowQuality = 2;
            m_settings.grassDensity = 2;
            m_settings.treeLOD = 2;
            m_settings.creatureDetail = 2;
            m_settings.enableSSAO = true;
            m_settings.enableBloom = true;
            m_settings.enableVolumetricFog = true;
            m_settings.enableWaterReflections = true;
            m_settings.enableDynamicShadows = true;
            break;

        case SettingsConfig::QualityPreset::ULTRA:
            m_settings.renderDistance = 1000;
            m_settings.shadowQuality = 3;
            m_settings.grassDensity = 3;
            m_settings.treeLOD = 3;
            m_settings.creatureDetail = 3;
            m_settings.enableSSAO = true;
            m_settings.enableBloom = true;
            m_settings.enableVolumetricFog = true;
            m_settings.enableWaterReflections = true;
            m_settings.enableDynamicShadows = true;
            break;

        default:
            break;
    }
}

ImVec4 MainMenu::getPresetColor(PlanetPreset preset) const {
    switch (preset) {
        case PlanetPreset::EARTH_LIKE: return ImVec4(0.4f, 0.7f, 0.4f, 1.0f);
        case PlanetPreset::ALIEN_PURPLE: return ImVec4(0.7f, 0.4f, 0.8f, 1.0f);
        case PlanetPreset::ALIEN_RED: return ImVec4(0.9f, 0.4f, 0.3f, 1.0f);
        case PlanetPreset::ALIEN_BLUE: return ImVec4(0.3f, 0.5f, 0.9f, 1.0f);
        case PlanetPreset::FROZEN_WORLD: return ImVec4(0.7f, 0.9f, 1.0f, 1.0f);
        case PlanetPreset::DESERT_WORLD: return ImVec4(0.9f, 0.7f, 0.4f, 1.0f);
        case PlanetPreset::OCEAN_WORLD: return ImVec4(0.2f, 0.5f, 0.8f, 1.0f);
        case PlanetPreset::VOLCANIC_WORLD: return ImVec4(0.9f, 0.3f, 0.1f, 1.0f);
        case PlanetPreset::BIOLUMINESCENT: return ImVec4(0.3f, 0.9f, 0.7f, 1.0f);
        default: return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    }
}

} // namespace ui
