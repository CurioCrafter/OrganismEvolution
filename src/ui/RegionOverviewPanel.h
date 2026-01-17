#pragma once

// RegionOverviewPanel - Per-island/region population and biome overview
// Phase 8 Agent 8: UI Integration Owner
//
// Features:
// - Per-island population counts by creature type
// - Biome distribution per island
// - Island health indicators
// - Quick selection to jump camera to island

#include "imgui.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <functional>

// Forward declarations
class Creature;
class Terrain;
class BiomeSystem;

namespace Forge {
class CreatureManager;
class CameraController;
}

namespace ui {

// ============================================================================
// Island/Region Statistics
// ============================================================================

struct RegionStats {
    int regionIndex = 0;
    std::string name;
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 100.0f;

    // Population counts
    int totalCreatures = 0;
    int herbivoreCount = 0;
    int carnivoreCount = 0;
    int aquaticCount = 0;
    int flyingCount = 0;

    // Additional stats
    int totalSpecies = 0;
    int maxGeneration = 0;
    float avgEnergy = 0.0f;
    float avgFitness = 0.0f;

    // Biome coverage (percentages, sum to 1.0)
    float forestCoverage = 0.0f;
    float grasslandCoverage = 0.0f;
    float desertCoverage = 0.0f;
    float tundraCoverage = 0.0f;
    float wetlandCoverage = 0.0f;
    float mountainCoverage = 0.0f;
    float volcanicCoverage = 0.0f;
    float coastalCoverage = 0.0f;
    float oceanCoverage = 0.0f;

    // Health indicator (0-100)
    float ecosystemHealth = 100.0f;

    // Flags
    bool isSelected = false;
    bool hasExtinctionRisk = false;
    bool hasOverpopulation = false;
};

// ============================================================================
// RegionOverviewPanel Class
// ============================================================================

class RegionOverviewPanel {
public:
    RegionOverviewPanel();
    ~RegionOverviewPanel() = default;

    // ========================================================================
    // Main Interface
    // ========================================================================

    // Render the panel
    void render();

    // Update statistics (call periodically, not every frame)
    void updateStats(Forge::CreatureManager& creatures, const BiomeSystem* biomes = nullptr);

    // ========================================================================
    // Configuration
    // ========================================================================

    // Set region definitions (from archipelago generator)
    void setRegions(const std::vector<RegionStats>& regions);

    // Add/remove a single region
    void addRegion(const RegionStats& region);
    void removeRegion(int index);
    void clearRegions();

    // Get current regions
    const std::vector<RegionStats>& getRegions() const { return m_regions; }

    // ========================================================================
    // Integration
    // ========================================================================

    // Set camera controller for "Jump to Island" functionality
    void setCameraController(Forge::CameraController* controller) { m_cameraController = controller; }

    // Callback when user selects a region
    using RegionSelectedCallback = std::function<void(int regionIndex, const RegionStats& stats)>;
    void setOnRegionSelected(RegionSelectedCallback cb) { m_onRegionSelected = cb; }

    // ========================================================================
    // Visibility
    // ========================================================================

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    void toggleVisible() { m_visible = !m_visible; }

    // Compact mode shows minimal info
    bool isCompact() const { return m_compact; }
    void setCompact(bool compact) { m_compact = compact; }

private:
    // Region data
    std::vector<RegionStats> m_regions;
    int m_selectedRegion = -1;

    // State
    bool m_visible = true;
    bool m_compact = false;
    float m_updateInterval = 1.0f;  // Seconds between stat updates
    float m_timeSinceUpdate = 0.0f;

    // Integration
    Forge::CameraController* m_cameraController = nullptr;
    RegionSelectedCallback m_onRegionSelected;

    // ========================================================================
    // Rendering
    // ========================================================================

    void renderRegionCard(RegionStats& region, int index);
    void renderPopulationBar(const RegionStats& region);
    void renderBiomeChart(const RegionStats& region);
    void renderHealthIndicator(const RegionStats& region);
    void renderCompactView();
    void renderExpandedView();

    // ========================================================================
    // Helpers
    // ========================================================================

    void updateRegionStats(RegionStats& region, Forge::CreatureManager& creatures);
    void updateBiomeCoverage(RegionStats& region, const BiomeSystem* biomes);
    void calculateEcosystemHealth(RegionStats& region);
    bool isCreatureInRegion(const Creature& creature, const RegionStats& region) const;
    ImVec4 getHealthColor(float health) const;
    const char* getBiomeName(int biomeIndex) const;
    ImVec4 getBiomeColor(int biomeIndex) const;
};

// ============================================================================
// Implementation
// ============================================================================

inline RegionOverviewPanel::RegionOverviewPanel() {
    // Initialize with a single default region if none specified
    RegionStats defaultRegion;
    defaultRegion.regionIndex = 0;
    defaultRegion.name = "Main Island";
    defaultRegion.center = glm::vec3(0.0f);
    defaultRegion.radius = 250.0f;
    m_regions.push_back(defaultRegion);
}

inline void RegionOverviewPanel::setRegions(const std::vector<RegionStats>& regions) {
    m_regions = regions;
    m_selectedRegion = -1;
}

inline void RegionOverviewPanel::addRegion(const RegionStats& region) {
    m_regions.push_back(region);
}

inline void RegionOverviewPanel::removeRegion(int index) {
    if (index >= 0 && index < static_cast<int>(m_regions.size())) {
        m_regions.erase(m_regions.begin() + index);
        if (m_selectedRegion >= static_cast<int>(m_regions.size())) {
            m_selectedRegion = -1;
        }
    }
}

inline void RegionOverviewPanel::clearRegions() {
    m_regions.clear();
    m_selectedRegion = -1;
}

inline void RegionOverviewPanel::render() {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Region Overview", &m_visible, flags)) {
        // Header
        ImGui::Text("Islands/Regions: %d", static_cast<int>(m_regions.size()));

        ImGui::SameLine(ImGui::GetWindowWidth() - 80);
        if (ImGui::SmallButton(m_compact ? "Expand" : "Compact")) {
            m_compact = !m_compact;
        }

        ImGui::Separator();

        if (m_regions.empty()) {
            ImGui::TextWrapped("No regions defined. Use the archipelago generator or add regions manually.");
        }
        else if (m_compact) {
            renderCompactView();
        }
        else {
            renderExpandedView();
        }
    }
    ImGui::End();
}

inline void RegionOverviewPanel::renderCompactView() {
    // Simple list with basic stats
    for (int i = 0; i < static_cast<int>(m_regions.size()); ++i) {
        auto& region = m_regions[i];

        ImGui::PushID(i);

        // Selectable row
        bool selected = (i == m_selectedRegion);
        if (ImGui::Selectable("##RegionRow", selected, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, 24))) {
            m_selectedRegion = i;
            region.isSelected = true;

            if (m_onRegionSelected) {
                m_onRegionSelected(i, region);
            }
        }

        ImGui::SameLine();

        // Health indicator dot
        ImGui::ColorButton("##Health", getHealthColor(region.ecosystemHealth),
                          ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(12, 12));

        ImGui::SameLine();

        // Name and population
        ImGui::Text("%s", region.name.c_str());

        ImGui::SameLine(180);
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "%d", region.totalCreatures);

        ImGui::SameLine(220);
        ImGui::TextDisabled("(%d species)", region.totalSpecies);

        ImGui::PopID();
    }
}

inline void RegionOverviewPanel::renderExpandedView() {
    // Full cards for each region
    ImGui::BeginChild("RegionList", ImVec2(0, 0), false);

    for (int i = 0; i < static_cast<int>(m_regions.size()); ++i) {
        renderRegionCard(m_regions[i], i);

        if (i < static_cast<int>(m_regions.size()) - 1) {
            ImGui::Separator();
            ImGui::Spacing();
        }
    }

    ImGui::EndChild();
}

inline void RegionOverviewPanel::renderRegionCard(RegionStats& region, int index) {
    ImGui::PushID(index);

    // Region header
    bool selected = (index == m_selectedRegion);
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed;
    if (selected) {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    // Health color for header
    ImVec4 healthColor = getHealthColor(region.ecosystemHealth);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(healthColor.x * 0.3f, healthColor.y * 0.3f, healthColor.z * 0.3f, 0.6f));

    bool open = ImGui::CollapsingHeader(region.name.c_str(), nodeFlags);

    ImGui::PopStyleColor();

    if (open) {
        ImGui::Indent();

        // Health indicator
        renderHealthIndicator(region);

        ImGui::Spacing();

        // Population breakdown
        ImGui::Text("Population: %d creatures (%d species)", region.totalCreatures, region.totalSpecies);
        renderPopulationBar(region);

        ImGui::Spacing();

        // Quick stats
        ImGui::Text("Max Generation: %d", region.maxGeneration);
        ImGui::Text("Avg Energy: %.1f", region.avgEnergy);
        ImGui::Text("Avg Fitness: %.2f", region.avgFitness);

        // Warnings
        if (region.hasExtinctionRisk) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "WARNING: Extinction Risk!");
        }
        if (region.hasOverpopulation) {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Warning: Overpopulation");
        }

        ImGui::Spacing();

        // Biome distribution
        if (ImGui::TreeNode("Biome Distribution")) {
            renderBiomeChart(region);
            ImGui::TreePop();
        }

        ImGui::Spacing();

        // Action buttons
        if (m_cameraController && ImGui::Button("Jump to Island")) {
            // Would call camera controller to focus on region center
            m_selectedRegion = index;
            region.isSelected = true;
        }

        ImGui::Unindent();
    }

    ImGui::PopID();
}

inline void RegionOverviewPanel::renderPopulationBar(const RegionStats& region) {
    // Stacked horizontal bar showing creature type breakdown
    float total = static_cast<float>(region.totalCreatures);
    if (total == 0) total = 1;  // Avoid division by zero

    float herbivoreRatio = region.herbivoreCount / total;
    float carnivoreRatio = region.carnivoreCount / total;
    float aquaticRatio = region.aquaticCount / total;
    float flyingRatio = region.flyingCount / total;

    float barWidth = ImGui::GetContentRegionAvail().x;
    float barHeight = 16.0f;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    // Background
    drawList->AddRectFilled(pos, ImVec2(pos.x + barWidth, pos.y + barHeight), IM_COL32(30, 30, 30, 255));

    // Segments
    float x = pos.x;

    // Herbivores (green)
    if (herbivoreRatio > 0) {
        float w = barWidth * herbivoreRatio;
        drawList->AddRectFilled(ImVec2(x, pos.y), ImVec2(x + w, pos.y + barHeight), IM_COL32(80, 180, 80, 255));
        x += w;
    }

    // Carnivores (red)
    if (carnivoreRatio > 0) {
        float w = barWidth * carnivoreRatio;
        drawList->AddRectFilled(ImVec2(x, pos.y), ImVec2(x + w, pos.y + barHeight), IM_COL32(200, 80, 80, 255));
        x += w;
    }

    // Aquatic (blue)
    if (aquaticRatio > 0) {
        float w = barWidth * aquaticRatio;
        drawList->AddRectFilled(ImVec2(x, pos.y), ImVec2(x + w, pos.y + barHeight), IM_COL32(80, 130, 200, 255));
        x += w;
    }

    // Flying (yellow)
    if (flyingRatio > 0) {
        float w = barWidth * flyingRatio;
        drawList->AddRectFilled(ImVec2(x, pos.y), ImVec2(x + w, pos.y + barHeight), IM_COL32(220, 200, 80, 255));
    }

    // Border
    drawList->AddRect(pos, ImVec2(pos.x + barWidth, pos.y + barHeight), IM_COL32(60, 60, 60, 255));

    // Advance cursor
    ImGui::Dummy(ImVec2(barWidth, barHeight + 2));

    // Legend
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "H:%d", region.herbivoreCount);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 1.0f), "C:%d", region.carnivoreCount);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.4f, 0.6f, 0.9f, 1.0f), "A:%d", region.aquaticCount);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.4f, 1.0f), "F:%d", region.flyingCount);
}

inline void RegionOverviewPanel::renderBiomeChart(const RegionStats& region) {
    // Simple text-based biome breakdown
    struct BiomeEntry {
        const char* name;
        float coverage;
        ImVec4 color;
    };

    BiomeEntry biomes[] = {
        {"Forest", region.forestCoverage, ImVec4(0.2f, 0.6f, 0.2f, 1.0f)},
        {"Grassland", region.grasslandCoverage, ImVec4(0.6f, 0.7f, 0.3f, 1.0f)},
        {"Desert", region.desertCoverage, ImVec4(0.9f, 0.8f, 0.5f, 1.0f)},
        {"Tundra", region.tundraCoverage, ImVec4(0.8f, 0.9f, 1.0f, 1.0f)},
        {"Wetland", region.wetlandCoverage, ImVec4(0.3f, 0.5f, 0.4f, 1.0f)},
        {"Mountain", region.mountainCoverage, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)},
        {"Volcanic", region.volcanicCoverage, ImVec4(0.8f, 0.3f, 0.1f, 1.0f)},
        {"Coastal", region.coastalCoverage, ImVec4(0.7f, 0.7f, 0.5f, 1.0f)},
        {"Ocean", region.oceanCoverage, ImVec4(0.2f, 0.4f, 0.7f, 1.0f)}
    };

    for (const auto& biome : biomes) {
        if (biome.coverage > 0.01f) {  // Only show if > 1%
            // Color indicator
            ImGui::ColorButton("##BiomeColor", biome.color,
                              ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(12, 12));
            ImGui::SameLine();

            // Progress bar
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, biome.color);
            char label[32];
            snprintf(label, sizeof(label), "%.0f%%", biome.coverage * 100.0f);
            ImGui::ProgressBar(biome.coverage, ImVec2(100, 14), label);
            ImGui::PopStyleColor();

            ImGui::SameLine();
            ImGui::Text("%s", biome.name);
        }
    }
}

inline void RegionOverviewPanel::renderHealthIndicator(const RegionStats& region) {
    ImVec4 color = getHealthColor(region.ecosystemHealth);

    ImGui::Text("Ecosystem Health:");
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
    char label[32];
    snprintf(label, sizeof(label), "%.0f%%", region.ecosystemHealth);
    ImGui::ProgressBar(region.ecosystemHealth / 100.0f, ImVec2(150, 18), label);
    ImGui::PopStyleColor();
}

inline void RegionOverviewPanel::updateStats(Forge::CreatureManager& creatures, const BiomeSystem* biomes) {
    for (auto& region : m_regions) {
        updateRegionStats(region, creatures);

        if (biomes) {
            updateBiomeCoverage(region, biomes);
        }

        calculateEcosystemHealth(region);
    }
}

inline void RegionOverviewPanel::updateRegionStats(RegionStats& region, Forge::CreatureManager& creatures) {
    // Reset counts
    region.totalCreatures = 0;
    region.herbivoreCount = 0;
    region.carnivoreCount = 0;
    region.aquaticCount = 0;
    region.flyingCount = 0;
    region.maxGeneration = 0;

    float totalEnergy = 0.0f;
    float totalFitness = 0.0f;

    std::set<uint32_t> speciesIds;

    creatures.forEach([&](Creature& creature, size_t) {
        if (!creature.isAlive()) return;

        if (isCreatureInRegion(creature, region)) {
            region.totalCreatures++;

            // Count by type
            switch (creature.getType()) {
                case CreatureType::HERBIVORE:
                    region.herbivoreCount++;
                    break;
                case CreatureType::CARNIVORE:
                    region.carnivoreCount++;
                    break;
                case CreatureType::AQUATIC:
                    region.aquaticCount++;
                    break;
                case CreatureType::FLYING:
                    region.flyingCount++;
                    break;
            }

            // Track generation
            if (creature.getGeneration() > region.maxGeneration) {
                region.maxGeneration = creature.getGeneration();
            }

            // Accumulate stats
            totalEnergy += creature.getEnergy();
            totalFitness += creature.getFitness();

            // Track species
            auto sid = creature.getSpeciesId();
            if (sid != genetics::INVALID_SPECIES_ID) {
                speciesIds.insert(sid);
            }
        }
    });

    // Calculate averages
    if (region.totalCreatures > 0) {
        region.avgEnergy = totalEnergy / region.totalCreatures;
        region.avgFitness = totalFitness / region.totalCreatures;
    } else {
        region.avgEnergy = 0.0f;
        region.avgFitness = 0.0f;
    }

    region.totalSpecies = static_cast<int>(speciesIds.size());
}

inline void RegionOverviewPanel::updateBiomeCoverage(RegionStats& region, const BiomeSystem* biomes) {
    // Sample biomes in a grid within the region
    // This is simplified - a real implementation would do proper area sampling

    if (!biomes) return;

    int forestCount = 0, grassCount = 0, desertCount = 0, tundraCount = 0;
    int wetlandCount = 0, mountainCount = 0, volcanicCount = 0, coastalCount = 0, oceanCount = 0;
    int totalSamples = 0;

    const int sampleCount = 10;  // Samples per axis
    float step = region.radius * 2.0f / sampleCount;

    for (int x = 0; x < sampleCount; ++x) {
        for (int z = 0; z < sampleCount; ++z) {
            float px = region.center.x - region.radius + x * step;
            float pz = region.center.z - region.radius + z * step;

            // Check if sample point is within circular region
            float dx = px - region.center.x;
            float dz = pz - region.center.z;
            if (dx * dx + dz * dz > region.radius * region.radius) continue;

            totalSamples++;

            BiomeType biome = biomes->getBiomeAt(px, pz);

            switch (biome) {
                case BiomeType::TROPICAL_FOREST:
                case BiomeType::TEMPERATE_FOREST:
                    forestCount++;
                    break;
                case BiomeType::GRASSLAND:
                case BiomeType::SAVANNA:
                    grassCount++;
                    break;
                case BiomeType::DESERT:
                    desertCount++;
                    break;
                case BiomeType::TUNDRA:
                case BiomeType::ALPINE:
                    tundraCount++;
                    break;
                case BiomeType::WETLAND:
                    wetlandCount++;
                    break;
                case BiomeType::VOLCANIC:
                    volcanicCount++;
                    break;
                case BiomeType::COASTAL:
                    coastalCount++;
                    break;
                case BiomeType::OCEAN:
                case BiomeType::DEEP_OCEAN:
                case BiomeType::CORAL_REEF:
                case BiomeType::KELP_FOREST:
                    oceanCount++;
                    break;
                default:
                    grassCount++;  // Default to grassland
                    break;
            }
        }
    }

    if (totalSamples > 0) {
        float inv = 1.0f / totalSamples;
        region.forestCoverage = forestCount * inv;
        region.grasslandCoverage = grassCount * inv;
        region.desertCoverage = desertCount * inv;
        region.tundraCoverage = tundraCount * inv;
        region.wetlandCoverage = wetlandCount * inv;
        region.volcanicCoverage = volcanicCount * inv;
        region.coastalCoverage = coastalCount * inv;
        region.oceanCoverage = oceanCount * inv;
    }
}

inline void RegionOverviewPanel::calculateEcosystemHealth(RegionStats& region) {
    // Simple health calculation based on:
    // - Population diversity (herbivore/carnivore balance)
    // - Total population vs area
    // - Species diversity

    float health = 100.0f;

    // Check for extinction risk (very low population)
    if (region.totalCreatures < 5) {
        region.hasExtinctionRisk = true;
        health -= 40.0f;
    } else {
        region.hasExtinctionRisk = false;
    }

    // Check population balance
    if (region.herbivoreCount > 0 && region.carnivoreCount > 0) {
        float ratio = static_cast<float>(region.carnivoreCount) / region.herbivoreCount;
        // Ideal ratio is around 0.2-0.3 (5:1 to 3:1 herbivore:carnivore)
        if (ratio > 0.5f) {
            health -= 20.0f;  // Too many predators
        } else if (ratio < 0.05f) {
            health -= 10.0f;  // Too few predators
        }
    }

    // Check for overpopulation (rough estimate based on region size)
    float areaKm2 = 3.14159f * region.radius * region.radius / 1000000.0f;
    float density = region.totalCreatures / std::max(areaKm2, 0.01f);
    if (density > 100.0f) {  // More than 100 creatures per km²
        region.hasOverpopulation = true;
        health -= 15.0f;
    } else {
        region.hasOverpopulation = false;
    }

    // Species diversity bonus
    if (region.totalSpecies > 10) {
        health += 10.0f;
    } else if (region.totalSpecies < 3 && region.totalCreatures > 20) {
        health -= 10.0f;  // Low diversity
    }

    // Clamp health
    region.ecosystemHealth = std::max(0.0f, std::min(100.0f, health));
}

inline bool RegionOverviewPanel::isCreatureInRegion(const Creature& creature, const RegionStats& region) const {
    glm::vec3 pos = creature.getPosition();
    float dx = pos.x - region.center.x;
    float dz = pos.z - region.center.z;
    return (dx * dx + dz * dz) <= (region.radius * region.radius);
}

inline ImVec4 RegionOverviewPanel::getHealthColor(float health) const {
    if (health >= 70.0f) {
        return ImVec4(0.3f, 0.8f, 0.3f, 1.0f);  // Green - healthy
    } else if (health >= 40.0f) {
        return ImVec4(0.8f, 0.7f, 0.2f, 1.0f);  // Yellow - warning
    } else {
        return ImVec4(0.9f, 0.3f, 0.2f, 1.0f);  // Red - critical
    }
}

} // namespace ui
