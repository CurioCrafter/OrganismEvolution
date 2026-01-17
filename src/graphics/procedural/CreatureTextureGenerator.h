#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include <glm/glm.hpp>
#include <d3d12.h>
#include <wrl/client.h>

namespace graphics {

// Pattern types for creature textures
enum class PatternType : uint8_t {
    Solid = 0,      // Single color
    Stripes,        // Tiger/zebra stripes
    Spots,          // Leopard/dalmatian spots
    Patches,        // Cow/giraffe patches
    Gradient,       // Smooth color transition
    Scales,         // Reptile/fish scales
    Feathers,       // Bird feather patterns
    Camouflage,     // Mottled camouflage
    Rings,          // Ring patterns
    Bands,          // Horizontal bands
    Speckled,       // Small random dots

    // Extended patterns (Phase 7)
    Marbled,        // Marble/veined patterns
    Mottled,        // Irregular blotches
    Rosettes,       // Jaguar-style rosettes (spots with outlines)
    Lightning,      // Lightning bolt/branch patterns
    Countershading, // Darker top, lighter bottom (natural camouflage)
    Eyespots,       // Intimidating eye-like patterns
    Tribal,         // Bold geometric patterns
    Brindle,        // Subtle streaks (dog coat pattern)

    // Phase 8 Archetype-specific patterns
    Segmented,      // Arthropod segmentation lines (for SEGMENTED archetype)
    PlatedArmor,    // Overlapping plate pattern (for PLATED archetype)
    RadialBurst,    // Radial symmetry pattern (for RADIAL archetype)
    Reticulated,    // Network/mesh pattern (for LONG_LIMBED archetype)
    Bioluminescent, // Glowing patterns (for deep-sea/RADIAL)

    Count
};

// Color gene structure for texture generation
struct ColorGenes {
    // Primary colors
    glm::vec3 primaryColor{0.5f, 0.5f, 0.5f};
    glm::vec3 secondaryColor{0.3f, 0.3f, 0.3f};
    glm::vec3 accentColor{0.8f, 0.8f, 0.8f};

    // Pattern configuration
    PatternType patternType = PatternType::Solid;
    float patternScale = 1.0f;          // Size of pattern features
    float patternDensity = 0.5f;        // How dense the pattern is
    float patternContrast = 0.5f;       // Contrast between colors
    float patternRandomness = 0.2f;     // Random variation

    // Symmetry
    bool bilateralSymmetry = true;
    float symmetryStrength = 0.9f;

    // Special effects
    float iridescence = 0.0f;           // Color shift effect
    float metallic = 0.0f;              // Metallic sheen
    float roughness = 0.5f;             // Surface roughness

    // Age/health modifiers
    float saturationMod = 1.0f;
    float brightnessMod = 1.0f;
};

// Texture generation parameters
struct TextureGenParams {
    uint32_t width = 256;
    uint32_t height = 256;
    uint32_t seed = 0;
    bool generateNormalMap = true;
    bool generateRoughnessMap = false;
    float uvScale = 1.0f;
};

// Generated texture data (CPU-side)
struct GeneratedTexture {
    std::vector<uint8_t> albedoData;    // RGBA8
    std::vector<uint8_t> normalData;    // RGB8 (optional)
    std::vector<uint8_t> roughnessData; // R8 (optional)
    uint32_t width = 0;
    uint32_t height = 0;
    bool hasNormalMap = false;
    bool hasRoughnessMap = false;
};

// Procedural texture generator
class CreatureTextureGenerator {
public:
    CreatureTextureGenerator();
    ~CreatureTextureGenerator() = default;

    // Generate texture from color genes
    GeneratedTexture generate(const ColorGenes& genes, const TextureGenParams& params);

    // Pattern-specific generators
    GeneratedTexture generateSolid(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateStripes(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateSpots(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generatePatches(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateGradient(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateScales(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateFeathers(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateCamouflage(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateRings(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateBands(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateSpeckled(const ColorGenes& genes, const TextureGenParams& params);

    // Extended pattern generators (Phase 7)
    GeneratedTexture generateMarbled(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateMottled(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateRosettes(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateLightning(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateCountershading(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateEyespots(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateTribal(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateBrindle(const ColorGenes& genes, const TextureGenParams& params);

    // Phase 8 Archetype-specific pattern generators
    GeneratedTexture generateSegmented(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generatePlatedArmor(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateRadialBurst(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateReticulated(const ColorGenes& genes, const TextureGenParams& params);
    GeneratedTexture generateBioluminescent(const ColorGenes& genes, const TextureGenParams& params);

    // Utility functions
    static ColorGenes genesFromGenome(const glm::vec3& color, float size, uint32_t speciesId);
    static PatternType patternFromSpeciesId(uint32_t speciesId);

private:
    // Noise functions
    float perlinNoise(float x, float y, uint32_t seed) const;
    float simplexNoise(float x, float y, uint32_t seed) const;
    float worleyNoise(float x, float y, uint32_t seed) const;
    float fbmNoise(float x, float y, uint32_t seed, int octaves = 4) const;

    // Pattern helpers
    float stripePattern(float x, float y, float scale, float angle) const;
    float spotPattern(float x, float y, float scale, float density, uint32_t seed) const;
    float patchPattern(float x, float y, float scale, uint32_t seed) const;
    float scalePattern(float x, float y, float scale) const;
    float featherPattern(float x, float y, float scale, float direction) const;

    // Color blending
    glm::vec3 blendColors(const glm::vec3& a, const glm::vec3& b, float t) const;
    glm::vec3 applySymmetry(const glm::vec3& color, float u, float v,
                            bool bilateral, float strength) const;

    // Normal map generation
    glm::vec3 calculateNormal(const std::vector<uint8_t>& heightData,
                               uint32_t width, uint32_t height,
                               uint32_t x, uint32_t y) const;

    // Internal state
    uint32_t m_seed = 0;
};

// DX12 texture upload helper
class TextureUploader {
public:
    TextureUploader(ID3D12Device* device);
    ~TextureUploader() = default;

    // Upload generated texture to GPU
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadTexture(
        const GeneratedTexture& texture,
        ID3D12GraphicsCommandList* commandList);

    // Create SRV for texture
    D3D12_CPU_DESCRIPTOR_HANDLE createSRV(
        ID3D12Resource* texture,
        ID3D12DescriptorHeap* srvHeap,
        uint32_t heapIndex);

private:
    ID3D12Device* m_device = nullptr;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_uploadBuffers;
};

} // namespace graphics
