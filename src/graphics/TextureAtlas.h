#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>
#include <glm/glm.hpp>
#include "procedural/CreatureTextureGenerator.h"

namespace graphics {

// Forward declarations
class CreatureTextureGenerator;

// Represents a region within a texture atlas
struct AtlasRegion {
    uint32_t atlasIndex = 0;    // Which atlas this region is in
    glm::vec4 uvBounds{0, 0, 1, 1};  // UV coordinates: (u0, v0, u1, v1)
    uint32_t width = 0;
    uint32_t height = 0;
    bool isValid = false;
};

// Atlas allocation node for bin packing
struct AtlasNode {
    uint32_t x = 0, y = 0;
    uint32_t width = 0, height = 0;
    bool used = false;
    std::unique_ptr<AtlasNode> left;
    std::unique_ptr<AtlasNode> right;

    AtlasNode() = default;
    AtlasNode(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
        : x(x), y(y), width(w), height(h), used(false) {}
};

// Single atlas texture
class TextureAtlasPage {
public:
    TextureAtlasPage(uint32_t width, uint32_t height);
    ~TextureAtlasPage() = default;

    // Try to allocate a region in this atlas
    bool allocate(uint32_t width, uint32_t height, uint32_t& outX, uint32_t& outY);

    // Get dimensions
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }

    // Check if atlas has space for given dimensions
    bool canFit(uint32_t width, uint32_t height) const;

    // Get usage statistics
    float getUsagePercent() const;
    uint32_t getAllocatedCount() const { return m_allocatedCount; }

    // Clear all allocations
    void clear();

private:
    AtlasNode* findNode(AtlasNode* node, uint32_t width, uint32_t height);
    AtlasNode* splitNode(AtlasNode* node, uint32_t width, uint32_t height);

    uint32_t m_width;
    uint32_t m_height;
    std::unique_ptr<AtlasNode> m_root;
    uint32_t m_allocatedCount = 0;
    uint32_t m_allocatedPixels = 0;
};

// Texture array configuration
struct TextureAtlasConfig {
    uint32_t atlasWidth = 2048;
    uint32_t atlasHeight = 2048;
    uint32_t maxAtlases = 8;            // Max texture array layers
    uint32_t textureWidth = 128;        // Per-creature texture size
    uint32_t textureHeight = 128;
    uint32_t padding = 2;               // Padding between textures
    bool generateMipmaps = true;
    uint32_t mipLevels = 4;
};

// Creature texture entry in the atlas
struct CreatureTextureEntry {
    uint32_t creatureId = 0;
    uint32_t speciesId = 0;
    AtlasRegion region;
    ColorGenes colorGenes;
    bool needsUpdate = false;
    float lastUsedTime = 0.0f;
};

// Main texture atlas manager
class TextureAtlasManager {
public:
    TextureAtlasManager();
    ~TextureAtlasManager() = default;

    // Initialize with DX12 device
    bool initialize(ID3D12Device* device, const TextureAtlasConfig& config = {});
    void shutdown();

    // Get or create texture for a creature
    const AtlasRegion& getCreatureTexture(uint32_t creatureId,
                                           uint32_t speciesId,
                                           const ColorGenes& genes,
                                           ID3D12GraphicsCommandList* commandList);

    // Force regenerate texture for a creature
    void regenerateTexture(uint32_t creatureId,
                           const ColorGenes& newGenes,
                           ID3D12GraphicsCommandList* commandList);

    // Remove creature texture (when creature dies)
    void removeCreatureTexture(uint32_t creatureId);

    // Update textures (process pending regenerations)
    void update(float deltaTime, ID3D12GraphicsCommandList* commandList);

    // Get GPU resources
    ID3D12Resource* getTextureArray() const { return m_textureArray.Get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE getTextureSRV() const { return m_srvGpuHandle; }

    // Configuration
    const TextureAtlasConfig& getConfig() const { return m_config; }

    // Statistics
    size_t getActiveTextureCount() const { return m_textureEntries.size(); }
    size_t getAtlasPageCount() const { return m_atlasPages.size(); }
    float getTotalUsagePercent() const;

    // Clear all textures (for reset/reload)
    void clear();

    // LOD management
    void setLODLevel(uint32_t level) { m_currentLOD = level; }
    uint32_t getLODLevel() const { return m_currentLOD; }

private:
    // Allocate region in atlas
    AtlasRegion allocateRegion(uint32_t width, uint32_t height);

    // Generate and upload texture
    void generateAndUploadTexture(CreatureTextureEntry& entry,
                                   ID3D12GraphicsCommandList* commandList);

    // Copy texture data to atlas
    void copyToAtlas(const GeneratedTexture& texture,
                     const AtlasRegion& region,
                     ID3D12GraphicsCommandList* commandList);

    // Create texture array resource
    bool createTextureArray();

    // Evict least recently used textures when full
    void evictLRUTextures(size_t count);

    // DX12 resources
    ID3D12Device* m_device = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_textureArray;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadBuffer;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
    D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpuHandle{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_srvCpuHandle{};

    // Configuration
    TextureAtlasConfig m_config;

    // Atlas pages (bin packing)
    std::vector<std::unique_ptr<TextureAtlasPage>> m_atlasPages;

    // Texture entries by creature ID
    std::unordered_map<uint32_t, CreatureTextureEntry> m_textureEntries;

    // Pending texture generations
    std::vector<uint32_t> m_pendingGenerations;

    // Texture generator
    std::unique_ptr<CreatureTextureGenerator> m_generator;

    // State
    bool m_initialized = false;
    uint32_t m_currentLOD = 0;
    float m_currentTime = 0.0f;

    // Staging data for uploads
    std::vector<uint8_t> m_stagingBuffer;
};

// Helper class for batching creature textures efficiently
class CreatureTextureBatcher {
public:
    CreatureTextureBatcher(TextureAtlasManager& atlasManager);

    // Begin batching textures for a frame
    void begin(ID3D12GraphicsCommandList* commandList);

    // Request texture for a creature (batched)
    const AtlasRegion& requestTexture(uint32_t creatureId,
                                       uint32_t speciesId,
                                       const ColorGenes& genes);

    // End batching and process all requests
    void end();

    // Get all requested regions for rendering
    const std::vector<std::pair<uint32_t, AtlasRegion>>& getRequestedRegions() const {
        return m_requestedRegions;
    }

private:
    TextureAtlasManager& m_atlasManager;
    ID3D12GraphicsCommandList* m_commandList = nullptr;
    std::vector<std::pair<uint32_t, AtlasRegion>> m_requestedRegions;
};

} // namespace graphics
