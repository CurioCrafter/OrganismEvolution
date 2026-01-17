#pragma once

#include "imgui.h"
#include "../entities/Creature.h"
#include "../entities/CreatureType.h"
#include "../entities/Genome.h"
#include <vector>
#include <functional>
#include <memory>
#include <string>

namespace ui {

// ============================================================================
// Creature Spawn Panel
// ============================================================================
// Provides comprehensive creature spawning controls including:
// - Spawn by type dropdown with all creature types
// - Custom genome editor for fine-tuned creatures
// - Batch spawn capabilities
// - Clone/kill selected creature
// - Mass extinction events

// Spawn request structure
struct SpawnRequest {
    CreatureType type = CreatureType::GRAZER;
    int count = 1;
    bool useCustomGenome = false;
    Genome customGenome;
    bool spawnAtCursor = false;
    glm::vec3 spawnPosition{0.0f};
};

class CreatureSpawnPanel {
public:
    CreatureSpawnPanel();
    ~CreatureSpawnPanel() = default;

    // Main render function
    void render();

    // Render as collapsible section (for embedding in other panels)
    void renderSection();

    // Callbacks
    using SpawnCallback = std::function<void(const SpawnRequest&)>;
    using CloneCallback = std::function<void(Creature*)>;
    using KillCallback = std::function<void(Creature*)>;
    using KillTypeCallback = std::function<void(CreatureType)>;
    using MassExtinctionCallback = std::function<void(float percentage)>;
    using GetCursorPositionCallback = std::function<glm::vec3()>;

    void setSpawnCallback(SpawnCallback cb) { m_spawnCallback = cb; }
    void setCloneCallback(CloneCallback cb) { m_cloneCallback = cb; }
    void setKillCallback(KillCallback cb) { m_killCallback = cb; }
    void setKillTypeCallback(KillTypeCallback cb) { m_killTypeCallback = cb; }
    void setMassExtinctionCallback(MassExtinctionCallback cb) { m_massExtinctionCallback = cb; }
    void setCursorPositionCallback(GetCursorPositionCallback cb) { m_getCursorPosition = cb; }

    // Selected creature (for clone/kill operations)
    void setSelectedCreature(Creature* creature) { m_selectedCreature = creature; }
    Creature* getSelectedCreature() const { return m_selectedCreature; }

    // Panel visibility
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    void toggleVisible() { m_visible = !m_visible; }

private:
    // UI State
    bool m_visible = true;
    int m_selectedTypeIndex = 0;
    int m_spawnCount = 5;
    bool m_useCustomGenome = false;
    bool m_spawnAtCursor = false;

    // Custom genome being edited
    Genome m_customGenome;
    bool m_genomeEditorExpanded = false;

    // Selected creature for operations
    Creature* m_selectedCreature = nullptr;

    // Callbacks
    SpawnCallback m_spawnCallback;
    CloneCallback m_cloneCallback;
    KillCallback m_killCallback;
    KillTypeCallback m_killTypeCallback;
    MassExtinctionCallback m_massExtinctionCallback;
    GetCursorPositionCallback m_getCursorPosition;

    // Helper functions
    void renderTypeSelector();
    void renderSpawnControls();
    void renderGenomeEditor();
    void renderSelectedCreatureActions();
    void renderChaosControls();
    void renderQuickSpawnButtons();

    // Get creature type from index
    CreatureType getTypeFromIndex(int index) const;
    int getIndexFromType(CreatureType type) const;

    // List of all spawnable creature types
    static const std::vector<std::pair<CreatureType, const char*>>& getCreatureTypes();
};

// ============================================================================
// Implementation
// ============================================================================

inline const std::vector<std::pair<CreatureType, const char*>>& CreatureSpawnPanel::getCreatureTypes() {
    static const std::vector<std::pair<CreatureType, const char*>> types = {
        // Herbivores
        {CreatureType::GRAZER, "Grazer (Cow/Deer)"},
        {CreatureType::BROWSER, "Browser (Giraffe)"},
        {CreatureType::FRUGIVORE, "Frugivore (Small Mammal)"},

        // Predators
        {CreatureType::SMALL_PREDATOR, "Small Predator (Fox)"},
        {CreatureType::OMNIVORE, "Omnivore (Bear)"},
        {CreatureType::APEX_PREDATOR, "Apex Predator (Wolf/Lion)"},
        {CreatureType::SCAVENGER, "Scavenger (Vulture)"},

        // Special
        {CreatureType::PARASITE, "Parasite"},
        {CreatureType::CLEANER, "Cleaner (Symbiont)"},

        // Flying
        {CreatureType::FLYING, "Flying (Generic)"},
        {CreatureType::FLYING_BIRD, "Bird"},
        {CreatureType::FLYING_INSECT, "Insect"},
        {CreatureType::AERIAL_PREDATOR, "Aerial Predator (Hawk/Eagle)"},

        // Aquatic
        {CreatureType::AQUATIC, "Fish (Generic)"},
        {CreatureType::AQUATIC_HERBIVORE, "Small Fish (Minnow)"},
        {CreatureType::AQUATIC_PREDATOR, "Predator Fish (Bass/Pike)"},
        {CreatureType::AQUATIC_APEX, "Shark"},
        {CreatureType::AMPHIBIAN, "Amphibian (Frog)"}
    };
    return types;
}

inline CreatureSpawnPanel::CreatureSpawnPanel() {
    m_customGenome.randomize();
}

inline CreatureType CreatureSpawnPanel::getTypeFromIndex(int index) const {
    const auto& types = getCreatureTypes();
    if (index >= 0 && index < static_cast<int>(types.size())) {
        return types[index].first;
    }
    return CreatureType::GRAZER;
}

inline int CreatureSpawnPanel::getIndexFromType(CreatureType type) const {
    const auto& types = getCreatureTypes();
    for (int i = 0; i < static_cast<int>(types.size()); ++i) {
        if (types[i].first == type) return i;
    }
    return 0;
}

inline void CreatureSpawnPanel::render() {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Creature Spawner", &m_visible)) {
        renderSection();
    }
    ImGui::End();
}

inline void CreatureSpawnPanel::renderSection() {
    // Quick spawn buttons at top
    if (ImGui::CollapsingHeader("Quick Spawn", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderQuickSpawnButtons();
    }

    // Type selector and spawn controls
    if (ImGui::CollapsingHeader("Custom Spawn", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderTypeSelector();
        ImGui::Separator();
        renderSpawnControls();
    }

    // Genome editor
    if (ImGui::CollapsingHeader("Genome Editor")) {
        renderGenomeEditor();
    }

    // Selected creature actions
    if (ImGui::CollapsingHeader("Selected Creature")) {
        renderSelectedCreatureActions();
    }

    // Chaos controls (mass extinction, etc.)
    if (ImGui::CollapsingHeader("Chaos Controls")) {
        renderChaosControls();
    }
}

inline void CreatureSpawnPanel::renderQuickSpawnButtons() {
    ImGui::Text("Herbivores:");
    if (ImGui::Button("+ 10 Grazers")) {
        if (m_spawnCallback) {
            SpawnRequest req;
            req.type = CreatureType::GRAZER;
            req.count = 10;
            m_spawnCallback(req);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("+ 5 Browsers")) {
        if (m_spawnCallback) {
            SpawnRequest req;
            req.type = CreatureType::BROWSER;
            req.count = 5;
            m_spawnCallback(req);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("+ 10 Frugivores")) {
        if (m_spawnCallback) {
            SpawnRequest req;
            req.type = CreatureType::FRUGIVORE;
            req.count = 10;
            m_spawnCallback(req);
        }
    }

    ImGui::Text("Predators:");
    if (ImGui::Button("+ 3 Small Pred.")) {
        if (m_spawnCallback) {
            SpawnRequest req;
            req.type = CreatureType::SMALL_PREDATOR;
            req.count = 3;
            m_spawnCallback(req);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("+ 2 Apex Pred.")) {
        if (m_spawnCallback) {
            SpawnRequest req;
            req.type = CreatureType::APEX_PREDATOR;
            req.count = 2;
            m_spawnCallback(req);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("+ 3 Omnivores")) {
        if (m_spawnCallback) {
            SpawnRequest req;
            req.type = CreatureType::OMNIVORE;
            req.count = 3;
            m_spawnCallback(req);
        }
    }

    ImGui::Text("Special:");
    if (ImGui::Button("+ 5 Birds")) {
        if (m_spawnCallback) {
            SpawnRequest req;
            req.type = CreatureType::FLYING_BIRD;
            req.count = 5;
            m_spawnCallback(req);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("+ 10 Fish")) {
        if (m_spawnCallback) {
            SpawnRequest req;
            req.type = CreatureType::AQUATIC;
            req.count = 10;
            m_spawnCallback(req);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("+ 1 Shark")) {
        if (m_spawnCallback) {
            SpawnRequest req;
            req.type = CreatureType::AQUATIC_APEX;
            req.count = 1;
            m_spawnCallback(req);
        }
    }
}

inline void CreatureSpawnPanel::renderTypeSelector() {
    const auto& types = getCreatureTypes();

    ImGui::Text("Creature Type:");

    // Build combo items
    std::vector<const char*> items;
    for (const auto& t : types) {
        items.push_back(t.second);
    }

    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##TypeCombo", &m_selectedTypeIndex, items.data(), static_cast<int>(items.size()));

    // Show creature info tooltip
    if (ImGui::IsItemHovered()) {
        CreatureType selectedType = getTypeFromIndex(m_selectedTypeIndex);
        ImGui::BeginTooltip();

        if (isHerbivore(selectedType)) {
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Herbivore - Eats plants");
        } else if (isPredator(selectedType)) {
            ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Predator - Hunts other creatures");
        } else if (isFlying(selectedType)) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1.0f), "Flying - Aerial creature");
        } else if (isAquatic(selectedType)) {
            ImGui::TextColored(ImVec4(0.3f, 0.6f, 0.9f, 1.0f), "Aquatic - Water creature");
        }

        ImGui::EndTooltip();
    }
}

inline void CreatureSpawnPanel::renderSpawnControls() {
    // Spawn count
    ImGui::Text("Spawn Count:");
    ImGui::SetNextItemWidth(120);
    ImGui::InputInt("##Count", &m_spawnCount);
    m_spawnCount = std::max(1, std::min(100, m_spawnCount));

    ImGui::SameLine();
    if (ImGui::Button("1")) m_spawnCount = 1;
    ImGui::SameLine();
    if (ImGui::Button("5")) m_spawnCount = 5;
    ImGui::SameLine();
    if (ImGui::Button("10")) m_spawnCount = 10;
    ImGui::SameLine();
    if (ImGui::Button("25")) m_spawnCount = 25;

    // Options
    ImGui::Checkbox("Use Custom Genome", &m_useCustomGenome);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Use the genome defined in the Genome Editor section");
    }

    ImGui::Checkbox("Spawn at Cursor", &m_spawnAtCursor);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Spawn creatures at the camera's target position");
    }

    ImGui::Spacing();

    // Main spawn button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));

    if (ImGui::Button("SPAWN", ImVec2(-1, 35))) {
        if (m_spawnCallback) {
            SpawnRequest req;
            req.type = getTypeFromIndex(m_selectedTypeIndex);
            req.count = m_spawnCount;
            req.useCustomGenome = m_useCustomGenome;
            req.customGenome = m_customGenome;
            req.spawnAtCursor = m_spawnAtCursor;
            if (m_spawnAtCursor && m_getCursorPosition) {
                req.spawnPosition = m_getCursorPosition();
            }
            m_spawnCallback(req);
        }
    }

    ImGui::PopStyleColor(2);
}

inline void CreatureSpawnPanel::renderGenomeEditor() {
    ImGui::TextWrapped("Edit genome traits for custom creature spawning:");
    ImGui::Separator();

    // Physical traits
    ImGui::Text("Physical Traits:");
    ImGui::SliderFloat("Size", &m_customGenome.size, 0.5f, 2.0f, "%.2f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Body size (0.5 = small, 2.0 = large)");

    ImGui::SliderFloat("Speed", &m_customGenome.speed, 5.0f, 20.0f, "%.1f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Movement speed");

    ImGui::SliderFloat("Vision Range", &m_customGenome.visionRange, 10.0f, 50.0f, "%.1f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("How far creature can see");

    ImGui::SliderFloat("Efficiency", &m_customGenome.efficiency, 0.5f, 1.5f, "%.2f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Energy consumption multiplier (lower = better)");

    ImGui::Separator();

    // Sensory traits
    ImGui::Text("Sensory Traits:");
    ImGui::SliderFloat("Vision FOV", &m_customGenome.visionFOV, 1.0f, 6.0f, "%.2f rad");
    ImGui::SliderFloat("Hearing Range", &m_customGenome.hearingRange, 10.0f, 100.0f, "%.1f");
    ImGui::SliderFloat("Smell Range", &m_customGenome.smellRange, 10.0f, 150.0f, "%.1f");
    ImGui::SliderFloat("Camouflage", &m_customGenome.camouflageLevel, 0.0f, 1.0f, "%.2f");

    ImGui::Separator();

    // Color
    ImGui::Text("Appearance:");
    ImGui::ColorEdit3("Color", &m_customGenome.color.x);

    ImGui::Separator();

    // Presets
    ImGui::Text("Presets:");
    if (ImGui::Button("Random")) {
        m_customGenome.randomize();
    }
    ImGui::SameLine();
    if (ImGui::Button("Fast Scout")) {
        m_customGenome.randomize();
        m_customGenome.size = 0.6f;
        m_customGenome.speed = 18.0f;
        m_customGenome.visionRange = 45.0f;
        m_customGenome.efficiency = 0.7f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Tank")) {
        m_customGenome.randomize();
        m_customGenome.size = 1.8f;
        m_customGenome.speed = 8.0f;
        m_customGenome.visionRange = 25.0f;
        m_customGenome.efficiency = 1.2f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stealth")) {
        m_customGenome.randomize();
        m_customGenome.size = 0.8f;
        m_customGenome.speed = 12.0f;
        m_customGenome.camouflageLevel = 0.9f;
        m_customGenome.color = glm::vec3(0.4f, 0.5f, 0.3f);
    }
}

inline void CreatureSpawnPanel::renderSelectedCreatureActions() {
    if (m_selectedCreature == nullptr) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No creature selected");
        ImGui::TextWrapped("Click on a creature in the inspector to select it.");
        return;
    }

    // Show selected creature info
    ImGui::Text("Selected: %s #%d",
        getCreatureTypeName(m_selectedCreature->getType()),
        m_selectedCreature->getID());
    ImGui::Text("Generation: %d, Fitness: %.2f",
        m_selectedCreature->getGeneration(),
        m_selectedCreature->getFitness());

    ImGui::Separator();

    // Clone button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
    if (ImGui::Button("Clone Creature", ImVec2(-1, 25))) {
        if (m_cloneCallback) {
            m_cloneCallback(m_selectedCreature);
        }
    }
    ImGui::PopStyleColor();

    // Copy genome to editor
    if (ImGui::Button("Copy Genome to Editor", ImVec2(-1, 25))) {
        m_customGenome = m_selectedCreature->getGenome();
        m_useCustomGenome = true;
    }

    ImGui::Spacing();

    // Kill button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("Kill Creature", ImVec2(-1, 25))) {
        if (m_killCallback) {
            m_killCallback(m_selectedCreature);
            m_selectedCreature = nullptr;
        }
    }
    ImGui::PopStyleColor();
}

inline void CreatureSpawnPanel::renderChaosControls() {
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "WARNING: Destructive actions!");
    ImGui::Separator();

    // Kill by type
    ImGui::Text("Kill All of Type:");

    if (ImGui::Button("Kill Herbivores", ImVec2(140, 0))) {
        if (m_killTypeCallback) {
            m_killTypeCallback(CreatureType::GRAZER);
            m_killTypeCallback(CreatureType::BROWSER);
            m_killTypeCallback(CreatureType::FRUGIVORE);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Kill Predators", ImVec2(140, 0))) {
        if (m_killTypeCallback) {
            m_killTypeCallback(CreatureType::APEX_PREDATOR);
            m_killTypeCallback(CreatureType::SMALL_PREDATOR);
        }
    }

    if (ImGui::Button("Kill Flying", ImVec2(140, 0))) {
        if (m_killTypeCallback) {
            m_killTypeCallback(CreatureType::FLYING);
            m_killTypeCallback(CreatureType::FLYING_BIRD);
            m_killTypeCallback(CreatureType::FLYING_INSECT);
            m_killTypeCallback(CreatureType::AERIAL_PREDATOR);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Kill Aquatic", ImVec2(140, 0))) {
        if (m_killTypeCallback) {
            m_killTypeCallback(CreatureType::AQUATIC);
            m_killTypeCallback(CreatureType::AQUATIC_HERBIVORE);
            m_killTypeCallback(CreatureType::AQUATIC_PREDATOR);
            m_killTypeCallback(CreatureType::AQUATIC_APEX);
        }
    }

    ImGui::Separator();

    // Mass extinction
    ImGui::Text("Mass Extinction:");

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.1f, 1.0f));

    if (ImGui::Button("25% Extinction", ImVec2(100, 30))) {
        if (m_massExtinctionCallback) m_massExtinctionCallback(0.25f);
    }
    ImGui::SameLine();
    if (ImGui::Button("50% Extinction", ImVec2(100, 30))) {
        if (m_massExtinctionCallback) m_massExtinctionCallback(0.50f);
    }
    ImGui::SameLine();
    if (ImGui::Button("90% Extinction", ImVec2(100, 30))) {
        if (m_massExtinctionCallback) m_massExtinctionCallback(0.90f);
    }

    ImGui::PopStyleColor(2);

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Randomly kill the specified percentage of all creatures");
    }
}

} // namespace ui
