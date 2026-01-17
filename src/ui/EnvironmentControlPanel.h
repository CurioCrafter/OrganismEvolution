#pragma once

#include "imgui.h"
#include <functional>
#include <string>

namespace ui {

// ============================================================================
// Environment Control Panel
// ============================================================================
// Provides controls for environment parameters:
// - Temperature and moisture
// - Food abundance
// - Day/night cycle
// - Season speed
// - Terrain parameters
// - Weather events

// Environment parameters that can be modified at runtime
struct EnvironmentParameters {
    // Climate
    float temperature = 0.5f;           // 0-1, cold to hot
    float moisture = 0.5f;              // 0-1, dry to wet
    float temperatureVariance = 0.1f;   // How much temperature varies spatially

    // Food
    float foodAbundance = 1.0f;         // 0-3, food spawn multiplier
    float foodNutrition = 1.0f;         // 0-2, energy per food
    float foodRegrowthRate = 1.0f;      // 0-3, how fast food regrows

    // Time
    float dayLengthSeconds = 120.0f;    // 30-600, seconds per day cycle
    float seasonLengthDays = 30.0f;     // 10-100, days per season
    float seasonStrength = 1.0f;        // 0-2, how much seasons affect climate

    // Terrain
    float waterLevel = 0.3f;            // 0-0.5, water coverage
    float terrainRoughness = 0.5f;      // 0-1, terrain variation
    float vegetationDensity = 1.0f;     // 0-2, plant coverage

    // Difficulty
    float predatorPressure = 1.0f;      // 0-3, predator spawn/aggression
    float diseaseRate = 0.0f;           // 0-1, disease occurrence
    float naturalDisasterRate = 0.0f;   // 0-1, disaster frequency
};

class EnvironmentControlPanel {
public:
    EnvironmentControlPanel();
    ~EnvironmentControlPanel() = default;

    // Main render function
    void render();

    // Render as collapsible section
    void renderSection();

    // Get/set parameters
    const EnvironmentParameters& getParameters() const { return m_params; }
    EnvironmentParameters& getParameters() { return m_params; }
    void setParameters(const EnvironmentParameters& params) { m_params = params; }

    // Callbacks
    using ParameterChangeCallback = std::function<void(const EnvironmentParameters&)>;
    using TriggerEventCallback = std::function<void(const std::string& eventName)>;
    using SetTimeOfDayCallback = std::function<void(float time)>;

    void setParameterChangeCallback(ParameterChangeCallback cb) { m_onParamsChanged = cb; }
    void setTriggerEventCallback(TriggerEventCallback cb) { m_triggerEvent = cb; }
    void setTimeOfDayCallback(SetTimeOfDayCallback cb) { m_setTimeOfDay = cb; }

    // Panel visibility
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    void toggleVisible() { m_visible = !m_visible; }

    // External state for display
    void setCurrentTimeOfDay(float time) { m_currentTimeOfDay = time; }
    void setCurrentSeason(const std::string& season) { m_currentSeason = season; }
    void setCurrentBiome(const std::string& biome) { m_currentBiome = biome; }

private:
    bool m_visible = true;
    EnvironmentParameters m_params;

    // Display state
    float m_currentTimeOfDay = 0.25f;
    std::string m_currentSeason = "Spring";
    std::string m_currentBiome = "Temperate";

    // Callbacks
    ParameterChangeCallback m_onParamsChanged;
    TriggerEventCallback m_triggerEvent;
    SetTimeOfDayCallback m_setTimeOfDay;

    // UI helpers
    void renderClimateControls();
    void renderFoodControls();
    void renderTimeControls();
    void renderTerrainControls();
    void renderDifficultyControls();
    void renderWeatherEvents();
    void renderPresets();

    void notifyParamsChanged();
};

// ============================================================================
// Implementation
// ============================================================================

inline EnvironmentControlPanel::EnvironmentControlPanel() = default;

inline void EnvironmentControlPanel::render() {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(380, 600), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Environment Controls", &m_visible)) {
        renderSection();
    }
    ImGui::End();
}

inline void EnvironmentControlPanel::renderSection() {
    // Current status display
    ImGui::Text("Current: %s, %s", m_currentSeason.c_str(), m_currentBiome.c_str());

    // Presets
    if (ImGui::CollapsingHeader("Environment Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderPresets();
    }

    // Climate controls
    if (ImGui::CollapsingHeader("Climate", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderClimateControls();
    }

    // Food controls
    if (ImGui::CollapsingHeader("Food & Resources", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderFoodControls();
    }

    // Time controls
    if (ImGui::CollapsingHeader("Time & Seasons")) {
        renderTimeControls();
    }

    // Terrain controls
    if (ImGui::CollapsingHeader("Terrain")) {
        renderTerrainControls();
    }

    // Difficulty controls
    if (ImGui::CollapsingHeader("Difficulty")) {
        renderDifficultyControls();
    }

    // Weather events
    if (ImGui::CollapsingHeader("Weather Events")) {
        renderWeatherEvents();
    }
}

inline void EnvironmentControlPanel::renderPresets() {
    ImGui::Text("Quick Presets:");

    if (ImGui::Button("Temperate", ImVec2(80, 0))) {
        m_params.temperature = 0.5f;
        m_params.moisture = 0.5f;
        m_params.foodAbundance = 1.0f;
        m_params.seasonStrength = 1.0f;
        notifyParamsChanged();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Balanced climate, moderate seasons");

    ImGui::SameLine();
    if (ImGui::Button("Tropical", ImVec2(80, 0))) {
        m_params.temperature = 0.8f;
        m_params.moisture = 0.8f;
        m_params.foodAbundance = 1.5f;
        m_params.seasonStrength = 0.2f;
        notifyParamsChanged();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Hot, wet, abundant food, no seasons");

    ImGui::SameLine();
    if (ImGui::Button("Arctic", ImVec2(80, 0))) {
        m_params.temperature = 0.1f;
        m_params.moisture = 0.3f;
        m_params.foodAbundance = 0.3f;
        m_params.seasonStrength = 1.5f;
        notifyParamsChanged();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Cold, dry, scarce food, harsh seasons");

    ImGui::SameLine();
    if (ImGui::Button("Desert", ImVec2(80, 0))) {
        m_params.temperature = 0.85f;
        m_params.moisture = 0.1f;
        m_params.foodAbundance = 0.2f;
        m_params.seasonStrength = 0.5f;
        notifyParamsChanged();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Hot, very dry, very scarce food");

    if (ImGui::Button("Paradise", ImVec2(80, 0))) {
        m_params.temperature = 0.6f;
        m_params.moisture = 0.7f;
        m_params.foodAbundance = 2.5f;
        m_params.predatorPressure = 0.3f;
        notifyParamsChanged();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Abundant food, few predators");

    ImGui::SameLine();
    if (ImGui::Button("Hell", ImVec2(80, 0))) {
        m_params.temperature = 0.9f;
        m_params.moisture = 0.2f;
        m_params.foodAbundance = 0.5f;
        m_params.predatorPressure = 2.5f;
        m_params.diseaseRate = 0.3f;
        notifyParamsChanged();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Harsh survival conditions");

    ImGui::SameLine();
    if (ImGui::Button("Ice Age", ImVec2(80, 0))) {
        m_params.temperature = 0.05f;
        m_params.moisture = 0.2f;
        m_params.foodAbundance = 0.15f;
        m_params.seasonStrength = 2.0f;
        notifyParamsChanged();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Extreme cold, survival challenge");

    ImGui::SameLine();
    if (ImGui::Button("Swamp", ImVec2(80, 0))) {
        m_params.temperature = 0.7f;
        m_params.moisture = 0.95f;
        m_params.waterLevel = 0.45f;
        m_params.foodAbundance = 1.2f;
        m_params.diseaseRate = 0.2f;
        notifyParamsChanged();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Very wet, high water coverage");
}

inline void EnvironmentControlPanel::renderClimateControls() {
    bool changed = false;

    // Temperature with visual indicator
    ImGui::Text("Global Temperature:");

    // Temperature color indicator
    ImVec4 tempColor;
    const char* tempLabel;
    if (m_params.temperature < 0.2f) {
        tempColor = ImVec4(0.3f, 0.5f, 1.0f, 1.0f);
        tempLabel = "Freezing";
    } else if (m_params.temperature < 0.4f) {
        tempColor = ImVec4(0.5f, 0.7f, 1.0f, 1.0f);
        tempLabel = "Cold";
    } else if (m_params.temperature < 0.6f) {
        tempColor = ImVec4(0.5f, 0.9f, 0.5f, 1.0f);
        tempLabel = "Temperate";
    } else if (m_params.temperature < 0.8f) {
        tempColor = ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
        tempLabel = "Warm";
    } else {
        tempColor = ImVec4(1.0f, 0.4f, 0.2f, 1.0f);
        tempLabel = "Hot";
    }

    ImGui::SameLine();
    ImGui::TextColored(tempColor, "(%s)", tempLabel);

    if (ImGui::SliderFloat("##Temp", &m_params.temperature, 0.0f, 1.0f, "%.2f")) {
        changed = true;
    }

    // Moisture
    ImGui::Text("Moisture/Rainfall:");

    ImVec4 moistColor;
    const char* moistLabel;
    if (m_params.moisture < 0.2f) {
        moistColor = ImVec4(0.9f, 0.7f, 0.4f, 1.0f);
        moistLabel = "Arid";
    } else if (m_params.moisture < 0.4f) {
        moistColor = ImVec4(0.8f, 0.8f, 0.5f, 1.0f);
        moistLabel = "Dry";
    } else if (m_params.moisture < 0.6f) {
        moistColor = ImVec4(0.5f, 0.8f, 0.5f, 1.0f);
        moistLabel = "Moderate";
    } else if (m_params.moisture < 0.8f) {
        moistColor = ImVec4(0.4f, 0.7f, 0.9f, 1.0f);
        moistLabel = "Wet";
    } else {
        moistColor = ImVec4(0.3f, 0.5f, 1.0f, 1.0f);
        moistLabel = "Tropical";
    }

    ImGui::SameLine();
    ImGui::TextColored(moistColor, "(%s)", moistLabel);

    if (ImGui::SliderFloat("##Moisture", &m_params.moisture, 0.0f, 1.0f, "%.2f")) {
        changed = true;
    }

    // Temperature variance
    ImGui::Text("Temperature Variance:");
    if (ImGui::SliderFloat("##TempVar", &m_params.temperatureVariance, 0.0f, 0.5f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("How much temperature varies across the map");
    }

    if (changed) notifyParamsChanged();
}

inline void EnvironmentControlPanel::renderFoodControls() {
    bool changed = false;

    // Food abundance
    ImGui::Text("Food Abundance:");
    if (ImGui::SliderFloat("##FoodAbund", &m_params.foodAbundance, 0.1f, 3.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Multiplier for food spawn rate\n1.0 = normal, 2.0 = double, 0.5 = half");
    }

    // Quick buttons
    if (ImGui::Button("Famine")) { m_params.foodAbundance = 0.2f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Scarce")) { m_params.foodAbundance = 0.5f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Normal")) { m_params.foodAbundance = 1.0f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Plenty")) { m_params.foodAbundance = 2.0f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Feast")) { m_params.foodAbundance = 3.0f; changed = true; }

    ImGui::Separator();

    // Food nutrition
    ImGui::Text("Food Nutrition:");
    if (ImGui::SliderFloat("##FoodNutr", &m_params.foodNutrition, 0.2f, 2.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Energy gained per food consumed");
    }

    // Regrowth rate
    ImGui::Text("Regrowth Rate:");
    if (ImGui::SliderFloat("##Regrowth", &m_params.foodRegrowthRate, 0.1f, 3.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("How fast food respawns after being eaten");
    }

    if (changed) notifyParamsChanged();
}

inline void EnvironmentControlPanel::renderTimeControls() {
    bool changed = false;

    // Current time display
    int hours = static_cast<int>(m_currentTimeOfDay * 24.0f);
    int minutes = static_cast<int>((m_currentTimeOfDay * 24.0f - hours) * 60.0f);
    ImGui::Text("Current Time: %02d:%02d", hours, minutes);

    // Time of day slider
    float tod = m_currentTimeOfDay;
    if (ImGui::SliderFloat("Time of Day", &tod, 0.0f, 1.0f, "")) {
        if (m_setTimeOfDay) m_setTimeOfDay(tod);
    }

    // Quick time buttons
    if (ImGui::Button("Dawn")) { if (m_setTimeOfDay) m_setTimeOfDay(0.22f); }
    ImGui::SameLine();
    if (ImGui::Button("Morning")) { if (m_setTimeOfDay) m_setTimeOfDay(0.35f); }
    ImGui::SameLine();
    if (ImGui::Button("Noon")) { if (m_setTimeOfDay) m_setTimeOfDay(0.5f); }
    ImGui::SameLine();
    if (ImGui::Button("Evening")) { if (m_setTimeOfDay) m_setTimeOfDay(0.65f); }
    ImGui::SameLine();
    if (ImGui::Button("Dusk")) { if (m_setTimeOfDay) m_setTimeOfDay(0.75f); }
    ImGui::SameLine();
    if (ImGui::Button("Night")) { if (m_setTimeOfDay) m_setTimeOfDay(0.0f); }

    ImGui::Separator();

    // Day length
    ImGui::Text("Day Length (seconds):");
    if (ImGui::SliderFloat("##DayLen", &m_params.dayLengthSeconds, 30.0f, 600.0f, "%.0f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Real-time seconds per full day/night cycle");
    }

    // Quick day length buttons
    if (ImGui::Button("30s")) { m_params.dayLengthSeconds = 30.0f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("1m")) { m_params.dayLengthSeconds = 60.0f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("2m")) { m_params.dayLengthSeconds = 120.0f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("5m")) { m_params.dayLengthSeconds = 300.0f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("10m")) { m_params.dayLengthSeconds = 600.0f; changed = true; }

    ImGui::Separator();

    // Season length
    ImGui::Text("Season Length (game days):");
    if (ImGui::SliderFloat("##SeasonLen", &m_params.seasonLengthDays, 10.0f, 100.0f, "%.0f")) {
        changed = true;
    }

    // Season strength
    ImGui::Text("Season Intensity:");
    if (ImGui::SliderFloat("##SeasonStr", &m_params.seasonStrength, 0.0f, 2.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("How much seasons affect temperature/moisture\n0 = no seasons, 2 = extreme seasons");
    }

    if (changed) notifyParamsChanged();
}

inline void EnvironmentControlPanel::renderTerrainControls() {
    bool changed = false;

    // Water level
    ImGui::Text("Water Level:");
    if (ImGui::SliderFloat("##WaterLvl", &m_params.waterLevel, 0.0f, 0.5f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Percentage of terrain covered by water");
    }

    // Water level quick buttons
    if (ImGui::Button("Dry")) { m_params.waterLevel = 0.1f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Rivers")) { m_params.waterLevel = 0.2f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Lakes")) { m_params.waterLevel = 0.3f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Islands")) { m_params.waterLevel = 0.4f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Ocean")) { m_params.waterLevel = 0.5f; changed = true; }

    ImGui::Separator();

    // Terrain roughness
    ImGui::Text("Terrain Roughness:");
    if (ImGui::SliderFloat("##TerrRough", &m_params.terrainRoughness, 0.0f, 1.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Height variation of terrain\nLow = flat, High = mountainous");
    }

    // Vegetation density
    ImGui::Text("Vegetation Density:");
    if (ImGui::SliderFloat("##VegDens", &m_params.vegetationDensity, 0.0f, 2.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Tree and grass coverage multiplier");
    }

    if (changed) notifyParamsChanged();
}

inline void EnvironmentControlPanel::renderDifficultyControls() {
    bool changed = false;

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Challenge Settings:");
    ImGui::Separator();

    // Predator pressure
    ImGui::Text("Predator Pressure:");
    if (ImGui::SliderFloat("##PredPress", &m_params.predatorPressure, 0.0f, 3.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Affects predator spawn rate and aggression\n0 = no predators, 3 = predator hell");
    }

    // Disease rate
    ImGui::Text("Disease Rate:");
    if (ImGui::SliderFloat("##Disease", &m_params.diseaseRate, 0.0f, 1.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Chance of disease outbreaks affecting creatures");
    }

    // Natural disaster rate
    ImGui::Text("Disaster Frequency:");
    if (ImGui::SliderFloat("##Disaster", &m_params.naturalDisasterRate, 0.0f, 1.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Frequency of natural disasters (fires, floods, etc.)");
    }

    // Difficulty presets
    ImGui::Separator();
    ImGui::Text("Difficulty Presets:");

    if (ImGui::Button("Easy", ImVec2(80, 0))) {
        m_params.predatorPressure = 0.3f;
        m_params.diseaseRate = 0.0f;
        m_params.naturalDisasterRate = 0.0f;
        m_params.foodAbundance = 2.0f;
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Normal", ImVec2(80, 0))) {
        m_params.predatorPressure = 1.0f;
        m_params.diseaseRate = 0.05f;
        m_params.naturalDisasterRate = 0.05f;
        m_params.foodAbundance = 1.0f;
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Hard", ImVec2(80, 0))) {
        m_params.predatorPressure = 2.0f;
        m_params.diseaseRate = 0.15f;
        m_params.naturalDisasterRate = 0.1f;
        m_params.foodAbundance = 0.6f;
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Brutal", ImVec2(80, 0))) {
        m_params.predatorPressure = 3.0f;
        m_params.diseaseRate = 0.3f;
        m_params.naturalDisasterRate = 0.2f;
        m_params.foodAbundance = 0.3f;
        changed = true;
    }

    if (changed) notifyParamsChanged();
}

inline void EnvironmentControlPanel::renderWeatherEvents() {
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Trigger Weather Events:");
    ImGui::Separator();

    ImGui::Text("Climate Events:");
    if (ImGui::Button("Drought", ImVec2(100, 25))) {
        if (m_triggerEvent) m_triggerEvent("drought");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reduce moisture for 30 game-days");

    ImGui::SameLine();
    if (ImGui::Button("Monsoon", ImVec2(100, 25))) {
        if (m_triggerEvent) m_triggerEvent("monsoon");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Increase moisture for 20 game-days");

    ImGui::SameLine();
    if (ImGui::Button("Heat Wave", ImVec2(100, 25))) {
        if (m_triggerEvent) m_triggerEvent("heat_wave");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Increase temperature significantly");

    if (ImGui::Button("Cold Snap", ImVec2(100, 25))) {
        if (m_triggerEvent) m_triggerEvent("cold_snap");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Decrease temperature significantly");

    ImGui::SameLine();
    if (ImGui::Button("Volcanic Winter", ImVec2(100, 25))) {
        if (m_triggerEvent) m_triggerEvent("volcanic_winter");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Long-term cooling from volcanic activity");

    ImGui::SameLine();
    if (ImGui::Button("Solar Max", ImVec2(100, 25))) {
        if (m_triggerEvent) m_triggerEvent("solar_maximum");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Long-term warming from solar activity");

    ImGui::Separator();

    ImGui::Text("Catastrophic Events:");
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));

    if (ImGui::Button("Start Ice Age", ImVec2(120, 25))) {
        if (m_triggerEvent) m_triggerEvent("ice_age_start");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Begin gradual global cooling");

    ImGui::SameLine();
    if (ImGui::Button("End Ice Age", ImVec2(120, 25))) {
        if (m_triggerEvent) m_triggerEvent("ice_age_end");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Begin gradual global warming");

    if (ImGui::Button("Meteor Strike", ImVec2(120, 25))) {
        if (m_triggerEvent) m_triggerEvent("meteor");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Catastrophic event killing 50% of creatures");

    ImGui::SameLine();
    if (ImGui::Button("Supervolcano", ImVec2(120, 25))) {
        if (m_triggerEvent) m_triggerEvent("supervolcano");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Massive eruption + volcanic winter");

    ImGui::PopStyleColor();
}

inline void EnvironmentControlPanel::notifyParamsChanged() {
    if (m_onParamsChanged) {
        m_onParamsChanged(m_params);
    }
}

} // namespace ui
