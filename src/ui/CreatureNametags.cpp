#include "CreatureNametags.h"
#include "../entities/Creature.h"
#include "../entities/SpeciesNaming.h"
#include "../graphics/Camera.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace ui {

CreatureNametags::CreatureNametags() {
    m_nametags.reserve(1000);
}

CreatureNametags::~CreatureNametags() {
    shutdown();
}

bool CreatureNametags::initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
    m_device = device;
    m_initialized = true;
    return true;
}

void CreatureNametags::shutdown() {
    m_nametags.clear();
    m_device = nullptr;
    m_initialized = false;
}

CreatureStatus getCreatureStatus(const Creature* creature) {
    if (!creature) return CreatureStatus::None;

    CreatureStatus status = CreatureStatus::None;

    // Check energy/hunger
    float energy = creature->getEnergy();
    float maxEnergy = creature->getGenome().maxEnergy;
    if (energy < maxEnergy * 0.3f) {
        status = status | CreatureStatus::Hungry;
    }

    // Check if injured (low health relative to max)
    // Assuming energy serves as health in this simulation
    if (energy < maxEnergy * 0.2f) {
        status = status | CreatureStatus::Injured;
    }

    // Check behavioral state based on velocity and position
    glm::vec3 velocity = creature->getVelocity();
    float speed = glm::length(velocity);
    float maxSpeed = creature->getGenome().speed;

    // High speed could indicate fleeing or attacking
    if (speed > maxSpeed * 0.8f) {
        if (creature->getType() == CreatureType::CARNIVORE ||
            creature->getType() == CreatureType::PREDATOR) {
            status = status | CreatureStatus::Attacking;
        } else {
            status = status | CreatureStatus::Fleeing;
        }
    }

    // Very low speed could indicate eating or sleeping
    if (speed < maxSpeed * 0.1f && energy > maxEnergy * 0.5f) {
        status = status | CreatureStatus::Eating;
    }

    return status;
}

naming::CreatureIdentity getCreatureIdentity(const Creature* creature) {
    naming::CreatureIdentity identity;
    if (!creature) return identity;

    identity.creatureId = creature->getId();
    identity.generation = creature->getGeneration();

    // Get species name from naming system
    auto speciesId = creature->getSpeciesId();
    auto& namingSystem = naming::getNamingSystem();

    // Build traits from creature - enhanced with more morphological detail
    naming::CreatureTraits traits;
    traits.primaryColor = creature->getGenome().color;
    traits.size = creature->getGenome().size;
    traits.speed = creature->getGenome().speed;

    // Creature type detection
    CreatureType type = creature->getType();
    traits.isPredator = (type == CreatureType::CARNIVORE ||
                         type == CreatureType::PREDATOR ||
                         type == CreatureType::APEX_PREDATOR ||
                         type == CreatureType::SMALL_PREDATOR ||
                         type == CreatureType::AERIAL_PREDATOR ||
                         type == CreatureType::AQUATIC_PREDATOR ||
                         type == CreatureType::AQUATIC_APEX);
    traits.isCarnivore = traits.isPredator;
    traits.isHerbivore = ::isHerbivore(type) ||
                          type == CreatureType::AQUATIC_HERBIVORE;
    traits.isOmnivore = (type == CreatureType::OMNIVORE);
    traits.canFly = ::isFlying(type);
    traits.livesInWater = ::isAquatic(type);
    traits.hasWings = traits.canFly;
    traits.hasFins = traits.livesInWater;

    // Get or create species name
    const naming::SpeciesName& speciesName = namingSystem.getOrCreateSpeciesName(speciesId, traits);
    identity.speciesName = &speciesName;

    // Generate individual name
    identity.individualName = namingSystem.generateIndividualName(
        speciesId,
        creature->getGeneration(),
        -1,  // Parent ID not tracked yet
        ""   // Parent name not tracked yet
    );

    return identity;
}

void CreatureNametags::updateNametags(const std::vector<Creature*>& creatures,
                                       const Camera& camera,
                                       int selectedCreatureId) {
    m_nametags.clear();
    m_nametags.reserve(creatures.size());

    glm::vec3 cameraPos = camera.getPosition();

    for (const Creature* creature : creatures) {
        if (!creature || !creature->isAlive()) continue;

        glm::vec3 creaturePos = creature->getPosition();
        float distance = glm::length(creaturePos - cameraPos);

        // Skip if too far
        if (distance > m_config.maxVisibleDistance) continue;

        // Skip if too close
        if (distance < m_config.minVisibleDistance) continue;

        NametagInstance nametag;
        nametag.creatureId = creature->getId();
        nametag.worldPosition = creaturePos + glm::vec3(0, m_config.verticalOffset * creature->getGenome().size, 0);
        nametag.distanceToCamera = distance;
        nametag.alpha = calculateAlpha(distance);
        nametag.isSelected = (creature->getId() == selectedCreatureId);

        // Get creature identity/names
        naming::CreatureIdentity identity = getCreatureIdentity(creature);
        nametag.individualName = identity.individualName.getDisplayName();
        if (identity.speciesName) {
            nametag.speciesName = identity.speciesName->commonName;
            nametag.scientificName = identity.speciesName->getAbbreviatedScientific();
            nametag.descriptor = identity.speciesName->getDescriptor();
        }

        // Get status
        nametag.status = getCreatureStatus(creature);

        // Get health/energy (using energy as health in this sim)
        float maxEnergy = creature->getGenome().maxEnergy;
        nametag.health = creature->getEnergy() / maxEnergy;
        nametag.energy = creature->getEnergy() / maxEnergy;

        nametag.isVisible = true;
        m_nametags.push_back(nametag);
    }

    // Sort by distance (farther first so closer ones render on top)
    std::sort(m_nametags.begin(), m_nametags.end(),
              [](const NametagInstance& a, const NametagInstance& b) {
                  return a.distanceToCamera > b.distanceToCamera;
              });
}

void CreatureNametags::renderImGui(const Camera& camera, const glm::mat4& viewProjection) {
    if (!m_config.showNametags) return;

    // Get screen size from ImGui
    ImGuiIO& io = ImGui::GetIO();
    m_screenSize = glm::vec2(io.DisplaySize.x, io.DisplaySize.y);

    // Calculate screen positions
    for (auto& nametag : m_nametags) {
        nametag.screenPosition = worldToScreen(nametag.worldPosition, viewProjection, m_screenSize);

        // Check if on screen
        nametag.isVisible = (nametag.screenPosition.x >= -50.0f &&
                             nametag.screenPosition.x <= m_screenSize.x + 50.0f &&
                             nametag.screenPosition.y >= -50.0f &&
                             nametag.screenPosition.y <= m_screenSize.y + 50.0f);
    }

    // Render each visible nametag
    for (const auto& nametag : m_nametags) {
        if (nametag.isVisible) {
            renderNametag(nametag);
        }
    }
}

void CreatureNametags::renderNametag(const NametagInstance& nametag) {
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    if (!drawList) return;

    glm::vec2 pos = nametag.screenPosition;
    float alpha = nametag.alpha;

    // Scale based on distance (closer = larger)
    float distanceScale = 1.0f - (nametag.distanceToCamera / m_config.maxVisibleDistance) * 0.5f;
    float scale = m_config.baseScale * distanceScale;

    // Highlight selected creatures
    if (nametag.isSelected && m_config.highlightSelected) {
        float glowRadius = 30.0f * scale;
        ImU32 glowColor = ImGui::ColorConvertFloat4ToU32(
            ImVec4(m_config.selectedHighlight.r,
                   m_config.selectedHighlight.g,
                   m_config.selectedHighlight.b,
                   m_config.selectedGlowIntensity * alpha));

        drawList->AddCircleFilled(ImVec2(pos.x, pos.y), glowRadius, glowColor, 32);
    }

    float yOffset = 0.0f;

    // =============================================================================
    // NEW LAYOUT: Species name first (primary identity)
    // =============================================================================
    if (m_config.showSpeciesName && !nametag.speciesName.empty()) {
        // Species name in bright color (primary focus)
        ImU32 speciesColor = ImGui::ColorConvertFloat4ToU32(
            ImVec4(1.0f, 1.0f, 0.9f, 1.0f * alpha));

        std::string displayName = m_config.showScientificName && !nametag.scientificName.empty()
            ? nametag.scientificName
            : nametag.speciesName;

        ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
        float textX = pos.x - textSize.x * 0.5f;

        // Draw shadow for readability
        ImU32 shadowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.8f * alpha));
        drawList->AddText(ImVec2(textX + 1, pos.y + yOffset + 1), shadowColor, displayName.c_str());
        drawList->AddText(ImVec2(textX, pos.y + yOffset), speciesColor, displayName.c_str());

        // Render similarity color chip if available
        if (m_config.showSimilarityColor && nametag.hasSimilarityColor) {
            float chipSize = 6.0f * scale;
            float chipX = textX + textSize.x + 4.0f;
            float chipY = pos.y + yOffset + (textSize.y - chipSize) * 0.5f;

            ImU32 chipColor = ImGui::ColorConvertFloat4ToU32(
                ImVec4(nametag.similarityColor.r, nametag.similarityColor.g,
                       nametag.similarityColor.b, alpha));
            ImU32 chipBorder = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.8f * alpha));

            drawList->AddRectFilled(ImVec2(chipX, chipY), ImVec2(chipX + chipSize, chipY + chipSize), chipColor, 2.0f);
            drawList->AddRect(ImVec2(chipX, chipY), ImVec2(chipX + chipSize, chipY + chipSize), chipBorder, 2.0f);
        }

        yOffset += textSize.y + 2;
    }

    // =============================================================================
    // Descriptor line (diet + locomotion) - smaller, lighter
    // =============================================================================
    if (m_config.showDescriptor && !nametag.descriptor.empty()) {
        // Descriptor in lighter, smaller style
        ImU32 descriptorColor = ImGui::ColorConvertFloat4ToU32(
            ImVec4(0.7f, 0.7f, 0.6f, 0.75f * alpha));

        ImVec2 textSize = ImGui::CalcTextSize(nametag.descriptor.c_str());
        float textX = pos.x - textSize.x * 0.5f;

        // Lighter shadow
        ImU32 shadowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.5f * alpha));
        drawList->AddText(ImVec2(textX + 1, pos.y + yOffset + 1), shadowColor, nametag.descriptor.c_str());
        drawList->AddText(ImVec2(textX, pos.y + yOffset), descriptorColor, nametag.descriptor.c_str());

        yOffset += textSize.y + 2;
    }

    // =============================================================================
    // Individual name (optional, below descriptor)
    // =============================================================================
    if (m_config.showIndividualName && !nametag.individualName.empty()) {
        ImU32 textColor = ImGui::ColorConvertFloat4ToU32(
            ImVec4(m_config.textColor.r,
                   m_config.textColor.g,
                   m_config.textColor.b,
                   m_config.textColor.a * 0.7f * alpha));

        // Center text
        ImVec2 textSize = ImGui::CalcTextSize(nametag.individualName.c_str());
        float textX = pos.x - textSize.x * 0.5f;

        // Draw shadow for readability
        ImU32 shadowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.5f * alpha));
        drawList->AddText(ImVec2(textX + 1, pos.y + yOffset + 1), shadowColor,
                          nametag.individualName.c_str());
        drawList->AddText(ImVec2(textX, pos.y + yOffset), textColor,
                          nametag.individualName.c_str());

        yOffset += textSize.y + 2;
    }

    // Render health bar
    if (m_config.showHealthBars) {
        float barWidth = m_config.healthBarWidth * scale;
        float barHeight = m_config.healthBarHeight * scale;
        glm::vec2 barPos(pos.x - barWidth * 0.5f, pos.y + yOffset);

        renderHealthBar(barPos, barWidth, barHeight,
                        nametag.health,
                        getHealthBarColor(nametag.health),
                        m_config.healthBarBgColor * glm::vec4(1, 1, 1, alpha));

        yOffset += barHeight + 1;
    }

    // Render energy bar
    if (m_config.showEnergyBars) {
        float barWidth = m_config.healthBarWidth * scale;
        float barHeight = m_config.energyBarHeight * scale;
        glm::vec2 barPos(pos.x - barWidth * 0.5f, pos.y + yOffset);

        glm::vec4 energyColor = m_config.energyBarFgColor;
        energyColor.a *= alpha;

        renderHealthBar(barPos, barWidth, barHeight,
                        nametag.energy,
                        energyColor,
                        m_config.energyBarBgColor * glm::vec4(1, 1, 1, alpha));

        yOffset += barHeight + 2;
    }

    // Render status icons
    if (m_config.showStatusIcons && nametag.status != CreatureStatus::None) {
        renderStatusIcons(glm::vec2(pos.x, pos.y + yOffset), nametag.status);
    }
}

void CreatureNametags::renderHealthBar(const glm::vec2& pos, float width, float height,
                                        float value, const glm::vec4& fgColor,
                                        const glm::vec4& bgColor) {
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    if (!drawList) return;

    value = std::clamp(value, 0.0f, 1.0f);

    // Background
    ImU32 bgCol = ImGui::ColorConvertFloat4ToU32(ImVec4(bgColor.r, bgColor.g, bgColor.b, bgColor.a));
    drawList->AddRectFilled(
        ImVec2(pos.x, pos.y),
        ImVec2(pos.x + width, pos.y + height),
        bgCol,
        2.0f  // Rounding
    );

    // Foreground (filled portion)
    if (value > 0.0f) {
        ImU32 fgCol = ImGui::ColorConvertFloat4ToU32(ImVec4(fgColor.r, fgColor.g, fgColor.b, fgColor.a));
        drawList->AddRectFilled(
            ImVec2(pos.x + 1, pos.y + 1),
            ImVec2(pos.x + 1 + (width - 2) * value, pos.y + height - 1),
            fgCol,
            1.0f  // Rounding
        );
    }

    // Border
    ImU32 borderCol = ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.3f, bgColor.a));
    drawList->AddRect(
        ImVec2(pos.x, pos.y),
        ImVec2(pos.x + width, pos.y + height),
        borderCol,
        2.0f  // Rounding
    );
}

void CreatureNametags::renderStatusIcons(const glm::vec2& pos, CreatureStatus status) {
    std::string icons = getStatusIconString(status);
    if (icons.empty()) return;

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    if (!drawList) return;

    ImVec2 textSize = ImGui::CalcTextSize(icons.c_str());
    float textX = pos.x - textSize.x * 0.5f;

    ImU32 iconColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.9f));
    drawList->AddText(ImVec2(textX, pos.y), iconColor, icons.c_str());
}

glm::vec2 CreatureNametags::worldToScreen(const glm::vec3& worldPos,
                                           const glm::mat4& viewProjection,
                                           const glm::vec2& screenSize) const {
    // Transform to clip space
    glm::vec4 clipPos = viewProjection * glm::vec4(worldPos, 1.0f);

    // Check if behind camera
    if (clipPos.w <= 0.0f) {
        return glm::vec2(-1000.0f, -1000.0f);  // Off screen
    }

    // Perspective divide to NDC
    glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

    // NDC to screen coordinates
    // NDC is [-1, 1], screen is [0, width/height]
    glm::vec2 screenPos;
    screenPos.x = (ndc.x * 0.5f + 0.5f) * screenSize.x;
    screenPos.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * screenSize.y;  // Flip Y

    return screenPos;
}

float CreatureNametags::calculateAlpha(float distance) const {
    if (distance <= m_config.fadeStartDistance) {
        return 1.0f;
    }

    float fadeRange = m_config.maxVisibleDistance - m_config.fadeStartDistance;
    if (fadeRange <= 0.0f) return 1.0f;

    float t = (distance - m_config.fadeStartDistance) / fadeRange;
    return std::max(0.0f, 1.0f - t);
}

glm::vec4 CreatureNametags::getHealthBarColor(float health) const {
    // Interpolate from red (low) to green (high)
    glm::vec4 lowColor = m_config.healthBarLowColor;
    glm::vec4 highColor = m_config.healthBarFgColor;

    // Use smoothstep for nicer transition
    float t = health * health * (3.0f - 2.0f * health);
    return glm::mix(lowColor, highColor, t);
}

std::string CreatureNametags::getStatusIconString(CreatureStatus status) const {
    std::string icons;

    // Use ASCII symbols that render well in most fonts
    if (hasStatus(status, CreatureStatus::Hungry)) icons += "!";  // Hungry
    if (hasStatus(status, CreatureStatus::Scared)) icons += "?";  // Scared
    if (hasStatus(status, CreatureStatus::Mating)) icons += "*";  // Mating
    if (hasStatus(status, CreatureStatus::Attacking)) icons += "X"; // Attacking
    if (hasStatus(status, CreatureStatus::Fleeing)) icons += "<";  // Fleeing
    if (hasStatus(status, CreatureStatus::Sleeping)) icons += "Z"; // Sleeping
    if (hasStatus(status, CreatureStatus::Eating)) icons += "o";   // Eating
    if (hasStatus(status, CreatureStatus::Injured)) icons += "+";  // Injured
    if (hasStatus(status, CreatureStatus::Pregnant)) icons += "@"; // Pregnant
    if (hasStatus(status, CreatureStatus::Leader)) icons += "^";   // Leader

    return icons;
}

int CreatureNametags::getCreatureAtScreenPos(const glm::vec2& screenPos, float tolerance) const {
    // Find closest nametag to click position
    float minDist = tolerance;
    int closestId = -1;

    for (const auto& nametag : m_nametags) {
        if (!nametag.isVisible) continue;

        float dist = glm::length(nametag.screenPosition - screenPos);
        if (dist < minDist) {
            minDist = dist;
            closestId = nametag.creatureId;
        }
    }

    return closestId;
}

size_t CreatureNametags::getVisibleNametagCount() const {
    return std::count_if(m_nametags.begin(), m_nametags.end(),
                         [](const NametagInstance& n) { return n.isVisible; });
}

void CreatureNametags::renderSettingsPanel() {
    if (ImGui::CollapsingHeader("Nametag Settings")) {
        ImGui::Checkbox("Show Nametags", &m_config.showNametags);

        if (m_config.showNametags) {
            ImGui::Indent();

            // Name display options
            ImGui::Text("Name Display:");
            ImGui::Checkbox("Species Names", &m_config.showSpeciesName);
            if (m_config.showSpeciesName) {
                ImGui::Indent();
                ImGui::Checkbox("Use Scientific Names", &m_config.showScientificName);
                ImGui::Checkbox("Show Trait Descriptor", &m_config.showDescriptor);
                ImGui::Checkbox("Show Similarity Color", &m_config.showSimilarityColor);
                ImGui::Unindent();
            }
            ImGui::Checkbox("Individual Names", &m_config.showIndividualName);

            ImGui::Separator();

            // Stats display
            ImGui::Text("Stats Display:");
            ImGui::Checkbox("Health Bars", &m_config.showHealthBars);
            ImGui::Checkbox("Energy Bars", &m_config.showEnergyBars);
            ImGui::Checkbox("Status Icons", &m_config.showStatusIcons);
            ImGui::Checkbox("Highlight Selected", &m_config.highlightSelected);

            ImGui::Separator();
            ImGui::Text("Visibility Range");
            ImGui::SliderFloat("Max Distance", &m_config.maxVisibleDistance, 10.0f, 200.0f);
            ImGui::SliderFloat("Fade Start", &m_config.fadeStartDistance, 5.0f, m_config.maxVisibleDistance);

            ImGui::Separator();
            ImGui::Text("Display");
            ImGui::SliderFloat("Base Scale", &m_config.baseScale, 0.5f, 2.0f);
            ImGui::SliderFloat("Vertical Offset", &m_config.verticalOffset, 1.0f, 5.0f);

            ImGui::Separator();
            ImGui::Text("Visible: %zu / %zu", getVisibleNametagCount(), getTotalNametagCount());

            ImGui::Unindent();
        }
    }
}

} // namespace ui
