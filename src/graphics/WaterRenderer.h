#pragma once

// Water Rendering System for OrganismEvolution
// Implements flat water planes with animated waves, fresnel reflections, and transparency

#include "Core/CoreMinimal.h"
#include "RHI/RHI.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>

namespace Forge::RHI {
    class IDevice;
    class IBuffer;
    class IShader;
    class IPipeline;
    class ICommandList;
}

// Water vertex format
struct WaterVertex {
    glm::vec3 position;
    glm::vec2 texCoord;
};

// Underwater visual parameters for post-processing
struct UnderwaterParams {
    glm::vec3 fogColor = glm::vec3(0.0f, 0.15f, 0.3f);      // Deep blue fog color
    float fogDensity = 0.02f;                                 // Fog density (lower = clearer)

    glm::vec3 absorptionRGB = glm::vec3(0.4f, 0.15f, 0.05f); // Color absorption per channel (red absorbs fastest)
    float clarityScalar = 1.0f;                               // Multiplier for visibility range

    float causticIntensity = 0.3f;                            // Animated caustic strength
    float causticScale = 0.02f;                               // Size of caustic pattern
    float lightShaftIntensity = 0.4f;                         // Sun shafts through water
    float lightShaftDecay = 0.95f;                            // Radial falloff for shafts

    float fogStart = 5.0f;                                    // Distance before fog begins
    float fogEnd = 150.0f;                                    // Distance at full fog
    float depthTintStrength = 0.3f;                           // How much depth affects tint
    float surfaceDistortion = 0.02f;                          // Distortion of surface from below

    // Quality settings
    int qualityLevel = 1;                                     // 0=off, 1=low, 2=medium, 3=high

    // Apply quality preset (adjusts multiple parameters at once)
    void ApplyQualityPreset(int level) {
        qualityLevel = level;
        switch (level) {
            case 0: // Off - no underwater effects
                fogDensity = 0.0f;
                causticIntensity = 0.0f;
                lightShaftIntensity = 0.0f;
                break;
            case 1: // Low - fog and absorption only
                fogDensity = 0.015f;
                fogStart = 8.0f;
                fogEnd = 180.0f;
                causticIntensity = 0.0f;
                lightShaftIntensity = 0.0f;
                clarityScalar = 1.2f;
                break;
            case 2: // Medium - adds caustics
                fogDensity = 0.02f;
                fogStart = 5.0f;
                fogEnd = 150.0f;
                causticIntensity = 0.25f;
                lightShaftIntensity = 0.0f;
                clarityScalar = 1.0f;
                break;
            case 3: // High - full effects with light shafts
            default:
                fogDensity = 0.02f;
                fogStart = 5.0f;
                fogEnd = 150.0f;
                causticIntensity = 0.3f;
                lightShaftIntensity = 0.4f;
                clarityScalar = 1.0f;
                break;
        }
    }
};

// Water constant buffer data (must be 256-byte aligned for DX12)
struct alignas(256) WaterConstants {
    // View/Projection matrices (64 bytes each)
    glm::mat4 view;              // 0-63
    glm::mat4 projection;        // 64-127
    glm::mat4 viewProjection;    // 128-191

    // Camera and lighting (48 bytes)
    glm::vec3 cameraPos;         // 192-203
    float time;                  // 204-207
    glm::vec3 lightDir;          // 208-219
    float _pad1;                 // 220-223
    glm::vec3 lightColor;        // 224-235
    float sunIntensity;          // 236-239

    // Water parameters (32 bytes)
    glm::vec4 waterColor;        // 240-255 (deep water color)
    glm::vec4 shallowColor;      // 256-271 (shallow water color)

    // Wave parameters (16 bytes)
    float waveScale;             // 272-275
    float waveSpeed;             // 276-279
    float waveHeight;            // 280-283
    float transparency;          // 284-287

    // Sky colors for reflection (32 bytes)
    glm::vec3 skyTopColor;       // 288-299
    float _pad2;                 // 300-303
    glm::vec3 skyHorizonColor;   // 304-315
    float fresnelPower;          // 316-319

    // Foam parameters (16 bytes)
    float foamIntensity;         // 320-323
    float foamScale;             // 324-327
    float specularPower;         // 328-331
    float specularIntensity;     // 332-335

    // Underwater parameters (16 bytes)
    float waterHeight;           // 336-339
    float underwaterDepth;       // 340-343
    float surfaceClarity;        // 344-347
    float _pad3;                 // 348-351

    // Padding to 512 bytes (next 256-byte boundary)
    float _padding[40];          // 352-511
};

static_assert(sizeof(WaterConstants) == 512, "WaterConstants must be 512 bytes for proper CB alignment");

class WaterRenderer {
public:
    WaterRenderer();
    ~WaterRenderer();

    // Non-copyable
    WaterRenderer(const WaterRenderer&) = delete;
    WaterRenderer& operator=(const WaterRenderer&) = delete;

    // Initialize with Forge RHI device
    bool Initialize(Forge::RHI::IDevice* device,
                    Forge::RHI::Format renderTargetFormat,
                    Forge::RHI::Format depthFormat);

    void Shutdown();

    // Generate water plane mesh
    // gridSize: number of vertices per side (e.g., 64 = 64x64 grid)
    // worldSize: size in world units (e.g., 500.0f = 500x500 units)
    void GenerateMesh(int gridSize, float worldSize, float waterHeight);

    // Update water parameters
    void SetWaterHeight(float height) { m_waterHeight = height; }
    void SetWaterColor(const glm::vec4& deepColor, const glm::vec4& shallowColor);
    void SetWaveParams(float scale, float speed, float height);
    void SetTransparency(float transparency) { m_transparency = transparency; }
    void SetFoamParams(float intensity, float scale);
    void SetSpecularParams(float power, float intensity);
    void SetSkyColors(const glm::vec3& topColor, const glm::vec3& horizonColor);

    // Render the water plane
    // Must be called after opaque geometry and before UI
    void Render(Forge::RHI::ICommandList* commandList,
                const glm::mat4& view,
                const glm::mat4& projection,
                const glm::vec3& cameraPos,
                const glm::vec3& lightDir,
                const glm::vec3& lightColor,
                float sunIntensity,
                float time);

    // Accessors
    float GetWaterHeight() const { return m_waterHeight; }
    bool IsInitialized() const { return m_initialized; }
    int GetVertexCount() const { return m_vertexCount; }
    int GetIndexCount() const { return m_indexCount; }

    // Underwater state accessors (computed from camera position vs water height)
    bool IsCameraUnderwater(const glm::vec3& cameraPos) const { return cameraPos.y < m_waterHeight; }
    float GetUnderwaterDepth(const glm::vec3& cameraPos) const;

    // Underwater parameters for post-processing
    void SetUnderwaterParams(const UnderwaterParams& params) { m_underwaterParams = params; }
    const UnderwaterParams& GetUnderwaterParams() const { return m_underwaterParams; }
    UnderwaterParams& GetUnderwaterParamsMutable() { return m_underwaterParams; }

    // Convenience setters for underwater parameters (for UI integration)
    void SetUnderwaterFogColor(const glm::vec3& color) { m_underwaterParams.fogColor = color; }
    void SetUnderwaterFogDensity(float density) { m_underwaterParams.fogDensity = density; }
    void SetUnderwaterClarity(float clarity) { m_underwaterParams.clarityScalar = clarity; }
    void SetUnderwaterAbsorption(const glm::vec3& absorption) { m_underwaterParams.absorptionRGB = absorption; }
    void SetCausticIntensity(float intensity) { m_underwaterParams.causticIntensity = intensity; }
    void SetLightShaftIntensity(float intensity) { m_underwaterParams.lightShaftIntensity = intensity; }
    void SetUnderwaterQuality(int level) { m_underwaterParams.qualityLevel = glm::clamp(level, 0, 3); }

private:
    bool CreateShaders();
    bool CreatePipeline(Forge::RHI::Format renderTargetFormat,
                        Forge::RHI::Format depthFormat);
    bool CreateBuffers();
    void UpdateConstantBuffer(const glm::mat4& view,
                              const glm::mat4& projection,
                              const glm::vec3& cameraPos,
                              const glm::vec3& lightDir,
                              const glm::vec3& lightColor,
                              float sunIntensity,
                              float time);

    Forge::RHI::IDevice* m_device = nullptr;

    // Shaders (using Forge::UniquePtr which is std::unique_ptr with default deleter)
    Forge::UniquePtr<Forge::RHI::IShader> m_vertexShader;
    Forge::UniquePtr<Forge::RHI::IShader> m_pixelShader;

    // Pipeline
    Forge::UniquePtr<Forge::RHI::IPipeline> m_pipeline;

    // Buffers
    Forge::UniquePtr<Forge::RHI::IBuffer> m_vertexBuffer;
    Forge::UniquePtr<Forge::RHI::IBuffer> m_indexBuffer;
    Forge::UniquePtr<Forge::RHI::IBuffer> m_constantBuffer;

    // Mesh data
    std::vector<WaterVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    int m_vertexCount = 0;
    int m_indexCount = 0;

    // Water parameters
    float m_waterHeight = 0.0f;
    glm::vec4 m_deepWaterColor = glm::vec4(0.0f, 0.2f, 0.4f, 1.0f);
    glm::vec4 m_shallowWaterColor = glm::vec4(0.0f, 0.5f, 0.6f, 1.0f);
    float m_waveScale = 0.1f;
    float m_waveSpeed = 0.5f;
    float m_waveHeight = 0.5f;
    float m_transparency = 0.7f;
    float m_foamIntensity = 0.3f;
    float m_foamScale = 10.0f;
    float m_specularPower = 64.0f;
    float m_specularIntensity = 1.0f;
    float m_fresnelPower = 3.0f;
    glm::vec3 m_skyTopColor = glm::vec3(0.3f, 0.5f, 0.9f);
    glm::vec3 m_skyHorizonColor = glm::vec3(0.6f, 0.7f, 0.9f);

    // Underwater rendering parameters
    UnderwaterParams m_underwaterParams;

    bool m_initialized = false;
};
