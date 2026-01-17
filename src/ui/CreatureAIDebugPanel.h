#pragma once

// CreatureAIDebugPanel - Visualize what a creature is "thinking"
// Shows neural network inputs/outputs, decision making, and behavior state
// Essential for understanding and debugging AI behavior

#include "imgui.h"
#include "../entities/Creature.h"
#include "../entities/NeuralNetwork.h"
#include <vector>
#include <string>
#include <deque>

namespace ui {

// ============================================================================
// AI Debug Configuration
// ============================================================================
struct AIDebugConfig {
    bool showInputs = true;
    bool showOutputs = true;
    bool showBehaviorState = true;
    bool showNeuralActivity = true;
    bool showVisionCone = true;
    bool showDecisionHistory = true;
    int historySize = 100;  // Number of decision frames to track
};

// ============================================================================
// Creature AI Debug Panel
// ============================================================================
class CreatureAIDebugPanel {
public:
    CreatureAIDebugPanel() = default;
    ~CreatureAIDebugPanel() = default;

    // Set the creature to debug
    void setTarget(const Creature* creature) { m_target = creature; }
    const Creature* getTarget() const { return m_target; }
    bool hasTarget() const { return m_target != nullptr; }

    // Update - call every frame with the target creature
    void update(float deltaTime);

    // Render the debug panel
    void render();

    // Render world-space debugging (vision cone, targets, etc.)
    void renderWorldDebug();

    // Configuration
    AIDebugConfig& getConfig() { return m_config; }
    const AIDebugConfig& getConfig() const { return m_config; }

    // Visibility
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    void toggleVisible() { m_visible = !m_visible; }

private:
    const Creature* m_target = nullptr;
    AIDebugConfig m_config;
    bool m_visible = true;

    // History tracking
    struct DecisionFrame {
        float timestamp;
        std::vector<float> inputs;
        NeuralOutputs outputs;
        std::string decision;  // What the creature decided to do
    };
    std::deque<DecisionFrame> m_decisionHistory;
    float m_time = 0.0f;

    // Cached input names for display
    static const char* getInputName(int index);
    static const char* getOutputName(int index);

    // Render sub-sections
    void renderInputsSection();
    void renderOutputsSection();
    void renderBehaviorState();
    void renderNeuralActivity();
    void renderDecisionHistory();
    void renderVisionSection();

    // Helper to interpret outputs
    std::string interpretDecision(const NeuralOutputs& outputs, CreatureType type);
};

// ============================================================================
// Implementation
// ============================================================================

inline const char* CreatureAIDebugPanel::getInputName(int index) {
    static const char* names[] = {
        "Food Distance",    // 0
        "Food Angle",       // 1
        "Threat Distance",  // 2
        "Threat Angle",     // 3
        "Energy Level",     // 4
        "Current Speed",    // 5
        "Allies Nearby",    // 6
        "Fear Level"        // 7
    };
    if (index >= 0 && index < 8) return names[index];
    return "Unknown";
}

inline const char* CreatureAIDebugPanel::getOutputName(int index) {
    static const char* names[] = {
        "Turn Angle",       // 0
        "Speed Mult",       // 1
        "Aggression",       // 2
        "Fear",             // 3
        "Social",           // 4
        "Exploration"       // 5
    };
    if (index >= 0 && index < 6) return names[index];
    return "Unknown";
}

inline void CreatureAIDebugPanel::update(float deltaTime) {
    m_time += deltaTime;

    if (!m_target || !m_target->isAlive()) {
        return;
    }

    // Record current decision frame (every 0.1 seconds)
    static float recordTimer = 0.0f;
    recordTimer += deltaTime;
    if (recordTimer >= 0.1f) {
        recordTimer = 0.0f;

        // Get current neural state (we'll need to cache this from creature)
        // For now, create a snapshot of what we can observe
        DecisionFrame frame;
        frame.timestamp = m_time;

        // We can't directly access neural inputs/outputs without modifying Creature
        // So we'll infer from observable state
        frame.decision = interpretDecision(NeuralOutputs(), m_target->getType());

        m_decisionHistory.push_back(frame);
        while (m_decisionHistory.size() > static_cast<size_t>(m_config.historySize)) {
            m_decisionHistory.pop_front();
        }
    }
}

inline std::string CreatureAIDebugPanel::interpretDecision(const NeuralOutputs& outputs, CreatureType type) {
    // Interpret based on creature's observable state
    if (!m_target) return "No target";

    float fear = m_target->getFear();
    float energy = m_target->getEnergy();

    if (fear > 0.5f) return "FLEEING";
    if (m_target->getType() == CreatureType::CARNIVORE && m_target->getKillCount() > 0) {
        return "HUNTING";
    }
    if (energy < 50.0f) return "SEEKING FOOD";
    if (energy > 150.0f) return "SEEKING MATE";
    return "EXPLORING";
}

inline void CreatureAIDebugPanel::render() {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Creature AI Debug", &m_visible)) {
        if (!m_target) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1.0f), "No creature selected");
            ImGui::TextWrapped("Select a creature to see its AI state and what it's 'thinking'.");
        } else if (!m_target->isAlive()) {
            ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Creature is dead");
        } else {
            // Header with creature info
            ImGui::Text("Creature #%d", m_target->getId());
            ImGui::SameLine();
            const char* typeName = getCreatureTypeName(m_target->getType());
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "(%s)", typeName);
            ImGui::Text("Generation: %d | Fitness: %.1f", m_target->getGeneration(), m_target->getFitness());
            ImGui::Separator();

            // Current decision with big indicator
            std::string decision = interpretDecision(NeuralOutputs(), m_target->getType());
            ImVec4 decisionColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            if (decision == "FLEEING") decisionColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
            else if (decision == "HUNTING") decisionColor = ImVec4(0.9f, 0.5f, 0.2f, 1.0f);
            else if (decision == "SEEKING FOOD") decisionColor = ImVec4(0.3f, 0.9f, 0.3f, 1.0f);
            else if (decision == "SEEKING MATE") decisionColor = ImVec4(0.9f, 0.3f, 0.9f, 1.0f);
            else if (decision == "EXPLORING") decisionColor = ImVec4(0.3f, 0.7f, 0.9f, 1.0f);

            ImGui::PushStyleColor(ImGuiCol_Text, decisionColor);
            ImGui::Text("Current Decision: %s", decision.c_str());
            ImGui::PopStyleColor();

            ImGui::Separator();

            // Tabbed view for different debug sections
            if (ImGui::BeginTabBar("AIDebugTabs")) {
                if (ImGui::BeginTabItem("Sensors")) {
                    renderInputsSection();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Brain Output")) {
                    renderOutputsSection();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Behavior")) {
                    renderBehaviorState();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Vision")) {
                    renderVisionSection();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
    }
    ImGui::End();
}

inline void CreatureAIDebugPanel::renderInputsSection() {
    ImGui::Text("Sensory Inputs (what the creature perceives):");
    ImGui::Separator();

    if (!m_target) return;

    const Genome& g = m_target->getGenome();

    // Create visual representations of sensory inputs
    ImGui::Columns(2, "SensorColumns", false);

    // Food awareness
    ImGui::Text("Food Sense:");
    float visionRange = g.visionRange;
    ImGui::ProgressBar(visionRange / 50.0f, ImVec2(-1, 0), "Vision Range");
    ImGui::Text("FOV: %.1f deg", g.visionFOV * 57.3f);

    ImGui::NextColumn();

    // Threat awareness
    ImGui::Text("Threat Sense:");
    float fear = m_target->getFear();
    ImGui::ProgressBar(fear, ImVec2(-1, 0), "Fear Level");
    ImGui::Text("Fear: %.2f", fear);

    ImGui::Columns(1);
    ImGui::Separator();

    // Energy state
    ImGui::Text("Internal State:");
    float energy = m_target->getEnergy() / 200.0f;  // Normalize to max energy
    ImVec4 energyColor = energy > 0.5f ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) :
                        energy > 0.25f ? ImVec4(0.8f, 0.8f, 0.2f, 1.0f) :
                                         ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, energyColor);
    ImGui::ProgressBar(energy, ImVec2(-1, 0), "Energy");
    ImGui::PopStyleColor();

    // Speed
    glm::vec3 vel = m_target->getVelocity();
    float speed = glm::length(vel);
    float maxSpeed = g.speed;
    ImGui::ProgressBar(speed / maxSpeed, ImVec2(-1, 0), "Current Speed");

    // Social
    ImGui::Text("Social Traits:");
    ImGui::BulletText("Schooling: %.2f", g.schoolingStrength);
    ImGui::BulletText("Display: %.2f", g.displayIntensity);
}

inline void CreatureAIDebugPanel::renderOutputsSection() {
    ImGui::Text("Neural Network Outputs (what the brain decides):");
    ImGui::Separator();

    if (!m_target) return;

    // Since we can't directly access m_neuralOutputs, we'll show behavioral indicators
    ImGui::Text("Behavior Modulation:");

    // These are visual representations of what the neural network is likely outputting
    // based on observable behavior

    float fear = m_target->getFear();

    // Turn angle - inferred from velocity change
    glm::vec3 vel = m_target->getVelocity();
    float heading = atan2(vel.z, vel.x);
    ImGui::SliderFloat("Heading", &heading, -3.14159f, 3.14159f, "%.2f rad");

    // Speed multiplier
    float speed = glm::length(vel);
    float normalizedSpeed = speed / m_target->getGenome().speed;
    ImGui::ProgressBar(normalizedSpeed, ImVec2(-1, 0), "Speed Multiplier");

    // Behavioral modifiers (inferred)
    ImGui::Separator();
    ImGui::Text("Behavioral Modifiers:");

    // Fear drives fleeing
    ImGui::TextColored(fear > 0.5f ? ImVec4(1,0.5f,0.5f,1) : ImVec4(0.5f,0.5f,0.5f,1),
                      "Fear Response: %.0f%%", fear * 100);

    // Aggression (for carnivores)
    if (m_target->getType() == CreatureType::CARNIVORE ||
        m_target->getType() == CreatureType::FLYING) {
        int kills = m_target->getKillCount();
        ImGui::TextColored(kills > 0 ? ImVec4(1,0.5f,0.2f,1) : ImVec4(0.5f,0.5f,0.5f,1),
                          "Hunt Mode: %s (kills: %d)",
                          kills > 0 ? "ACTIVE" : "passive", kills);
    }

    // Social drive
    ImGui::Text("Social Drive: %.0f%%", m_target->getGenome().schoolingStrength * 100);

    // Exploration
    float age = m_target->getAge();
    float exploration = std::min(age / 60.0f, 1.0f);  // Young creatures explore more
    ImGui::ProgressBar(1.0f - exploration, ImVec2(-1, 0), "Exploration Drive");
}

inline void CreatureAIDebugPanel::renderBehaviorState() {
    ImGui::Text("Current Behavior State:");
    ImGui::Separator();

    if (!m_target) return;

    // Type-specific behavior analysis
    CreatureType type = m_target->getType();

    ImGui::Text("Primary Behavior:");
    switch (type) {
        case CreatureType::HERBIVORE:
            ImGui::BulletText("Role: Prey / Grazer");
            ImGui::BulletText("Goal: Find food, avoid predators");
            if (m_target->getFear() > 0.3f) {
                ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "  -> Currently FLEEING");
            } else if (m_target->getEnergy() < 100.0f) {
                ImGui::TextColored(ImVec4(0.3f,1,0.3f,1), "  -> Seeking food");
            } else {
                ImGui::TextColored(ImVec4(0.5f,0.5f,1,1), "  -> Wandering/social");
            }
            break;

        case CreatureType::CARNIVORE:
            ImGui::BulletText("Role: Predator");
            ImGui::BulletText("Goal: Hunt herbivores");
            if (m_target->getKillCount() >= 2 && m_target->getEnergy() > 150.0f) {
                ImGui::TextColored(ImVec4(0.9f,0.3f,0.9f,1), "  -> Ready to reproduce!");
            } else {
                ImGui::TextColored(ImVec4(1,0.5f,0.2f,1), "  -> Hunting (kills: %d/2)",
                                  m_target->getKillCount());
            }
            break;

        case CreatureType::AQUATIC:
            ImGui::BulletText("Role: Aquatic creature");
            ImGui::BulletText("Goal: School behavior, find food");
            ImGui::TextColored(ImVec4(0.3f,0.6f,1,1), "  -> Schooling/swimming");
            break;

        case CreatureType::FLYING:
            ImGui::BulletText("Role: Aerial predator/scavenger");
            ImGui::BulletText("Goal: Hunt from above");
            ImGui::TextColored(ImVec4(0.8f,0.8f,0.3f,1), "  -> Patrolling");
            break;

        default:
            ImGui::Text("Unknown type");
    }

    ImGui::Separator();

    // Stats
    ImGui::Text("Lifetime Statistics:");
    ImGui::BulletText("Age: %.1f seconds", m_target->getAge());
    ImGui::BulletText("Fitness: %.1f", m_target->getFitness());
    ImGui::BulletText("Generation: %d", m_target->getGeneration());
    if (m_target->canReproduce()) {
        ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.9f, 1.0f), "CAN REPRODUCE!");
    }
}

inline void CreatureAIDebugPanel::renderVisionSection() {
    ImGui::Text("Sensory Systems:");
    ImGui::Separator();

    if (!m_target) return;

    const Genome& g = m_target->getGenome();

    // Vision
    ImGui::Text("Vision:");
    ImGui::BulletText("Range: %.1f units", g.visionRange);
    ImGui::BulletText("FOV: %.1f degrees", g.visionFOV * 57.3f);
    ImGui::BulletText("Acuity: %.0f%%", g.visionAcuity * 100);
    ImGui::BulletText("Motion Detection: %.0f%%", g.motionDetection * 100);

    ImGui::Separator();

    // Hearing
    ImGui::Text("Hearing:");
    ImGui::BulletText("Range: %.1f units", g.hearingRange);
    ImGui::BulletText("Directionality: %.0f%%", g.hearingDirectionality * 100);
    if (g.echolocationAbility > 0.1f) {
        ImGui::BulletText("Echolocation: %.0f%%", g.echolocationAbility * 100);
    }

    ImGui::Separator();

    // Smell
    ImGui::Text("Smell:");
    ImGui::BulletText("Range: %.1f units", g.smellRange);
    ImGui::BulletText("Sensitivity: %.0f%%", g.smellSensitivity * 100);

    ImGui::Separator();

    // Camouflage
    if (g.camouflageLevel > 0.1f) {
        ImGui::Text("Camouflage: %.0f%%", g.camouflageLevel * 100);
    }
}

inline void CreatureAIDebugPanel::renderWorldDebug() {
    // This would render debug visualization in 3D space
    // For now it's a placeholder - would need access to rendering context
    if (!m_target || !m_config.showVisionCone) return;

    // Vision cone, threat indicators, etc. would be rendered here
    // This requires integration with the rendering system
}

} // namespace ui
