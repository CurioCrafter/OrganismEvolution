#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <glm/glm.hpp>
#include <d3d12.h>
#include <wrl/client.h>

// Forward declarations
class Camera;
class Creature;

namespace naming {
    struct CreatureIdentity;
    class SpeciesNamingSystem;
}

namespace ui {

// Status icons that can be displayed on nametags
enum class CreatureStatus : uint32_t {
    None        = 0,
    Hungry      = 1 << 0,
    Scared      = 1 << 1,
    Mating      = 1 << 2,
    Attacking   = 1 << 3,
    Fleeing     = 1 << 4,
    Sleeping    = 1 << 5,
    Eating      = 1 << 6,
    Injured     = 1 << 7,
    Pregnant    = 1 << 8,
    Leader      = 1 << 9
};

inline CreatureStatus operator|(CreatureStatus a, CreatureStatus b) {
    return static_cast<CreatureStatus>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline CreatureStatus operator&(CreatureStatus a, CreatureStatus b) {
    return static_cast<CreatureStatus>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool hasStatus(CreatureStatus flags, CreatureStatus check) {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(check)) != 0;
}

// Configuration for nametag display
struct NametagConfig {
    // Visibility settings
    bool showNametags = true;
    bool showHealthBars = true;
    bool showEnergyBars = true;
    bool showStatusIcons = true;
    bool showSpeciesName = true;
    bool showIndividualName = true;
    bool showScientificName = false;
    bool showDescriptor = true;           // Show trait descriptor (e.g., "carnivore, aquatic")
    bool showSimilarityColor = true;      // Show similarity cluster color chip

    // Distance settings (in world units)
    float maxVisibleDistance = 50.0f;
    float fadeStartDistance = 35.0f;
    float minVisibleDistance = 2.0f;

    // Size settings
    float baseScale = 1.0f;
    float healthBarWidth = 40.0f;
    float healthBarHeight = 4.0f;
    float energyBarHeight = 3.0f;
    float verticalOffset = 2.5f;  // Height above creature

    // Colors
    glm::vec4 textColor{1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec4 healthBarBgColor{0.2f, 0.2f, 0.2f, 0.8f};
    glm::vec4 healthBarFgColor{0.2f, 0.8f, 0.2f, 1.0f};
    glm::vec4 healthBarLowColor{0.8f, 0.2f, 0.2f, 1.0f};
    glm::vec4 energyBarBgColor{0.2f, 0.2f, 0.2f, 0.8f};
    glm::vec4 energyBarFgColor{0.2f, 0.5f, 0.9f, 1.0f};
    glm::vec4 selectedHighlight{1.0f, 0.8f, 0.0f, 1.0f};

    // Selection
    bool highlightSelected = true;
    float selectedGlowIntensity = 0.5f;
};

// Data for a single nametag instance
struct NametagInstance {
    int creatureId = -1;
    glm::vec3 worldPosition{0.0f};
    glm::vec2 screenPosition{0.0f};

    std::string individualName;
    std::string speciesName;
    std::string scientificName;
    std::string descriptor;          // Trait descriptor (e.g., "carnivore, aquatic")
    glm::vec3 similarityColor{0.5f}; // Color from similarity cluster (Agent 2)

    float health = 1.0f;
    float energy = 1.0f;
    float distanceToCamera = 0.0f;
    float alpha = 1.0f;

    CreatureStatus status = CreatureStatus::None;
    bool isSelected = false;
    bool isVisible = true;
    bool hasSimilarityColor = false;
};

// Callback for creature selection
using CreatureSelectCallback = std::function<void(int creatureId)>;

// Main nametag rendering system
class CreatureNametags {
public:
    CreatureNametags();
    ~CreatureNametags();

    // Initialize with DX12 device (for future GPU-accelerated rendering)
    bool initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
    void shutdown();

    // Configuration
    NametagConfig& getConfig() { return m_config; }
    const NametagConfig& getConfig() const { return m_config; }
    void setConfig(const NametagConfig& config) { m_config = config; }

    // Update nametag data from creatures
    void updateNametags(const std::vector<Creature*>& creatures,
                        const Camera& camera,
                        int selectedCreatureId = -1);

    // Render nametags using ImGui (called during ImGui frame)
    void renderImGui(const Camera& camera, const glm::mat4& viewProjection);

    // Click handling
    void setSelectCallback(CreatureSelectCallback callback) { m_selectCallback = callback; }
    int getCreatureAtScreenPos(const glm::vec2& screenPos, float tolerance = 20.0f) const;

    // Toggle visibility
    void setVisible(bool visible) { m_config.showNametags = visible; }
    bool isVisible() const { return m_config.showNametags; }
    void toggleVisibility() { m_config.showNametags = !m_config.showNametags; }

    // Statistics
    size_t getVisibleNametagCount() const;
    size_t getTotalNametagCount() const { return m_nametags.size(); }

    // UI Panel for settings
    void renderSettingsPanel();

private:
    // Helper methods
    glm::vec2 worldToScreen(const glm::vec3& worldPos,
                            const glm::mat4& viewProjection,
                            const glm::vec2& screenSize) const;

    float calculateAlpha(float distance) const;
    glm::vec4 getHealthBarColor(float health) const;
    std::string getStatusIconString(CreatureStatus status) const;

    // Render individual nametag
    void renderNametag(const NametagInstance& nametag);
    void renderHealthBar(const glm::vec2& pos, float width, float height,
                         float value, const glm::vec4& fgColor, const glm::vec4& bgColor);
    void renderStatusIcons(const glm::vec2& pos, CreatureStatus status);

    // Data
    NametagConfig m_config;
    std::vector<NametagInstance> m_nametags;
    CreatureSelectCallback m_selectCallback;

    // Screen dimensions (updated each frame)
    glm::vec2 m_screenSize{1920.0f, 1080.0f};

    // DX12 resources (for future GPU text rendering)
    ID3D12Device* m_device = nullptr;
    bool m_initialized = false;
};

// Helper to extract creature status from creature state
CreatureStatus getCreatureStatus(const Creature* creature);

// Helper to get creature traits for naming
naming::CreatureIdentity getCreatureIdentity(const Creature* creature);

} // namespace ui
