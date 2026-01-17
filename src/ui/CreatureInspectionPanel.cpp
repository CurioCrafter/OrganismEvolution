#include "CreatureInspectionPanel.h"
#include "../entities/genetics/Species.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace ui {

// ============================================================================
// Constructor
// ============================================================================

CreatureInspectionPanel::CreatureInspectionPanel() = default;

// ============================================================================
// Main Interface
// ============================================================================

void CreatureInspectionPanel::setInspectedCreature(Creature* creature) {
    if (creature == m_inspectedCreature) return;

    m_inspectedCreature = creature;
    m_inspectedCreatureId = creature ? creature->getID() : -1;

    if (creature) {
        m_mode = InspectionMode::VIEWING;
        m_visible = true;
    } else {
        m_mode = InspectionMode::NONE;
    }
}

void CreatureInspectionPanel::clearInspection() {
    // Release camera if tracking
    if (m_mode == InspectionMode::TRACKING || m_mode == InspectionMode::FOCUSED) {
        if (m_releaseCameraCallback) {
            m_releaseCameraCallback();
        }
    }

    m_inspectedCreature = nullptr;
    m_inspectedCreatureId = -1;
    m_mode = InspectionMode::NONE;
}

bool CreatureInspectionPanel::validateCreature() {
    if (!m_inspectedCreature) {
        m_mode = InspectionMode::NONE;
        return false;
    }

    // Check if creature died or ID mismatch (pointer reused)
    if (!m_inspectedCreature->isAlive() || m_inspectedCreature->getID() != m_inspectedCreatureId) {
        // Creature despawned or pointer was reused
        if (m_mode == InspectionMode::TRACKING || m_mode == InspectionMode::FOCUSED) {
            if (m_releaseCameraCallback) {
                m_releaseCameraCallback();
            }
        }
        m_inspectedCreature = nullptr;
        m_inspectedCreatureId = -1;
        m_mode = InspectionMode::NONE;
        return false;
    }

    return true;
}

// ============================================================================
// Rendering
// ============================================================================

void CreatureInspectionPanel::render() {
    if (!m_visible) return;

    // Validate creature is still alive
    if (!validateCreature()) {
        // Show "No creature selected" state
        ImGui::SetNextWindowSize(ImVec2(320, 150), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Creature Inspection", &m_visible, ImGuiWindowFlags_NoCollapse)) {
            ImGui::TextWrapped("No creature selected.");
            ImGui::TextWrapped("Click on a creature in the world to inspect it.");
        }
        ImGui::End();
        return;
    }

    Creature* c = m_inspectedCreature;

    // Set up window
    ImGui::SetNextWindowSize(ImVec2(340, 600), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    char windowTitle[128];
    snprintf(windowTitle, sizeof(windowTitle), "Inspecting: %s###CreatureInspection",
             c->getSpeciesDisplayName().empty() ? "Unknown Species" : c->getSpeciesDisplayName().c_str());

    if (ImGui::Begin(windowTitle, &m_visible, flags)) {
        // Camera control buttons at the top
        renderCameraControls();

        ImGui::Separator();

        // Color palette strip
        renderColorPalette(c);

        ImGui::Separator();

        // Scrollable content area
        ImGui::BeginChild("InspectionContent", ImVec2(0, 0), false, ImGuiWindowFlags_None);

        // Identity Section
        if (m_showIdentity && ImGui::CollapsingHeader("Identity", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderIdentitySection(c);
        }

        // Biology Section
        if (m_showBiology && ImGui::CollapsingHeader("Biology", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderBiologySection(c);
        }

        // Morphology Section
        if (m_showMorphology && ImGui::CollapsingHeader("Morphology", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderMorphologySection(c);
        }

        // Status Section
        if (m_showStatus && ImGui::CollapsingHeader("Status", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderStatusSection(c);
        }

        // Environment Section
        if (m_showEnvironment && ImGui::CollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderEnvironmentSection(c);
        }

        // Genetics Section (collapsed by default)
        if (m_showGenetics && ImGui::CollapsingHeader("Genetics")) {
            renderGeneticsSection(c);
        }

        // Brain Section (collapsed by default)
        if (m_showBrain && ImGui::CollapsingHeader("Brain")) {
            renderBrainSection(c);
        }

        ImGui::EndChild();
    }
    ImGui::End();

    // Handle window close
    if (!m_visible) {
        clearInspection();
    }
}

void CreatureInspectionPanel::renderScreenIndicator(const Camera& camera, float screenWidth, float screenHeight) {
    if (!validateCreature()) return;

    Creature* c = m_inspectedCreature;
    glm::vec3 worldPos = c->getPosition();

    // Project world position to screen
    glm::mat4 view = const_cast<Camera&>(camera).getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(screenWidth / screenHeight);
    glm::mat4 vp = proj * view;

    glm::vec4 clipPos = vp * glm::vec4(worldPos, 1.0f);

    // Behind camera check
    if (clipPos.w <= 0) return;

    // Perspective divide
    glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

    // Convert to screen coords
    float screenX = (ndc.x + 1.0f) * 0.5f * screenWidth;
    float screenY = (1.0f - ndc.y) * 0.5f * screenHeight;

    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    // Draw indicator based on mode
    float baseRadius = 25.0f / (clipPos.w * 0.05f);
    baseRadius = glm::clamp(baseRadius, 15.0f, 50.0f);

    ImU32 color;
    float thickness;

    switch (m_mode) {
        case InspectionMode::TRACKING:
            color = IM_COL32(50, 255, 100, 220);  // Bright green for tracking
            thickness = 3.0f;
            // Draw targeting brackets
            {
                float s = baseRadius;
                float corner = s * 0.3f;
                // Top-left bracket
                drawList->AddLine(ImVec2(screenX - s, screenY - s + corner), ImVec2(screenX - s, screenY - s), color, thickness);
                drawList->AddLine(ImVec2(screenX - s, screenY - s), ImVec2(screenX - s + corner, screenY - s), color, thickness);
                // Top-right bracket
                drawList->AddLine(ImVec2(screenX + s - corner, screenY - s), ImVec2(screenX + s, screenY - s), color, thickness);
                drawList->AddLine(ImVec2(screenX + s, screenY - s), ImVec2(screenX + s, screenY - s + corner), color, thickness);
                // Bottom-left bracket
                drawList->AddLine(ImVec2(screenX - s, screenY + s - corner), ImVec2(screenX - s, screenY + s), color, thickness);
                drawList->AddLine(ImVec2(screenX - s, screenY + s), ImVec2(screenX - s + corner, screenY + s), color, thickness);
                // Bottom-right bracket
                drawList->AddLine(ImVec2(screenX + s - corner, screenY + s), ImVec2(screenX + s, screenY + s), color, thickness);
                drawList->AddLine(ImVec2(screenX + s, screenY + s - corner), ImVec2(screenX + s, screenY + s), color, thickness);
            }
            break;

        case InspectionMode::FOCUSED:
            color = IM_COL32(100, 200, 255, 200);  // Blue for focused
            thickness = 2.5f;
            drawList->AddCircle(ImVec2(screenX, screenY), baseRadius, color, 32, thickness);
            drawList->AddCircle(ImVec2(screenX, screenY), baseRadius * 0.4f, color, 16, thickness);
            break;

        case InspectionMode::VIEWING:
        default:
            color = IM_COL32(255, 200, 50, 180);  // Gold for viewing
            thickness = 2.0f;
            drawList->AddCircle(ImVec2(screenX, screenY), baseRadius, color, 32, thickness);
            break;
    }

    // Draw small label below indicator
    if (m_mode != InspectionMode::NONE) {
        const char* modeLabel = (m_mode == InspectionMode::TRACKING) ? "TRACKING" :
                                (m_mode == InspectionMode::FOCUSED) ? "FOCUSED" : "SELECTED";
        ImVec2 textSize = ImGui::CalcTextSize(modeLabel);
        float labelX = screenX - textSize.x * 0.5f;
        float labelY = screenY + baseRadius + 5.0f;

        drawList->AddRectFilled(
            ImVec2(labelX - 4, labelY - 2),
            ImVec2(labelX + textSize.x + 4, labelY + textSize.y + 2),
            IM_COL32(0, 0, 0, 150)
        );
        drawList->AddText(ImVec2(labelX, labelY), color, modeLabel);
    }
}

// ============================================================================
// Camera Controls
// ============================================================================

void CreatureInspectionPanel::renderCameraControls() {
    ImGui::Text("Camera:");
    ImGui::SameLine();

    // Focus button
    bool isFocused = (m_mode == InspectionMode::FOCUSED);
    if (isFocused) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
    }
    if (ImGui::Button("Focus")) {
        if (m_focusCameraCallback && m_inspectedCreature) {
            m_focusCameraCallback(m_inspectedCreature);
            m_mode = InspectionMode::FOCUSED;
        } else if (m_cameraController && m_inspectedCreature) {
            m_cameraController->startFollowTarget(m_inspectedCreature);
            m_cameraController->lockTarget(true);
            m_mode = InspectionMode::FOCUSED;
        }
    }
    if (isFocused) {
        ImGui::PopStyleColor();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Move camera to creature");
    }

    ImGui::SameLine();

    // Track button
    bool isTracking = (m_mode == InspectionMode::TRACKING);
    if (isTracking) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
    }
    if (ImGui::Button("Track")) {
        if (m_trackCameraCallback && m_inspectedCreature) {
            m_trackCameraCallback(m_inspectedCreature);
            m_mode = InspectionMode::TRACKING;
        } else if (m_cameraController && m_inspectedCreature) {
            m_cameraController->setMode(Forge::CameraMode::CINEMATIC_FOLLOW_TARGET, true);
            m_cameraController->startFollowTarget(m_inspectedCreature);
            m_cameraController->lockTarget(true);
            m_mode = InspectionMode::TRACKING;
        }
    }
    if (isTracking) {
        ImGui::PopStyleColor();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Follow creature with camera");
    }

    ImGui::SameLine();

    // Release button
    if (ImGui::Button("Release")) {
        if (m_releaseCameraCallback) {
            m_releaseCameraCallback();
        } else if (m_cameraController) {
            m_cameraController->lockTarget(false);
            m_cameraController->clearTargetOverride();
            m_cameraController->setMode(Forge::CameraMode::FREE, true);
        }
        m_mode = InspectionMode::VIEWING;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Release camera to free movement");
    }

    ImGui::SameLine();

    // Close button (deselect creature)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("X")) {
        clearInspection();
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Stop inspecting");
    }
}

// ============================================================================
// Section Implementations
// ============================================================================

void CreatureInspectionPanel::renderIdentitySection(Creature* c) {
    // Species name with color chip
    ImVec4 speciesColor = getSpeciesSimilarityColor(c);
    ImGui::ColorButton("##SpeciesColor", speciesColor, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(16, 16));
    ImGui::SameLine();

    const std::string& displayName = c->getSpeciesDisplayName();
    ImGui::Text("Species: %s", displayName.empty() ? "Unknown" : displayName.c_str());

    // Creature type
    ImGui::Text("Type: %s", getCreatureTypeString(c->getType()));

    // ID and Generation
    ImGui::Text("ID: #%d", c->getID());
    ImGui::SameLine();
    ImGui::Text("  Generation: %d", c->getGeneration());

    // Species ID (if available)
    auto speciesId = c->getSpeciesId();
    if (speciesId != genetics::INVALID_SPECIES_ID) {
        ImGui::Text("Species ID: %u", speciesId);
    }
}

void CreatureInspectionPanel::renderBiologySection(Creature* c) {
    const Genome& g = c->getGenome();

    // Size
    ImGui::Text("Size: %.2f", g.size);

    // Age
    float ageSeconds = c->getAge();
    if (ageSeconds < 60.0f) {
        ImGui::Text("Age: %.1f seconds", ageSeconds);
    } else {
        int minutes = (int)(ageSeconds / 60.0f);
        int seconds = (int)ageSeconds % 60;
        ImGui::Text("Age: %d:%02d", minutes, seconds);
    }

    // Sex (determined by genome if applicable)
    bool isFemale = (g.sex == 0);  // Assuming 0 = female, 1 = male
    ImGui::Text("Sex: %s", isFemale ? "Female" : "Male");

    // Chemistry affinity (diet preference)
    const char* dietStr;
    if (c->getType() == CreatureType::HERBIVORE) {
        dietStr = "Herbivore (Plants)";
    } else if (c->getType() == CreatureType::CARNIVORE) {
        dietStr = "Carnivore (Meat)";
    } else if (c->getType() == CreatureType::FLYING) {
        dietStr = "Omnivore (Mixed)";
    } else {
        dietStr = "Omnivore (Filter/Mixed)";
    }
    ImGui::Text("Diet: %s", dietStr);

    // Sterility indicator
    if (c->isSterile()) {
        ImGui::TextColored(ImVec4(0.8f, 0.5f, 0.2f, 1.0f), "Sterile (Hybrid)");
    }
}

void CreatureInspectionPanel::renderMorphologySection(Creature* c) {
    const Genome& g = c->getGenome();

    // Archetype based on type
    const char* archetype;
    switch (c->getType()) {
        case CreatureType::HERBIVORE: archetype = "Terrestrial Grazer"; break;
        case CreatureType::CARNIVORE: archetype = "Terrestrial Predator"; break;
        case CreatureType::AQUATIC: archetype = "Aquatic Swimmer"; break;
        case CreatureType::FLYING: archetype = "Aerial Flyer"; break;
        default: archetype = "Unknown"; break;
    }
    ImGui::Text("Archetype: %s", archetype);

    // Key features
    ImGui::Text("Speed Gene: %.2f", g.speed);
    ImGui::Text("Vision Range: %.1f", g.visionRange);
    ImGui::Text("Efficiency: %.2f", g.efficiency);

    // Camouflage
    if (g.camouflageLevel > 0.1f) {
        ImGui::Text("Camouflage: %.0f%%", g.camouflageLevel * 100.0f);
    }

    // Bioluminescence
    if (c->hasBioluminescence()) {
        glm::vec3 biolumColor = c->getBiolumColor();
        ImGui::Text("Bioluminescence:");
        ImGui::SameLine();
        ImGui::ColorButton("##BiolumColor",
            ImVec4(biolumColor.r, biolumColor.g, biolumColor.b, 1.0f),
            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(16, 16));
        ImGui::SameLine();
        ImGui::Text("%.0f%% intensity", c->getBiolumIntensity() * 100.0f);
    }

    // Pattern type
    int pattern = c->getSpeciesPatternType();
    const char* patternStr;
    switch (pattern) {
        case 0: patternStr = "Solid"; break;
        case 1: patternStr = "Striped"; break;
        case 2: patternStr = "Spotted"; break;
        case 3: patternStr = "Gradient"; break;
        default: patternStr = "Mixed"; break;
    }
    ImGui::Text("Pattern: %s", patternStr);
}

void CreatureInspectionPanel::renderStatusSection(Creature* c) {
    // Health/Energy bar
    float energyRatio = c->getEnergy() / c->getMaxEnergy();
    ImVec4 energyColor = energyRatio > 0.5f ? ImVec4(0.3f, 0.8f, 0.3f, 1.0f) :
                         energyRatio > 0.25f ? ImVec4(0.8f, 0.8f, 0.3f, 1.0f) :
                                               ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, energyColor);
    char energyLabel[32];
    snprintf(energyLabel, sizeof(energyLabel), "%.0f / %.0f", c->getEnergy(), c->getMaxEnergy());
    ImGui::ProgressBar(energyRatio, ImVec2(-1, 0), energyLabel);
    ImGui::PopStyleColor();

    // Fitness
    ImGui::Text("Fitness: %.2f", c->getFitness());

    // Fear level (for prey)
    if (c->getType() == CreatureType::HERBIVORE || c->getType() == CreatureType::AQUATIC) {
        float fear = c->getFear();
        if (fear > 0.1f) {
            ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.2f, 1.0f), "Fear: %.0f%%", fear * 100.0f);
        } else {
            ImGui::Text("Fear: Calm");
        }
    }

    // Kill count (for predators)
    if (c->getType() == CreatureType::CARNIVORE || c->getType() == CreatureType::FLYING) {
        ImGui::Text("Kills: %d", c->getKillCount());
    }

    // Activity state
    const char* activity = getActivityStateString(c);
    ImGui::Text("Activity: %s", activity);

    // Hunting status
    if (c->isBeingHunted()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "WARNING: Being Hunted!");
    }

    // Migration status
    if (c->isMigrating()) {
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Migrating");
    }

    // Climate stress
    float stress = c->getClimateStress();
    if (stress > 0.2f) {
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.0f), "Climate Stress: %.0f%%", stress * 100.0f);
    }

    // Reproduction status
    if (c->canReproduce()) {
        ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.5f, 1.0f), "Ready to Reproduce");
    }
}

void CreatureInspectionPanel::renderEnvironmentSection(Creature* c) {
    const glm::vec3& pos = c->getPosition();

    // Position
    ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);

    // Velocity/Speed
    glm::vec3 vel = c->getVelocity();
    float speed = glm::length(vel);
    ImGui::Text("Speed: %.2f", speed);

    // Depth (for aquatic or when underwater)
    if (c->getType() == CreatureType::AQUATIC || pos.y < 0.0f) {
        ImGui::Text("Depth: %.1f m", -pos.y);
    }

    // Altitude (for flying)
    if (c->getType() == CreatureType::FLYING) {
        ImGui::Text("Altitude: %.1f m", pos.y);
    }

    // Biome (if biome system available)
    if (m_biomeSystem) {
        std::string biomeName = getBiomeNameAtCreature(c);
        ImGui::Text("Biome: %s", biomeName.c_str());
    }

    // Temperature comfort (based on optimal temp)
    float optimalTemp = c->getOptimalTemperature();
    ImGui::Text("Optimal Temp: %.0f%%", optimalTemp * 100.0f);
}

void CreatureInspectionPanel::renderGeneticsSection(Creature* c) {
    const Genome& g = c->getGenome();

    ImGui::Text("Core Genes:");
    ImGui::Indent();
    ImGui::Text("Size: %.3f", g.size);
    ImGui::Text("Speed: %.3f", g.speed);
    ImGui::Text("Vision Range: %.3f", g.visionRange);
    ImGui::Text("Efficiency: %.3f", g.efficiency);
    ImGui::Text("Aggression: %.3f", g.aggression);
    ImGui::Text("Social Tendency: %.3f", g.socialTendency);
    ImGui::Unindent();

    ImGui::Separator();

    ImGui::Text("Color Genes:");
    ImGui::Indent();
    ImGui::ColorEdit3("Base Color", (float*)&g.color, ImGuiColorEditFlags_NoInputs);
    ImGui::Unindent();

    // Mutation rate
    ImGui::Separator();
    ImGui::Text("Mutation Rate: %.1f%%", g.mutationRate * 100.0f);

    // Diploid genome info
    const auto& diploid = c->getDiploidGenome();
    ImGui::Separator();
    ImGui::Text("Diploid Genome:");
    ImGui::Indent();
    ImGui::Text("Total Genes: %zu", diploid.getGeneCount());
    ImGui::Unindent();
}

void CreatureInspectionPanel::renderBrainSection(Creature* c) {
    const auto& sensory = c->getSensory();

    ImGui::Text("Sensory System:");
    ImGui::Indent();
    ImGui::Text("Vision Range: %.1f", c->getVisionRange());
    ImGui::Text("Detection Radius: %.1f", sensory.detectionRadius);
    ImGui::Unindent();

    ImGui::Separator();

    // NEAT brain info
    if (c->hasNEATBrain()) {
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "NEAT Brain Active");

        const auto* brain = c->getNEATBrain();
        if (brain) {
            // Brain complexity info would go here
            ImGui::Text("Neural Network: Evolved");
        }
    } else {
        ImGui::Text("Brain: Simple Neural Network");
    }

    ImGui::Separator();

    // Current neural outputs/behavior
    ImGui::Text("Current Behavior:");
    ImGui::Indent();

    // Infer behavior from state
    if (c->isBeingHunted()) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "Fleeing");
    } else if (c->getEnergy() < c->getMaxEnergy() * 0.3f) {
        ImGui::Text("Seeking Food");
    } else if (c->canReproduce()) {
        ImGui::Text("Seeking Mate");
    } else if (c->isMigrating()) {
        ImGui::Text("Migrating");
    } else {
        ImGui::Text("Wandering");
    }
    ImGui::Unindent();
}

void CreatureInspectionPanel::renderColorPalette(Creature* c) {
    const Genome& g = c->getGenome();

    ImGui::Text("Color Palette:");
    ImGui::SameLine();

    // Main body color
    ImGui::ColorButton("##BodyColor", ImVec4(g.color.r, g.color.g, g.color.b, 1.0f),
                       ImGuiColorEditFlags_NoTooltip, ImVec2(24, 24));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Body Color");
    }

    ImGui::SameLine();

    // Species-tinted color
    glm::vec3 tinted = c->getSpeciesTintedColor();
    ImGui::ColorButton("##TintedColor", ImVec4(tinted.r, tinted.g, tinted.b, 1.0f),
                       ImGuiColorEditFlags_NoTooltip, ImVec2(24, 24));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Species Tint");
    }

    // Bioluminescence color if present
    if (c->hasBioluminescence()) {
        ImGui::SameLine();
        glm::vec3 biolum = c->getBiolumColor();
        ImGui::ColorButton("##BiolumColor", ImVec4(biolum.r, biolum.g, biolum.b, 1.0f),
                           ImGuiColorEditFlags_NoTooltip, ImVec2(24, 24));
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Bioluminescence");
        }
    }
}

// ============================================================================
// Helpers
// ============================================================================

const char* CreatureInspectionPanel::getCreatureTypeString(CreatureType type) const {
    switch (type) {
        case CreatureType::HERBIVORE: return "Herbivore";
        case CreatureType::CARNIVORE: return "Carnivore";
        case CreatureType::AQUATIC: return "Aquatic";
        case CreatureType::FLYING: return "Flying";
        default: return "Unknown";
    }
}

const char* CreatureInspectionPanel::getActivityStateString(Creature* c) const {
    if (!c->isAlive()) return "Dead";
    if (c->isBeingHunted()) return "Fleeing";
    if (c->isMigrating()) return "Migrating";

    float energy = c->getEnergy();
    float maxEnergy = c->getMaxEnergy();

    if (energy < maxEnergy * 0.2f) return "Starving";
    if (energy < maxEnergy * 0.4f) return "Hungry";
    if (c->canReproduce()) return "Seeking Mate";

    // Check velocity for movement state
    float speed = glm::length(c->getVelocity());
    if (speed < 0.5f) return "Resting";
    if (speed > 5.0f) return "Running";

    return "Wandering";
}

std::string CreatureInspectionPanel::getBiomeNameAtCreature(Creature* c) const {
    if (!m_biomeSystem) return "Unknown";

    const glm::vec3& pos = c->getPosition();
    BiomeType biome = m_biomeSystem->getBiomeAt(pos.x, pos.z);

    switch (biome) {
        case BiomeType::TROPICAL_FOREST: return "Tropical Forest";
        case BiomeType::TEMPERATE_FOREST: return "Temperate Forest";
        case BiomeType::GRASSLAND: return "Grassland";
        case BiomeType::SAVANNA: return "Savanna";
        case BiomeType::DESERT: return "Desert";
        case BiomeType::TUNDRA: return "Tundra";
        case BiomeType::VOLCANIC: return "Volcanic";
        case BiomeType::COASTAL: return "Coastal";
        case BiomeType::WETLAND: return "Wetland";
        case BiomeType::ALPINE: return "Alpine";
        case BiomeType::OCEAN: return "Ocean";
        case BiomeType::DEEP_OCEAN: return "Deep Ocean";
        case BiomeType::CORAL_REEF: return "Coral Reef";
        case BiomeType::KELP_FOREST: return "Kelp Forest";
        default: return "Unknown";
    }
}

ImVec4 CreatureInspectionPanel::getSpeciesSimilarityColor(Creature* c) const {
    // Generate a color based on species ID for visual distinction
    auto speciesId = c->getSpeciesId();

    if (speciesId == genetics::INVALID_SPECIES_ID) {
        return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);  // Gray for unknown
    }

    // Use species ID to generate a consistent hue
    float hue = std::fmod((float)speciesId * 0.618034f, 1.0f);  // Golden ratio for distribution

    // Convert HSV to RGB (saturation=0.7, value=0.9)
    float s = 0.7f;
    float v = 0.9f;

    float c_val = v * s;
    float x = c_val * (1.0f - std::fabs(std::fmod(hue * 6.0f, 2.0f) - 1.0f));
    float m = v - c_val;

    float r, g, b;
    if (hue < 1.0f/6.0f) { r = c_val; g = x; b = 0; }
    else if (hue < 2.0f/6.0f) { r = x; g = c_val; b = 0; }
    else if (hue < 3.0f/6.0f) { r = 0; g = c_val; b = x; }
    else if (hue < 4.0f/6.0f) { r = 0; g = x; b = c_val; }
    else if (hue < 5.0f/6.0f) { r = x; g = 0; b = c_val; }
    else { r = c_val; g = 0; b = x; }

    return ImVec4(r + m, g + m, b + m, 1.0f);
}

} // namespace ui
