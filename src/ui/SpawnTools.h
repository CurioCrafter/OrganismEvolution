#pragma once

// SpawnTools - Advanced creature spawning tools for God Mode
// Provides spawn brushes, custom genome spawning, and mass spawn capabilities

#include "imgui.h"
#include "../entities/Creature.h"
#include "../entities/CreatureType.h"
#include "../entities/Genome.h"
#include "../core/CreatureManager.h"
#include "../environment/Terrain.h"
#include "../graphics/Camera.h"
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <random>

namespace ui {

// Spawn pattern types
enum class SpawnPattern {
    POINT,          // Single point spawn
    CIRCLE,         // Spawn in a circle
    GRID,           // Spawn in a grid pattern
    RANDOM_AREA,    // Random positions in area
    LINE,           // Spawn in a line
    CLUSTER         // Tight cluster with randomization
};

// Spawn request structure
struct AdvancedSpawnRequest {
    CreatureType type = CreatureType::GRAZER;
    int count = 1;
    glm::vec3 position{0.0f};
    float radius = 20.0f;
    SpawnPattern pattern = SpawnPattern::POINT;
    bool useCustomGenome = false;
    Genome customGenome;
    float genomeMutationRate = 0.0f;  // Random mutation for spawned creatures
};

class SpawnTools {
public:
    SpawnTools();
    ~SpawnTools() = default;

    // Main render function
    void renderUI();

    // Render as section
    void renderSection();

    // Set dependencies
    void setCreatureManager(Forge::CreatureManager* creatures) { m_creatures = creatures; }
    void setTerrain(const Terrain* terrain) { m_terrain = terrain; }
    void setCamera(const Camera* camera) { m_camera = camera; }

    // Update brush position
    void update(float screenWidth, float screenHeight);

    // Spawn operations
    void spawnCreature(CreatureType type, const glm::vec3& position);
    void spawnWithGenome(const Genome& genome, const glm::vec3& position, CreatureType type);
    void massSpawn(const AdvancedSpawnRequest& request);

    // Spawn at current brush position
    void spawnAtBrush();

    // Quick spawn presets
    void spawnHerbivorePopulation(const glm::vec3& center, int count);
    void spawnPredatorPack(const glm::vec3& center, int count);
    void spawnFishSchool(const glm::vec3& center, int count);
    void spawnBirdFlock(const glm::vec3& center, int count);

    // Spawn brush
    bool isBrushActive() const { return m_brushActive; }
    glm::vec3 getBrushPosition() const { return m_brushPosition; }
    float getBrushRadius() const { return m_spawnRadius; }

    // Settings
    void setSelectedType(CreatureType type) { m_selectedType = type; }
    CreatureType getSelectedType() const { return m_selectedType; }

    void setSpawnCount(int count) { m_spawnCount = std::max(1, std::min(1000, count)); }
    int getSpawnCount() const { return m_spawnCount; }

    void setSpawnRadius(float radius) { m_spawnRadius = glm::clamp(radius, 1.0f, 200.0f); }
    float getSpawnRadius() const { return m_spawnRadius; }

    void setSpawnPattern(SpawnPattern pattern) { m_pattern = pattern; }
    SpawnPattern getSpawnPattern() const { return m_pattern; }

    // Custom genome
    Genome& getCustomGenome() { return m_customGenome; }
    void setUseCustomGenome(bool use) { m_useCustomGenome = use; }
    bool isUsingCustomGenome() const { return m_useCustomGenome; }

    // Panel visibility
    bool isVisible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }
    void toggleVisible() { m_visible = !m_visible; }

    // Render spawn preview (call after main render)
    void renderSpawnPreview(float screenWidth, float screenHeight);

private:
    // Dependencies
    Forge::CreatureManager* m_creatures = nullptr;
    const Terrain* m_terrain = nullptr;
    const Camera* m_camera = nullptr;

    // Spawn settings
    CreatureType m_selectedType = CreatureType::GRAZER;
    int m_spawnCount = 5;
    float m_spawnRadius = 20.0f;
    SpawnPattern m_pattern = SpawnPattern::RANDOM_AREA;
    bool m_useCustomGenome = false;
    Genome m_customGenome;
    float m_mutationRate = 0.1f;

    // Brush state
    glm::vec3 m_brushPosition{0.0f};
    bool m_brushActive = false;

    // UI state
    bool m_visible = true;
    bool m_genomeEditorExpanded = false;
    int m_typeFilterIndex = 0;  // For type dropdown

    // Random generator
    std::mt19937 m_rng;

    // Screen dimensions (cached from update)
    float m_screenWidth = 1920.0f;
    float m_screenHeight = 1080.0f;

    // PHASE 11 - Agent 8: Spawn tracking
    int m_lastSpawnAttempts = 0;
    int m_lastSpawnSuccesses = 0;

    // Helper methods
    glm::vec3 getValidSpawnPosition(const glm::vec3& basePos, CreatureType type);
    std::vector<glm::vec3> generateSpawnPositions(const glm::vec3& center, int count);
    bool isValidSpawnLocation(const glm::vec3& pos, CreatureType type);

    void renderTypeSelector();
    void renderSpawnControls();
    void renderPatternSettings();
    void renderGenomeEditor();
    void renderQuickSpawnButtons();
    void renderPresetSpawns();

    // Get world position from screen
    bool getWorldPosition(float screenX, float screenY, glm::vec3& outPos);

    // Type list helpers
    static const std::vector<std::pair<CreatureType, const char*>>& getCreatureTypes();
};

// ============================================================================
// Implementation
// ============================================================================

inline const std::vector<std::pair<CreatureType, const char*>>& SpawnTools::getCreatureTypes() {
    static const std::vector<std::pair<CreatureType, const char*>> types = {
        {CreatureType::GRAZER, "Grazer"},
        {CreatureType::BROWSER, "Browser"},
        {CreatureType::FRUGIVORE, "Frugivore"},
        {CreatureType::SMALL_PREDATOR, "Small Predator"},
        {CreatureType::OMNIVORE, "Omnivore"},
        {CreatureType::APEX_PREDATOR, "Apex Predator"},
        {CreatureType::SCAVENGER, "Scavenger"},
        {CreatureType::FLYING, "Flying (Generic)"},
        {CreatureType::FLYING_BIRD, "Bird"},
        {CreatureType::FLYING_INSECT, "Insect"},
        {CreatureType::AERIAL_PREDATOR, "Aerial Predator"},
        {CreatureType::AQUATIC, "Fish"},
        {CreatureType::AQUATIC_HERBIVORE, "Small Fish"},
        {CreatureType::AQUATIC_PREDATOR, "Predator Fish"},
        {CreatureType::AQUATIC_APEX, "Shark"},
        {CreatureType::AMPHIBIAN, "Amphibian"}
    };
    return types;
}

inline SpawnTools::SpawnTools() {
    m_customGenome.randomize();
    m_rng.seed(static_cast<unsigned int>(time(nullptr)));
}

inline bool SpawnTools::getWorldPosition(float screenX, float screenY, glm::vec3& outPos) {
    if (!m_camera || !m_terrain) return false;

    // Screen to normalized device coords
    float x = (2.0f * screenX) / m_screenWidth - 1.0f;
    float y = 1.0f - (2.0f * screenY) / m_screenHeight;

    glm::mat4 view = const_cast<Camera*>(m_camera)->getViewMatrix();
    glm::mat4 proj = m_camera->getProjectionMatrix(m_screenWidth / m_screenHeight);
    glm::mat4 invVP = glm::inverse(proj * view);

    glm::vec4 nearPoint(x, y, -1.0f, 1.0f);
    glm::vec4 farPoint(x, y, 1.0f, 1.0f);

    glm::vec4 nearWorld = invVP * nearPoint;
    glm::vec4 farWorld = invVP * farPoint;

    nearWorld /= nearWorld.w;
    farWorld /= farWorld.w;

    glm::vec3 rayOrigin(nearWorld);
    glm::vec3 rayDir = glm::normalize(glm::vec3(farWorld) - glm::vec3(nearWorld));

    // Simple terrain raycast
    float t = 0.0f;
    float step = 1.0f;

    while (t < 1000.0f) {
        glm::vec3 point = rayOrigin + rayDir * t;

        if (m_terrain->isInBounds(point.x, point.z)) {
            float terrainHeight = m_terrain->getHeight(point.x, point.z);

            if (point.y <= terrainHeight) {
                outPos = point;
                outPos.y = terrainHeight;
                return true;
            }
        }

        t += step;
    }

    return false;
}

inline void SpawnTools::update(float screenWidth, float screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        m_brushActive = false;
        return;
    }

    m_brushActive = getWorldPosition(io.MousePos.x, io.MousePos.y, m_brushPosition);
}

inline bool SpawnTools::isValidSpawnLocation(const glm::vec3& pos, CreatureType type) {
    if (!m_terrain) return false;  // PHASE 11 - Agent 8: Reject if no terrain

    if (!m_terrain->isInBounds(pos.x, pos.z)) return false;

    // PHASE 11 - Agent 8: Use consistent water detection with CreatureManager
    float terrainHeight = m_terrain->getHeight(pos.x, pos.z);
    constexpr float waterLevel = 10.5f;  // SwimBehavior::getWaterLevelConstant()
    bool isWater = (terrainHeight < waterLevel);

    if (isAquatic(type)) {
        // Aquatic creatures need water AND position below surface
        return isWater && (pos.y < waterLevel) && (pos.y > terrainHeight);
    } else if (isFlying(type)) {
        return true;  // Flying creatures can spawn anywhere
    } else {
        // Land creatures need land (terrain above or at water level)
        return terrainHeight >= waterLevel;
    }
}

inline glm::vec3 SpawnTools::getValidSpawnPosition(const glm::vec3& basePos, CreatureType type) {
    glm::vec3 pos = basePos;

    if (m_terrain) {
        // PHASE 11 - Agent 8: Improved spawn position calculation
        float terrainHeight = m_terrain->getHeight(pos.x, pos.z);
        constexpr float waterLevel = 10.5f;

        if (isAquatic(type)) {
            // Aquatic creatures spawn underwater
            if (terrainHeight < waterLevel) {
                // Valid water location - spawn in water column
                float waterDepth = waterLevel - terrainHeight;
                float spawnDepth = std::min(2.0f, waterDepth * 0.5f);  // 50% depth or 2 units
                pos.y = waterLevel - spawnDepth;
            } else {
                // Land terrain - keep at terrain height, CreatureManager will search for water
                pos.y = terrainHeight;
            }
        } else if (isFlying(type)) {
            // Flying creatures spawn above terrain with uniform random distribution
            std::uniform_real_distribution<float> heightDist(15.0f, 35.0f);
            pos.y = terrainHeight + heightDist(m_rng);
        } else {
            // Land creatures spawn at terrain height
            pos.y = terrainHeight;
        }
    }

    return pos;
}

inline std::vector<glm::vec3> SpawnTools::generateSpawnPositions(const glm::vec3& center, int count) {
    std::vector<glm::vec3> positions;
    positions.reserve(count);

    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    switch (m_pattern) {
        case SpawnPattern::POINT:
            for (int i = 0; i < count; ++i) {
                positions.push_back(center);
            }
            break;

        case SpawnPattern::CIRCLE:
            for (int i = 0; i < count; ++i) {
                float angle = (2.0f * 3.14159f * i) / count;
                glm::vec3 pos = center;
                pos.x += cos(angle) * m_spawnRadius;
                pos.z += sin(angle) * m_spawnRadius;
                positions.push_back(pos);
            }
            break;

        case SpawnPattern::GRID: {
            int gridSize = static_cast<int>(sqrt(static_cast<float>(count))) + 1;
            float spacing = (m_spawnRadius * 2) / gridSize;

            for (int i = 0; i < count; ++i) {
                int gx = i % gridSize;
                int gz = i / gridSize;
                glm::vec3 pos = center;
                pos.x += (gx - gridSize / 2.0f) * spacing;
                pos.z += (gz - gridSize / 2.0f) * spacing;
                positions.push_back(pos);
            }
            break;
        }

        case SpawnPattern::RANDOM_AREA:
            for (int i = 0; i < count; ++i) {
                float r = sqrt(dist(m_rng) * 0.5f + 0.5f) * m_spawnRadius;
                float angle = dist(m_rng) * 3.14159f;
                glm::vec3 pos = center;
                pos.x += cos(angle) * r;
                pos.z += sin(angle) * r;
                positions.push_back(pos);
            }
            break;

        case SpawnPattern::LINE: {
            glm::vec3 dir(1.0f, 0.0f, 0.0f);  // Default line direction
            float totalLength = m_spawnRadius * 2;
            float spacing = totalLength / std::max(1, count - 1);

            for (int i = 0; i < count; ++i) {
                glm::vec3 pos = center + dir * (i * spacing - m_spawnRadius);
                positions.push_back(pos);
            }
            break;
        }

        case SpawnPattern::CLUSTER:
            for (int i = 0; i < count; ++i) {
                // Gaussian-like distribution
                float r = abs(dist(m_rng)) * m_spawnRadius * 0.5f;
                float angle = dist(m_rng) * 3.14159f;
                glm::vec3 pos = center;
                pos.x += cos(angle) * r;
                pos.z += sin(angle) * r;
                positions.push_back(pos);
            }
            break;
    }

    return positions;
}

inline void SpawnTools::spawnCreature(CreatureType type, const glm::vec3& position) {
    if (!m_creatures) return;

    glm::vec3 spawnPos = getValidSpawnPosition(position, type);

    if (m_useCustomGenome) {
        Genome genome = m_customGenome;
        if (m_mutationRate > 0.0f) {
            genome.mutate(m_mutationRate, 0.1f);
        }
        m_creatures->spawnWithGenome(spawnPos, genome);
    } else {
        m_creatures->spawn(type, spawnPos);
    }
}

inline void SpawnTools::spawnWithGenome(const Genome& genome, const glm::vec3& position, CreatureType type) {
    if (!m_creatures) return;

    glm::vec3 spawnPos = getValidSpawnPosition(position, type);
    m_creatures->spawnWithGenome(spawnPos, genome);
}

inline void SpawnTools::massSpawn(const AdvancedSpawnRequest& request) {
    if (!m_creatures) return;

    auto positions = generateSpawnPositions(request.position, request.count);

    // PHASE 11 - Agent 8: Track spawn attempts and results
    m_lastSpawnAttempts = 0;
    m_lastSpawnSuccesses = 0;

    for (const auto& pos : positions) {
        m_lastSpawnAttempts++;

        if (!isValidSpawnLocation(pos, request.type)) {
            std::cout << "[SPAWN TOOLS] Skipping invalid location for "
                      << getCreatureTypeName(request.type) << " at ("
                      << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
            continue;
        }

        glm::vec3 spawnPos = getValidSpawnPosition(pos, request.type);

        Forge::CreatureHandle handle;
        if (request.useCustomGenome) {
            Genome genome = request.customGenome;
            if (request.genomeMutationRate > 0.0f) {
                genome.mutate(request.genomeMutationRate, 0.1f);
            }
            handle = m_creatures->spawnWithGenome(spawnPos, genome);
        } else {
            handle = m_creatures->spawn(request.type, spawnPos);
        }

        if (handle.isValid()) {
            m_lastSpawnSuccesses++;
        }
    }

    // PHASE 11 - Agent 8: Report spawn results
    if (m_lastSpawnAttempts > 0) {
        float successRate = (100.0f * m_lastSpawnSuccesses) / m_lastSpawnAttempts;
        std::cout << "[SPAWN TOOLS] Mass spawn complete: " << m_lastSpawnSuccesses << "/"
                  << m_lastSpawnAttempts << " (" << successRate << "%) "
                  << getCreatureTypeName(request.type) << " spawned" << std::endl;
    }
}

inline void SpawnTools::spawnAtBrush() {
    if (!m_brushActive) return;

    AdvancedSpawnRequest request;
    request.type = m_selectedType;
    request.count = m_spawnCount;
    request.position = m_brushPosition;
    request.radius = m_spawnRadius;
    request.pattern = m_pattern;
    request.useCustomGenome = m_useCustomGenome;
    request.customGenome = m_customGenome;
    request.genomeMutationRate = m_mutationRate;

    massSpawn(request);
}

inline void SpawnTools::spawnHerbivorePopulation(const glm::vec3& center, int count) {
    AdvancedSpawnRequest request;
    request.type = CreatureType::GRAZER;
    request.count = count;
    request.position = center;
    request.radius = 30.0f;
    request.pattern = SpawnPattern::RANDOM_AREA;

    massSpawn(request);
}

inline void SpawnTools::spawnPredatorPack(const glm::vec3& center, int count) {
    AdvancedSpawnRequest request;
    request.type = CreatureType::APEX_PREDATOR;
    request.count = count;
    request.position = center;
    request.radius = 15.0f;
    request.pattern = SpawnPattern::CLUSTER;

    massSpawn(request);
}

inline void SpawnTools::spawnFishSchool(const glm::vec3& center, int count) {
    AdvancedSpawnRequest request;
    request.type = CreatureType::AQUATIC;
    request.count = count;
    request.position = center;
    request.radius = 20.0f;
    request.pattern = SpawnPattern::CLUSTER;

    massSpawn(request);
}

inline void SpawnTools::spawnBirdFlock(const glm::vec3& center, int count) {
    AdvancedSpawnRequest request;
    request.type = CreatureType::FLYING_BIRD;
    request.count = count;
    request.position = center;
    request.radius = 25.0f;
    request.pattern = SpawnPattern::CLUSTER;

    massSpawn(request);
}

inline void SpawnTools::renderUI() {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(350, 550), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Spawn Tools", &m_visible)) {
        renderSection();
    }
    ImGui::End();
}

inline void SpawnTools::renderSection() {
    // Quick spawn buttons at top
    if (ImGui::CollapsingHeader("Quick Spawn", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderQuickSpawnButtons();
    }

    // Type selector and main spawn controls
    if (ImGui::CollapsingHeader("Custom Spawn", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderTypeSelector();
        ImGui::Separator();
        renderSpawnControls();
    }

    // Pattern settings
    if (ImGui::CollapsingHeader("Spawn Pattern")) {
        renderPatternSettings();
    }

    // Genome editor
    if (ImGui::CollapsingHeader("Genome Editor")) {
        renderGenomeEditor();
    }

    // Preset spawns
    if (ImGui::CollapsingHeader("Presets")) {
        renderPresetSpawns();
    }
}

inline void SpawnTools::renderTypeSelector() {
    const auto& types = getCreatureTypes();

    ImGui::Text("Creature Type:");

    std::vector<const char*> items;
    for (const auto& t : types) {
        items.push_back(t.second);
    }

    // Find current type index
    int currentIdx = 0;
    for (int i = 0; i < static_cast<int>(types.size()); ++i) {
        if (types[i].first == m_selectedType) {
            currentIdx = i;
            break;
        }
    }

    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##TypeCombo", &currentIdx, items.data(), static_cast<int>(items.size()))) {
        m_selectedType = types[currentIdx].first;

        // Randomize genome for new type
        if (m_useCustomGenome) {
            if (isFlying(m_selectedType)) {
                m_customGenome.randomizeFlying();
            } else if (isAquatic(m_selectedType)) {
                m_customGenome.randomizeAquatic();
            } else {
                m_customGenome.randomize();
            }
        }
    }

    // Show type category
    if (isHerbivore(m_selectedType)) {
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Category: Herbivore");
    } else if (isPredator(m_selectedType)) {
        ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Category: Predator");
    } else if (isFlying(m_selectedType)) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1.0f), "Category: Flying");
    } else if (isAquatic(m_selectedType)) {
        ImGui::TextColored(ImVec4(0.3f, 0.6f, 0.9f, 1.0f), "Category: Aquatic");
    }
}

inline void SpawnTools::renderSpawnControls() {
    // Spawn count
    ImGui::Text("Spawn Count:");
    ImGui::SetNextItemWidth(120);
    ImGui::InputInt("##Count", &m_spawnCount);
    m_spawnCount = std::max(1, std::min(1000, m_spawnCount));

    ImGui::SameLine();
    if (ImGui::Button("1")) m_spawnCount = 1;
    ImGui::SameLine();
    if (ImGui::Button("5")) m_spawnCount = 5;
    ImGui::SameLine();
    if (ImGui::Button("20")) m_spawnCount = 20;
    ImGui::SameLine();
    if (ImGui::Button("50")) m_spawnCount = 50;

    // Spawn radius
    ImGui::SliderFloat("Radius", &m_spawnRadius, 1.0f, 100.0f, "%.1f");

    // Custom genome option
    ImGui::Checkbox("Use Custom Genome", &m_useCustomGenome);

    if (m_useCustomGenome) {
        ImGui::SliderFloat("Mutation Rate", &m_mutationRate, 0.0f, 0.5f, "%.2f");
    }

    ImGui::Spacing();

    // Main spawn button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));

    if (ImGui::Button("SPAWN AT BRUSH", ImVec2(-1, 35))) {
        spawnAtBrush();
    }

    ImGui::PopStyleColor(2);

    // Brush position info
    if (m_brushActive) {
        ImGui::Text("Brush: (%.1f, %.1f, %.1f)",
            m_brushPosition.x, m_brushPosition.y, m_brushPosition.z);
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
            "Move mouse over terrain");
    }
}

inline void SpawnTools::renderPatternSettings() {
    const char* patterns[] = {"Point", "Circle", "Grid", "Random Area", "Line", "Cluster"};
    int patternIdx = static_cast<int>(m_pattern);

    if (ImGui::Combo("Pattern", &patternIdx, patterns, 6)) {
        m_pattern = static_cast<SpawnPattern>(patternIdx);
    }

    // Pattern-specific info
    switch (m_pattern) {
        case SpawnPattern::POINT:
            ImGui::TextWrapped("All creatures spawn at the exact brush position.");
            break;
        case SpawnPattern::CIRCLE:
            ImGui::TextWrapped("Creatures spawn evenly distributed around a circle.");
            break;
        case SpawnPattern::GRID:
            ImGui::TextWrapped("Creatures spawn in a grid pattern.");
            break;
        case SpawnPattern::RANDOM_AREA:
            ImGui::TextWrapped("Creatures spawn randomly within the radius.");
            break;
        case SpawnPattern::LINE:
            ImGui::TextWrapped("Creatures spawn in a horizontal line.");
            break;
        case SpawnPattern::CLUSTER:
            ImGui::TextWrapped("Creatures spawn tightly clustered near center.");
            break;
    }
}

inline void SpawnTools::renderGenomeEditor() {
    ImGui::Checkbox("Enable Custom Genome", &m_useCustomGenome);

    if (!m_useCustomGenome) {
        ImGui::TextDisabled("Enable custom genome to edit traits");
        return;
    }

    ImGui::Separator();

    // Physical traits
    ImGui::Text("Physical:");
    ImGui::SliderFloat("Size", &m_customGenome.size, 0.5f, 2.0f);
    ImGui::SliderFloat("Speed", &m_customGenome.speed, 5.0f, 20.0f);
    ImGui::SliderFloat("Vision", &m_customGenome.visionRange, 10.0f, 50.0f);
    ImGui::SliderFloat("Efficiency", &m_customGenome.efficiency, 0.5f, 1.5f);

    ImGui::Separator();

    // Sensory
    ImGui::Text("Sensory:");
    ImGui::SliderFloat("Vision FOV", &m_customGenome.visionFOV, 1.0f, 6.0f);
    ImGui::SliderFloat("Hearing", &m_customGenome.hearingRange, 10.0f, 100.0f);
    ImGui::SliderFloat("Smell", &m_customGenome.smellRange, 10.0f, 150.0f);
    ImGui::SliderFloat("Camouflage", &m_customGenome.camouflageLevel, 0.0f, 1.0f);

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
    if (ImGui::Button("Fast")) {
        m_customGenome.randomize();
        m_customGenome.size = 0.7f;
        m_customGenome.speed = 18.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Large")) {
        m_customGenome.randomize();
        m_customGenome.size = 1.8f;
        m_customGenome.speed = 8.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stealthy")) {
        m_customGenome.randomize();
        m_customGenome.camouflageLevel = 0.9f;
        m_customGenome.color = glm::vec3(0.4f, 0.5f, 0.3f);
    }
}

inline void SpawnTools::renderQuickSpawnButtons() {
    // Herbivores row
    ImGui::Text("Herbivores:");
    if (ImGui::Button("+10 Grazers", ImVec2(100, 0))) {
        m_selectedType = CreatureType::GRAZER;
        m_spawnCount = 10;
        spawnAtBrush();
    }
    ImGui::SameLine();
    if (ImGui::Button("+5 Browsers", ImVec2(100, 0))) {
        m_selectedType = CreatureType::BROWSER;
        m_spawnCount = 5;
        spawnAtBrush();
    }

    // Predators row
    ImGui::Text("Predators:");
    if (ImGui::Button("+3 Wolves", ImVec2(100, 0))) {
        m_selectedType = CreatureType::APEX_PREDATOR;
        m_spawnCount = 3;
        spawnAtBrush();
    }
    ImGui::SameLine();
    if (ImGui::Button("+5 Foxes", ImVec2(100, 0))) {
        m_selectedType = CreatureType::SMALL_PREDATOR;
        m_spawnCount = 5;
        spawnAtBrush();
    }

    // Special row
    ImGui::Text("Special:");
    if (ImGui::Button("+10 Birds", ImVec2(100, 0))) {
        m_selectedType = CreatureType::FLYING_BIRD;
        m_spawnCount = 10;
        spawnAtBrush();
    }
    ImGui::SameLine();
    if (ImGui::Button("+15 Fish", ImVec2(100, 0))) {
        m_selectedType = CreatureType::AQUATIC;
        m_spawnCount = 15;
        spawnAtBrush();
    }
    ImGui::SameLine();
    if (ImGui::Button("+1 Shark", ImVec2(80, 0))) {
        m_selectedType = CreatureType::AQUATIC_APEX;
        m_spawnCount = 1;
        spawnAtBrush();
    }
}

inline void SpawnTools::renderPresetSpawns() {
    ImGui::TextWrapped("Spawn preset groups at brush position:");

    if (ImGui::Button("Herbivore Herd (30)", ImVec2(-1, 0))) {
        if (m_brushActive) {
            spawnHerbivorePopulation(m_brushPosition, 30);
        }
    }

    if (ImGui::Button("Predator Pack (5)", ImVec2(-1, 0))) {
        if (m_brushActive) {
            spawnPredatorPack(m_brushPosition, 5);
        }
    }

    if (ImGui::Button("Fish School (25)", ImVec2(-1, 0))) {
        if (m_brushActive) {
            spawnFishSchool(m_brushPosition, 25);
        }
    }

    if (ImGui::Button("Bird Flock (15)", ImVec2(-1, 0))) {
        if (m_brushActive) {
            spawnBirdFlock(m_brushPosition, 15);
        }
    }

    ImGui::Separator();

    // Balanced ecosystem
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.3f, 1.0f));
    if (ImGui::Button("Balanced Ecosystem", ImVec2(-1, 30))) {
        if (m_brushActive) {
            spawnHerbivorePopulation(m_brushPosition, 40);
            spawnPredatorPack(m_brushPosition, 5);
            spawnBirdFlock(m_brushPosition, 10);
        }
    }
    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Spawns: 40 herbivores, 5 predators, 10 birds");
    }
}

inline void SpawnTools::renderSpawnPreview(float screenWidth, float screenHeight) {
    if (!m_brushActive || !m_visible) return;

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    // Project brush position to screen
    if (!m_camera) return;

    glm::mat4 view = const_cast<Camera*>(m_camera)->getViewMatrix();
    glm::mat4 proj = m_camera->getProjectionMatrix(screenWidth / screenHeight);
    glm::mat4 vp = proj * view;

    glm::vec4 clipPos = vp * glm::vec4(m_brushPosition, 1.0f);
    if (clipPos.w <= 0) return;

    glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
    float screenX = (ndc.x + 1.0f) * 0.5f * screenWidth;
    float screenY = (1.0f - ndc.y) * 0.5f * screenHeight;

    // Draw spawn radius circle
    float radiusOnScreen = m_spawnRadius * 5.0f / clipPos.w;  // Approximate screen space radius
    radiusOnScreen = glm::clamp(radiusOnScreen, 20.0f, 200.0f);

    // Fill
    drawList->AddCircleFilled(ImVec2(screenX, screenY), radiusOnScreen,
        IM_COL32(100, 200, 100, 30), 32);

    // Border
    drawList->AddCircle(ImVec2(screenX, screenY), radiusOnScreen,
        IM_COL32(100, 200, 100, 150), 32, 2.0f);

    // Center crosshair
    drawList->AddLine(ImVec2(screenX - 10, screenY), ImVec2(screenX + 10, screenY),
        IM_COL32(255, 255, 255, 200), 2.0f);
    drawList->AddLine(ImVec2(screenX, screenY - 10), ImVec2(screenX, screenY + 10),
        IM_COL32(255, 255, 255, 200), 2.0f);

    // Info text
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%s x%d", getCreatureTypeName(m_selectedType), m_spawnCount);
    drawList->AddText(ImVec2(screenX + 15, screenY - 20), IM_COL32(255, 255, 255, 200), buffer);
}

} // namespace ui
