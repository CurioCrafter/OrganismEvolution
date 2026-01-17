#pragma once

#include "imgui.h"
#include <functional>
#include <string>
#include <vector>

namespace ui {

// ============================================================================
// Evolution Control Panel
// ============================================================================
// Provides controls for evolution parameters:
// - Mutation rate and strength
// - Crossover probability
// - Selection pressure
// - Speciation threshold
// - Enable/disable specific mutations
// - Directed evolution (boost specific traits)

// Evolution parameters that can be modified at runtime
struct EvolutionParameters {
    // Core evolution rates
    float mutationRate = 0.15f;          // 0-1, probability of mutation per gene
    float mutationStrength = 0.3f;       // 0-1, magnitude of mutations
    float crossoverProbability = 0.7f;   // 0-1, probability of crossover during reproduction

    // Selection parameters
    float selectionPressure = 1.0f;      // 0-3, how strongly fitness affects reproduction
    float sexualSelectionStrength = 0.5f;// 0-1, importance of mate choice
    float speciationThreshold = 0.5f;    // 0-1, genetic distance for speciation

    // Mutation toggles (enable/disable specific mutations)
    bool mutateSizeEnabled = true;
    bool mutateSpeedEnabled = true;
    bool mutateVisionEnabled = true;
    bool mutateEfficiencyEnabled = true;
    bool mutateColorEnabled = true;
    bool mutateSensoryEnabled = true;
    bool mutateNeuralEnabled = true;

    // Directed evolution boosts
    float sizeBoost = 0.0f;              // -1 to 1, bias towards smaller/larger
    float speedBoost = 0.0f;             // -1 to 1, bias towards slower/faster
    float visionBoost = 0.0f;            // -1 to 1, bias towards less/more vision
    float efficiencyBoost = 0.0f;        // -1 to 1, bias towards less/more efficiency

    // Advanced options
    float elitismRate = 0.05f;           // 0-0.2, fraction of best preserved unchanged
    float noveltyWeight = 0.2f;          // 0-1, weight given to behavioral novelty
    bool enableNeuroevolution = true;    // Whether neural networks can evolve
    int maxPopulation = 200;             // Population cap
    int minPopulation = 10;              // Minimum population (trigger emergency spawns)
};

class EvolutionControlPanel {
public:
    EvolutionControlPanel();
    ~EvolutionControlPanel() = default;

    // Main render function
    void render();

    // Render as collapsible section
    void renderSection();

    // Get/set parameters
    const EvolutionParameters& getParameters() const { return m_params; }
    EvolutionParameters& getParameters() { return m_params; }
    void setParameters(const EvolutionParameters& params) { m_params = params; }

    // Callbacks for when parameters change
    using ParameterChangeCallback = std::function<void(const EvolutionParameters&)>;
    void setParameterChangeCallback(ParameterChangeCallback cb) { m_onParamsChanged = cb; }

    // Panel visibility
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    void toggleVisible() { m_visible = !m_visible; }

    // Presets
    void applyPreset(const std::string& presetName);
    std::vector<std::string> getPresetNames() const;

private:
    bool m_visible = true;
    EvolutionParameters m_params;
    ParameterChangeCallback m_onParamsChanged;

    // UI helpers
    void renderCoreRates();
    void renderSelectionControls();
    void renderMutationToggles();
    void renderDirectedEvolution();
    void renderAdvancedOptions();
    void renderPresets();

    void notifyParamsChanged();
};

// ============================================================================
// Implementation
// ============================================================================

inline EvolutionControlPanel::EvolutionControlPanel() = default;

inline void EvolutionControlPanel::render() {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(380, 550), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Evolution Controls", &m_visible)) {
        renderSection();
    }
    ImGui::End();
}

inline void EvolutionControlPanel::renderSection() {
    // Presets at top
    if (ImGui::CollapsingHeader("Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderPresets();
    }

    // Core evolution rates
    if (ImGui::CollapsingHeader("Mutation Rates", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderCoreRates();
    }

    // Selection controls
    if (ImGui::CollapsingHeader("Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderSelectionControls();
    }

    // Mutation toggles
    if (ImGui::CollapsingHeader("Mutation Types")) {
        renderMutationToggles();
    }

    // Directed evolution
    if (ImGui::CollapsingHeader("Directed Evolution")) {
        renderDirectedEvolution();
    }

    // Advanced options
    if (ImGui::CollapsingHeader("Advanced")) {
        renderAdvancedOptions();
    }
}

inline void EvolutionControlPanel::renderPresets() {
    ImGui::Text("Quick Presets:");

    if (ImGui::Button("Default", ImVec2(80, 0))) {
        applyPreset("Default");
    }
    ImGui::SameLine();
    if (ImGui::Button("Stable", ImVec2(80, 0))) {
        applyPreset("Stable");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Low mutation, high selection - stable evolution");
    }
    ImGui::SameLine();
    if (ImGui::Button("Chaotic", ImVec2(80, 0))) {
        applyPreset("Chaotic");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("High mutation, low selection - rapid change");
    }
    ImGui::SameLine();
    if (ImGui::Button("Rapid", ImVec2(80, 0))) {
        applyPreset("Rapid");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("High mutation AND selection - fast speciation");
    }

    if (ImGui::Button("Speciation", ImVec2(80, 0))) {
        applyPreset("Speciation");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Promote species formation");
    }
    ImGui::SameLine();
    if (ImGui::Button("Neural", ImVec2(80, 0))) {
        applyPreset("Neural");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Focus on brain evolution");
    }
    ImGui::SameLine();
    if (ImGui::Button("Physical", ImVec2(80, 0))) {
        applyPreset("Physical");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Focus on body trait evolution");
    }
    ImGui::SameLine();
    if (ImGui::Button("Frozen", ImVec2(80, 0))) {
        applyPreset("Frozen");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("No mutations - observe current population");
    }
}

inline void EvolutionControlPanel::renderCoreRates() {
    bool changed = false;

    // Mutation rate with visual indicator
    ImGui::Text("Mutation Rate:");
    ImGui::SameLine();
    ImGui::TextColored(
        m_params.mutationRate > 0.3f ? ImVec4(1.0f, 0.5f, 0.0f, 1.0f) :
        m_params.mutationRate > 0.15f ? ImVec4(0.8f, 0.8f, 0.3f, 1.0f) :
        ImVec4(0.3f, 0.8f, 0.3f, 1.0f),
        m_params.mutationRate > 0.3f ? "(High)" :
        m_params.mutationRate > 0.15f ? "(Medium)" : "(Low)");

    if (ImGui::SliderFloat("##MutRate", &m_params.mutationRate, 0.0f, 0.5f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Probability of each gene mutating (0.10-0.20 typical)");
    }

    // Mutation strength
    ImGui::Text("Mutation Strength:");
    if (ImGui::SliderFloat("##MutStr", &m_params.mutationStrength, 0.0f, 1.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Magnitude of mutations when they occur (0.2-0.4 typical)");
    }

    // Crossover probability
    ImGui::Text("Crossover Probability:");
    if (ImGui::SliderFloat("##Crossover", &m_params.crossoverProbability, 0.0f, 1.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Probability of genetic crossover during reproduction");
    }

    if (changed) notifyParamsChanged();
}

inline void EvolutionControlPanel::renderSelectionControls() {
    bool changed = false;

    // Selection pressure
    ImGui::Text("Selection Pressure:");
    ImGui::SameLine();
    ImGui::TextColored(
        m_params.selectionPressure > 2.0f ? ImVec4(0.9f, 0.3f, 0.3f, 1.0f) :
        m_params.selectionPressure > 1.0f ? ImVec4(0.8f, 0.8f, 0.3f, 1.0f) :
        ImVec4(0.3f, 0.8f, 0.3f, 1.0f),
        m_params.selectionPressure > 2.0f ? "(Harsh)" :
        m_params.selectionPressure > 1.0f ? "(Strong)" : "(Gentle)");

    if (ImGui::SliderFloat("##SelPress", &m_params.selectionPressure, 0.0f, 3.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("How strongly fitness affects reproductive success\n"
                          "Low = anyone can reproduce\n"
                          "High = only the fittest reproduce");
    }

    // Sexual selection
    ImGui::Text("Sexual Selection:");
    if (ImGui::SliderFloat("##SexSel", &m_params.sexualSelectionStrength, 0.0f, 1.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Importance of mate choice (affects sexual dimorphism)");
    }

    // Speciation threshold
    ImGui::Text("Speciation Threshold:");
    if (ImGui::SliderFloat("##Speciation", &m_params.speciationThreshold, 0.1f, 1.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Genetic distance required to form new species\n"
                          "Lower = more species\n"
                          "Higher = fewer, more distinct species");
    }

    if (changed) notifyParamsChanged();
}

inline void EvolutionControlPanel::renderMutationToggles() {
    bool changed = false;

    ImGui::TextWrapped("Enable/disable specific mutation types:");
    ImGui::Separator();

    // Physical traits
    ImGui::Text("Physical Traits:");
    if (ImGui::Checkbox("Size Mutations", &m_params.mutateSizeEnabled)) changed = true;
    ImGui::SameLine();
    if (ImGui::Checkbox("Speed Mutations", &m_params.mutateSpeedEnabled)) changed = true;

    if (ImGui::Checkbox("Vision Mutations", &m_params.mutateVisionEnabled)) changed = true;
    ImGui::SameLine();
    if (ImGui::Checkbox("Efficiency Mutations", &m_params.mutateEfficiencyEnabled)) changed = true;

    ImGui::Separator();

    // Other traits
    ImGui::Text("Other Traits:");
    if (ImGui::Checkbox("Color Mutations", &m_params.mutateColorEnabled)) changed = true;
    ImGui::SameLine();
    if (ImGui::Checkbox("Sensory Mutations", &m_params.mutateSensoryEnabled)) changed = true;

    ImGui::Separator();

    // Neural mutations (special)
    ImGui::Text("Brain:");
    if (ImGui::Checkbox("Neural Network Mutations", &m_params.mutateNeuralEnabled)) changed = true;
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Allow neural network weights and topology to evolve");
    }

    // Quick toggles
    ImGui::Separator();
    if (ImGui::Button("Enable All")) {
        m_params.mutateSizeEnabled = true;
        m_params.mutateSpeedEnabled = true;
        m_params.mutateVisionEnabled = true;
        m_params.mutateEfficiencyEnabled = true;
        m_params.mutateColorEnabled = true;
        m_params.mutateSensoryEnabled = true;
        m_params.mutateNeuralEnabled = true;
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Disable All")) {
        m_params.mutateSizeEnabled = false;
        m_params.mutateSpeedEnabled = false;
        m_params.mutateVisionEnabled = false;
        m_params.mutateEfficiencyEnabled = false;
        m_params.mutateColorEnabled = false;
        m_params.mutateSensoryEnabled = false;
        m_params.mutateNeuralEnabled = false;
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Physical Only")) {
        m_params.mutateSizeEnabled = true;
        m_params.mutateSpeedEnabled = true;
        m_params.mutateVisionEnabled = true;
        m_params.mutateEfficiencyEnabled = true;
        m_params.mutateColorEnabled = false;
        m_params.mutateSensoryEnabled = false;
        m_params.mutateNeuralEnabled = false;
        changed = true;
    }

    if (changed) notifyParamsChanged();
}

inline void EvolutionControlPanel::renderDirectedEvolution() {
    bool changed = false;

    ImGui::TextWrapped("Bias evolution towards specific traits (experimental):");
    ImGui::Separator();

    // Size bias
    ImGui::Text("Size Direction:");
    if (ImGui::SliderFloat("##SizeBias", &m_params.sizeBoost, -1.0f, 1.0f, "%.2f")) {
        changed = true;
    }
    ImGui::SameLine();
    ImGui::TextColored(
        m_params.sizeBoost < -0.3f ? ImVec4(0.3f, 0.7f, 1.0f, 1.0f) :
        m_params.sizeBoost > 0.3f ? ImVec4(1.0f, 0.5f, 0.3f, 1.0f) :
        ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
        m_params.sizeBoost < -0.3f ? "Smaller" :
        m_params.sizeBoost > 0.3f ? "Larger" : "Neutral");

    // Speed bias
    ImGui::Text("Speed Direction:");
    if (ImGui::SliderFloat("##SpeedBias", &m_params.speedBoost, -1.0f, 1.0f, "%.2f")) {
        changed = true;
    }
    ImGui::SameLine();
    ImGui::TextColored(
        m_params.speedBoost < -0.3f ? ImVec4(0.5f, 0.8f, 0.5f, 1.0f) :
        m_params.speedBoost > 0.3f ? ImVec4(1.0f, 0.8f, 0.3f, 1.0f) :
        ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
        m_params.speedBoost < -0.3f ? "Slower" :
        m_params.speedBoost > 0.3f ? "Faster" : "Neutral");

    // Vision bias
    ImGui::Text("Vision Direction:");
    if (ImGui::SliderFloat("##VisionBias", &m_params.visionBoost, -1.0f, 1.0f, "%.2f")) {
        changed = true;
    }
    ImGui::SameLine();
    ImGui::TextColored(
        m_params.visionBoost < -0.3f ? ImVec4(0.5f, 0.5f, 0.5f, 1.0f) :
        m_params.visionBoost > 0.3f ? ImVec4(0.8f, 0.8f, 1.0f, 1.0f) :
        ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
        m_params.visionBoost < -0.3f ? "Blind" :
        m_params.visionBoost > 0.3f ? "Eagle-eyed" : "Neutral");

    // Efficiency bias
    ImGui::Text("Efficiency Direction:");
    if (ImGui::SliderFloat("##EffBias", &m_params.efficiencyBoost, -1.0f, 1.0f, "%.2f")) {
        changed = true;
    }
    ImGui::SameLine();
    ImGui::TextColored(
        m_params.efficiencyBoost < -0.3f ? ImVec4(0.9f, 0.4f, 0.4f, 1.0f) :
        m_params.efficiencyBoost > 0.3f ? ImVec4(0.4f, 0.9f, 0.4f, 1.0f) :
        ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
        m_params.efficiencyBoost < -0.3f ? "Wasteful" :
        m_params.efficiencyBoost > 0.3f ? "Efficient" : "Neutral");

    // Reset button
    ImGui::Separator();
    if (ImGui::Button("Reset All Biases")) {
        m_params.sizeBoost = 0.0f;
        m_params.speedBoost = 0.0f;
        m_params.visionBoost = 0.0f;
        m_params.efficiencyBoost = 0.0f;
        changed = true;
    }

    if (changed) notifyParamsChanged();
}

inline void EvolutionControlPanel::renderAdvancedOptions() {
    bool changed = false;

    // Elitism
    ImGui::Text("Elitism Rate:");
    if (ImGui::SliderFloat("##Elitism", &m_params.elitismRate, 0.0f, 0.2f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Fraction of best creatures preserved unchanged each generation");
    }

    // Novelty weight
    ImGui::Text("Novelty Weight:");
    if (ImGui::SliderFloat("##Novelty", &m_params.noveltyWeight, 0.0f, 1.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Weight given to novel behaviors vs raw fitness\n"
                          "Higher = reward exploration, prevent convergence");
    }

    // Neuroevolution toggle
    if (ImGui::Checkbox("Enable Neuroevolution", &m_params.enableNeuroevolution)) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Allow neural network topology to evolve (NEAT-style)");
    }

    ImGui::Separator();

    // Population limits
    ImGui::Text("Population Limits:");

    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("Max", &m_params.maxPopulation)) {
        m_params.maxPopulation = std::max(20, std::min(1000, m_params.maxPopulation));
        changed = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("Min", &m_params.minPopulation)) {
        m_params.minPopulation = std::max(5, std::min(m_params.maxPopulation - 10, m_params.minPopulation));
        changed = true;
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Emergency spawns trigger below minimum population");
    }

    if (changed) notifyParamsChanged();
}

inline void EvolutionControlPanel::applyPreset(const std::string& presetName) {
    if (presetName == "Default") {
        m_params = EvolutionParameters();
    }
    else if (presetName == "Stable") {
        m_params.mutationRate = 0.08f;
        m_params.mutationStrength = 0.2f;
        m_params.selectionPressure = 1.5f;
        m_params.speciationThreshold = 0.6f;
    }
    else if (presetName == "Chaotic") {
        m_params.mutationRate = 0.35f;
        m_params.mutationStrength = 0.6f;
        m_params.selectionPressure = 0.5f;
        m_params.speciationThreshold = 0.3f;
    }
    else if (presetName == "Rapid") {
        m_params.mutationRate = 0.25f;
        m_params.mutationStrength = 0.4f;
        m_params.selectionPressure = 2.0f;
        m_params.speciationThreshold = 0.35f;
    }
    else if (presetName == "Speciation") {
        m_params.mutationRate = 0.2f;
        m_params.mutationStrength = 0.35f;
        m_params.speciationThreshold = 0.25f;
        m_params.noveltyWeight = 0.4f;
    }
    else if (presetName == "Neural") {
        m_params.mutateNeuralEnabled = true;
        m_params.mutateSizeEnabled = false;
        m_params.mutateSpeedEnabled = false;
        m_params.mutationRate = 0.2f;
        m_params.enableNeuroevolution = true;
    }
    else if (presetName == "Physical") {
        m_params.mutateNeuralEnabled = false;
        m_params.mutateSizeEnabled = true;
        m_params.mutateSpeedEnabled = true;
        m_params.mutateVisionEnabled = true;
        m_params.mutateEfficiencyEnabled = true;
        m_params.mutationRate = 0.2f;
    }
    else if (presetName == "Frozen") {
        m_params.mutationRate = 0.0f;
        m_params.mutationStrength = 0.0f;
    }

    notifyParamsChanged();
}

inline std::vector<std::string> EvolutionControlPanel::getPresetNames() const {
    return {"Default", "Stable", "Chaotic", "Rapid", "Speciation", "Neural", "Physical", "Frozen"};
}

inline void EvolutionControlPanel::notifyParamsChanged() {
    if (m_onParamsChanged) {
        m_onParamsChanged(m_params);
    }
}

} // namespace ui
