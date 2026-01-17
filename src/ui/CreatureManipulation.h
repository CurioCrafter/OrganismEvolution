#pragma once

// CreatureManipulation - Direct creature manipulation tools for God Mode
// Provides clone, kill, heal, force-reproduce, and stat editing capabilities

#include "imgui.h"
#include "SelectionSystem.h"
#include "../entities/Creature.h"
#include "../entities/Genome.h"
#include "../core/CreatureManager.h"
#include <glm/glm.hpp>
#include <functional>
#include <string>
#include <deque>

namespace ui {

// Action types for undo system
enum class ManipulationActionType {
    KILL,
    HEAL,
    CLONE,
    MODIFY_GENOME,
    FORCE_REPRODUCE
};

// Undo action record
struct ManipulationAction {
    ManipulationActionType type;
    int creatureId;
    Genome previousGenome;
    float previousEnergy;
    std::string description;
};

class CreatureManipulation {
public:
    CreatureManipulation();
    ~CreatureManipulation() = default;

    // Main UI render
    void render();

    // Render as embedded section
    void renderSection(SelectionSystem& selection, Forge::CreatureManager& creatures);

    // Set dependencies
    void setSelectionSystem(SelectionSystem* selection) { m_selection = selection; }
    void setCreatureManager(Forge::CreatureManager* creatures) { m_creatures = creatures; }

    // Creature operations
    void cloneCreature(Creature* source);
    void cloneCreatureAt(Creature* source, const glm::vec3& position);
    void killCreature(Creature* target, const std::string& cause = "God Mode");
    void healCreature(Creature* target);
    void healToFull(Creature* target);
    void damageCreature(Creature* target, float amount);
    void setEnergy(Creature* target, float energy);

    // Reproduction control
    void forceReproduce(Creature* parent1, Creature* parent2);
    void forceAsexualReproduce(Creature* parent);

    // Stat modifications
    void boostSpeed(Creature* target, float multiplier);
    void boostSize(Creature* target, float multiplier);
    void boostVision(Creature* target, float multiplier);
    void makeInvincible(Creature* target, bool invincible);
    void setSterile(Creature* target, bool sterile);

    // Bulk operations
    void killAllOfType(CreatureType type);
    void healAllOfType(CreatureType type);
    void cloneMultiple(Creature* source, int count);

    // Undo system
    void undo();
    bool canUndo() const { return !m_undoStack.empty(); }
    int getUndoCount() const { return static_cast<int>(m_undoStack.size()); }

    // Settings
    void setUndoLimit(int limit) { m_undoLimit = limit; }
    int getUndoLimit() const { return m_undoLimit; }

    // Callbacks for external notification
    using CreatureKilledCallback = std::function<void(Creature*)>;
    using CreatureClonedCallback = std::function<void(Creature*, Creature*)>;

    void setOnCreatureKilled(CreatureKilledCallback cb) { m_onKilled = cb; }
    void setOnCreatureCloned(CreatureClonedCallback cb) { m_onCloned = cb; }

    // Panel state
    bool isVisible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }
    void toggleVisible() { m_visible = !m_visible; }

private:
    // Dependencies
    SelectionSystem* m_selection = nullptr;
    Forge::CreatureManager* m_creatures = nullptr;

    // UI state
    bool m_visible = true;
    bool m_genomeEditorExpanded = false;
    bool m_showAdvancedOptions = false;

    // Clone spawn offset
    glm::vec3 m_cloneOffset{5.0f, 0.0f, 0.0f};

    // Bulk operation count
    int m_bulkCloneCount = 5;

    // Undo system
    std::deque<ManipulationAction> m_undoStack;
    int m_undoLimit = 50;

    // Callbacks
    CreatureKilledCallback m_onKilled;
    CreatureClonedCallback m_onCloned;

    // Temporary genome for editing
    Genome m_editGenome;
    bool m_genomeModified = false;

    // Helper methods
    void renderCreatureInfo(Creature* creature);
    void renderQuickActions(Creature* creature);
    void renderGenomeEditor(Creature* creature);
    void renderBulkOperations();
    void renderUndoControls();

    void pushUndoAction(const ManipulationAction& action);
    void recordState(Creature* creature, ManipulationActionType type, const std::string& desc);
};

// ============================================================================
// Implementation
// ============================================================================

inline CreatureManipulation::CreatureManipulation() {
    m_editGenome.randomize();
}

inline void CreatureManipulation::cloneCreature(Creature* source) {
    if (!source || !m_creatures) return;

    glm::vec3 newPos = source->getPosition() + m_cloneOffset;
    cloneCreatureAt(source, newPos);
}

inline void CreatureManipulation::cloneCreatureAt(Creature* source, const glm::vec3& position) {
    if (!source || !m_creatures) return;

    Forge::CreatureHandle handle = m_creatures->spawnWithGenome(position, source->getGenome());
    Creature* newCreature = m_creatures->get(handle);

    if (newCreature && m_onCloned) {
        m_onCloned(source, newCreature);
    }

    recordState(source, ManipulationActionType::CLONE, "Cloned creature");
}

inline void CreatureManipulation::killCreature(Creature* target, const std::string& cause) {
    if (!target || !m_creatures) return;

    recordState(target, ManipulationActionType::KILL, "Killed: " + cause);

    // Find handle for creature and kill it
    m_creatures->forEach([&](Creature& c, size_t idx) {
        if (&c == target) {
            Forge::CreatureHandle handle;
            handle.index = static_cast<uint32_t>(idx);
            handle.generation = 1;  // Simplified - real implementation tracks generation
            m_creatures->kill(handle, cause);
        }
    });

    if (m_onKilled) {
        m_onKilled(target);
    }
}

inline void CreatureManipulation::healCreature(Creature* target) {
    if (!target) return;

    recordState(target, ManipulationActionType::HEAL, "Healed creature");

    // Heal by adding energy
    target->consumeFood(50.0f);
}

inline void CreatureManipulation::healToFull(Creature* target) {
    if (!target) return;

    recordState(target, ManipulationActionType::HEAL, "Healed to full");

    // Add enough energy to reach max (200 is max from Creature.h)
    float needed = 200.0f - target->getEnergy();
    if (needed > 0) {
        target->consumeFood(needed);
    }
}

inline void CreatureManipulation::damageCreature(Creature* target, float amount) {
    if (!target) return;

    recordState(target, ManipulationActionType::HEAL, "Damaged creature");
    target->takeDamage(amount);
}

inline void CreatureManipulation::setEnergy(Creature* target, float energy) {
    if (!target) return;

    recordState(target, ManipulationActionType::HEAL, "Set energy");

    float current = target->getEnergy();
    if (energy > current) {
        target->consumeFood(energy - current);
    } else {
        target->takeDamage(current - energy);
    }
}

inline void CreatureManipulation::forceReproduce(Creature* parent1, Creature* parent2) {
    if (!parent1 || !parent2 || !m_creatures) return;

    // Calculate midpoint position for offspring
    glm::vec3 midpoint = (parent1->getPosition() + parent2->getPosition()) * 0.5f;
    midpoint += glm::vec3((rand() % 10) - 5, 0.0f, (rand() % 10) - 5);

    // Create offspring genome from both parents
    Genome offspringGenome(parent1->getGenome(), parent2->getGenome());
    offspringGenome.mutate(0.1f, 0.1f);

    m_creatures->spawnWithGenome(midpoint, offspringGenome);

    recordState(parent1, ManipulationActionType::FORCE_REPRODUCE, "Forced reproduction");
}

inline void CreatureManipulation::forceAsexualReproduce(Creature* parent) {
    if (!parent || !m_creatures) return;

    glm::vec3 newPos = parent->getPosition() + glm::vec3(
        (rand() % 10) - 5, 0.0f, (rand() % 10) - 5
    );

    Genome offspringGenome = parent->getGenome();
    offspringGenome.mutate(0.15f, 0.2f);  // Higher mutation for asexual

    m_creatures->spawnWithGenome(newPos, offspringGenome);

    recordState(parent, ManipulationActionType::FORCE_REPRODUCE, "Forced asexual reproduction");
}

inline void CreatureManipulation::boostSpeed(Creature* target, float multiplier) {
    if (!target) return;
    // Note: Genome modifications require careful handling since Genome is const ref
    // This would need Creature to expose mutable genome access or use a different approach
}

inline void CreatureManipulation::boostSize(Creature* target, float multiplier) {
    if (!target) return;
    // Similar to boostSpeed - requires mutable genome access
}

inline void CreatureManipulation::boostVision(Creature* target, float multiplier) {
    if (!target) return;
    // Similar to boostSpeed - requires mutable genome access
}

inline void CreatureManipulation::makeInvincible(Creature* target, bool invincible) {
    if (!target) return;
    // Would need to add invincibility flag to Creature class
}

inline void CreatureManipulation::setSterile(Creature* target, bool sterile) {
    if (!target) return;
    target->setSterile(sterile);
}

inline void CreatureManipulation::killAllOfType(CreatureType type) {
    if (!m_creatures) return;

    std::vector<Creature*> toKill;
    m_creatures->forEachOfType(type, [&](Creature& c) {
        toKill.push_back(&c);
    });

    for (auto* c : toKill) {
        killCreature(c, "Mass extinction");
    }
}

inline void CreatureManipulation::healAllOfType(CreatureType type) {
    if (!m_creatures) return;

    m_creatures->forEachOfType(type, [&](Creature& c) {
        healToFull(&c);
    });
}

inline void CreatureManipulation::cloneMultiple(Creature* source, int count) {
    if (!source || !m_creatures) return;

    for (int i = 0; i < count; ++i) {
        float angle = (2.0f * 3.14159f * i) / count;
        glm::vec3 offset(cos(angle) * 10.0f, 0.0f, sin(angle) * 10.0f);
        cloneCreatureAt(source, source->getPosition() + offset);
    }
}

inline void CreatureManipulation::undo() {
    if (m_undoStack.empty()) return;

    // Pop and discard - true undo would require storing more state
    m_undoStack.pop_back();
}

inline void CreatureManipulation::pushUndoAction(const ManipulationAction& action) {
    m_undoStack.push_back(action);

    // Trim if over limit
    while (static_cast<int>(m_undoStack.size()) > m_undoLimit) {
        m_undoStack.pop_front();
    }
}

inline void CreatureManipulation::recordState(Creature* creature, ManipulationActionType type, const std::string& desc) {
    if (!creature) return;

    ManipulationAction action;
    action.type = type;
    action.creatureId = creature->getID();
    action.previousGenome = creature->getGenome();
    action.previousEnergy = creature->getEnergy();
    action.description = desc;

    pushUndoAction(action);
}

inline void CreatureManipulation::render() {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(400, 550), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Creature Manipulation", &m_visible)) {
        if (m_selection && m_creatures) {
            renderSection(*m_selection, *m_creatures);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
                "Selection system or creature manager not set!");
        }
    }
    ImGui::End();
}

inline void CreatureManipulation::renderSection(SelectionSystem& selection, Forge::CreatureManager& creatures) {
    m_selection = &selection;
    m_creatures = &creatures;

    Creature* selected = selection.getSelectedCreature();

    // Creature info section
    if (ImGui::CollapsingHeader("Selected Creature", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (selected) {
            renderCreatureInfo(selected);
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No creature selected");
            ImGui::TextWrapped("Click on a creature to select it, or use Shift+Click for multi-select.");
        }
    }

    // Quick actions
    if (selected && ImGui::CollapsingHeader("Quick Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderQuickActions(selected);
    }

    // Multi-selection actions
    const auto& multiSel = selection.getMultiSelection();
    if (multiSel.size() > 1 && ImGui::CollapsingHeader("Multi-Selection Actions")) {
        ImGui::Text("Selected: %d creatures", static_cast<int>(multiSel.size()));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("Heal All Selected", ImVec2(-1, 0))) {
            for (auto* c : multiSel) {
                healToFull(c);
            }
        }
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Kill All Selected", ImVec2(-1, 0))) {
            for (auto* c : multiSel) {
                killCreature(c, "Mass kill");
            }
            selection.clearSelection();
        }
        ImGui::PopStyleColor();

        // Force breed if exactly 2 selected
        if (multiSel.size() == 2) {
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.6f, 1.0f));
            if (ImGui::Button("Force Breed These Two", ImVec2(-1, 0))) {
                forceReproduce(multiSel[0], multiSel[1]);
            }
            ImGui::PopStyleColor();
        }
    }

    // Genome editor
    if (selected && ImGui::CollapsingHeader("Genome Viewer")) {
        renderGenomeEditor(selected);
    }

    // Bulk operations
    if (ImGui::CollapsingHeader("Bulk Operations")) {
        renderBulkOperations();
    }

    // Undo controls
    if (ImGui::CollapsingHeader("Undo History")) {
        renderUndoControls();
    }
}

inline void CreatureManipulation::renderCreatureInfo(Creature* creature) {
    if (!creature) return;

    // Basic info
    ImGui::Text("Type: %s", getCreatureTypeName(creature->getType()));
    ImGui::Text("ID: %d", creature->getID());
    ImGui::Text("Generation: %d", creature->getGeneration());

    ImGui::Separator();

    // Stats
    ImGui::Text("Energy: %.1f / 200", creature->getEnergy());

    // Energy bar
    float energyPercent = creature->getEnergy() / 200.0f;
    ImVec4 energyColor = energyPercent > 0.5f
        ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f)
        : (energyPercent > 0.25f
            ? ImVec4(0.8f, 0.8f, 0.2f, 1.0f)
            : ImVec4(0.8f, 0.2f, 0.2f, 1.0f));

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, energyColor);
    ImGui::ProgressBar(energyPercent, ImVec2(-1, 0), "");
    ImGui::PopStyleColor();

    ImGui::Text("Age: %.1f", creature->getAge());
    ImGui::Text("Fitness: %.2f", creature->getFitness());

    if (isPredator(creature->getType())) {
        ImGui::Text("Kills: %d", creature->getKillCount());
    }

    ImGui::Separator();

    // Position
    glm::vec3 pos = creature->getPosition();
    ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);

    glm::vec3 vel = creature->getVelocity();
    float speed = glm::length(vel);
    ImGui::Text("Speed: %.2f", speed);

    // Status flags
    if (creature->isSterile()) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "STERILE");
    }
    if (creature->isBeingHunted()) {
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "BEING HUNTED");
    }
}

inline void CreatureManipulation::renderQuickActions(Creature* creature) {
    if (!creature) return;

    // Row 1: Health
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    if (ImGui::Button("Heal +50", ImVec2(100, 0))) {
        healCreature(creature);
    }
    ImGui::SameLine();
    if (ImGui::Button("Heal Full", ImVec2(100, 0))) {
        healToFull(creature);
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.4f, 0.2f, 1.0f));
    if (ImGui::Button("Damage 25", ImVec2(100, 0))) {
        damageCreature(creature, 25.0f);
    }
    ImGui::PopStyleColor();

    // Row 2: Clone
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
    if (ImGui::Button("Clone x1", ImVec2(100, 0))) {
        cloneCreature(creature);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clone x5", ImVec2(100, 0))) {
        cloneMultiple(creature, 5);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clone x10", ImVec2(100, 0))) {
        cloneMultiple(creature, 10);
    }
    ImGui::PopStyleColor();

    // Row 3: Reproduce
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.6f, 1.0f));
    if (ImGui::Button("Asexual Reproduce", ImVec2(150, 0))) {
        forceAsexualReproduce(creature);
    }
    ImGui::PopStyleColor();

    // Row 4: Kill/Status
    ImGui::Separator();

    bool sterile = creature->isSterile();
    if (ImGui::Checkbox("Sterile", &sterile)) {
        setSterile(creature, sterile);
    }

    ImGui::SameLine(200);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("KILL", ImVec2(100, 25))) {
        killCreature(creature, "God Mode");
        if (m_selection) {
            m_selection->clearSelection();
        }
    }
    ImGui::PopStyleColor();
}

inline void CreatureManipulation::renderGenomeEditor(Creature* creature) {
    if (!creature) return;

    const Genome& genome = creature->getGenome();

    ImGui::Text("Physical Traits:");
    ImGui::BulletText("Size: %.2f", genome.size);
    ImGui::BulletText("Speed: %.1f", genome.speed);
    ImGui::BulletText("Vision Range: %.1f", genome.visionRange);
    ImGui::BulletText("Efficiency: %.2f", genome.efficiency);

    ImGui::Separator();

    ImGui::Text("Sensory Traits:");
    ImGui::BulletText("Vision FOV: %.2f rad", genome.visionFOV);
    ImGui::BulletText("Hearing Range: %.1f", genome.hearingRange);
    ImGui::BulletText("Smell Range: %.1f", genome.smellRange);
    ImGui::BulletText("Camouflage: %.2f", genome.camouflageLevel);

    ImGui::Separator();

    ImGui::Text("Color:");
    ImGui::ColorButton("##color", ImVec4(genome.color.x, genome.color.y, genome.color.z, 1.0f),
                       ImGuiColorEditFlags_NoPicker, ImVec2(50, 20));

    // Type-specific traits
    if (isFlying(creature->getType())) {
        ImGui::Separator();
        ImGui::Text("Flying Traits:");
        ImGui::BulletText("Wing Span: %.2f", genome.wingSpan);
        ImGui::BulletText("Glide Ratio: %.2f", genome.glideRatio);
        ImGui::BulletText("Preferred Altitude: %.1f", genome.preferredAltitude);
    }

    if (isAquatic(creature->getType())) {
        ImGui::Separator();
        ImGui::Text("Aquatic Traits:");
        ImGui::BulletText("Fin Size: %.2f", genome.finSize);
        ImGui::BulletText("Swim Frequency: %.2f", genome.swimFrequency);
        ImGui::BulletText("Preferred Depth: %.2f", genome.preferredDepth);
    }
}

inline void CreatureManipulation::renderBulkOperations() {
    ImGui::Text("Bulk Clone:");
    ImGui::SetNextItemWidth(100);
    ImGui::InputInt("Count", &m_bulkCloneCount);
    m_bulkCloneCount = std::max(1, std::min(100, m_bulkCloneCount));

    ImGui::SameLine();
    if (m_selection && m_selection->getSelectedCreature()) {
        if (ImGui::Button("Clone Selected")) {
            cloneMultiple(m_selection->getSelectedCreature(), m_bulkCloneCount);
        }
    } else {
        ImGui::TextDisabled("(select a creature first)");
    }

    ImGui::Separator();

    ImGui::Text("Kill by Type:");

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));

    if (ImGui::Button("Herbivores", ImVec2(100, 0))) {
        killAllOfType(CreatureType::GRAZER);
        killAllOfType(CreatureType::BROWSER);
        killAllOfType(CreatureType::FRUGIVORE);
    }
    ImGui::SameLine();
    if (ImGui::Button("Predators", ImVec2(100, 0))) {
        killAllOfType(CreatureType::APEX_PREDATOR);
        killAllOfType(CreatureType::SMALL_PREDATOR);
    }
    ImGui::SameLine();
    if (ImGui::Button("Flying", ImVec2(100, 0))) {
        killAllOfType(CreatureType::FLYING);
        killAllOfType(CreatureType::FLYING_BIRD);
        killAllOfType(CreatureType::AERIAL_PREDATOR);
    }

    if (ImGui::Button("Aquatic", ImVec2(100, 0))) {
        killAllOfType(CreatureType::AQUATIC);
        killAllOfType(CreatureType::AQUATIC_PREDATOR);
        killAllOfType(CreatureType::AQUATIC_APEX);
    }

    ImGui::PopStyleColor();

    ImGui::Separator();

    ImGui::Text("Heal by Type:");

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));

    if (ImGui::Button("All Herbivores", ImVec2(100, 0))) {
        healAllOfType(CreatureType::GRAZER);
        healAllOfType(CreatureType::BROWSER);
        healAllOfType(CreatureType::FRUGIVORE);
    }
    ImGui::SameLine();
    if (ImGui::Button("All Predators", ImVec2(100, 0))) {
        healAllOfType(CreatureType::APEX_PREDATOR);
        healAllOfType(CreatureType::SMALL_PREDATOR);
    }

    ImGui::PopStyleColor();
}

inline void CreatureManipulation::renderUndoControls() {
    ImGui::Text("Undo Stack: %d / %d", getUndoCount(), m_undoLimit);

    if (ImGui::Button("Undo", ImVec2(80, 0))) {
        undo();
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear History", ImVec2(100, 0))) {
        m_undoStack.clear();
    }

    // Show recent actions
    if (!m_undoStack.empty()) {
        ImGui::Separator();
        ImGui::Text("Recent Actions:");

        int showCount = std::min(5, static_cast<int>(m_undoStack.size()));
        for (int i = 0; i < showCount; ++i) {
            const auto& action = m_undoStack[m_undoStack.size() - 1 - i];
            ImGui::BulletText("%s", action.description.c_str());
        }
    }
}

} // namespace ui
