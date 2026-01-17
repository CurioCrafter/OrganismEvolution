#pragma once

#include "imgui.h"
#include "EvolutionControlPanel.h"
#include "EnvironmentControlPanel.h"
#include "CreatureSpawnPanel.h"
#include <functional>
#include <string>
#include <vector>
#include <map>

namespace ui {

// ============================================================================
// Scenario Presets System
// ============================================================================
// Provides pre-built simulation scenarios for experimentation:
// - 10+ preset scenarios (Cambrian Explosion, Ice Age, Paradise Island, etc.)
// - Custom scenario builder
// - Save/load custom presets
// - Scenario descriptions and goals

// Initial creature population for a scenario
struct ScenarioCreatureSpawn {
    CreatureType type;
    int count;
    bool useCustomGenome = false;
    Genome customGenome;
};

// Complete scenario definition
struct ScenarioPreset {
    std::string name;
    std::string description;
    std::string goals;  // What to observe/achieve
    std::string difficulty;

    // Parameters
    EvolutionParameters evolution;
    EnvironmentParameters environment;

    // Initial population
    std::vector<ScenarioCreatureSpawn> initialPopulation;

    // Special flags
    bool clearExisting = true;  // Clear all creatures before spawning
    float initialFoodMultiplier = 1.0f;
    bool triggerEvent = false;
    std::string eventToTrigger;
};

class ScenarioPresetsPanel {
public:
    ScenarioPresetsPanel();
    ~ScenarioPresetsPanel() = default;

    // Main render function
    void render();

    // Render as section in another panel
    void renderSection();

    // Callbacks
    using ApplyScenarioCallback = std::function<void(const ScenarioPreset&)>;
    using SaveCurrentAsPresetCallback = std::function<ScenarioPreset()>;
    using ExportPresetCallback = std::function<bool(const ScenarioPreset&, const std::string& filename)>;
    using ImportPresetCallback = std::function<bool(const std::string& filename)>;

    void setApplyScenarioCallback(ApplyScenarioCallback cb) { m_applyScenario = cb; }
    void setSaveCurrentCallback(SaveCurrentAsPresetCallback cb) { m_saveCurrentAsPreset = cb; }
    void setExportCallback(ExportPresetCallback cb) { m_exportPreset = cb; }
    void setImportCallback(ImportPresetCallback cb) { m_importPreset = cb; }

    // Access presets
    const std::vector<ScenarioPreset>& getPresets() const { return m_presets; }
    void addCustomPreset(const ScenarioPreset& preset);

    // Panel visibility
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    void toggleVisible() { m_visible = !m_visible; }

private:
    bool m_visible = true;
    std::vector<ScenarioPreset> m_presets;
    int m_selectedPresetIndex = 0;

    // Custom preset builder state
    char m_customNameBuffer[64] = "My Custom Scenario";
    char m_customDescBuffer[256] = "";
    bool m_showCustomBuilder = false;

    // Callbacks
    ApplyScenarioCallback m_applyScenario;
    SaveCurrentAsPresetCallback m_saveCurrentAsPreset;
    ExportPresetCallback m_exportPreset;
    ImportPresetCallback m_importPreset;

    // Initialize built-in presets
    void initializePresets();

    // UI helpers
    void renderPresetList();
    void renderPresetDetails();
    void renderCustomBuilder();

    // Built-in preset creators
    ScenarioPreset createCambrianExplosion();
    ScenarioPreset createIceAge();
    ScenarioPreset createParadiseIsland();
    ScenarioPreset createPredatorHell();
    ScenarioPreset createAquaticWorld();
    ScenarioPreset createSkyKingdom();
    ScenarioPreset createInsectPlanet();
    ScenarioPreset createAlienWorld();
    ScenarioPreset createDarwinsFinches();
    ScenarioPreset createMassExtinction();
    ScenarioPreset createBalancedEcosystem();
    ScenarioPreset createSurvivalOfTheFittest();
};

// ============================================================================
// Implementation
// ============================================================================

inline ScenarioPresetsPanel::ScenarioPresetsPanel() {
    initializePresets();
}

inline void ScenarioPresetsPanel::initializePresets() {
    m_presets.clear();

    m_presets.push_back(createBalancedEcosystem());
    m_presets.push_back(createCambrianExplosion());
    m_presets.push_back(createIceAge());
    m_presets.push_back(createParadiseIsland());
    m_presets.push_back(createPredatorHell());
    m_presets.push_back(createAquaticWorld());
    m_presets.push_back(createSkyKingdom());
    m_presets.push_back(createInsectPlanet());
    m_presets.push_back(createAlienWorld());
    m_presets.push_back(createDarwinsFinches());
    m_presets.push_back(createMassExtinction());
    m_presets.push_back(createSurvivalOfTheFittest());
}

inline void ScenarioPresetsPanel::render() {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Scenario Presets", &m_visible)) {
        renderSection();
    }
    ImGui::End();
}

inline void ScenarioPresetsPanel::renderSection() {
    // Preset list on left, details on right
    ImGui::Columns(2, "ScenarioColumns");

    // Left column: Preset list
    ImGui::Text("Available Scenarios:");
    ImGui::Separator();
    renderPresetList();

    ImGui::NextColumn();

    // Right column: Details and apply
    renderPresetDetails();

    ImGui::Columns(1);

    ImGui::Separator();

    // Custom builder toggle
    if (ImGui::CollapsingHeader("Custom Scenario Builder")) {
        renderCustomBuilder();
    }
}

inline void ScenarioPresetsPanel::renderPresetList() {
    ImGui::BeginChild("PresetList", ImVec2(0, 350), true);

    for (int i = 0; i < static_cast<int>(m_presets.size()); ++i) {
        const auto& preset = m_presets[i];

        // Color code by difficulty
        ImVec4 diffColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        if (preset.difficulty == "Easy") diffColor = ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
        else if (preset.difficulty == "Medium") diffColor = ImVec4(0.8f, 0.8f, 0.3f, 1.0f);
        else if (preset.difficulty == "Hard") diffColor = ImVec4(0.9f, 0.5f, 0.2f, 1.0f);
        else if (preset.difficulty == "Extreme") diffColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
        else if (preset.difficulty == "Experimental") diffColor = ImVec4(0.7f, 0.3f, 0.9f, 1.0f);

        bool isSelected = (i == m_selectedPresetIndex);

        ImGui::PushID(i);

        if (ImGui::Selectable("##preset", isSelected, 0, ImVec2(0, 40))) {
            m_selectedPresetIndex = i;
        }

        // Overlay the text on the selectable
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetContentRegionAvail().x);

        ImGui::BeginGroup();
        ImGui::Text("%s", preset.name.c_str());
        ImGui::TextColored(diffColor, "[%s]", preset.difficulty.c_str());
        ImGui::EndGroup();

        ImGui::PopID();
    }

    ImGui::EndChild();
}

inline void ScenarioPresetsPanel::renderPresetDetails() {
    if (m_selectedPresetIndex < 0 || m_selectedPresetIndex >= static_cast<int>(m_presets.size())) {
        ImGui::Text("Select a scenario");
        return;
    }

    const auto& preset = m_presets[m_selectedPresetIndex];

    // Name and difficulty
    ImGui::Text("%s", preset.name.c_str());
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Difficulty: %s", preset.difficulty.c_str());

    ImGui::Separator();

    // Description
    ImGui::Text("Description:");
    ImGui::TextWrapped("%s", preset.description.c_str());

    ImGui::Spacing();

    // Goals
    ImGui::Text("Goals/What to Observe:");
    ImGui::TextWrapped("%s", preset.goals.c_str());

    ImGui::Separator();

    // Initial population summary
    ImGui::Text("Starting Population:");
    for (const auto& spawn : preset.initialPopulation) {
        ImGui::BulletText("%d x %s", spawn.count, getCreatureTypeName(spawn.type));
    }

    ImGui::Spacing();

    // Environment summary
    ImGui::Text("Environment: Temp=%.1f, Moisture=%.1f, Food=%.1fx",
        preset.environment.temperature,
        preset.environment.moisture,
        preset.environment.foodAbundance);

    ImGui::Separator();

    // Apply button
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));

    if (ImGui::Button("APPLY SCENARIO", ImVec2(-1, 40))) {
        if (m_applyScenario) {
            m_applyScenario(preset);
        }
    }

    ImGui::PopStyleColor(2);

    if (preset.clearExisting) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Warning: This will clear existing creatures");
    }
}

inline void ScenarioPresetsPanel::renderCustomBuilder() {
    ImGui::Text("Create a custom scenario from current settings:");

    ImGui::InputText("Name", m_customNameBuffer, sizeof(m_customNameBuffer));
    ImGui::InputTextMultiline("Description", m_customDescBuffer, sizeof(m_customDescBuffer),
        ImVec2(-1, 60));

    if (ImGui::Button("Save Current as Preset")) {
        if (m_saveCurrentAsPreset) {
            ScenarioPreset custom = m_saveCurrentAsPreset();
            custom.name = m_customNameBuffer;
            custom.description = m_customDescBuffer;
            custom.difficulty = "Custom";
            addCustomPreset(custom);
        }
    }

    ImGui::Separator();

    ImGui::Text("Import/Export:");
    if (ImGui::Button("Export Selected")) {
        if (m_exportPreset && m_selectedPresetIndex >= 0) {
            m_exportPreset(m_presets[m_selectedPresetIndex], "scenario_export.json");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Import from File")) {
        if (m_importPreset) {
            m_importPreset("scenario_import.json");
        }
    }
}

inline void ScenarioPresetsPanel::addCustomPreset(const ScenarioPreset& preset) {
    m_presets.push_back(preset);
    m_selectedPresetIndex = static_cast<int>(m_presets.size()) - 1;
}

// ============================================================================
// Built-in Preset Creators
// ============================================================================

inline ScenarioPreset ScenarioPresetsPanel::createBalancedEcosystem() {
    ScenarioPreset preset;
    preset.name = "Balanced Ecosystem";
    preset.description = "A stable starting point with balanced predator-prey ratios and moderate conditions. "
                        "Good for observing natural evolution without extreme pressures.";
    preset.goals = "Observe how species diversify over time. Watch for speciation events and food chain dynamics.";
    preset.difficulty = "Easy";

    preset.environment.temperature = 0.5f;
    preset.environment.moisture = 0.5f;
    preset.environment.foodAbundance = 1.0f;
    preset.environment.predatorPressure = 1.0f;

    preset.evolution.mutationRate = 0.15f;
    preset.evolution.selectionPressure = 1.0f;

    preset.initialPopulation = {
        {CreatureType::GRAZER, 30},
        {CreatureType::BROWSER, 15},
        {CreatureType::FRUGIVORE, 20},
        {CreatureType::SMALL_PREDATOR, 8},
        {CreatureType::APEX_PREDATOR, 5},
        {CreatureType::OMNIVORE, 5}
    };

    return preset;
}

inline ScenarioPreset ScenarioPresetsPanel::createCambrianExplosion() {
    ScenarioPreset preset;
    preset.name = "Cambrian Explosion";
    preset.description = "Rapid speciation scenario with high mutation rates and many empty ecological niches. "
                        "Watch life diversify explosively into many new forms.";
    preset.goals = "Observe rapid speciation, emergence of new body plans, and filling of ecological niches.";
    preset.difficulty = "Medium";

    preset.environment.temperature = 0.6f;
    preset.environment.moisture = 0.7f;
    preset.environment.foodAbundance = 2.0f;
    preset.environment.waterLevel = 0.4f;

    preset.evolution.mutationRate = 0.35f;
    preset.evolution.mutationStrength = 0.5f;
    preset.evolution.speciationThreshold = 0.25f;
    preset.evolution.noveltyWeight = 0.5f;

    preset.initialPopulation = {
        {CreatureType::GRAZER, 20},
        {CreatureType::FRUGIVORE, 20},
        {CreatureType::AQUATIC_HERBIVORE, 30},
        {CreatureType::AQUATIC, 15}
    };

    preset.initialFoodMultiplier = 2.0f;

    return preset;
}

inline ScenarioPreset ScenarioPresetsPanel::createIceAge() {
    ScenarioPreset preset;
    preset.name = "Ice Age";
    preset.description = "A harsh, cold world with scarce resources. Only the most efficient and adapted will survive. "
                        "Tests evolution under extreme environmental pressure.";
    preset.goals = "See which adaptations emerge for survival: larger size for heat retention, efficiency, hibernation behaviors.";
    preset.difficulty = "Hard";

    preset.environment.temperature = 0.1f;
    preset.environment.moisture = 0.25f;
    preset.environment.foodAbundance = 0.25f;
    preset.environment.seasonStrength = 2.0f;

    preset.evolution.mutationRate = 0.2f;
    preset.evolution.selectionPressure = 2.5f;
    preset.evolution.efficiencyBoost = 0.5f;
    preset.evolution.sizeBoost = 0.3f;

    preset.initialPopulation = {
        {CreatureType::GRAZER, 25},
        {CreatureType::BROWSER, 10},
        {CreatureType::APEX_PREDATOR, 5},
        {CreatureType::SCAVENGER, 5}
    };

    preset.triggerEvent = true;
    preset.eventToTrigger = "ice_age_start";

    return preset;
}

inline ScenarioPreset ScenarioPresetsPanel::createParadiseIsland() {
    ScenarioPreset preset;
    preset.name = "Paradise Island";
    preset.description = "Abundant food, warm climate, minimal predators. A paradise for herbivores. "
                        "Watch what happens when survival pressure is removed.";
    preset.goals = "Observe population explosions, relaxed selection, and potential for 'lazy' evolution.";
    preset.difficulty = "Easy";

    preset.environment.temperature = 0.65f;
    preset.environment.moisture = 0.7f;
    preset.environment.foodAbundance = 3.0f;
    preset.environment.predatorPressure = 0.1f;
    preset.environment.waterLevel = 0.35f;

    preset.evolution.mutationRate = 0.1f;
    preset.evolution.selectionPressure = 0.3f;

    preset.initialPopulation = {
        {CreatureType::GRAZER, 40},
        {CreatureType::BROWSER, 25},
        {CreatureType::FRUGIVORE, 35},
        {CreatureType::FLYING_BIRD, 15}
    };

    preset.initialFoodMultiplier = 3.0f;

    return preset;
}

inline ScenarioPreset ScenarioPresetsPanel::createPredatorHell() {
    ScenarioPreset preset;
    preset.name = "Predator Hell";
    preset.description = "Overwhelming predator presence. Herbivores must evolve extreme survival adaptations or perish.";
    preset.goals = "Observe evolution of defensive traits: speed, camouflage, herd behavior, early reproduction.";
    preset.difficulty = "Extreme";

    preset.environment.temperature = 0.55f;
    preset.environment.moisture = 0.4f;
    preset.environment.foodAbundance = 0.8f;
    preset.environment.predatorPressure = 3.0f;

    preset.evolution.mutationRate = 0.25f;
    preset.evolution.selectionPressure = 2.0f;
    preset.evolution.speedBoost = 0.3f;

    preset.initialPopulation = {
        {CreatureType::GRAZER, 30},
        {CreatureType::FRUGIVORE, 30},
        {CreatureType::SMALL_PREDATOR, 15},
        {CreatureType::APEX_PREDATOR, 12},
        {CreatureType::AERIAL_PREDATOR, 8}
    };

    return preset;
}

inline ScenarioPreset ScenarioPresetsPanel::createAquaticWorld() {
    ScenarioPreset preset;
    preset.name = "Aquatic World";
    preset.description = "A water-dominated world where fish and aquatic creatures rule. "
                        "Observe marine ecosystem evolution and the arms race between predator and prey fish.";
    preset.goals = "Watch schooling behavior emerge, predator hunting strategies, and aquatic speciation.";
    preset.difficulty = "Medium";

    preset.environment.temperature = 0.55f;
    preset.environment.moisture = 0.9f;
    preset.environment.foodAbundance = 1.2f;
    preset.environment.waterLevel = 0.5f;

    preset.evolution.mutationRate = 0.18f;
    preset.evolution.speciationThreshold = 0.35f;

    preset.initialPopulation = {
        {CreatureType::AQUATIC, 40},
        {CreatureType::AQUATIC_HERBIVORE, 50},
        {CreatureType::AQUATIC_PREDATOR, 15},
        {CreatureType::AQUATIC_APEX, 5},
        {CreatureType::AMPHIBIAN, 10}
    };

    return preset;
}

inline ScenarioPreset ScenarioPresetsPanel::createSkyKingdom() {
    ScenarioPreset preset;
    preset.name = "Sky Kingdom";
    preset.description = "A world dominated by flying creatures. Birds, insects, and aerial predators compete for sky supremacy.";
    preset.goals = "Observe aerial combat strategies, migration patterns, and competition for airspace.";
    preset.difficulty = "Medium";

    preset.environment.temperature = 0.5f;
    preset.environment.moisture = 0.5f;
    preset.environment.foodAbundance = 1.0f;
    preset.environment.terrainRoughness = 0.7f;

    preset.evolution.mutationRate = 0.2f;
    preset.evolution.selectionPressure = 1.5f;

    preset.initialPopulation = {
        {CreatureType::FLYING, 20},
        {CreatureType::FLYING_BIRD, 30},
        {CreatureType::FLYING_INSECT, 40},
        {CreatureType::AERIAL_PREDATOR, 10},
        {CreatureType::FRUGIVORE, 20}  // Ground prey
    };

    return preset;
}

inline ScenarioPreset ScenarioPresetsPanel::createInsectPlanet() {
    ScenarioPreset preset;
    preset.name = "Insect Planet";
    preset.description = "Small creatures dominate this world. Fast reproduction, high mutation rates, and swarm behaviors.";
    preset.goals = "Observe rapid generational turnover and evolution at an accelerated pace.";
    preset.difficulty = "Medium";

    preset.environment.temperature = 0.7f;
    preset.environment.moisture = 0.6f;
    preset.environment.foodAbundance = 1.5f;

    preset.evolution.mutationRate = 0.3f;
    preset.evolution.mutationStrength = 0.4f;
    preset.evolution.sizeBoost = -0.5f;  // Favor smaller creatures

    preset.initialPopulation = {
        {CreatureType::FLYING_INSECT, 100},
        {CreatureType::FRUGIVORE, 50},
        {CreatureType::SMALL_PREDATOR, 10}
    };

    return preset;
}

inline ScenarioPreset ScenarioPresetsPanel::createAlienWorld() {
    ScenarioPreset preset;
    preset.name = "Alien World";
    preset.description = "An unpredictable world with extreme mutations and bizarre conditions. "
                        "Expect the unexpected as life takes strange forms.";
    preset.goals = "See what bizarre adaptations emerge from chaotic evolution. Pure experimentation.";
    preset.difficulty = "Experimental";

    preset.environment.temperature = 0.4f + (rand() % 50) / 100.0f;
    preset.environment.moisture = 0.2f + (rand() % 60) / 100.0f;
    preset.environment.foodAbundance = 0.5f + (rand() % 200) / 100.0f;
    preset.environment.seasonStrength = 1.5f;

    preset.evolution.mutationRate = 0.4f;
    preset.evolution.mutationStrength = 0.7f;
    preset.evolution.speciationThreshold = 0.2f;
    preset.evolution.noveltyWeight = 0.6f;
    preset.evolution.enableNeuroevolution = true;

    preset.initialPopulation = {
        {CreatureType::GRAZER, 15},
        {CreatureType::FRUGIVORE, 15},
        {CreatureType::OMNIVORE, 15},
        {CreatureType::AQUATIC, 15},
        {CreatureType::FLYING, 15}
    };

    return preset;
}

inline ScenarioPreset ScenarioPresetsPanel::createDarwinsFinches() {
    ScenarioPreset preset;
    preset.name = "Darwin's Finches";
    preset.description = "Isolated island populations with varying food sources. "
                        "Classic demonstration of adaptive radiation and speciation.";
    preset.goals = "Observe speciation into distinct ecological niches based on available food types.";
    preset.difficulty = "Medium";

    preset.environment.temperature = 0.6f;
    preset.environment.moisture = 0.5f;
    preset.environment.foodAbundance = 0.8f;
    preset.environment.waterLevel = 0.4f;

    preset.evolution.mutationRate = 0.18f;
    preset.evolution.speciationThreshold = 0.3f;
    preset.evolution.selectionPressure = 1.5f;

    preset.initialPopulation = {
        {CreatureType::FLYING_BIRD, 50},
        {CreatureType::FRUGIVORE, 20}
    };

    return preset;
}

inline ScenarioPreset ScenarioPresetsPanel::createMassExtinction() {
    ScenarioPreset preset;
    preset.name = "Mass Extinction Recovery";
    preset.description = "Start just after a catastrophic extinction event. Only 10% of creatures survived. "
                        "Watch life recover and diversify to fill empty niches.";
    preset.goals = "Observe recovery dynamics, opportunistic species, and rapid adaptive radiation.";
    preset.difficulty = "Hard";

    preset.environment.temperature = 0.35f;
    preset.environment.moisture = 0.4f;
    preset.environment.foodAbundance = 0.5f;

    preset.evolution.mutationRate = 0.25f;
    preset.evolution.speciationThreshold = 0.25f;
    preset.evolution.noveltyWeight = 0.4f;

    // Very small starting population
    preset.initialPopulation = {
        {CreatureType::GRAZER, 5},
        {CreatureType::FRUGIVORE, 5},
        {CreatureType::SMALL_PREDATOR, 2},
        {CreatureType::SCAVENGER, 3}
    };

    preset.triggerEvent = true;
    preset.eventToTrigger = "volcanic_winter";

    return preset;
}

inline ScenarioPreset ScenarioPresetsPanel::createSurvivalOfTheFittest() {
    ScenarioPreset preset;
    preset.name = "Survival of the Fittest";
    preset.description = "Extreme selection pressure where only the absolute best survive. "
                        "Low mutation, high competition, scarce resources.";
    preset.goals = "See which traits become dominant under intense selection. Observe fitness optimization.";
    preset.difficulty = "Extreme";

    preset.environment.temperature = 0.45f;
    preset.environment.moisture = 0.35f;
    preset.environment.foodAbundance = 0.4f;
    preset.environment.predatorPressure = 2.0f;

    preset.evolution.mutationRate = 0.08f;
    preset.evolution.mutationStrength = 0.15f;
    preset.evolution.selectionPressure = 3.0f;
    preset.evolution.elitismRate = 0.1f;

    preset.initialPopulation = {
        {CreatureType::GRAZER, 40},
        {CreatureType::FRUGIVORE, 30},
        {CreatureType::APEX_PREDATOR, 10}
    };

    return preset;
}

} // namespace ui
