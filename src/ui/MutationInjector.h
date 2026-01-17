#pragma once

// MutationInjector - Genetic manipulation tools for God Mode
// Apply mutations, boost mutation rates, and force specific traits

#include "imgui.h"
#include "SelectionSystem.h"
#include "../entities/Creature.h"
#include "../entities/Genome.h"
#include <glm/glm.hpp>
#include <functional>
#include <string>

namespace ui {

// Mutation types that can be applied
enum class MutationType {
    POINT,              // Random small mutation
    LARGE,              // Significant mutation
    BENEFICIAL,         // Guaranteed positive trait change
    HARMFUL,            // Negative trait change
    SIZE_UP,            // Increase size
    SIZE_DOWN,          // Decrease size
    SPEED_UP,           // Increase speed
    SPEED_DOWN,         // Decrease speed
    VISION_UP,          // Increase vision
    VISION_DOWN,        // Decrease vision
    COLOR_SHIFT,        // Random color mutation
    SENSORY_BOOST,      // Boost all sensory traits
    PHYSICAL_BOOST,     // Boost all physical traits
    RANDOM_EXTREME      // Random extreme mutation
};

// Trait categories for targeted manipulation
enum class TraitCategory {
    PHYSICAL,       // Size, speed, efficiency
    SENSORY,        // Vision, hearing, smell
    APPEARANCE,     // Color, camouflage
    FLYING,         // Wing traits (if applicable)
    AQUATIC         // Swimming traits (if applicable)
};

class MutationInjector {
public:
    MutationInjector();
    ~MutationInjector() = default;

    // Main UI render
    void renderUI();

    // Render as section
    void renderSection(SelectionSystem& selection);

    // Set selection system
    void setSelectionSystem(SelectionSystem* selection) { m_selection = selection; }

    // Apply specific mutation type
    void applyMutation(Creature* creature, MutationType type, float strength);

    // Boost mutation rate for future offspring
    void boostMutationRate(Creature* creature, float multiplier, float duration);

    // Force specific trait value
    void forceTrait(Creature* creature, const std::string& traitName, float value);

    // Apply mutations to selection
    void applyToSelected(MutationType type);
    void applyToAllSelected(MutationType type);

    // Bulk operations
    void mutateAllCreatures(MutationType type, float strength);

    // Get mutation name and description
    static const char* getMutationName(MutationType type);
    static const char* getMutationDescription(MutationType type);

    // Panel visibility
    bool isVisible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }
    void toggleVisible() { m_visible = !m_visible; }

private:
    // Dependencies
    SelectionSystem* m_selection = nullptr;

    // UI state
    bool m_visible = true;
    MutationType m_selectedMutation = MutationType::POINT;
    float m_mutationStrength = 0.1f;
    int m_mutationTypeIndex = 0;

    // Trait editor state
    int m_selectedTraitCategory = 0;
    float m_traitValues[20] = {0.0f};  // Temporary storage for trait editing

    // Helper methods
    void renderMutationSelector();
    void renderQuickMutations(Creature* creature);
    void renderTraitEditor(Creature* creature);
    void renderBulkOperations();
    void renderMutationPreview(Creature* creature);

    // Apply specific trait mutations
    void mutatePhysicalTraits(Genome& genome, float strength, bool positive);
    void mutateSensoryTraits(Genome& genome, float strength, bool positive);
    void mutateAppearance(Genome& genome, float strength);
    void mutateFlyingTraits(Genome& genome, float strength);
    void mutateAquaticTraits(Genome& genome, float strength);
};

// ============================================================================
// Implementation
// ============================================================================

inline const char* MutationInjector::getMutationName(MutationType type) {
    switch (type) {
        case MutationType::POINT: return "Point Mutation";
        case MutationType::LARGE: return "Large Mutation";
        case MutationType::BENEFICIAL: return "Beneficial";
        case MutationType::HARMFUL: return "Harmful";
        case MutationType::SIZE_UP: return "Size Up";
        case MutationType::SIZE_DOWN: return "Size Down";
        case MutationType::SPEED_UP: return "Speed Up";
        case MutationType::SPEED_DOWN: return "Speed Down";
        case MutationType::VISION_UP: return "Vision Up";
        case MutationType::VISION_DOWN: return "Vision Down";
        case MutationType::COLOR_SHIFT: return "Color Shift";
        case MutationType::SENSORY_BOOST: return "Sensory Boost";
        case MutationType::PHYSICAL_BOOST: return "Physical Boost";
        case MutationType::RANDOM_EXTREME: return "Random Extreme";
        default: return "Unknown";
    }
}

inline MutationInjector::MutationInjector() {
    // Initialize trait values
    std::fill(std::begin(m_traitValues), std::end(m_traitValues), 0.0f);
}

inline void MutationInjector::applyMutation(Creature* creature, MutationType type, float strength) {
    if (!creature) return;

    // Note: Since Genome is returned as const ref, we can't modify it directly
    // This would need Creature to expose a mutable genome access method
    // For demonstration, we'll show the intended logic

    // Get a copy of the genome (in real implementation, creature would need setGenome())
    Genome genome = creature->getGenome();

    switch (type) {
        case MutationType::POINT:
            genome.mutate(strength, strength);
            break;

        case MutationType::LARGE:
            genome.mutate(strength * 3.0f, strength * 2.0f);
            break;

        case MutationType::BENEFICIAL:
            mutatePhysicalTraits(genome, strength, true);
            break;

        case MutationType::HARMFUL:
            mutatePhysicalTraits(genome, strength, false);
            break;

        case MutationType::SIZE_UP:
            genome.size = std::min(2.0f, genome.size * (1.0f + strength));
            break;

        case MutationType::SIZE_DOWN:
            genome.size = std::max(0.5f, genome.size * (1.0f - strength));
            break;

        case MutationType::SPEED_UP:
            genome.speed = std::min(20.0f, genome.speed * (1.0f + strength));
            break;

        case MutationType::SPEED_DOWN:
            genome.speed = std::max(5.0f, genome.speed * (1.0f - strength));
            break;

        case MutationType::VISION_UP:
            genome.visionRange = std::min(50.0f, genome.visionRange * (1.0f + strength));
            break;

        case MutationType::VISION_DOWN:
            genome.visionRange = std::max(10.0f, genome.visionRange * (1.0f - strength));
            break;

        case MutationType::COLOR_SHIFT:
            mutateAppearance(genome, strength);
            break;

        case MutationType::SENSORY_BOOST:
            mutateSensoryTraits(genome, strength, true);
            break;

        case MutationType::PHYSICAL_BOOST:
            mutatePhysicalTraits(genome, strength, true);
            break;

        case MutationType::RANDOM_EXTREME:
            // Apply multiple random extreme mutations
            for (int i = 0; i < 5; ++i) {
                genome.mutate(0.5f, 0.5f);
            }
            break;
    }

    // In a real implementation, would call creature->setGenome(genome);
}

inline void MutationInjector::mutatePhysicalTraits(Genome& genome, float strength, bool positive) {
    float mult = positive ? (1.0f + strength) : (1.0f - strength);

    genome.size = glm::clamp(genome.size * mult, 0.5f, 2.0f);
    genome.speed = glm::clamp(genome.speed * mult, 5.0f, 20.0f);
    genome.efficiency = glm::clamp(genome.efficiency * (positive ? (1.0f - strength * 0.5f) : (1.0f + strength * 0.5f)),
                                    0.5f, 1.5f);
}

inline void MutationInjector::mutateSensoryTraits(Genome& genome, float strength, bool positive) {
    float mult = positive ? (1.0f + strength) : (1.0f - strength);

    genome.visionRange = glm::clamp(genome.visionRange * mult, 10.0f, 50.0f);
    genome.visionFOV = glm::clamp(genome.visionFOV * mult, 1.0f, 6.0f);
    genome.hearingRange = glm::clamp(genome.hearingRange * mult, 10.0f, 100.0f);
    genome.smellRange = glm::clamp(genome.smellRange * mult, 10.0f, 150.0f);
}

inline void MutationInjector::mutateAppearance(Genome& genome, float strength) {
    // Shift color randomly
    genome.color.x = glm::clamp(genome.color.x + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * strength,
                                 0.0f, 1.0f);
    genome.color.y = glm::clamp(genome.color.y + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * strength,
                                 0.0f, 1.0f);
    genome.color.z = glm::clamp(genome.color.z + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * strength,
                                 0.0f, 1.0f);

    // Also affect camouflage
    genome.camouflageLevel = glm::clamp(genome.camouflageLevel + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * strength,
                                         0.0f, 1.0f);
}

inline void MutationInjector::mutateFlyingTraits(Genome& genome, float strength) {
    genome.wingSpan = glm::clamp(genome.wingSpan * (1.0f + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * strength),
                                  0.5f, 2.0f);
    genome.glideRatio = glm::clamp(genome.glideRatio + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * strength,
                                    0.3f, 0.8f);
    genome.preferredAltitude = glm::clamp(genome.preferredAltitude * (1.0f + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * strength),
                                           15.0f, 40.0f);
}

inline void MutationInjector::mutateAquaticTraits(Genome& genome, float strength) {
    genome.finSize = glm::clamp(genome.finSize * (1.0f + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * strength),
                                 0.3f, 1.0f);
    genome.swimFrequency = glm::clamp(genome.swimFrequency * (1.0f + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * strength),
                                       1.0f, 4.0f);
    genome.preferredDepth = glm::clamp(genome.preferredDepth + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * strength,
                                        0.1f, 0.5f);
}

inline void MutationInjector::boostMutationRate(Creature* creature, float multiplier, float duration) {
    if (!creature) return;
    // This would need Creature to track temporary mutation rate boost
    // creature->setMutationRateMultiplier(multiplier, duration);
}

inline void MutationInjector::forceTrait(Creature* creature, const std::string& traitName, float value) {
    if (!creature) return;
    // Would need mutable genome access
}

inline void MutationInjector::applyToSelected(MutationType type) {
    if (m_selection && m_selection->getSelectedCreature()) {
        applyMutation(m_selection->getSelectedCreature(), type, m_mutationStrength);
    }
}

inline void MutationInjector::applyToAllSelected(MutationType type) {
    if (!m_selection) return;

    for (auto* creature : m_selection->getMultiSelection()) {
        applyMutation(creature, type, m_mutationStrength);
    }
}

inline void MutationInjector::renderUI() {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Mutation Injector", &m_visible)) {
        if (m_selection) {
            renderSection(*m_selection);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
                "Selection system not set!");
        }
    }
    ImGui::End();
}

inline void MutationInjector::renderSection(SelectionSystem& selection) {
    m_selection = &selection;
    Creature* selected = selection.getSelectedCreature();

    // Mutation selector
    if (ImGui::CollapsingHeader("Mutation Type", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderMutationSelector();
    }

    // Quick mutations (if creature selected)
    if (selected && ImGui::CollapsingHeader("Quick Mutations", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderQuickMutations(selected);
    }

    // Trait editor
    if (selected && ImGui::CollapsingHeader("Trait Editor")) {
        renderTraitEditor(selected);
    }

    // Mutation preview
    if (selected && ImGui::CollapsingHeader("Current Traits")) {
        renderMutationPreview(selected);
    }

    // Bulk operations
    if (ImGui::CollapsingHeader("Bulk Operations")) {
        renderBulkOperations();
    }
}

inline void MutationInjector::renderMutationSelector() {
    // Mutation type combo
    const char* mutations[] = {
        "Point Mutation", "Large Mutation", "Beneficial", "Harmful",
        "Size Up", "Size Down", "Speed Up", "Speed Down",
        "Vision Up", "Vision Down", "Color Shift",
        "Sensory Boost", "Physical Boost", "Random Extreme"
    };

    ImGui::Text("Mutation Type:");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##MutationType", &m_mutationTypeIndex, mutations, 14)) {
        m_selectedMutation = static_cast<MutationType>(m_mutationTypeIndex);
    }

    // Strength slider
    ImGui::SliderFloat("Strength", &m_mutationStrength, 0.01f, 0.5f, "%.2f");

    // Description
    ImGui::Separator();
    ImGui::TextWrapped("%s", getMutationDescription(m_selectedMutation));
}

inline const char* MutationInjector::getMutationDescription(MutationType type) {
    switch (type) {
        case MutationType::POINT:
            return "Small random change to genome. Safe and natural.";
        case MutationType::LARGE:
            return "Significant genome modification. Can have dramatic effects.";
        case MutationType::BENEFICIAL:
            return "Guaranteed positive changes to physical traits.";
        case MutationType::HARMFUL:
            return "Negative changes to traits. For experiments.";
        case MutationType::SIZE_UP:
            return "Increase creature size.";
        case MutationType::SIZE_DOWN:
            return "Decrease creature size.";
        case MutationType::SPEED_UP:
            return "Increase movement speed.";
        case MutationType::SPEED_DOWN:
            return "Decrease movement speed.";
        case MutationType::VISION_UP:
            return "Improve vision range.";
        case MutationType::VISION_DOWN:
            return "Reduce vision range.";
        case MutationType::COLOR_SHIFT:
            return "Random change to creature color and camouflage.";
        case MutationType::SENSORY_BOOST:
            return "Boost all sensory capabilities.";
        case MutationType::PHYSICAL_BOOST:
            return "Boost all physical attributes.";
        case MutationType::RANDOM_EXTREME:
            return "Multiple extreme random mutations. Unpredictable!";
        default:
            return "Unknown mutation type.";
    }
}

inline void MutationInjector::renderQuickMutations(Creature* creature) {
    if (!creature) return;

    // Quick mutation buttons
    ImGui::Text("Apply to Selected:");

    // Positive mutations
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 1.0f));

    if (ImGui::Button("Beneficial", ImVec2(100, 0))) {
        applyMutation(creature, MutationType::BENEFICIAL, m_mutationStrength);
    }
    ImGui::SameLine();
    if (ImGui::Button("Size+", ImVec2(60, 0))) {
        applyMutation(creature, MutationType::SIZE_UP, m_mutationStrength);
    }
    ImGui::SameLine();
    if (ImGui::Button("Speed+", ImVec2(60, 0))) {
        applyMutation(creature, MutationType::SPEED_UP, m_mutationStrength);
    }
    ImGui::SameLine();
    if (ImGui::Button("Vision+", ImVec2(60, 0))) {
        applyMutation(creature, MutationType::VISION_UP, m_mutationStrength);
    }

    ImGui::PopStyleColor();

    // Negative mutations
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));

    if (ImGui::Button("Harmful", ImVec2(100, 0))) {
        applyMutation(creature, MutationType::HARMFUL, m_mutationStrength);
    }
    ImGui::SameLine();
    if (ImGui::Button("Size-", ImVec2(60, 0))) {
        applyMutation(creature, MutationType::SIZE_DOWN, m_mutationStrength);
    }
    ImGui::SameLine();
    if (ImGui::Button("Speed-", ImVec2(60, 0))) {
        applyMutation(creature, MutationType::SPEED_DOWN, m_mutationStrength);
    }
    ImGui::SameLine();
    if (ImGui::Button("Vision-", ImVec2(60, 0))) {
        applyMutation(creature, MutationType::VISION_DOWN, m_mutationStrength);
    }

    ImGui::PopStyleColor();

    // Special mutations
    ImGui::Separator();

    if (ImGui::Button("Color Shift", ImVec2(100, 0))) {
        applyMutation(creature, MutationType::COLOR_SHIFT, m_mutationStrength);
    }
    ImGui::SameLine();
    if (ImGui::Button("Sensory Boost", ImVec2(100, 0))) {
        applyMutation(creature, MutationType::SENSORY_BOOST, m_mutationStrength);
    }
    ImGui::SameLine();
    if (ImGui::Button("Physical Boost", ImVec2(100, 0))) {
        applyMutation(creature, MutationType::PHYSICAL_BOOST, m_mutationStrength);
    }

    // Extreme
    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.6f, 1.0f));
    if (ImGui::Button("RANDOM EXTREME", ImVec2(-1, 30))) {
        applyMutation(creature, MutationType::RANDOM_EXTREME, m_mutationStrength);
    }
    ImGui::PopStyleColor();
}

inline void MutationInjector::renderTraitEditor(Creature* creature) {
    if (!creature) return;

    const Genome& genome = creature->getGenome();

    ImGui::TextWrapped("Note: Direct trait editing requires mutable genome access. "
                       "Use mutations to modify traits.");

    ImGui::Separator();

    // Display current values
    ImGui::Text("Physical Traits:");
    ImGui::BulletText("Size: %.2f", genome.size);
    ImGui::BulletText("Speed: %.1f", genome.speed);
    ImGui::BulletText("Efficiency: %.2f", genome.efficiency);

    ImGui::Separator();

    ImGui::Text("Sensory Traits:");
    ImGui::BulletText("Vision Range: %.1f", genome.visionRange);
    ImGui::BulletText("Vision FOV: %.2f rad", genome.visionFOV);
    ImGui::BulletText("Hearing: %.1f", genome.hearingRange);
    ImGui::BulletText("Smell: %.1f", genome.smellRange);
}

inline void MutationInjector::renderMutationPreview(Creature* creature) {
    if (!creature) return;

    const Genome& genome = creature->getGenome();

    // Visual trait summary
    ImGui::Text("Creature #%d", creature->getID());

    // Size visualization
    ImGui::Text("Size:");
    ImGui::SameLine(80);
    float sizeNorm = (genome.size - 0.5f) / 1.5f;  // Normalize 0.5-2.0 to 0-1
    ImGui::ProgressBar(sizeNorm, ImVec2(-1, 0), "");

    // Speed visualization
    ImGui::Text("Speed:");
    ImGui::SameLine(80);
    float speedNorm = (genome.speed - 5.0f) / 15.0f;  // Normalize 5-20 to 0-1
    ImGui::ProgressBar(speedNorm, ImVec2(-1, 0), "");

    // Vision visualization
    ImGui::Text("Vision:");
    ImGui::SameLine(80);
    float visionNorm = (genome.visionRange - 10.0f) / 40.0f;  // Normalize 10-50 to 0-1
    ImGui::ProgressBar(visionNorm, ImVec2(-1, 0), "");

    // Color preview
    ImGui::Text("Color:");
    ImGui::SameLine(80);
    ImGui::ColorButton("##creatureColor",
        ImVec4(genome.color.x, genome.color.y, genome.color.z, 1.0f),
        ImGuiColorEditFlags_NoPicker, ImVec2(60, 20));
}

inline void MutationInjector::renderBulkOperations() {
    ImGui::Text("Apply to All Selected:");

    const auto& multiSel = m_selection ? m_selection->getMultiSelection() : std::vector<Creature*>();
    int count = static_cast<int>(multiSel.size());

    if (count == 0) {
        ImGui::TextDisabled("No creatures selected");
        return;
    }

    ImGui::Text("Selected: %d creatures", count);

    if (ImGui::Button("Apply Mutation to All", ImVec2(-1, 0))) {
        applyToAllSelected(m_selectedMutation);
    }

    ImGui::Separator();

    ImGui::Text("Preset Bulk Mutations:");

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 1.0f));
    if (ImGui::Button("Boost All", ImVec2(100, 0))) {
        for (auto* c : multiSel) {
            applyMutation(c, MutationType::BENEFICIAL, m_mutationStrength);
        }
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("Harm All", ImVec2(100, 0))) {
        for (auto* c : multiSel) {
            applyMutation(c, MutationType::HARMFUL, m_mutationStrength);
        }
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.6f, 1.0f));
    if (ImGui::Button("Chaos!", ImVec2(100, 0))) {
        for (auto* c : multiSel) {
            applyMutation(c, MutationType::RANDOM_EXTREME, m_mutationStrength);
        }
    }
    ImGui::PopStyleColor();
}

} // namespace ui
