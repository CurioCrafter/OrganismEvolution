#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <d3d12.h>
#include <wrl/client.h>

// Forward declarations
class Camera;
class Creature;

namespace ui {
    class CreatureNametags;
    struct NametagConfig;
}

namespace graphics {
    class TextureAtlasManager;
    class CreatureTextureGenerator;
    struct ColorGenes;
}

namespace naming {
    class SpeciesNamingSystem;
}

// Unified creature identification and visual system
// Manages nametags, procedural textures, and species naming
class CreatureIdentityRenderer {
public:
    CreatureIdentityRenderer();
    ~CreatureIdentityRenderer();

    // Initialize with DX12 device
    bool initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
    void shutdown();

    // Update all identity systems
    void update(float deltaTime,
                const std::vector<std::unique_ptr<Creature>>& creatures,
                const Camera& camera,
                int selectedCreatureId = -1);

    // Render nametags (call during ImGui frame)
    void renderNametags(const Camera& camera, const glm::mat4& viewProjection);

    // Get texture atlas for creature textures
    graphics::TextureAtlasManager* getTextureAtlas() { return m_textureAtlas.get(); }
    const graphics::TextureAtlasManager* getTextureAtlas() const { return m_textureAtlas.get(); }

    // Get nametag system
    ui::CreatureNametags* getNametags() { return m_nametags.get(); }
    const ui::CreatureNametags* getNametags() const { return m_nametags.get(); }

    // Get naming system
    naming::SpeciesNamingSystem& getNamingSystem();

    // Configuration
    void setNametagsVisible(bool visible);
    bool getNametagsVisible() const;
    void toggleNametags();

    // Creature selection
    int getCreatureAtScreenPos(const glm::vec2& screenPos) const;
    void setSelectCallback(std::function<void(int)> callback);

    // Texture generation for a specific creature
    void regenerateCreatureTexture(const Creature* creature, ID3D12GraphicsCommandList* commandList);

    // Get color genes from creature
    static graphics::ColorGenes getColorGenesFromCreature(const Creature* creature);

    // UI panel for settings
    void renderSettingsPanel();

    // Statistics
    size_t getActiveTextureCount() const;
    size_t getVisibleNametagCount() const;

private:
    // Sub-systems
    std::unique_ptr<ui::CreatureNametags> m_nametags;
    std::unique_ptr<graphics::TextureAtlasManager> m_textureAtlas;

    // DX12 resources
    ID3D12Device* m_device = nullptr;
    bool m_initialized = false;

    // Current command list for texture operations
    ID3D12GraphicsCommandList* m_currentCommandList = nullptr;
};

// Helper to integrate with main render loop
class CreatureIdentityIntegration {
public:
    // Call at start of frame
    static void beginFrame(CreatureIdentityRenderer& renderer,
                           ID3D12GraphicsCommandList* commandList);

    // Call during creature update phase
    static void updateCreatures(CreatureIdentityRenderer& renderer,
                                float deltaTime,
                                const std::vector<std::unique_ptr<Creature>>& creatures,
                                const Camera& camera,
                                int selectedCreatureId);

    // Call during ImGui render phase
    static void renderUI(CreatureIdentityRenderer& renderer,
                         const Camera& camera,
                         const glm::mat4& viewProjection);

    // Call at end of frame
    static void endFrame(CreatureIdentityRenderer& renderer);
};
